/*
 * * Acknowledgment: 
 * Parts of this code were developed with the assistance of Google's Gemini AI.
 * * Reference:
 * Gemini. (3.1 Pro) Google. Accessed: May 17, 2026. [Online]. Available: https://gemini.google.com
 */

#define AP_INT_MAX_W 4096
#include <ap_int.h>
#include <hls_stream.h>
#include "correlator.h" 

const int FFT_SIZE = 32;
const int SAT_MAX =  (1 << 15) - 1; 
const int SAT_MIN = -(1 << 15);     


void calculate_bank(
    ap_uint<CODE_LENGTH> code_snap, 
    ap_uint<TOTAL_SAMPLES> stream_reg, 
    ap_int<8> scores_out[NUMBER_OF_SUBCOR] 
) {
    #pragma HLS INLINE

    for(int ch = 0; ch < NUMBER_OF_SUBCOR; ch++) {
        #pragma HLS UNROLL
        
        int low_sample = ch * SAMPLES_PER_SUBCOR;
        int high_sample = low_sample + SAMPLES_PER_SUBCOR - 1;
        ap_uint<SAMPLES_PER_SUBCOR> channel_samples = stream_reg.range(high_sample, low_sample);

        int low_code = ch * CHIPS_PER_SUBCOR;
        int high_code = low_code + CHIPS_PER_SUBCOR - 1;
        ap_uint<CHIPS_PER_SUBCOR> c_word = code_snap.range(high_code, low_code);

        ap_int<8> local_score = 0;

        for(int i = 0; i < CHIPS_PER_SUBCOR; i++) {
            #pragma HLS UNROLL
            ap_uint<1> s_even = channel_samples[2 * i];
            ap_uint<1> s_odd  = channel_samples[2 * i + 1];
            ap_uint<1> c_bit  = c_word[i];

            if (s_even == s_odd) {
                if (s_even == c_bit) {
                    local_score++;
                } else {
                    local_score--;
                }
            }
        }
        scores_out[ch] = local_score;
    }
}
void subcorrelator_top(ap_uint<1> sample_ni_i, ap_uint<1> sample_nq_i, ap_uint<1> sample_pi_i, ap_uint<1> sample_pq_i, ap_uint<1> sample_valid_i,
    ap_uint<1> code_i, ap_uint<1> code_valid_i, ap_uint<1> code_last_i, ap_uint<2> mode_i, hls::stream<ap_uint<1024> > &fft_out_stream) {

    #pragma HLS INTERFACE ap_ctrl_none port=return
    #pragma HLS INTERFACE ap_none port=sample_ni_i
    #pragma HLS INTERFACE ap_none port=sample_nq_i
    #pragma HLS INTERFACE ap_none port=sample_pi_i
    #pragma HLS INTERFACE ap_none port=sample_pq_i
    #pragma HLS INTERFACE ap_none port=sample_valid_i
    #pragma HLS INTERFACE ap_none port=code_i
    #pragma HLS INTERFACE ap_none port=code_valid_i
    #pragma HLS INTERFACE ap_none port=code_last_i
    #pragma HLS INTERFACE ap_none port=mode_i
    #pragma HLS INTERFACE axis port=fft_out_stream
    
    #pragma HLS PIPELINE II=2
    
    static ap_uint<CODE_LENGTH> internal_code_shift_reg = 0;
    static ap_uint<CODE_LENGTH> internal_code_snap = 0;

    if (code_valid_i == 1) {
        internal_code_shift_reg >>= 1; 
        internal_code_shift_reg[CODE_LENGTH - 1] = code_i;
    }

    if (code_last_i == 1) {
        internal_code_snap = internal_code_shift_reg;
    }

    static ap_uint<TOTAL_SAMPLES> shift_reg_ni = 0;
    static ap_uint<TOTAL_SAMPLES> shift_reg_nq = 0;
    static ap_uint<TOTAL_SAMPLES> shift_reg_pi = 0;
    static ap_uint<TOTAL_SAMPLES> shift_reg_pq = 0;

    ap_int<8> scores_ni[NUMBER_OF_SUBCOR], scores_nq[NUMBER_OF_SUBCOR];
    ap_int<8> scores_pi[NUMBER_OF_SUBCOR], scores_pq[NUMBER_OF_SUBCOR];
    #pragma HLS ARRAY_PARTITION variable=scores_ni complete dim=1
    #pragma HLS ARRAY_PARTITION variable=scores_nq complete dim=1
    #pragma HLS ARRAY_PARTITION variable=scores_pi complete dim=1
    #pragma HLS ARRAY_PARTITION variable=scores_pq complete dim=1

    if (sample_valid_i == 1) {
        
        shift_reg_ni >>= 1; shift_reg_ni[TOTAL_SAMPLES - 1] = sample_ni_i;
        shift_reg_nq >>= 1; shift_reg_nq[TOTAL_SAMPLES - 1] = sample_nq_i;
        shift_reg_pi >>= 1; shift_reg_pi[TOTAL_SAMPLES - 1] = sample_pi_i;
        shift_reg_pq >>= 1; shift_reg_pq[TOTAL_SAMPLES - 1] = sample_pq_i;
        
        calculate_bank(internal_code_snap, shift_reg_ni, scores_ni);
        calculate_bank(internal_code_snap, shift_reg_nq, scores_nq);
        calculate_bank(internal_code_snap, shift_reg_pi, scores_pi);
        calculate_bank(internal_code_snap, shift_reg_pq, scores_pq);

        ap_uint<1024> frame_n_packed = 0; 
        ap_uint<1024> frame_p_packed = 0;

        if (mode_i == 0) {
            for(int i = 0; i < 16; i++) {
                #pragma HLS UNROLL
                int re_n = (int)scores_ni[i]; 
                int im_n = (int)scores_nq[i]; 
                re_n = (re_n > SAT_MAX) ? SAT_MAX : (re_n < SAT_MIN) ? SAT_MIN : re_n;
                im_n = (im_n > SAT_MAX) ? SAT_MAX : (im_n < SAT_MIN) ? SAT_MIN : im_n;
                data_t fixed_re_n = re_n; data_t fixed_im_n = im_n; 
                
                frame_n_packed.range(i*32 + 15, i*32)      = fixed_re_n.range(15, 0);
                frame_n_packed.range(i*32 + 31, i*32 + 16) = fixed_im_n.range(15, 0);

                int re_p = (int)scores_pi[i]; 
                int im_p = (int)scores_pq[i]; 
                re_p = (re_p > SAT_MAX) ? SAT_MAX : (re_p < SAT_MIN) ? SAT_MIN : re_p;
                im_p = (im_p > SAT_MAX) ? SAT_MAX : (im_p < SAT_MIN) ? SAT_MIN : im_p;
                data_t fixed_re_p = re_p; data_t fixed_im_p = im_p; 
                
                frame_p_packed.range(i*32 + 15, i*32)      = fixed_re_p.range(15, 0);
                frame_p_packed.range(i*32 + 31, i*32 + 16) = fixed_im_p.range(15, 0);
            }
        } 
        else if (mode_i == 1) {
            for(int i = 0; i < 8; i++) {
                #pragma HLS UNROLL
                int idx = i * 2; 
                int re_n = (int)scores_ni[idx] + (int)scores_ni[idx+1];
                int im_n = (int)scores_nq[idx] + (int)scores_nq[idx+1];
                re_n = (re_n > SAT_MAX) ? SAT_MAX : (re_n < SAT_MIN) ? SAT_MIN : re_n;
                im_n = (im_n > SAT_MAX) ? SAT_MAX : (im_n < SAT_MIN) ? SAT_MIN : im_n;
                data_t fixed_re_n = re_n; data_t fixed_im_n = im_n; 
                
                frame_n_packed.range(i*32 + 15, i*32)      = fixed_re_n.range(15, 0);
                frame_n_packed.range(i*32 + 31, i*32 + 16) = fixed_im_n.range(15, 0);

                int re_p = (int)scores_pi[idx] + (int)scores_pi[idx+1];
                int im_p = (int)scores_pq[idx] + (int)scores_pq[idx+1];
                re_p = (re_p > SAT_MAX) ? SAT_MAX : (re_p < SAT_MIN) ? SAT_MIN : re_p;
                im_p = (im_p > SAT_MAX) ? SAT_MAX : (im_p < SAT_MIN) ? SAT_MIN : im_p;
                data_t fixed_re_p = re_p; data_t fixed_im_p = im_p; 
                
                frame_p_packed.range(i*32 + 15, i*32)      = fixed_re_p.range(15, 0);
                frame_p_packed.range(i*32 + 31, i*32 + 16) = fixed_im_p.range(15, 0);
            }
        } 
        else {
            for(int i = 0; i < 4; i++) {
                #pragma HLS UNROLL
                int idx = i * 4; 
                int re_n = (int)scores_ni[idx] + (int)scores_ni[idx+1] + (int)scores_ni[idx+2] + (int)scores_ni[idx+3];
                int im_n = (int)scores_nq[idx] + (int)scores_nq[idx+1] + (int)scores_nq[idx+2] + (int)scores_nq[idx+3];
                re_n = (re_n > SAT_MAX) ? SAT_MAX : (re_n < SAT_MIN) ? SAT_MIN : re_n;
                im_n = (im_n > SAT_MAX) ? SAT_MAX : (im_n < SAT_MIN) ? SAT_MIN : im_n;
                data_t fixed_re_n = re_n; data_t fixed_im_n = im_n; 
                
                frame_n_packed.range(i*32 + 15, i*32)      = fixed_re_n.range(15, 0);
                frame_n_packed.range(i*32 + 31, i*32 + 16) = fixed_im_n.range(15, 0);

                int re_p = (int)scores_pi[idx] + (int)scores_pi[idx+1] + (int)scores_pi[idx+2] + (int)scores_pi[idx+3];
                int im_p = (int)scores_pq[idx] + (int)scores_pq[idx+1] + (int)scores_pq[idx+2] + (int)scores_pq[idx+3];
                re_p = (re_p > SAT_MAX) ? SAT_MAX : (re_p < SAT_MIN) ? SAT_MIN : re_p;
                im_p = (im_p > SAT_MAX) ? SAT_MAX : (im_p < SAT_MIN) ? SAT_MIN : im_p;
                data_t fixed_re_p = re_p; data_t fixed_im_p = im_p; 
                
                frame_p_packed.range(i*32 + 15, i*32)      = fixed_re_p.range(15, 0);
                frame_p_packed.range(i*32 + 31, i*32 + 16) = fixed_im_p.range(15, 0);
            }
        }
        //output
        fft_out_stream.write(frame_n_packed);
        fft_out_stream.write(frame_p_packed);
    }
}