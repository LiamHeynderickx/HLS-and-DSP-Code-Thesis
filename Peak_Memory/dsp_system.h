#ifndef DSP_SYSTEM_H
#define DSP_SYSTEM_H

#include <hls_stream.h>
#include <ap_int.h>
#include <ap_fixed.h>

#define N 32
#define LOG2_N 5

typedef ap_ufixed<33, 25, AP_TRN, AP_WRAP> mag_sq_t;

typedef ap_uint<16> peak_val_t;

struct mag_stream_type {
    mag_sq_t data[N];
};

struct peak_packet {
    ap_uint<LOG2_N> bin;
    peak_val_t      val;
};

struct peak_record {
    peak_val_t      val; 
    ap_uint<LOG2_N> bin;
    ap_uint<32>     count;
};

struct top_4_packet {
    peak_record peaks[4];
};

typedef hls::stream<peak_packet> peak_stream_type;
typedef hls::stream<top_4_packet> top_4_stream_type;

void peak_detector(mag_stream_type& mag_in, peak_stream_type& peak_out);

void top_peak_memory(
    peak_stream_type& peak_in,      
    top_4_stream_type& peak_out_stream, 
    bool& interrupt                 
);

#endif