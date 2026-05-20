/*
 * * Acknowledgment: 
 * Parts of this code were developed with the assistance of Google's Gemini AI.
 * * Reference:
 * Gemini. (3.1 Pro) Google. Accessed: May 17, 2026. [Online]. Available: https://gemini.google.com
 */

#include "dsp_system.h"

void sum_of_squares(fft_stream_type& fft_in, mag_stream_type& mag_out) {
    #pragma HLS INLINE 
    // we bind the operation to a DSP slice and ensure it has a latency of 4 to match II=4
    #pragma HLS BIND_OP op=mul impl=dsp latency=4

    for (int i = 0; i < N; i++) {
        #pragma HLS UNROLL
        //MAC
        mag_sq_t acc = fft_in.data[i].re * fft_in.data[i].re;
        acc += fft_in.data[i].im * fft_in.data[i].im;
        
        mag_out.data[i] = acc; // results in 33 bit number that must be scaled down in peak detector
    }
}