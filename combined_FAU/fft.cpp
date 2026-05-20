/*
 * * Acknowledgment: 
 * Parts of this code were developed with the assistance of Google's Gemini AI.
 * * Reference:
 * Gemini. (3.1 Pro) Google. Accessed: May 17, 2026. [Online]. Available: https://gemini.google.com
 */
 
#include "fft.h"

// INLINE bit reversal at input
unsigned int reverse_bits(unsigned int x) {
    #pragma HLS INLINE
    unsigned int result = 0;
    for (int i = 0; i < LOG2_N; i++) {
        #pragma HLS UNROLL
        result = (result << 1) | (x & 1);
        x >>= 1;
    }
    return result;
}


//top function
void fft_top(fft_stream_type& x_i, fft_stream_type& y_o) {

    #pragma HLS AGGREGATE variable=x_i compact=bit
    #pragma HLS AGGREGATE variable=y_o compact=bit
    #pragma HLS INLINE // Changed from PIPELINE II=4 to allow the dsp top to manage Interval
    #pragma HLS BIND_OP op=mul impl=dsp latency=4

    static const TwiddleTable<N / 2> twiddle_rom;
    inputVectorType reg_layers[LOG2_N + 1][N]; 
    #pragma HLS ARRAY_PARTITION variable=reg_layers type=complete dim=0

    fft_stream_type x_local = x_i; 
    fft_stream_type y_local;

    // Call bit reversal on Inputs to match DIT
    for (int j = 0; j < N; j++) {
        #pragma HLS UNROLL
        reg_layers[0][reverse_bits(j)] = x_local.data[j]; 
    }

    //construciton of FFT structure, forward butterfly layer instantiation
    // UNROLL ensures it is done in parallel and not sequentially

    stage_instantiation: for (int stage = 0; stage < LOG2_N; stage++) { //this loop builds log2N layers
        #pragma HLS UNROLL
        int stride = 1 << stage;               // Defines distance between butterfly points (1, 2, 4, 8, 16)
        int step = (N >> 1) >> stage;          // which twiddle to use, how many indicies to skip (16, 8, 4, 2, 1)

        loop_groups: for (int start = 0; start < N; start += (stride * 2)) { //groups butterflys in each stage till all in final form 1 group
            #pragma HLS UNROLL

            loop_butterflies: for (int k = 0; k < stride; k++) { //creates physical butterflys
                #pragma HLS UNROLL
                inputVectorType a = reg_layers[stage][start + k];
                inputVectorType b = reg_layers[stage][start + k + stride];
                
                int tw_idx = k * step;
                twiddleVectorType tw;
                tw.re = twiddle_rom.re[tw_idx];
                tw.im = twiddle_rom.im[tw_idx];

                butterfly(a, b, tw); // call butterfly op

                reg_layers[stage + 1][start + k] = a;
                reg_layers[stage + 1][start + k + stride] = b;
            }
        }
    }

    //read outputs into registers
    write_loop: for (int j = 0; j < N; j++) {
        #pragma HLS UNROLL
        y_local.data[j] = reg_layers[LOG2_N][j];
    }

    y_o = y_local; 
}