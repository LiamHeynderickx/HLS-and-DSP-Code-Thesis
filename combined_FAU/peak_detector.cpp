/*
 * * Acknowledgment: 
 * Parts of this code were developed with the assistance of Google's Gemini AI.
 * * Reference:
 * Gemini. (3.1 Pro) Google. Accessed: May 17, 2026. [Online]. Available: https://gemini.google.com
 */

#include "dsp_system.h"

void peak_detector(mag_stream_type& mag_in, peak_stream_type& peak_out) {
    #pragma HLS AGGREGATE variable=mag_in compact=bit
    #pragma HLS INLINE 

    mag_stream_type local_mag = mag_in;
    #pragma HLS ARRAY_PARTITION variable=local_mag.data type=complete dim=1

    // use 16 bit instead of 32 to save resources
    peak_val_t tree_val[LOG2_N + 1][N];
    ap_uint<LOG2_N> tree_idx[LOG2_N + 1][N];
    #pragma HLS ARRAY_PARTITION variable=tree_val type=complete dim=0
    #pragma HLS ARRAY_PARTITION variable=tree_idx type=complete dim=0

    // Initialize & Reduction Tree Logic
    for (int i = 0; i < N; i++) {
        #pragma HLS UNROLL
        
        // Look at the upper 17 bits [32:16]
        ap_uint<17> overflow_check = local_mag.data[i].range(32, 16);
        
        // If any of those upper bits are 1 we saturate, the value is > 255.99
        if (overflow_check != 0) {
            // Saturate to max 16-bit value
            tree_val[0][i] = 0xFFFF; 
        } else {
            // Safe to slice the bottom 16 bits
            tree_val[0][i] = (peak_val_t)local_mag.data[i].range(15, 0); 
        }
        
        tree_idx[0][i] = i;
    }

    int stage = 0;
    for (int step = N / 2; step > 0; step >>= 1) {  // builds decision tree, compares 2 points at a time
        #pragma HLS UNROLL
        for (int i = 0; i < step; i++) {
            #pragma HLS UNROLL
            if (tree_val[stage][i + step] > tree_val[stage][i]) {
                tree_val[stage + 1][i] = tree_val[stage][i + step];
                tree_idx[stage + 1][i] = tree_idx[stage][i + step];
            } else {
                tree_val[stage + 1][i] = tree_val[stage][i];
                tree_idx[stage + 1][i] = tree_idx[stage][i];
            }
        }
        stage++;
    }

    //output is winning input 
    peak_packet pkt;
    pkt.bin = tree_idx[LOG2_N][0];
    pkt.val = tree_val[LOG2_N][0];
    
    peak_out.write(pkt);
}