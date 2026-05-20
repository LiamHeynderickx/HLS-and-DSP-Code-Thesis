#ifndef DSP_SYSTEM_H
#define DSP_SYSTEM_H

#include "fft.h" // Brings in your N, LOG2_N, fft_stream_type definitions
#include "ap_int.h"

// Define type for squared magnitude 
typedef ap_ufixed<33, 25, AP_TRN, AP_WRAP> mag_sq_t;

// Pack the 32 squared magnitudes for the AXI bus connecting SoS to Peak Detector
struct mag_stream_type {
    mag_sq_t data[N];
};

#endif