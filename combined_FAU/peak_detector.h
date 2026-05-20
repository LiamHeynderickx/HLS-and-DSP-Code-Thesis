#ifndef PEAK_DETECTOR_H
#define PEAK_DETECTOR_H

#include "fft.h"
#include "dsp_system.h" 
#include "ap_int.h"
#include "hls_stream.h"

void peak_detector(mag_stream_type& mag_in, ap_uint<LOG2_N>& peak_idx, mag_sq_t& peak_val);

#endif