#ifndef PEAK_DETECTOR_H
#define PEAK_DETECTOR_H

#include "fft.h"
#include "ap_int.h"
#include "hls_stream.h"

typedef ap_ufixed<33, 25, AP_TRN, AP_WRAP> mag_sq_t;

struct mag_array_type {
    mag_sq_t data[N];
};

typedef ap_uint<16> peak_val_t;

struct peak_packet {
    ap_uint<LOG2_N> bin;
    peak_val_t val;
};

typedef hls::stream<mag_array_type> mag_stream_type;
typedef hls::stream<peak_packet> peak_stream_type;

void peak_detector(mag_stream_type& mag_in, peak_stream_type& peak_out);

#endif