#include "dsp_system.h"

void sum_of_squares(fft_stream_type& fft_in, mag_stream_type& mag_out) {
    #pragma HLS PIPELINE II=4
    
    // We can bring back the allocation limit to strictly enforce 32 DSPs
    #pragma HLS ALLOCATION operation instances=mul limit=N
    
    #pragma HLS INTERFACE axis port=fft_in
    #pragma HLS INTERFACE axis port=mag_out
    #pragma HLS INTERFACE ap_ctrl_none port=return

    #pragma HLS AGGREGATE variable=fft_in compact=bit
    #pragma HLS AGGREGATE variable=mag_out compact=bit

    fft_stream_type local_in = fft_in;
    #pragma HLS ARRAY_PARTITION variable=local_in.data complete dim=1

    mag_stream_type local_out;
    #pragma HLS ARRAY_PARTITION variable=local_out.data complete dim=1

    for (int i = 0; i < N; i++) {
        #pragma HLS UNROLL
        
        // Step 1: Calculate Real Squared
        mag_sq_t acc = local_in.data[i].re * local_in.data[i].re;
                                     
        // Step 2: Calculate Imag Squared and ACCUMULATE
        // Writing it this way strongly hints to HLS to use the DSP's internal MAC loop
        acc += local_in.data[i].im * local_in.data[i].im;
        
        local_out.data[i] = acc;
    }

    mag_out = local_out;
}