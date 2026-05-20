/*
 * * Acknowledgment: 
 * Parts of this code were developed with the assistance of Google's Gemini AI.
 * * Reference:
 * Gemini. (3.1 Pro) Google. Accessed: May 17, 2026. [Online]. Available: https://gemini.google.com
 */
 
#ifndef CORRELATOR_H  
#define CORRELATOR_H

#include "ap_fixed.h"
#include <ap_int.h>       
#include <hls_stream.h>   
#include <cmath>
   
const int UF = 4;      

// Spatial Correlator Config
const int CHIPS_PER_SUBCOR = 64;
const int NUMBER_OF_SUBCOR = 16;
const int SAMPLES_PER_SUBCOR = CHIPS_PER_SUBCOR * 2; 
const int TOTAL_SAMPLES = NUMBER_OF_SUBCOR * SAMPLES_PER_SUBCOR; // 2048
const int CODE_LENGTH = NUMBER_OF_SUBCOR * CHIPS_PER_SUBCOR;     // 1024

// AXI Stream Data Types
typedef ap_fixed<16, 12, AP_TRN, AP_WRAP> data_t; 

// --- UPDATED SIGNATURE ---
void subcorrelator_top(
    ap_uint<1> sample_ni_i,
    ap_uint<1> sample_nq_i,
    ap_uint<1> sample_pi_i,
    ap_uint<1> sample_pq_i,
    ap_uint<1> sample_valid_i,
    ap_uint<1> code_i,          
    ap_uint<1> code_valid_i,    
    ap_uint<1> code_last_i,     
    ap_uint<2> mode_i,
    hls::stream<ap_uint<1024> > &fft_out_stream 
);

#endif // CORRELATOR_H