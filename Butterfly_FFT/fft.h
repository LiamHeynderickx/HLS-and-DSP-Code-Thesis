/*
 * * Acknowledgment: 
 * Parts of this code were developed with the assistance of Google's Gemini AI.
 * * Reference:
 * Gemini. (3.1 Pro) Google. Accessed: May 17, 2026. [Online]. Available: https://gemini.google.com
 */
 
#ifndef FFT_H
#define FFT_H

#include "ap_fixed.h"
#include <cmath>

#define N 32
#define LOG2_N 5

//input data
typedef ap_fixed<16,12, AP_TRN, AP_WRAP> data_t; 

struct inputVectorType {
    data_t re;
    data_t im;
};

//twidles
typedef ap_fixed<16, 10, AP_TRN, AP_WRAP> twiddle_t; 
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


//input and output streams
struct fft_stream_type {
    inputVectorType data[N];
};



void butterfly(inputVectorType& a, inputVectorType& b, twiddleVectorType tw);

void fft_top(fft_stream_type& x_i, fft_stream_type& y_o);

#endif