Project was created using GCC version 4.2.1 on a Mac.

Simple Information Architecture (SIA) was created by one of my professors at the University at Albany as part of an Assembly class. 
An explanation of SIA (provided by the professor) can be found in the SIA.txt file. 

The Assembler (Assembler.c) takes in a .txt file with the appropriate SIA instructions and outputs a binary file. This binary file is then passed to the Virtual Machine (siavmPipeline2.c) for it to run and execute the actual instructions. Note that the VM file has pipeline in its name since it was modified as part of the final project to support pipelining. 

The Assembler folder contains a series of different unit test files for the various SIA instructions, which will be passed to the Assembler. An example of how to run the Assembler is: "./Assembler Add.txt Add.bin", where "Add.bin" is the output binary file produced by the Assembler. Please note that if creating your own test file, the last line needs to have a carriage return at the very end for it to count the last word of the last line. The Assembler will print out each line of input it received when it runs. 

The binary file must then be moved into the same directory as the siavmPipeline2 program so it can be run through the VM. An example for how to run the VM would be: "./siavmPipeline2 Add.bin". Each unit test provided prints out the registers except for the unit test in PrintMemory which will print out all of the VM's memory.

