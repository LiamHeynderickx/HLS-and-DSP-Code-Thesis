#  Benchmarking of Vitis HLS
## Monolithic implementation of FAU
This branch contains the files for monolithic implementation of the FAU. The C++, header and testbench files are present. The input and output files for the testebenches were removed as they are proprietary to Antwerp Space.

## Acknowledgments & AI Use Disclaimer

Google's Gemini AI was used as an assistive tool during the development of this project in particular to help with:
* **Code:** C++ logic for High-Level Synthesis.
* **Pragma Optimization:** Assisting with the syntax and understanding of Vitis HLS compiler directives (e.g., `#pragma HLS UNROLL`, `#pragma HLS ARRAY_PARTITION`).

All AI-generated code was reviewed, manually modified, and tested through Vitis and Vivado simulation to ensure functional integrity and compliance with the timing constraints.