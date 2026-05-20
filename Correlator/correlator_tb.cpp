/*
 * * Acknowledgment: 
 * Parts of this code were developed with the assistance of Google's Gemini AI.
 * * Reference:
 * Gemini. (3.1 Pro) Google. Accessed: May 17, 2026. [Online]. Available: https://gemini.google.com
 */

#define AP_INT_MAX_W 4096
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip> 
#include "correlator.h" 
#include <hls_stream.h>

int main() {
    std::cout << "--- STARTING C++ DIAGNOSTIC SIMULATION ---" << std::endl;

    // Notice we removed the rf_in_stream here!
    hls::stream<ap_uint<1024> > fft_out_stream("fft_out");
    
    ap_uint<2> test_mode = 0;

    // =======================================================
    // PHASE 1: LOAD THE CODE SERIALLY
    // =======================================================
    std::cout << "Loading PRN code serially into the IP..." << std::endl;
    std::ifstream file_code("correlator_code_in.txt");
    if (!file_code.is_open()) {
        std::cerr << "ERROR: Could not open correlator_code_in.txt" << std::endl;
        return 1;
    }
    
    std::string line;
    int bit_idx = 0;
    int total_code_bits = NUMBER_OF_SUBCOR * CHIPS_PER_SUBCOR; // 1024

    while (std::getline(file_code, line) && bit_idx < total_code_bits) {
        if (line.empty() || line[0] == '#') continue; 
        
        ap_uint<1> c_bit = (line[0] == '1');
        ap_uint<1> c_valid = 1;
        ap_uint<1> c_last = (bit_idx == total_code_bits - 1) ? 1 : 0; // Trigger snapshot on the last bit

        // Call the IP to shift one bit in. 
        // We set all RF inputs to 0, and crucially, sample_valid_i = 0.
        // Because sample_valid_i is 0, the IP won't try to output any FFT data!
        subcorrelator_top(
            0, 0, 0, 0, 0,           // ni, nq, pi, pq, sample_valid
            c_bit, c_valid, c_last,  // code pins
            test_mode, 
            fft_out_stream
        );

        // Dummy fft_out_stream.read() calls have been safely removed!

        bit_idx++;
    }
    file_code.close();
    std::cout << "Successfully loaded " << bit_idx << " code bits." << std::endl;


    // =======================================================
    // PHASE 2: STREAM THE RF DATA
    // =======================================================
    std::ifstream file_ni("correlator_sample_ni_i.txt");
    std::ifstream file_nq("correlator_sample_nq_i.txt");
    std::ifstream file_pi("correlator_sample_pi_i.txt");
    std::ifstream file_pq("correlator_sample_pq_i.txt");

    std::ofstream csim_dump_n("csim_dump_n.txt"); 
    std::ofstream csim_dump_p("csim_dump_p.txt"); 

    if (!file_ni.is_open() || !csim_dump_n.is_open()) {
        std::cerr << "ERROR: Could not open RF sample text files!" << std::endl;
        return 1;
    }

    std::string str_ni, str_nq, str_pi, str_pq;
    int cycle = 0;
    
    std::cout << "Streaming data through C++ IP and dumping raw frames..." << std::endl;

    while (std::getline(file_ni, str_ni) && 
           std::getline(file_nq, str_nq) && 
           std::getline(file_pi, str_pi) && 
           std::getline(file_pq, str_pq)) 
    {
        if (str_ni.empty() || str_ni[0] == '#') continue;

        // Parse individual bits instead of packing them into a 4-bit struct
        ap_uint<1> curr_ni = (str_ni[0] == '1');
        ap_uint<1> curr_nq = (str_nq[0] == '1');
        ap_uint<1> curr_pi = (str_pi[0] == '1');
        ap_uint<1> curr_pq = (str_pq[0] == '1');
        ap_uint<1> sample_valid = 1;

        // Call the IP. RF inputs are valid, and Code inputs are tied to 0.
        subcorrelator_top(
            curr_ni, curr_nq, curr_pi, curr_pq, sample_valid, // RF Pins
            0, 0, 0,                                          // Code pins (idle)
            test_mode, 
            fft_out_stream
        );
        
        // Because sample_valid was 1, the IP generated an output for us to read!
        ap_uint<1024> out_frame_n = fft_out_stream.read();
        ap_uint<1024> out_frame_p = fft_out_stream.read();

        csim_dump_n << "--- FRAME " << (cycle * 2 + 1) << " ---\n";
        for (int ch = 0; ch < 16; ch++) {
            ap_int<16> raw_re = out_frame_n.range(ch*32 + 15, ch*32);
            ap_int<16> raw_im = out_frame_n.range(ch*32 + 31, ch*32 + 16);
            
            int re = (int)raw_re >> 4;
            int im = (int)raw_im >> 4;
            
            csim_dump_n << std::setw(5) << re << " " << std::setw(5) << im << " ";
        }
        csim_dump_n << "\n";
        
        ap_int<16> raw_ch0_re = out_frame_n.range(15, 0);
        ap_int<16> raw_ch0_im = out_frame_n.range(31, 16);
        ap_int<16> raw_ch1_re = out_frame_n.range(47, 32);
        ap_int<16> raw_ch1_im = out_frame_n.range(63, 48);
        ap_int<16> raw_ch2_re = out_frame_n.range(79, 64);

        int test_ch0_re = (int)raw_ch0_re >> 4;
        int test_ch0_im = (int)raw_ch0_im >> 4;
        int test_ch1_re = (int)raw_ch1_re >> 4;
        int test_ch1_im = (int)raw_ch1_im >> 4;
        int test_ch2_re = (int)raw_ch2_re >> 4;

        if (test_ch0_re == -2 && test_ch0_im == 5 && 
            test_ch1_re == -2 && test_ch1_im == -2 && 
            test_ch2_re == -8) {
            
            std::cerr << "\n---> SUCCESS: MATCH FOUND AT FRAME " << (cycle * 2 + 1) << " <---" << std::endl;
        }

        cycle++;
    }

    file_ni.close(); file_nq.close(); file_pi.close(); file_pq.close(); 
    csim_dump_n.close(); csim_dump_p.close();

    std::cout << "\n--- C-SIMULATION COMPLETE ---" << std::endl;
    std::cout << "Processed " << cycle << " samples." << std::endl;
    std::cout << "Check 'csim_dump_n.txt' to see what the C++ math actually generated." << std::endl;

    return 0; 
}