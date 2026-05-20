/*
 * * Acknowledgment: 
 * Parts of this code were developed with the assistance of Google's Gemini AI.
 * * Reference:
 * Gemini. (3.1 Pro) Google. Accessed: May 17, 2026. [Online]. Available: https://gemini.google.com
 */
 
#include "fft.h"

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

void fft_top(fft_stream_type& x_i, fft_stream_type& y_o) {

    #pragma HLS INTERFACE axis port=x_i
    #pragma HLS INTERFACE axis port=y_o
    #pragma HLS INTERFACE ap_ctrl_none port=return

    #pragma HLS AGGREGATE variable=x_i compact=bit
    #pragma HLS AGGREGATE variable=y_o compact=bit

    #pragma HLS PIPELINE II=4

    static const TwiddleTable<N / 2> twiddle_rom;
    static inputVectorType reg_layers[LOG2_N + 1][N];
    #pragma HLS ARRAY_PARTITION variable=reg_layers type=complete dim=0

    fft_stream_type x_local = x_i; 
    fft_stream_type y_local;

    //Bit reversal on input
    for (int j = 0; j < N; j++) {
        #pragma HLS UNROLL
        reg_layers[0][reverse_bits(j)] = x_local.data[j]; 
    }

    //butterflies
    stage_instantiation: for (int stage = 0; stage < LOG2_N; stage++) { 
        #pragma HLS UNROLL
        int stride = 1 << stage;          
        int step = (N >> 1) >> stage;          

        loop_groups: for (int start = 0; start < N; start += (stride * 2)) { 
            #pragma HLS UNROLL
            loop_butterflies: for (int k = 0; k < stride; k++) { 
                #pragma HLS UNROLL
                inputVectorType a = reg_layers[stage][start + k];
                inputVectorType b = reg_layers[stage][start + k + stride];
                
                int tw_idx = k * step;
                twiddleVectorType tw;
                tw.re = twiddle_rom.re[tw_idx];
                tw.im = twiddle_rom.im[tw_idx];

                butterfly(a, b, tw);

                reg_layers[stage + 1][start + k] = a;
                reg_layers[stage + 1][start + k + stride] = b;
            }
        }
    }

    //read output
    write_loop: for (int j = 0; j < N; j++) {
        #pragma HLS UNROLL
        y_local.data[j] = reg_layers[LOG2_N][j];
    }

    y_o = y_local; 
}