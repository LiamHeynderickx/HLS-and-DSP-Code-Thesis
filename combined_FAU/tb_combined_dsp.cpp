/*
 * * Acknowledgment: 
 * Parts of this code were developed with the assistance of Google's Gemini AI.
 * * Reference:
 * Gemini. (3.1 Pro) Google. Accessed: May 17, 2026. [Online]. Available: https://gemini.google.com
 */

#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <sstream>
#include <cctype>
#include "dsp_system.h"

int main() {
    std::cout << "--- STARTING VHDL-EQUIVALENT C++ SIMULATION ---" << std::endl;

    top_4_stream_type peak_out_stream("peak_out");
    
    ap_uint<2> test_mode = 0; 

    std::ifstream file_code("code.txt");
    if (!file_code.is_open()) {
        std::cerr << "ERROR: Could not open code.txt!" << std::endl;
        return 1;
    }
    
    std::string line_code;
    int bit_idx = 0;
    std::cout << "Loading PRN Code Serially..." << std::endl;
    
    // We need a dummy struct to safely catch the output
    fft_stream_type dummy_fft;

    while (std::getline(file_code, line_code) && bit_idx < CODE_LENGTH) {
        // Trim leading spaces
        int start = 0;
        while (start < line_code.length() && std::isspace(line_code[start])) start++;
        if (start < line_code.length()) line_code = line_code.substr(start);

        // Skip comments and empty lines
        if (line_code.empty() || line_code[0] == '#') continue; 
        
        ap_uint<1> c_bit = (line_code[0] == '1');
        ap_uint<1> c_valid = 1;
        ap_uint<1> c_last = (bit_idx == CODE_LENGTH - 1) ? 1 : 0;

        integrated_correlator(
            0, 0, 0, 0, 0,   
            c_bit, c_valid, c_last, 
            test_mode,
            dummy_fft  
        );
        
        bit_idx++;
    }
    file_code.close();
    std::cout << "Successfully loaded " << bit_idx << " PRN Code bits." << std::endl;


    std::ifstream file_in("input.txt");
    if (!file_in.is_open()) {
        std::cerr << "ERROR: Could not open input.txt!" << std::endl;
        return 1;
    }

    // Output File mimicking fau_peaks.txt
    std::ofstream file_out_peaks("csim_fau_peaks.txt");

    int val_ni, val_nq, val_pi, val_pq;
    int sample_count = 0;
    int interrupt_count = 0;

    std::cout << "Streaming data through IP..." << std::endl;

    std::string line_in;
    
    while (std::getline(file_in, line_in)) {
        
        // Trim leading spaces
        int start = 0;
        while (start < line_in.length() && std::isspace(line_in[start])) start++;
        if (start < line_in.length()) line_in = line_in.substr(start);

        // Skip empty lines or comments starting with '#'
        if (line_in.empty() || line_in[0] == '#') {
            continue;
        }

        // Replace any potential commas with spaces (to be safe)
        for (char& c : line_in) {
            if (c == ',') c = ' ';
        }

        // Extract the 4 integers safely
        std::stringstream ss(line_in);
        if (ss >> val_ni >> val_nq >> val_pi >> val_pq) {
            
            // Map to bits: negative numbers become 1, positive become 0
            ap_uint<1> curr_ni = (val_ni < 0) ? 1 : 0;
            ap_uint<1> curr_nq = (val_nq < 0) ? 1 : 0;
            ap_uint<1> curr_pi = (val_pi < 0) ? 1 : 0;
            ap_uint<1> curr_pq = (val_pq < 0) ? 1 : 0;


            combined_dsp_top(
                curr_ni, curr_nq, curr_pi, curr_pq, 1,
                0, 0, 0,                              
                test_mode, 
                peak_out_stream
            );

            combined_dsp_top(
                curr_ni, curr_nq, curr_pi, curr_pq, 0, 
                0, 0, 0,                               
                test_mode, 
                peak_out_stream
            );

            sample_count++;

            // check for interrupt
            if (!peak_out_stream.empty()) {
                interrupt_count++;
                top_4_packet result = peak_out_stream.read();
                
                std::cout << "\n--- INTERRUPT " << interrupt_count << " (Sample " << sample_count << ") ---" << std::endl;
                
                file_out_peaks << "# INTERRUPT " << interrupt_count << " at sample " << sample_count << "\n";
                file_out_peaks << "# signals = {'epoch_samples', 'bin', 'power'}\n";
                
                // Print out the 4 highest peaks
                std::cout << "--- PRINTING PEAKS ---" << std::endl;
                for (int i = 0; i < 4; i++) {
                    int count = result.peaks[i].count.to_uint(); 
                    int bin   = result.peaks[i].bin.to_int(); 
                    
                    if (bin >= 16) {
                        bin = bin - 32;
                    }

                    double val = result.peaks[i].val.to_double();
                    
                    std::cout << "Peak Rank " << i << ": (cnt, bin, val) = (" 
                              << count << ", " << bin << ", " << val << ")" << std::endl;
                    
                    file_out_peaks << "   " << count << "\t" << bin << "\t" << val << "\n";
                }
            }
        }
    }

    file_in.close();
    file_out_peaks.close();

    std::cout << "\n======================================" << std::endl;
    std::cout << "Processed " << sample_count << " total samples." << std::endl;
    std::cout << "Verified " << interrupt_count << " Peak Memory Latches." << std::endl;
    std::cout << "Simulation Complete. Results saved to 'csim_fau_peaks.txt'." << std::endl;
    
    return 0;
}