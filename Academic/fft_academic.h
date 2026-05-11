#ifndef FFT_H
#define FFT_H

#include "ap_fixed.h"
#include <cmath>

//configuration
const int N = 32;           
const int LOG2_N = 5;       
const int UF = 4;           


typedef ap_fixed<16, 12, AP_TRN, AP_WRAP> data_t; 

struct inputVectorType {
    data_t re;
    data_t im;
};

struct fft_stream_type {
    inputVectorType data[N];
};

typedef ap_fixed<16, 2, AP_TRN, AP_WRAP> twiddle_t; 

struct twiddleVectorType {
    twiddle_t re;
    twiddle_t im;
};

template <int SIZE>
struct TwiddleTable {
    twiddle_t re[SIZE]; 
    twiddle_t im[SIZE]; 

    TwiddleTable() {
        for (int i = 0; i < SIZE; i++) {
            double angle = -2.0 * M_PI * i / (SIZE * 2); 
            re[i] = (twiddle_t)std::cos(angle);
            im[i] = (twiddle_t)std::sin(angle);
        }
    }
};

void fft_top(fft_stream_type x_i, fft_stream_type &y_o);

#endif