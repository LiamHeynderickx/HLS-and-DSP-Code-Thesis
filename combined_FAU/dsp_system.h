/*
 * * Acknowledgment: 
 * Parts of this code were developed with the assistance of Google's Gemini AI.
 * * Reference:
 * Gemini. (3.1 Pro) Google. Accessed: May 17, 2026. [Online]. Available: https://gemini.google.com
 */
 
#ifndef DSP_SYSTEM_H
#define DSP_SYSTEM_H

#ifndef AP_INT_MAX_W
#define AP_INT_MAX_W 4096 //defines high max width to override default 
#endif

// Standard Xilinx Includes
#include "ap_int.h"
#include "hls_stream.h"

#include "fft.h" 
#include "correlator.h" 

// Type Definitions
typedef ap_ufixed<33, 25, AP_TRN, AP_WRAP> mag_sq_t;

const ap_uint<32> INT_PERIOD = 20000; // 10000 samples * 2 phases

typedef ap_uint<16> peak_val_t;

struct mag_stream_type { //sos type
    mag_sq_t data[N];
};

struct peak_packet { //peak out
    ap_uint<LOG2_N> bin;
    peak_val_t        val;
};

struct peak_record {
    peak_val_t val;
    ap_uint<LOG2_N> bin;
    ap_uint<32> count;
};

struct top_4_packet { //top 4 tracker type
    peak_record peaks[4];
};

// define streams
typedef hls::stream<peak_packet> peak_stream_type;
typedef hls::stream<top_4_packet> top_4_stream_type;


// function definitions

// --- UPDATED SIGNATURE ---
void integrated_correlator(
    ap_uint<1> sample_ni_i,
    ap_uint<1> sample_nq_i,
    ap_uint<1> sample_pi_i,
    ap_uint<1> sample_pq_i,
    ap_uint<1> sample_valid_i,
    ap_uint<1> code_i,          
    ap_uint<1> code_valid_i,    
    ap_uint<1> code_last_i,     
    ap_uint<2> mode_i,
    fft_stream_type &fft_out
);

void sum_of_squares(fft_stream_type& fft_in, mag_stream_type& mag_out);

void peak_detector(mag_stream_type& mag_in, peak_stream_type& peak_out);

void top_peak_memory(
    peak_stream_type& peak_in, 
    top_4_stream_type& peak_out_stream
);

// --- UPDATED SIGNATURE (Master Wrapper) ---
void combined_dsp_top(
    ap_uint<1> sample_ni_i,
    ap_uint<1> sample_nq_i,
    ap_uint<1> sample_pi_i,
    ap_uint<1> sample_pq_i,
    ap_uint<1> sample_valid_i,
    ap_uint<1> code_i,          
    ap_uint<1> code_valid_i,    
    ap_uint<1> code_last_i,     
    ap_uint<2> mode_i,
    top_4_stream_type& peak_out_stream
);

#endif // DSP_SYSTEM_H