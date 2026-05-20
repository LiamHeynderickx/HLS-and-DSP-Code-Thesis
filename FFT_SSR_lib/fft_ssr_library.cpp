/*
 * * Acknowledgment: 
 * Parts of this code were developed with the assistance of Google's Gemini AI.
 * * Reference:
 * Gemini. (3.1 Pro) Google. Accessed: May 17, 2026. [Online]. Available: https://gemini.google.com
 */
 
#include <ap_fixed.h>
#include <hls_stream.h>
#include <complex>
#include "vt_fft.hpp"

using namespace xf::dsp::fft;

//for 256 fft
static const int N = 256; 
static const int R = 64;  //256 samples/4 cycles = 64 parallel streams

struct fftParams : ssr_fft_default_params {
    static const int N = ::N;
    static const int R = ::R;
    static const scaling_mode_enum scaling_mode = SSR_FFT_GROW_TO_MAX_WIDTH;
    static const int twiddle_table_word_length = 14;
    static const int twiddle_table_intger_part_length = 2;
};

using T_in  = std::complex<ap_fixed<16, 8>>; 
typedef typename ssr_fft_output_type<fftParams, T_in>::t_ssr_fft_out T_out;

struct vectorized_frame {
    T_in data[N];
};

void fft_ssr_256(vectorized_frame &frame_in, bool valid_in, T_out bins_out[N], bool &valid_out) {
    #pragma HLS INTERFACE ap_ctrl_none port=return
    #pragma HLS INTERFACE axis port=frame_in
    #pragma HLS AGGREGATE variable=frame_in compact=bit 
    #pragma HLS INTERFACE axis port=bins_out
    #pragma HLS AGGREGATE variable=bins_out compact=bit

    static hls::stream<T_in> fft_in[R];
    static hls::stream<T_out> fft_out[R];
    #pragma HLS STREAM variable=fft_in depth=4
    #pragma HLS STREAM variable=fft_out depth=4

        valid_out = false;

        if (valid_in) {
            vectorized_frame local_frame = frame_in;
            
            for (int beat = 0; beat < N/R; beat++) {
                #pragma HLS PIPELINE II=1
                for (int lane = 0; lane < R; lane++) {
                    #pragma HLS UNROLL
                    fft_in[lane].write(local_frame.data[beat * R + lane]);
                }
            }
        }

        xf::dsp::fft::fft<fftParams, 0>(fft_in, fft_out);

        if (!fft_out[0].empty()) {
            
            T_out local_bins[N];
            #pragma HLS ARRAY_PARTITION variable=local_bins type=complete
            
            for (int beat = 0; beat < N/R; beat++) {
                #pragma HLS PIPELINE II=1
                for (int lane = 0; lane < R; lane++) {
                    #pragma HLS UNROLL
                    local_bins[beat * R + lane] = fft_out[lane].read();
                }
            }
            
            for (int i = 0; i < N; i++) {
                #pragma HLS UNROLL
                bins_out[i] = local_bins[i];
            }
            
            valid_out = true;
        }
    }