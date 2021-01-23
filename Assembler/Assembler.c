//Sandeep Bindra
//CSI 404
//Assignment 2: Assembler
/*
 * SIA assembler which takes in a .txt file and outputs a binary file that is meant to be run on the VM.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char *words[5];

/*
Please note the getWords function is my professor's code.
*/

/* Take a string. Split it into different words, putting them in the words array. For example:
 * This is a string
 * Becomes:
 * words[0] = This
 * words[1] = is
 * words[2] = a
 * words[3] = string
 * Only safe when words is big enough...
 */
void getWords(char *string) { 
	printf ("input: %s\n",string);
	int curWord = 0;
	char *cur = string;
	words[curWord] = string;
	while (*cur != 0) {
		if (*cur == '\n' || *cur == '\r') *cur = ' ';
		if (*cur == ' ') {
			*cur = 0; // replace space with NUL
			curWord++;
			words[curWord] = cur+1; // set the start of the next word to the character after this one
		} 
		cur++; 
	} 
		for (int i=0;i<curWord;i++)
			printf ("word %d = %s\n",i,words[i]);
}

// takes a string and returns register number or -1 if the string doesn't start with "r" or "R"
int getRegister (char* string) {
	if (string[0] != 'R' && string[0] != 'r') 
		return -1;
	return atoi(string+1);
}

//Use this function to shift the bits appropriately 
//for any operations that use the 3R instruction format and store the results in the char array, 
//'byte', which really just represents the char array 'bytes' from the main function.
void threeR(int opcode, char *byte) {
	byte[0] = (opcode << 4 | getRegister(words[1]));
	byte[1] = (getRegister(words[2]) << 4 | getRegister(words[3]));
}

//Use this function to shift the bits appropriately for the branch operations 
//that use the br1 instruction format and store the results in the char array, 'byte', which 
//really just represents the char array 'bytes' from the main function.
//The opcode is known to be seven for every branch operation so it is not passed as an argument.
void br1(int branchType, char *byte){
	//offset will be used to represent the number of words moved by the branch operation
	int offset = (atoi(words[3]) / 2);
	byte[0] = (7 << 4 | branchType);
	byte[1] = (getRegister(words[1]) << 4 | getRegister(words[2]));
	//Get the first 8 bits (reading binary from right to left)
	byte[3] = (offset & 255);
	//Get the next 8 bits
	byte[2] = (offset >> 8 & 255);
}

//Use this function to shift the bits appropriately 
//for the branch operations that use the br2 instruction format and store the 
//results in the char array, byte, which really just represents the char array bytes from the main function.
//The opcode is known to be seven for the call and jump operation so it is not passed as an argument.
void br2(int branchType, char *byte){
	//This variable will be used to represent the number of words moved by the call/jump operation
	int offset = (atoi(words[1]) / 2);
	words[2] = words[1];
	byte[0] = (7 << 4 | branchType);
	//Get the first 8 bits of the number (reading binary from right to left).
	byte[3] = (offset & 255);
	//Get the next 8 bits of the number
	byte[2] = (offset >> 8 & 255);
	//Get the last 8 bits
	byte[1] = (offset >> 16 & 255);
}

//Use this function to shift the bits appropriately 
//for the branch operations that use the ls instruction format and store the 
//results in the char array, byte, which really just represents the char array bytes from the main function.
void ls(int opcode, char *byte){
	//This variable will be used to represent the number of words moved by the load/store operation
	int offset = (atoi(words[3]) / 2);
	byte[0] = (opcode << 4 | getRegister(words[1]));
	byte[1] = (getRegister(words[2]) << 4 | offset);
}

//Use this function to shift bits appropriately for the stack operations and
//store the results in the char array, byte, which really just 
//represents the char array bytes from the main function.
//The opcode is known to be 10 for each operation so it is not passed as an argument.
//The parameter "stackOperation" is used to determine whether push, pop, or return is being used.
void stack(int stackOperation, char* byte){
	byte[0] = (10 << 4 | getRegister(words[1]));
	byte[1] = (stackOperation << 4 | 0); 
}

//Use this function to shift bits appropriately for the move and interrupt
//operations and store the results in the char array, byte, which 
//really just represents the char array bytes from the main function.
void move(int opcode, char* byte){
	//The register matters for the move operation and will need to use bitwise OR
	//with the register and the opcode.
	if(opcode == 11){
		byte[0] = opcode << 4 | getRegister(words[2]);
		byte[1] = atoi(words[1]);
	}
	//The register does not matter for the interrupt operation, so we will
	//just use a bitwise OR with the opcode and 0. The integer passed does matter and 
	//will be stored in the byte array.
	else{
		byte[0] = opcode << 4 | 0;
		byte[1] = atoi(words[2]);
	}
	
}

// Figure out from the first word which operation we are doing and do it...
int assembleLine(char *string, char *bytes) {
	getWords(string);

	if (strcmp(words[0] ,"add") == 0 || strcmp(words[0] ,"Add") == 0) {
		bytes[0] = (1 << 4) | getRegister(words[1]);
		bytes[1] = (getRegister(words[2]) << 4) | getRegister(words[3]);
		return 2;
	}

	//Check if the first word is "and"
	else if(strcmp(words[0], "and") == 0 || strcmp(words[0], "And") == 0) {
		threeR(2, bytes);
		return 2;
	} //end if statement for "and"

	//Check if the first word is "divide"
	else if(strcmp(words[0], "divide") == 0 || strcmp(words[0], "Divide") == 0){
		threeR(3, bytes);
		return 2;
	}

	//Check if the first word is "halt"
	else if(strcmp(words[0], "halt") == 0 || strcmp(words[0], "Halt") == 0){
		threeR(0, bytes);
		return 2;
	}

	//Check if the first word is multiply
	else if(strcmp(words[0], "multiply") == 0 || strcmp(words[0], "Multiply") == 0){
		threeR(4, bytes);
		return 2;
	}

	//Check if the first word is "or"
	else if(strcmp(words[0], "or") == 0 || strcmp(words[0], "Or") == 0){
		threeR(6, bytes);
		return 2;
	}

	//Check if the first word is subtract
	else if(strcmp(words[0], "subtract") == 0 || strcmp(words[0], "Subtract") == 0){
		threeR(5, bytes);
		return 2;
	}

	//Following operations use the br1 instruction format and will require the "br1" function

	//Check if the first word is "branchIfLess"
	else if (strcmp(words[0], "branchIfLess") == 0 || strcmp(words[0], "BranchIfLess") == 0){
		br1(0, bytes);
		return 4;
	}

	//Check if the first word is "branchIfLessOrEqual"
	else if (strcmp(words[0], "branchIfLessOrEqual") == 0 || strcmp(words[0], "BranchIfLessOrEqual") == 0 ){
		br1(1, bytes);
		return 4;
	}

	//Check if the first word is "branchIfEqual"
	else if (strcmp(words[0], "branchIfEqual") == 0 || strcmp(words[0], "BranchIfEqual") == 0){
		br1(2, bytes);
		return 4;
	}

	//Check if the first word is "branchIfNotEqual"
	else if (strcmp(words[0], "branchIfNotEqual") == 0 || strcmp(words[0], "BranchIfNotEqual") == 0){
		br1(3, bytes);
		return 4;
	}

	//Check if the first word is "branchIfGreater"
	else if (strcmp(words[0], "branchIfGreater") == 0 || strcmp(words[0], "BranchIfGreater") == 0){
		br1(4, bytes);
		return 4;
	}

	//Check if the first word is "branchIfGreaterOrEqual"
	else if (strcmp(words[0], "branchIfGreaterOrEqual") == 0 || strcmp(words[0], "BranchIfGreaterOrEqual") == 0){
		br1(5, bytes);
		return 4;
	}

	//Following operations use the br2 instruction format and will require the "br2" function

	//Check if the first word is "call"
	else if(strcmp(words[0], "call") == 0 || strcmp(words[0], "Call") == 0){
		br2(6, bytes);
		return 4;
	}

	//Check if the first word is "jump"
	else if(strcmp(words[0], "jump") == 0 || strcmp(words[0], "Jump") == 0){
		br2(7, bytes);
		return 4;
	}

	//Following operations use the ls instruction format and will require the "ls" function

	//Check if the first word is "load"
	else if(strcmp(words[0], "load") == 0 || strcmp(words[0], "Load") == 0){
		ls(8, bytes);
		return 2;
	}

	//Check if the first word is "store"
	else if (strcmp(words[0], "store") == 0 || strcmp(words[0], "Store") == 0){
		ls(9, bytes);
		return 2;
	}

	//Following operations use the Stack instruction format and will require the "stack" function

	//Check if the first word is "pop" or "Pop".
	//"pop" is represented by an 8 so this will be passed
	//as the argument for the stackOperation parameter for the "stack" function
	else if (strcmp(words[0], "pop") == 0 || strcmp(words[0], "Pop") == 0){
		stack(8, bytes);
		return 2;
	}

	//Check if the first word is "push" or "Push".
	//"push" is represented by a 4 so this will be passed
	//as the argument for the stackOperation parameter for the "stack" function
	else if (strcmp(words[0], "push") == 0 || strcmp(words[0], "Push") == 0){
		stack(4, bytes);
		return 2;
	}

	//Check if the first word is "return" or "Return".
	//"return" is represented by a 0 so this will be passed
	//as the value for the stackOperation parameter for the "stack" function
	else if (strcmp(words[0], "return") == 0 || strcmp(words[0], "Return") == 0) {
		stack(0, bytes);
		return 2;
	}

	//Following operations use the move instruction format and will require the "move" function

	//Check if the first word is "move" or "Move"
	//"move" is repsented with an opcode of 11 so this will be passed
	//as the argument for the opcode parameter in the "move" function. 
	else if(strcmp(words[0], "move") == 0 || strcmp(words[0], "Move") == 0){
		move(11, bytes);
		return 2;
	}

	//Check if the first word is "interrupt" or "Interrupt"
	//"interrupt" is repsented with an opcode of 12 so this will be passed
	//as the argument for the opcode parameter in the "move" function. 
	else if(strcmp(words[0], "interrupt") == 0 || strcmp(words[0], "Interrupt") == 0){
		move(12, bytes);
		return 2;
	}

	else{
		printf("\nInvalid opcode.");
		return 0;
	}

}

/*
* Please note the main function is my professor's code.
*/
int main (int argc, char **argv)  {
	if (argc != 3)  {printf ("assemble inputFile outputFile\n"); exit(1); }
	FILE *in = fopen(argv[1],"r");
	if (in == NULL) { printf ("unable to open input file\n"); exit(1); }
	FILE *out = fopen(argv[2],"wb");
	if (out == NULL) { printf ("unable to open output file\n"); exit(1); }

	char bytes[4], inputLine[100];
	while (!feof(in)) {
		if (NULL != fgets(inputLine,100,in)) {
			int outSize = assembleLine(inputLine,bytes);
			fwrite(bytes,outSize,1,out);
		}
	}
	fclose(in);
	fclose(out);
}