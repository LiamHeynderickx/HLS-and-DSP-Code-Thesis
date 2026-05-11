#include "fft_academic.h" 


template<int N_BITS>  ap_uint<N_BITS> bit_reverse(ap_uint<N_BITS> input) {
    #pragma HLS inline
    ap_uint<N_BITS> reversed;
    for (int bit_i = 0; bit_i < N_BITS; bit_i++) {
        #pragma HLS UNROLL
        reversed.range(bit_i, bit_i) = input.range(N_BITS-1-bit_i, N_BITS-1-bit_i);
    }
    return reversed;
}


void RADIX2_BFLY_double_buffer_quarter_CY(inputVectorType* data_ld, inputVectorType* data_st, int i0, int i1, bool inv_i1_enable, bool tw_enable, twiddle_t tw_re, twiddle_t tw_im) {
    #pragma HLS inline
    inputVectorType d0 = data_ld[i0];                     
    inputVectorType d1 = data_ld[i1];
    
    data_t d0_real = d0.re, d0_imag = d0.im;
    data_t d1_real, d1_imag;
    
    if(inv_i1_enable){
        d1_real = d1.im;
        d1_imag = -d1.re;
    } else if(tw_enable){
        data_t a = d1.re, b = d1.im;
        twiddle_t c = tw_re, d = tw_im; 
        
        data_t ac = a * c;
        data_t bd = b * d;
        data_t ad = a * d;
        data_t bc = b * c;
        
        #pragma HLS BIND_OP variable=ac op=mul impl=dsp latency=3
        #pragma HLS BIND_OP variable=bd op=mul impl=dsp latency=3
        #pragma HLS BIND_OP variable=ad op=mul impl=dsp latency=3
        #pragma HLS BIND_OP variable=bc op=mul impl=dsp latency=3

        d1_real = ac - bd; 
        d1_imag = ad + bc; 

        #pragma HLS BIND_OP variable=d1_real op=sub latency=2
        #pragma HLS BIND_OP variable=d1_imag op=add latency=2
    } else {
        d1_real = d1.re;
        d1_imag = d1.im;
    }

    //drop LSB
    data_t out0_r = (d0_real + d1_real) >> 1;
    data_t out0_i = (d0_imag + d1_imag) >> 1;
    data_t out1_r = (d0_real - d1_real) >> 1;
    data_t out1_i = (d0_imag - d1_imag) >> 1;

    #pragma HLS BIND_OP variable=out0_r op=add latency=3
    #pragma HLS BIND_OP variable=out0_i op=add latency=3
    #pragma HLS BIND_OP variable=out1_r op=sub latency=3
    #pragma HLS BIND_OP variable=out1_i op=sub latency=3

    data_st[i0].re = out0_r; data_st[i0].im = out0_i;
    data_st[i1].re = out1_r; data_st[i1].im = out1_i;
}

template<int stage> void FFT_stage(inputVectorType data_ld[N], inputVectorType data_st[N]) {
    static const TwiddleTable<N/2> table; 
    #pragma HLS ARRAY_PARTITION variable=table.re complete
    #pragma HLS ARRAY_PARTITION variable=table.im complete
    
    int bflySize = 1 << stage; 
    int bflyStep = bflySize >> 1; 

    for (int b = 0; b < N / 2; b++) {
        #pragma HLS PIPELINE II=1
        #pragma HLS UNROLL factor=UF 
        #pragma HLS LATENCY min=12
        
        int k = b % bflyStep;
        int m = (b / bflyStep) * bflySize;
        int index = (N >> stage) * k;
        
        RADIX2_BFLY_double_buffer_quarter_CY(data_ld, data_st, m+k, m+k+bflyStep, k>0 && k==bflyStep>>1, k>0, table.re[index], table.im[index]);  
    }
}

void process_input(fft_stream_type in_stream_port, inputVectorType data_0[N]) {
    #pragma HLS PIPELINE II=4
    fft_stream_type local_in = in_stream_port; 
    
    for(int i = 0; i < N; i++) {
        #pragma HLS UNROLL
        int rev_idx = bit_reverse<LOG2_N>(i);
        data_0[rev_idx].re = local_in.data[i].re;
        data_0[rev_idx].im = local_in.data[i].im;
    }
}


void process_output(inputVectorType data_final[N], fft_stream_type &out_stream_port) {
    #pragma HLS PIPELINE II=4
    fft_stream_type local_out;
    
    for(int i = 0; i < N; i++) {
        #pragma HLS UNROLL
        local_out.data[i].re = data_final[i].re;
        local_out.data[i].im = data_final[i].im;
    }
    
    out_stream_port = local_out;
}

void fft_top(fft_stream_type x_i, fft_stream_type &y_o) {
    #pragma HLS INTERFACE axis port=x_i
    #pragma HLS INTERFACE axis port=y_o
    #pragma HLS AGGREGATE variable=x_i compact=bit
    #pragma HLS AGGREGATE variable=y_o compact=bit
    #pragma HLS INTERFACE ap_ctrl_none port=return
    #pragma HLS DATAFLOW
    
    inputVectorType data_0[N], data_1[N], data_2[N];
    inputVectorType data_3[N], data_4[N], data_5[N];

    #pragma HLS array_partition variable=data_0 complete
    #pragma HLS array_partition variable=data_1 complete
    #pragma HLS array_partition variable=data_2 complete
    #pragma HLS array_partition variable=data_3 complete
    #pragma HLS array_partition variable=data_4 complete
    #pragma HLS array_partition variable=data_5 complete

    process_input(x_i, data_0);
    FFT_stage<1>(data_0, data_1);
    FFT_stage<2>(data_1, data_2);
    FFT_stage<3>(data_2, data_3);
    FFT_stage<4>(data_3, data_4);
    FFT_stage<5>(data_4, data_5);
    process_output(data_5, y_o);
}