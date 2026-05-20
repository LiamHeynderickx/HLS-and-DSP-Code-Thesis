/*
 * * Acknowledgment: 
 * Parts of this code were developed with the assistance of Google's Gemini AI.
 * * Reference:
 * Gemini. (3.1 Pro) Google. Accessed: May 17, 2026. [Online]. Available: https://gemini.google.com
 */
 
#include "dsp_system.h"

void top_peak_memory(
    peak_stream_type& peak_in,      
    top_4_stream_type& peak_out_stream 
) {
    #pragma HLS INLINE 

    static peak_record max_peaks[4];
    #pragma HLS ARRAY_PARTITION variable=max_peaks type=complete
    
    static ap_uint<32> timer = 0;

    peak_packet curr = peak_in.read(); 

    // Flushes every Interrupt Pertiod
    if (timer >= INT_PERIOD - 1) { //packs top4 peaks every period and outputs
        
        top_4_packet output_data;
        for (int i = 0; i < 4; i++) {
            #pragma HLS UNROLL
            output_data.peaks[i] = max_peaks[i];
            max_peaks[i].val = 0;
            max_peaks[i].bin = 0;
            max_peaks[i].count = 0;
        }
        
        peak_out_stream.write(output_data);
        
        max_peaks[0].val = curr.val;
        max_peaks[0].bin = curr.bin;
        max_peaks[0].count = 0; 
        
        timer = 1; 
        
    } else { // if no flush happens, the incomming peak is checked against those stored in registers
        peak_record p0 = max_peaks[0];
        peak_record p1 = max_peaks[1];
        peak_record p2 = max_peaks[2];
        peak_record p3 = max_peaks[3];

        //Insertion Sort logic:

        if (curr.val > p0.val) {
            max_peaks[0].val = curr.val; max_peaks[0].bin = curr.bin; max_peaks[0].count = timer / 2;
            max_peaks[1] = p0; max_peaks[2] = p1; max_peaks[3] = p2;
        } else if (curr.val > p1.val) {
            max_peaks[1].val = curr.val; max_peaks[1].bin = curr.bin; max_peaks[1].count = timer / 2;
            max_peaks[2] = p1; max_peaks[3] = p2;
        } else if (curr.val > p2.val) {
            max_peaks[2].val = curr.val; max_peaks[2].bin = curr.bin; max_peaks[2].count = timer / 2;
            max_peaks[3] = p2;
        } else if (curr.val > p3.val) {
            max_peaks[3].val = curr.val; max_peaks[3].bin = curr.bin; max_peaks[3].count = timer / 2;
        }
        
        timer++;
    }
}