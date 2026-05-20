/*
 * * Acknowledgment: 
 * Parts of this code were developed with the assistance of Google's Gemini AI.
 * * Reference:
 * Gemini. (3.1 Pro) Google. Accessed: May 17, 2026. [Online]. Available: https://gemini.google.com
 */

#include "dsp_system.h"

void combined_dsp_top(
    ap_uint<1> sample_ni_i,
    ap_uint<1> sample_nq_i,
    ap_uint<1> sample_pi_i,
    ap_uint<1> sample_pq_i,
    ap_uint<1> sample_valid_i,
    ap_uint<1> code_i,          
    ap_uint<1> code_valid_i,    
    ap_uint<1> code_last_i,     
    ap_uint<2> mode_i,
    top_4_stream_type& peak_out_stream
) {
    #pragma HLS INTERFACE ap_none port=sample_ni_i
    #pragma HLS INTERFACE ap_none port=sample_nq_i
    #pragma HLS INTERFACE ap_none port=sample_pi_i
    #pragma HLS INTERFACE ap_none port=sample_pq_i
    #pragma HLS INTERFACE ap_none port=sample_valid_i
    #pragma HLS INTERFACE ap_none port=code_i
    #pragma HLS INTERFACE ap_none port=code_valid_i
    #pragma HLS INTERFACE ap_none port=code_last_i
    #pragma HLS INTERFACE ap_none port=mode_i
    #pragma HLS INTERFACE axis port=peak_out_stream
    #pragma HLS INTERFACE ap_ctrl_none port=return
    
    #pragma HLS ALLOCATION operation instances=mul limit=112 //total DSP limit

    #pragma HLS PIPELINE II=4 //throughput constraint

    fft_stream_type corr_to_fft;
    fft_stream_type fft_to_sos;
    mag_stream_type sos_to_peak;
    peak_stream_type peak_to_mem; 

    // Pass the raw pins directly into the correlator logic
    integrated_correlator(
        sample_ni_i, sample_nq_i, sample_pi_i, sample_pq_i, sample_valid_i,
        code_i, code_valid_i, code_last_i,
        mode_i, 
        corr_to_fft
    );
    
    fft_top(corr_to_fft, fft_to_sos);
    sum_of_squares(fft_to_sos, sos_to_peak);
    peak_detector(sos_to_peak, peak_to_mem);
    
    top_peak_memory(peak_to_mem, peak_out_stream);
}