/*
 * * Acknowledgment: 
 * Parts of this code were developed with the assistance of Google's Gemini AI.
 * * Reference:
 * Gemini. (3.1 Pro) Google. Accessed: May 17, 2026. [Online]. Available: https://gemini.google.com
 */

#include "butterfly.h"

void butterfly(inputVectorType& a, inputVectorType& b, twiddleVectorType tw) {
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=4
    
    #pragma HLS ALLOCATION operation instances=mul limit=1
    #pragma HLS BIND_OP op=mul impl=dsp latency=4 

    data_t a_re_scaled = a.re >> 1;
    data_t a_im_scaled = a.im >> 1;
    data_t b_re_scaled = b.re >> 1;
    data_t b_im_scaled = b.im >> 1;

    data_t p1 = b_re_scaled * tw.re;
    data_t p2 = b_im_scaled * tw.im;
    data_t p3 = b_re_scaled * tw.im;
    data_t p4 = b_im_scaled * tw.re;

    inputVectorType bw;
    bw.re = p1 - p2; 
    bw.im = p3 + p4; 

    inputVectorType a_out, b_out;
    a_out.re = a_re_scaled + bw.re;
    a_out.im = a_im_scaled + bw.im;
    b_out.re = a_re_scaled - bw.re;
    b_out.im = a_im_scaled - bw.im;

    a = a_out;
    b = b_out;
}