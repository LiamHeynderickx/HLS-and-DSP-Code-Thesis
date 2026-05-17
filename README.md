# Benchmarking of Vitis HLS
## A Comparative Analysis of HLS against hand-coded RTL for High-Constraint DSP Algorithms

Satellite communication systems rely on high-performance Digital Signal Processing (DSP) algorithms for data modulation and demodulation. These algorithms face strict timing and resource-use constraints to optimize performance while limiting hardware overhead. One of the more demanding functions in terms of speed and resources is the Fast Acquisition Unit (FAU), which enables fast carrier/code frequency and timing detection. Currently, companies such as Antwerp Space rely on hand-coded Register Transfer Level (RTL) designs. While this allows fine-grained control, it requires extensive development time and specialized knowledge. High-Level Synthesis (HLS) offers an alternative by raising the abstraction level and allowing designers to program using widely known languages such as C++. To evaluate how this tool compares to traditional RTL, this thesis benchmarks the viability of HLS, namely AMD’s Vitis HLS, against an industry-standard VHDL baseline for an FAU. 

To accurately benchmark against Antwerp Space’s reference algorithm, the FAU was divided into its five main computational blocks: the correlator, FFT, Sum of Squares, Peak Detector, and Top-4 Peak Tracker. Two distinct architectural methodologies were explored. First, a modular approach was taken where blocks were developed, optimized, and synthesised individually. Second, a monolithic architecture was developed, merging all computational blocks into a single C++ project to allow the HLS software to optimize and synthesise the entire algorithm.  

Results showed that the HLS-generated algorithm matched the VHDL reference functionality and successfully met the timing requirements. While on smaller blocks the HLS had higher resource usage, as the blocks increased in complexity and the entire monolithic algorithm was synthesized, the resource efficiency matched or even outperformed the VHDL reference. Therefore, while HLS loses some fine-grained control over small isolated blocks, it is highly viable for complex algorithms. In the monolithic approach, the compiler gained full data path visibility, and thus the interface management and pipeline scheduling were taken over directly by the Vitis HLS engine, resulting in the best results. This research  thus demonstrated the viability of using Vitis HLS to develop high-performance DSP algorithms.

# Repository Structure
To keep the different architectural explorations isolated and organized, this repository is divided into three branches:

- ModularHLS: Contains the Vitis HLS source files for the modular approach. Each functional block of the Fast Acquisition Unit (FAU) is implemented, optimized, and synthesized as an individual IP.

- MonolithicHLS: Contains the Vitis HLS files for the monolithic approach. The entire FAU algorithm is integrated into a single C++ project.

- ExtraHLS: Contains additional research and alternative implementations not featured in the main body of the thesis (using external libraries and academic FFT architectures).

**Important note regarding testbenches:** > While the C++ testbench files are included, the required input and reference output files are were removed. These data files are proprietary to Antwerp Space and cannot be distributed.

## Acknowledgments & AI Use Disclaimer

Google's Gemini AI was used as an assistive tool during the development of this project in particular to help with:
* **Code:** C++ logic for High-Level Synthesis.
* **Pragma Optimization:** Assisting with the syntax and understanding of Vitis HLS compiler directives (e.g., `#pragma HLS UNROLL`, `#pragma HLS ARRAY_PARTITION`).

All AI-generated code was reviewed, manually modified, and tested through Vitis and Vivado simulation to ensure functional integrity and compliance with the timing constraints.