# Systems Software Project - 2022/2023
This repository contains the project assignments for the Systems Software course (13E113SS, 13S113SS) at the University of Belgrade, School of Electrical Engineering. The project involves developing tools in the compilation chain and an emulator for an abstract computer system.

## Project Components
* Assembler: A two-pass assembler for a specified processor, converting assembly language programs into machine code.
* Linker: An architecture-independent linker that combines multiple object files generated by the assembler.
* Emulator: An emulator to execute the compiled machine code on a Linux console application.

## Requirements
* The project is implemented in C++ and is designed to run on a Linux operating system.
* The code should be compiled and executed as a console application.

## Usage
# Assembler
To assemble an input file, use the following command:

./assembler -o <output_file> <input_file>

Example:

./assembler -o output.o input.s

# Linker
To link multiple object files into a single executable, use the following command:

./linker -o <executable_file> <input_files>

Example:

./linker -o executable output1.o output2.o

# Emulator
To run the executable file on the emulator, use the following command:

./emulator <executable_file>

Example:

./emulator executable

# File Structure
* assembler/: Contains the source code for the assembler.
* linker/: Contains the source code for the linker.
* emulator/: Contains the source code for the emulator.
* tests/: Contains test programs and scripts.

# Testing

Test scripts and sample programs are provided in the tests directory. 
