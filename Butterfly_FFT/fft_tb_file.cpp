/*
 * * Acknowledgment: 
 * Parts of this code were developed with the assistance of Google's Gemini AI.
 * * Reference:
 * Gemini. (3.1 Pro) Google. Accessed: May 17, 2026. [Online]. Available: https://gemini.google.com
 */
 
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>
#include "fft.h" 

int main() {
    fft_stream_type x_i_stream;
    fft_stream_type y_o_stream;

    int error_count = 0;
    
 const double TOLERANCE = 10.0; 

    std::cout << "--- Starting File-Based AXI-Stream DIT-FFT Simulation (N=" << N << ") ---" << std::endl;

    //read input
    std::ifstream fin("fft_input_32_scaled.txt");
    if (!fin.is_open()) {
        std::cerr << "[!] ERROR: Could not open fft_input_32.txt in the simulation directory." << std::endl;
        return 1;
    }

    std::string line;
    int in_idx = 0;
    while (std::getline(fin, line) && in_idx < N) {
        if (line.empty() || line[0] == '#' || line[0] == '[') continue; 

        std::stringstream ss(line);
        double re, im;
        if (ss >> re >> im) {
            x_i_stream.data[in_idx].re = (data_t)re;
            x_i_stream.data[in_idx].im = (data_t)im;
            in_idx++;
        }
    }
    fin.close();
    
    if (in_idx != N) {
        std::cerr << "[!] WARNING: Read " << in_idx << " samples, but N=" << N << std::endl;
    } else {
        std::cout << ">> Successfully read " << in_idx << " input samples." << std::endl;
    }

    fft_top(x_i_stream, y_o_stream);
    std::cout << ">> FFT Execution Complete." << std::endl;

    //read output files and compares
    std::ifstream fout("fft_output_32_scaled.txt");
    if (!fout.is_open()) {
        std::cerr << "[!] ERROR: Could not open fft_output_32.txt in the simulation directory." << std::endl;
        return 1;
    }

    std::cout << "\n==========================================================================================" << std::endl;
    std::cout << " Index | Expected (Re, Im)             | Actual HLS (Re, Im)     | Diff (Re, Im) | Status" << std::endl;
    std::cout << "==========================================================================================" << std::endl;

    int out_idx = 0;
    while (std::getline(fout, line) && out_idx < N) {
        if (line.empty() || line[0] == '#' || line[0] == '[') continue;

        std::stringstream ss(line);
        double exp_re, exp_im;
        
        if (ss >> exp_re >> exp_im) {
            
            //scale file for hardware (bit removal at every stage)
            exp_re = exp_re / N;
            exp_im = exp_im / N;

            double act_re = (double)y_o_stream.data[out_idx].re;
            double act_im = (double)y_o_stream.data[out_idx].im;

            double diff_re = std::abs(exp_re - act_re);
            double diff_im = std::abs(exp_im - act_im);

            //check with tolerance
            bool pass = (diff_re <= TOLERANCE) && (diff_im <= TOLERANCE);
            if (!pass) error_count++;

            std::cout << std::setw(6) << out_idx << " | "
                      << std::setw(11) << std::fixed << std::setprecision(4) << exp_re << ", " 
                      << std::setw(11) << exp_im << " | "
                      << std::setw(8) << act_re << ", " 
                      << std::setw(8) << act_im << " | "
                      << std::setw(6) << diff_re << ", "
                      << std::setw(5) << diff_im << " | "
                      << (pass ? "  PASS" : ">>FAIL") << std::endl;
            out_idx++;
        }
    }
    fout.close();


    std::cout << "\nFile-based Simulation Complete!" << std::endl;
    std::cout << "Total Arithmetic Mismatches Detected: " << error_count << std::endl;
    
    if (error_count == 0) {
        std::cout << ">>> SIMULATION PASSED <<<" << std::endl;
        return 0; 
    } else {
        std::cout << ">>> SIMULATION FAILED <<<" << std::endl;
        return 1; 
    }
}