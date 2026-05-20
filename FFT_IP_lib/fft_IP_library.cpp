/*
 * * Acknowledgment: 
 * Parts of this code were developed with the assistance of Google's Gemini AI.
 * * Reference:
 * Gemini. (3.1 Pro) Google. Accessed: May 17, 2026. [Online]. Available: https://gemini.google.com
 */
 
#include "hls_fft.h"
#include <ap_fixed.h>
#include <complex>

static const int N = 32;

// 1. YOUR CUSTOM TYPES
typedef ap_fixed<16, 12, AP_TRN, AP_WRAP> data_t; 

struct inputVectorType {
    data_t re;
    data_t im;
};

struct fft_stream_type {
    inputVectorType data[N];
};

// 2. THE INTERNAL XILINX TYPE
// We define a std::complex version just for the black-box to use
// 2. THE INTERNAL XILINX TYPE
// The IP core FORCES the type to ap_fixed<16, 1>. 
typedef ap_fixed<16, 1> fft_ip_type;
using T_internal = std::complex<fft_ip_type>;
// 3. FFT CONFIGURATION
struct fft_config_traits : hls::ip_fft::params_t {
    static const unsigned ordering_opt = hls::ip_fft::natural_order;
    static const unsigned max_nfft = 5; 
    static const unsigned input_width = 16;
    static const unsigned output_width = 16;
    static const unsigned phase_factor_width = 16; 
};

typedef hls::ip_fft::config_t<fft_config_traits> config_t;
typedef hls::ip_fft::status_t<fft_config_traits> status_t;


// 4. TOP LEVEL FUNCTION
void fft_standard_32(fft_stream_type &frame_in, bool valid_in, fft_stream_type &bins_out, bool &valid_out) {
    #pragma HLS INTERFACE ap_ctrl_none port=return
    #pragma HLS INTERFACE axis port=frame_in
    #pragma HLS AGGREGATE variable=frame_in compact=bit 
    #pragma HLS INTERFACE axis port=bins_out
    #pragma HLS AGGREGATE variable=bins_out compact=bit

    #pragma HLS DATAFLOW

    hls::stream<T_internal> fft_in;
    hls::stream<T_internal> fft_out;
    hls::stream<config_t> conf;
    hls::stream<status_t> stat;

    valid_out = false;

    if (valid_in) {
        fft_stream_type local_frame = frame_in;

        config_t dummy_conf;
        dummy_conf.setDir(1); 
        conf.write(dummy_conf);
        
        for (int i = 0; i < N; i++) {
            #pragma HLS PIPELINE II=1
            
            fft_ip_type ip_re, ip_im;
            
            ip_re.range(15, 0) = local_frame.data[i].re.range(15, 0);
            ip_im.range(15, 0) = local_frame.data[i].im.range(15, 0);
            
            T_internal val(ip_re, ip_im);
            fft_in.write(val);
        }
    }

    hls::fft<fft_config_traits>(fft_in, fft_out, stat, conf);

    if (!fft_out.empty()) {
        status_t dummy_stat = stat.read(); 
        
        fft_stream_type local_bins;
        #pragma HLS ARRAY_PARTITION variable=local_bins.data complete
        
        for (int i = 0; i < N; i++) {
            #pragma HLS PIPELINE II=1
            
            T_internal val = fft_out.read();
            
            local_bins.data[i].re.range(15, 0) = val.real().range(15, 0);
            local_bins.data[i].im.range(15, 0) = val.imag().range(15, 0);
        }
        
        bins_out = local_bins;
        valid_out = true;
    }
}