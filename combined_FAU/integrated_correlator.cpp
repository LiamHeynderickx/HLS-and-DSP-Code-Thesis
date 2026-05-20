/*
 * * Acknowledgment: 
 * Parts of this code were developed with the assistance of Google's Gemini AI.
 * * Reference:
 * Gemini. (3.1 Pro) Google. Accessed: May 17, 2026. [Online]. Available: https://gemini.google.com
 */

#include "dsp_system.h"
#include "correlator.h"

const int SAT_MAX =  (1 << 15) - 1; 
const int SAT_MIN = -(1 << 15);     

// match bank
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
// top function adjusted to match II=4 and scalar bare-metal inputs
void integrated_correlator(
    // --- 1. NEW SCALAR RF INPUT PINS ---
    ap_uint<1> sample_ni_i,
    ap_uint<1> sample_nq_i,
    ap_uint<1> sample_pi_i,
    ap_uint<1> sample_pq_i,
    ap_uint<1> sample_valid_i,
    
    // --- 2. NEW SERIAL CODE PINS ---
    ap_uint<1> code_i,          
    ap_uint<1> code_valid_i,    
    ap_uint<1> code_last_i,     
    
    // --- 3. CONTROL & OUTPUT ---
    ap_uint<2> mode_i,
    fft_stream_type &fft_out
) {
    #pragma HLS INLINE // Force this to melt into the master II=4 pipeline

    // --- INTERNAL SERIAL-TO-PARALLEL CODE LOGIC ---
    static ap_uint<CODE_LENGTH> internal_code_shift_reg = 0;
    static ap_uint<CODE_LENGTH> internal_code_snap = 0;

    // Shift new bits in serially
    if (code_valid_i == 1) {
        internal_code_shift_reg >>= 1; 
        internal_code_shift_reg[CODE_LENGTH - 1] = code_i;
    }

    // Take the massive parallel snapshot when instructed
    if (code_last_i == 1) {
        internal_code_snap = internal_code_shift_reg;
    }
    // ----------------------------------------------

    static ap_uint<1> phase = 0;
    
    // Shift Registers
    static ap_uint<TOTAL_SAMPLES> shift_reg_ni = 0;
    static ap_uint<TOTAL_SAMPLES> shift_reg_nq = 0;
    static ap_uint<TOTAL_SAMPLES> shift_reg_pi = 0;
    static ap_uint<TOTAL_SAMPLES> shift_reg_pq = 0;

    // Save P scores so they survive the gap to Phase 1
    static ap_int<8> scores_pi_saved[NUMBER_OF_SUBCOR];
    static ap_int<8> scores_pq_saved[NUMBER_OF_SUBCOR];
    #pragma HLS ARRAY_PARTITION variable=scores_pi_saved complete dim=1
    #pragma HLS ARRAY_PARTITION variable=scores_pq_saved complete dim=1

    fft_stream_type local_fft;

    // Initialize all 32 bins to zero (avoids garbage data in unused upper bins)
    for(int i = 0; i < N; i++) {
        #pragma HLS UNROLL
        local_fft.data[i].re = 0;
        local_fft.data[i].im = 0;
    }

    // --- VALIDATION & PHASE WRAPPER ---
    // Execute IF we have a new valid RF sample (Phase 0) 
    // OR if we are pending a P-frame output (Phase 1)
    if (sample_valid_i == 1 || phase == 1) {
        
        if (phase == 0) {
            // Read scalar inputs, run math, output N frame
            shift_reg_ni >>= 1; shift_reg_ni[TOTAL_SAMPLES - 1] = sample_ni_i;
            shift_reg_nq >>= 1; shift_reg_nq[TOTAL_SAMPLES - 1] = sample_nq_i;
            shift_reg_pi >>= 1; shift_reg_pi[TOTAL_SAMPLES - 1] = sample_pi_i;
            shift_reg_pq >>= 1; shift_reg_pq[TOTAL_SAMPLES - 1] = sample_pq_i;

            ap_int<8> scores_ni[NUMBER_OF_SUBCOR], scores_nq[NUMBER_OF_SUBCOR];
            ap_int<8> scores_pi[NUMBER_OF_SUBCOR], scores_pq[NUMBER_OF_SUBCOR];
            
            // Pass the internal code snap to the math banks!
            calculate_bank(internal_code_snap, shift_reg_ni, scores_ni);
            calculate_bank(internal_code_snap, shift_reg_nq, scores_nq);
            calculate_bank(internal_code_snap, shift_reg_pi, scores_pi);
            calculate_bank(internal_code_snap, shift_reg_pq, scores_pq);

            // Lock the P scores into static registers for the next cycle
            for(int i = 0; i < NUMBER_OF_SUBCOR; i++) {
                #pragma HLS UNROLL
                scores_pi_saved[i] = scores_pi[i];
                scores_pq_saved[i] = scores_pq[i];
            }

            // Output logic for N
            if (mode_i == 0) {
                for(int i = 0; i < 16; i++) {
                    #pragma HLS UNROLL
                    int re_n = (int)scores_ni[i]; int im_n = (int)scores_nq[i]; 
                    re_n = (re_n > SAT_MAX) ? SAT_MAX : (re_n < SAT_MIN) ? SAT_MIN : re_n;
                    im_n = (im_n > SAT_MAX) ? SAT_MAX : (im_n < SAT_MIN) ? SAT_MIN : im_n;
                    local_fft.data[i].re = re_n; local_fft.data[i].im = im_n; 
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
                    local_fft.data[i].re = re_n; local_fft.data[i].im = im_n; 
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
                    local_fft.data[i].re = re_n; local_fft.data[i].im = im_n; 
                }
            }
        } 
        else {
            // Do NOT read input. Output saved P frame
            if (mode_i == 0) {
                for(int i = 0; i < 16; i++) {
                    #pragma HLS UNROLL
                    int re_p = (int)scores_pi_saved[i]; int im_p = (int)scores_pq_saved[i]; 
                    re_p = (re_p > SAT_MAX) ? SAT_MAX : (re_p < SAT_MIN) ? SAT_MIN : re_p;
                    im_p = (im_p > SAT_MAX) ? SAT_MAX : (im_p < SAT_MIN) ? SAT_MIN : im_p;
                    local_fft.data[i].re = re_p; local_fft.data[i].im = im_p; 
                }
            } 
            else if (mode_i == 1) {
                for(int i = 0; i < 8; i++) {
                    #pragma HLS UNROLL
                    int idx = i * 2; 
                    int re_p = (int)scores_pi_saved[idx] + (int)scores_pi_saved[idx+1];
                    int im_p = (int)scores_pq_saved[idx] + (int)scores_pq_saved[idx+1];
                    re_p = (re_p > SAT_MAX) ? SAT_MAX : (re_p < SAT_MIN) ? SAT_MIN : re_p;
                    im_p = (im_p > SAT_MAX) ? SAT_MAX : (im_p < SAT_MIN) ? SAT_MIN : im_p;
                    local_fft.data[i].re = re_p; local_fft.data[i].im = im_p; 
                }
            } 
            else {
                for(int i = 0; i < 4; i++) {
                    #pragma HLS UNROLL
                    int idx = i * 4; 
                    int re_p = (int)scores_pi_saved[idx] + (int)scores_pi_saved[idx+1] + (int)scores_pi_saved[idx+2] + (int)scores_pi_saved[idx+3];
                    int im_p = (int)scores_pq_saved[idx] + (int)scores_pq_saved[idx+1] + (int)scores_pq_saved[idx+2] + (int)scores_pq_saved[idx+3];
                    re_p = (re_p > SAT_MAX) ? SAT_MAX : (re_p < SAT_MIN) ? SAT_MIN : re_p;
                    im_p = (im_p > SAT_MAX) ? SAT_MAX : (im_p < SAT_MIN) ? SAT_MIN : im_p;
                    local_fft.data[i].re = re_p; local_fft.data[i].im = im_p; 
                }
            }
        }

        // Toggle phase and output to FFT
        phase = ~phase;
        fft_out = local_fft;
    }
}