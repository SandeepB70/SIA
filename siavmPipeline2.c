//Sandeep Bindra
//CSI 404
//Assignment 4: SIA Virtual Machine with pipeline
/*
 * VM of SIA with pipeline support so the four main functions, fetch, decode, execute, and store, can run in any order.
 * Fetch will read instruction bytes from memory and determine how large the current instruction is.
 * Decode will read from the array of bytes for the current instruction and set up the "OP1" and "OP2" registers.
 * Execute will carry out the instruction and store any results in the "result" register.
 * Store will write the result into the VM's memory or the required register and update the program counter.
 * Double buffering was used to allow the VM to support pipelining so there are duplicates of some components.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

//Represents the memory of the virtual machine
unsigned char memory[1000] = {0};

//Represents the index we are at in the array "memory" during execution of the load function, as well as the size.
size_t memIndex = 0;

//One of these arrays will be loaded up with the current instruction each time the fetch function is executed.
//The array that is used will depend on whether or not that fetched instruction is available for fetch to write to
//which will be indicated by the boolean values. 
//Since the largest instruction in SIA is 4 bytes, the arrays will only hold an instruction that has a 
//maximum of 4 bytes.
char fetchedInstruction1[4] = {0};
char fetchedInstruction2[4] = {0};

//Registers that represents how many bytes the current instruction is. instruction1 will be tied to fetchedInstruction1 and
//instruction2 will be tied to fetchedInstruction2. 
int instruction1 = 0;
int instruction2 = 0;

//The two canFetch variables will be used to indicate whether or not the fetch function can read in the next instruction
//for the VM. A value of true will indicate that fetch can get the next instruction to be executed, while false, means it
//cannot and should check the other canFetch variable. canFetch1 will initially be set to true so the first fetch that is run
//can begin reading in an instruction. canFetch1 will be tied to fetchedInstruction1 and canFetch2 will be tied to 
//fetchedInstruction2.
bool canFetch1 = true;
bool canFetch2 = false;

//The two canDecode variables will be used to indicate whether or not the decode function can decode the next instruction
//that has been output from fetch. A value of true will indicate an instruction is available to be decoded and a value of false
//indicates decode should check the other variable to check if there is an instruction available to be decoded. Both 
//variables will be set to false as their will be no instruction to be read initially. canDecode1 will be tied to 
//fetchedInstruction1 and canDecode2 will be tied to fetchedInstruction2.
bool canDecode1 = false;
bool canDecode2 = false;

//These registers represent the operands that a function will be performed on (such as addition for example). 
//There will be two pairs of OP1 and OP2, with the first pair tied to fetchedInstruction1 and the second pair
//tied to fetchedInstruction2.
char firstOP1 = 0;
char firstOP2 = 0;
int secondOP1 = 0;
int secondOP2 = 0; 

//Will be used to indicate if the decode function can write to the OP1 and OP2 registers with decodeFirstOP being
//tied to the first pair of the OP registers and decodeSecondOP being tied to the second. decodeFirstOP will be set
//to false until the first instruction has been decoded because once it has been decoded, 
//it will need to access the first pair of OP registers when filling the pipeline. 
bool decodeFirstOP = false;
bool decodeSecondOP = false;

//Will be used to indicate if the execute function can write to the OP1 and OP2 registers. executeFirstOP will be tied
//to the first pair of OP registers and executeSecondOP will be tied to the second pair. 
bool executeFirstOP = false;
bool executeSecondOP = false;

//Will both be used to represent the results of an executed operation. result1 will be tied to the pathway of fetchedInstruction1
//and result2 will be tied to the pathway of fetchedInstruction2. 
int result1 = 0;
int result2 = 0;

//Indicates if the execute function can write to either of the result registers. executeResult1 is tied to fetchedInstruction1
//and executeResult2 is tied to fetchedInstruction2.
bool executeResult1 = false;
bool executeResult2 = false;

//Indicates if the store function can read from either of the store registers. storeResult1 is tied to pathway of 
//fetchedInstruction1 and storeResult2 is tied to the pathway of fetchedInstruction2. This means the first pair of OP1 and OP2
//registers will be used when store is working with storeResult1 and the second pair will be used with storeResult2.
bool storeResult1 = false;
bool storeResult2 = false;

//Will use this mask to obtain the opcode and register numbers of the current instruction.
unsigned char mask = 15;

//Each member of the array will serve as one of the 16 registers used for sia instructions.
int siaRegister[16] = {0};
//Represents the last register that was written to.
char lastRegisterUsed = 0;
//Used as the program counter
unsigned int PC = 0;
//This variable will be used for the number of words we are moving in the br1 branch operations. 
short br1Words = 0;
//This variable will be used for the address we are moving to in the br2 branch operations. 
int br2Words = 0;

//Will use this as a flag variable to determine if the condition required for the branch to take place is true or false.
bool branchTF = false;

//Used as a helper function for the load operation when we are trying to get a value from a specified address (the parameter) 
//in the memory.
int loadValue(int address)
{
	//The variable that will return the value at the specified address in memory.
	int value = 0;
	value = memory[address];
	//Shift left since these are the top 8 bits of the number.
	value <<= 8;
	for(int i = 0; i < 3; i++)
	{
		address++;
		value |= memory[address];
		//Do not left shift for the last bitwise OR
		if(i != 2)
		{
			value <<= 8;
		}
	}
	return value;
}//end loadValue

//Will be used as a helper function for the store operation where we will store a value from a register ('value') 
//into a specified address of the memory ('address'). Unlike the stackPush operation, we are moving up the memory
//not down the memory like the stack pointer.
void storeInMemory(int value, int address)
{
	//store the top 8 bits of the value first, and then
	//keep taking the next 8 bits until we have stored the 32 bit number.
	for(int i = 3; i >= 0; i--)
	{
		memory[address] = value>>(8*i);
		address++;
	}
}//end storeInMemory


//Carries out the push operation, storing a value into the VM's memory and decrementing the stack pointer to point
//to this value.
void stackPush(int value)
{	

	//Decrement the stack pointer and take in the 32 bit number being passed.
	memory[siaRegister[15]] = value;
	siaRegister[15]--;
	memory[siaRegister[15]] = value>>8;
	siaRegister[15]--;
	memory[siaRegister[15]] = value>>16;
	siaRegister[15]--;
	//Obtain the top 8 bits decrement the stack pointer to point to the next available address.
	memory[siaRegister[15]] = value>>24;
	siaRegister[15]--;
}//end stackPush

//Carries out the pop operation, returning the value at the current location being pointed to in the stack and
//incrementing the stack pointer. 
int stackPop()
{	
	//The stack pointer currently points to a free portion of memory but we want the value stored in the stack.
	siaRegister[15]++;
	unsigned int address = memory[siaRegister[15]];
	//These are the top 8 bits of the value so shift them to the left.
	address <<= 8;
	for(int i = 0; i < 3; i++)
	{
		siaRegister[15]++;
		address |= memory[siaRegister[15]];
		//Do not left shift on the last bitwise OR
		if(i != 2)
		{
			address <<= 8;
		}
	}//end for
	return address;
}//end stackPop

//Used as a helper function for br1TF. It will simply set the br1Words variable to the
//number of words the VM is to move forward or backwards from the current PC. The bufferNum will represent
//which buffer we are in to obtain the correct current instruction.
void setTrueBR1(int bufferNum)
{	
	//Use this as a temp variable for the instruction we are on.
	char fetchedInstruction[4] = {0};
	//If bufferNum is 1, the VM must use the first buffer pathway associated with fetchedInstruction1. 
	//If bufferNum is 2, the VM will use the second buffer pathway associated with fetchedInstruction2.
	if(bufferNum == 1) {
		memcpy(fetchedInstruction, fetchedInstruction1, 4);
	}
	else if(bufferNum == 2) {
		memcpy(fetchedInstruction, fetchedInstruction2, 4);
	}
	else {
		puts("Error with br1 buffer number.");
		puts("Program will terminate.");
		exit(1);
	}
	//The condition required for the branch operation is true.
	branchTF = true;
	//Take the top 8 bits of the address.
	br1Words = fetchedInstruction[2];
	//Left shift the top 8 bits
	br1Words <<= 8;
	//Combine the top 8 bits and bottom 8 bits to get the number of words
	br1Words |= fetchedInstruction[3];
}//End setTrueBR1

//Used to set up execution for call and jump operations. It will obtain the number of words the VM will be moving
//forwards or backwards from the current PC. bufferNum will indicate which buffer is being used.
void setTrueBR2(int bufferNum)
{
	char fetchedInstruction[4] = {0};
	if(bufferNum == 1) {
		memcpy(fetchedInstruction, fetchedInstruction1, 4);
	}
	else if (bufferNum == 2) {
		memcpy(fetchedInstruction, fetchedInstruction2, 4);
	}
	else {
		puts("Error with br2 buffer number.");
		puts("Program will terminate.");
		exit(1);
	}
	//Get the top 8 bits out of the 24 bits used to represents the number of words the VM will be moving 
	//from the current PC.
	br2Words = fetchedInstruction[1];
	//Shift the current bits 8 to the left to obtain the next 8 bits of the number of words specified.
	br2Words <<= 8;
	//Obtain the next 8 bits of the number of words specified.
	br2Words |= fetchedInstruction[2];
	//Shift the current bits 8 to the left to obtain the last 8 bits of the number of words specified.
	br2Words <<= 8;
	//Obtain the last 8 bits of the number of words specified. 
	br2Words |= fetchedInstruction[3];
}//end setTrueBR2

//This function will be used for the br1 instructions with OP1 and OP2 each carrying the value in a siaRegister. 
//It will check which type of branch is being used, carry out the comparison, and then set OP1 to either true or false. 
//The br1Words variable will be set to the number of words the VM needs to move forward or backward if the comparison is true.
//Under the condition the comparison is true, the helper function setTrueBR1 will carry out the appropriate assignments
//as described above.  
void br1TF(int brType, int bufferNum, int firstOP, int secondOP)
{
	int OP1 = firstOP;
	int OP2 = secondOP;

	switch (brType)
	{	
		//The branch operation is branchIfLess and will only be true if OP1 < OP2.
		case 0:
			if(OP1 < OP2) {
				setTrueBR1(bufferNum);
			}
			else {
				branchTF = false;
			}
			break;
		//The branch operation is branchIfLessOrEqual and will only be true if OP1 is less than or equal to OP2.
		case 1:
			if(OP1 <= OP2) {
				setTrueBR1(bufferNum);
			}
			else {
				branchTF = false;
			}
			break;
		//The branch operation is branchIfEqual and will only be true if OP1 is equal to OP2.
		case 2:
			if(OP1 == OP2) {
				setTrueBR1(bufferNum);
			}
			else {
				branchTF = false;
			}
			break;
		//The branch operation is branchIfNotEqual and will only be true if OP1 is not equal to OP2. 
		case 3:
			if(OP1 != OP2) {
				setTrueBR1(bufferNum);
			}
			else {
				branchTF = false;
			}
			break;
		//The branch operation is branchIfGreater and will only be true if OP1 is greater than OP2.
		case 4:
			if(OP1 > OP2) {
				setTrueBR1(bufferNum);
			}
			else{
				branchTF = false;
			}
			break;
		//The branch operation is branchIfGreaterOrEqual and will only be true if OP1 is greater or equal to OP2. 
		case 5:
			if(OP1 >= OP2) {
				setTrueBR1(bufferNum);
			}
			else {
				branchTF = false;
			}
			break;
		default:
			puts("Error br1 branch type.");
			puts("Program will terminate.");
			exit(1);
	}

}//end br1TF

//This function will be used to perform register forwarding in the execute function
void registerForward(int *OP1, int *OP2, char bufferNum) {
	//Check which buffer has the current instruction
	if(bufferNum == 1) {
		//Check the first and second register being used in the instruction and see if it was used 
		//as output in the last instruction executed. If it is, we assign that register's value to the operator 
		//being in the execution.
		if( (fetchedInstruction1[0] & mask) == lastRegisterUsed) {
			*OP1 = siaRegister[lastRegisterUsed];
		}
		if( (fetchedInstruction1[1]>>4 & mask) == lastRegisterUsed){
			*OP2 = siaRegister[lastRegisterUsed];
		}
	}
	else if(bufferNum == 2) {
		if( (fetchedInstruction2[0] & mask) == lastRegisterUsed) {
			*OP1 = siaRegister[lastRegisterUsed];
		}
		if( (fetchedInstruction2[1]>>4 & mask) == lastRegisterUsed){
			*OP2 = siaRegister[lastRegisterUsed];
		}
	}
	else {
		puts("Error with registerForward bufferNum.");
		puts("Program will terminate.");
		exit(1);
	}
}//end checkOP

//Takes in a binary file and reads it into the memory of the VM. The binary file should be 
//the output from an assembler.
int load(char *fileName)
{
	FILE *fptr;

	//Access the binary file created by the Assembler program
	if( (fptr = fopen(fileName, "rb")) == NULL)
	{
		puts("Input file could not be opened.");
		puts("Program will terminate.");
		exit(1);
	}

	//Use this variable as a flag to know when the end of the file has been reached
	unsigned int endOfFile = 2;

	//Keep reading through the file until there is nothing left or until the VM reaches its maximum memory capacity 
	while(endOfFile != 0)
	{
		endOfFile = fread(&memory[memIndex], sizeof(char), 1, fptr);
		memIndex++;

		if (memIndex > 1000)
		{
			return 1;
		}
	}

	fclose(fptr);
	return 0;
}

//fetch will determine the number of bytes the current instruction takes up in memory
//and how far decode will have to read into the memory based on the opcode value.
void fetch(void)
{	
	//Both of these variables will serve as temporary variables simply to obtain the 
	//number of bytes of the current instruction (for the variable "instruction") and to 
	//take in the instruction itself (for the variable "fetchedInstruction"). They will then
	//be used to load up either instruction1 or instruction2 and fetchedInstruction1 or fetchedInstruction2.
	int instruction = 0;
	char fetchedInstruction[4] = {0};

	//For the 3R, load/store, stack, and move instructions, the instruction is 2 bytes.
	if(((memory[PC]>>4 & mask) >= 0 && (memory[PC]>>4 & mask) <= 6) || ( (memory[PC]>>4 & mask) >= 8 && (memory[PC]>>4 & mask) <= 12))
	{
		instruction = 2;
		//Load up the fetchedInstruction array with the current instruction to be executed.
		for(size_t i = 0; i < instruction; i++)
		{
			fetchedInstruction[i] = memory[PC+i];
		}
	}
	//For the branch instructions, the instruction is 4 bytes.
	else if((memory[PC]>>4 & mask) == 7)
	{
		instruction = 4;
		//Load up the fetchedInstruction array with the current instruction to be executed.

		for(size_t i = 0; i < instruction; i++)
		{
			fetchedInstruction[i] = memory[PC+i];
		}
	}
	else
	{
		puts("Opcode error.");
		puts("Program will terminate.");
		exit(1);
	}

	//Check to see which buffers will be loaded up and then load them up. We will also set the appropriate boolean values
	//so that the buffer can be decoded
	if(canFetch1) {
		instruction1 = instruction;
		memcpy(fetchedInstruction1, fetchedInstruction, 4);
		canFetch1 = false;
		canDecode1 = true;
		decodeFirstOP = true;
		//Allow the opposing buffer to begin taking in instructions.
		canFetch2 = true;
	}
	else if(canFetch2){
		instruction2 = instruction;
		memcpy(fetchedInstruction2, fetchedInstruction, 4);
		canFetch2 = false;
		canDecode2 = true;
		decodeSecondOP = true;
		//Allow the opposing buffer to begin taking in instructions.
		canFetch1 = true;
	}
	else {
		puts("Error in pipeline for fetch.");
		puts("Program will terminate.");
		exit(1);
	}

	//If this is a duplicate instruction, skip it.
	if(strcmp(fetchedInstruction1, fetchedInstruction2) == 0){
		return;
	}

}//End fetch

//decode will load up the OP1 and OP2 operators if necessary and determine
//what needs to be done by the execute function
void decode(void)
{
	if(strcmp(fetchedInstruction1, fetchedInstruction2) == 0){
		return;
	}
	//Similar to fetch, these variables serve as temporary variables where fetchedInstruction obtains its value
	//from the corresponding equivalent in the appropriate buffer. OP1 and OP2 will obtain their values through
	//the course of the decode function and these values will be assigned to the OP1 and OP2 registers from the 
	//appropriate buffer.
	char fetchedInstruction[4] = {0};
	int OP1 = 0;
	int OP2 = 0;
	int instruction = 0;

	//Load up fetchedInstruction with the current instruction in the buffer available to be decoded.
	if(canDecode1) {
		memcpy(fetchedInstruction, fetchedInstruction1, 4);
		instruction = instruction1;
	}
	else if(canDecode2) {
		memcpy(fetchedInstruction, fetchedInstruction2, 4);
		instruction = instruction2;
	}
	//This is the first cycle and there is nothing to decode, so the VM does nothing in decode.
	else {
		return;
	}

	//This is a 3R, load/store, stack, or move instruction.
	if(instruction == 2)
	{	
		//This is one of the other 3R instructions. Will load up the OP1 and OP2 registers
		//with the values from the appropriate registers according to the instruction currently being read.
		if( (fetchedInstruction[0]>>4 & mask) >= 1 && (fetchedInstruction[0]>>4 & mask) <= 6)
		{
			OP1 = (siaRegister[ fetchedInstruction[0] & mask ]);
			OP2 = (siaRegister[ fetchedInstruction[1]>>4 & mask ]);
		}

		//This is the load opcode. Register OP1 will hold the register number of the register that will 
		//be loaded with a value. Register OP2 will contain the address we are jumping to.
		else if( (fetchedInstruction[0]>>4 & mask) == 8)
		{
			OP1 = (fetchedInstruction[0] & mask);
			OP2 = ( (siaRegister[ fetchedInstruction[1]>>4 & mask ]) + (fetchedInstruction[1] & mask) );
		}

		//This is the store opcode. Register OP1 will contain the value that will be stored at a particular address
		//and register OP2 will contain that address.
		else if( (fetchedInstruction[0]>>4 & mask) == 9)
		{
			OP1 = (siaRegister[ fetchedInstruction[0] & mask]);
			OP2 = ( (siaRegister[ fetchedInstruction[1]>>4 & mask ]) + (fetchedInstruction[1] & mask) );
		}

		//This is one of the stack instructions. First load up OP1 with the actual stack instruction (push, pop, or return).
		//If OP1 represents the pop or push instruction, then we care about register number, otherwise we ignore it.
		else if( (fetchedInstruction[0]>>4 & mask) == 10)
		{
			OP1 = (fetchedInstruction[1]>>4 & mask);
			//4 represents push so OP2 will store the value we are going to push onto the stack from the specified siaRegister.
			if(OP1 == 4)
			{
				OP2 = (siaRegister[ fetchedInstruction[0] & mask ]);
			}
			//8 represents pop so OP2 will store the siaRegister number we are storing a value from the stack to.
			else if(OP1 == 8)
			{
				OP2 = (fetchedInstruction[0] & mask);
			}
				
		}

		//This is the move operation. Will load up OP1 with the register number and OP2 with the value 
		//that the register will store.
		else if( (fetchedInstruction[0]>>4 & mask) == 11)
		{
			OP1 = fetchedInstruction[0] & mask;
			OP2 = fetchedInstruction[1];
		}

		//This is the interrupt operation. Will load OP1 with the integer value and ignore the register number. 
		//If it is 0, we print registers 0 through 15. If it is 1, we print out all the memory of the VM.
		else if( (fetchedInstruction[0]>>4 & mask) == 12)
		{
			OP1 = fetchedInstruction[1];
		}

	}
	//This is a 4 byte instruction so we know it will be a branch instruction format. 
	else
	{
		if( (fetchedInstruction[0]>>4 & mask) != 7)
		{
			puts("Error with branch instruction opcode.");
			puts("Program will terminate.");
			exit(1);
		}

		//This is one of the actual branch operations (for ex. branchIfLess) and will follow the br1
		//instruction format. OP1 will be set to the value in the first register and 
		//OP2 will be set to the value in the second register.
		else if( (fetchedInstruction[0] & mask) >= 0 && (fetchedInstruction[0] & mask) <= 5)
		{
			OP1 = (siaRegister[ fetchedInstruction[1]>>4 & mask ]);
			OP2 = (siaRegister[ fetchedInstruction[1] & mask ]);
			//Check which buffer we are in and then check if we will branch in that buffer.
			if (canDecode1) {
				br1TF( (fetchedInstruction[0] & mask), 1, OP1, OP2);
			}
			else if (canDecode2) {
				br1TF ( (fetchedInstruction[0] & mask), 2, OP1, OP2);
			}
			else {
				puts("Error with branchTF buffer");
				puts("Program will terminate.");
				exit(1);
			}
		}

		//This is the call operation and follows the br2 instruction format. The variable br2Words will be set to the 
		//number of words the VM is moving forwards or backwards. 
		else if( (fetchedInstruction[0] & mask) == 6 || (fetchedInstruction[0] & mask) == 7)
		{	

			if (canDecode1) {
				//1 indicates first buffer is being used.
				setTrueBR2(1);
			}
			else if(canDecode2) {
				//2 indicates the second buffer is being used.
				setTrueBR2(2);
			}
			else {
				puts("Error with setTrueBR2 decode.");
				puts("Program will terminate.");
				exit(1);
			}
			
		}

		else
		{
			puts("Error with branch operation type.");
			puts("Branch type not between valid range.");
			exit(1);
		}	
	}

	//Update the OP registers in the appropriate buffer and allow fetch to write to the buffer again. decodeFirstOP/decodeSecondOP
	//will be set to false so the VM knows to write to the alternate buffer and execute can carry out its job on the 
	//other buffer.
	if(decodeFirstOP) {
		firstOP1 = OP1;
		firstOP2 = OP2;
		canDecode1 = false;
		decodeFirstOP = false;
		executeFirstOP = true;
		executeResult1 = true;
	}

	else if (decodeSecondOP) {
		secondOP1 = OP1;
		secondOP2 = OP2;
		canDecode2 = false;
		decodeSecondOP = false;
		executeSecondOP = true;
		executeResult2 = true;
	}
	else{
		puts("Error in pipeline while decoding.");
		puts("Program will terminate.");
		exit(1);
	}

}//end decode

//Perform the specified action by the operation of the current instruction 
//and store the outcome in the result register if applicable.
void execute()
{
	//Set up the temporary variables for the appropriate buffers.
	char fetchedInstruction[4] = {0};
	int result = 0;
	int OP1 = 0;
	int OP2 = 0;

	int bufferNum = 0;

	//Assign the value in the buffer to the corresponding temporary variable.
	if (executeFirstOP) {
		memcpy(fetchedInstruction, fetchedInstruction1, 4);
		OP1 = firstOP1;
		OP2 = firstOP2;
		bufferNum = 1;
	}
	else if (executeSecondOP) {
			memcpy(fetchedInstruction, fetchedInstruction2, 4);
			OP1 = secondOP1;
			OP2 = secondOP2;
			bufferNum = 2;
	}
	//This must be the first cycle or execute is not able to carry anything out at this time so the VM will not do anything.
	else {
		return;
	}
	switch(fetchedInstruction[0]>>4 & mask)
	{	
		//The operation is halt, so we will "stop" the CPU (in this case, terminate the program).
		case 0:
			exit(0);
			break;
		//The operation is add so we will add the two values in registers OP1 and OP2 and use the result
		//register to hold the sum. 
		case 1:
			registerForward(&OP1, &OP2, bufferNum);
			result = OP1 + OP2;
			break;
		//The operation is bitwise AND (&) so we will use the two values in registers OP1 and OP2 to execute this
		//and store the result in the result register.
		case 2:
			registerForward(&OP1, &OP2, bufferNum);
			result = OP1 & OP2;
			break;
		//The operation is integer division so we will divide the value in OP1 by the value in OP2 and the store the result
		//in the result register. If there is a decimal portion, it will be discarded.
		case 3:
			registerForward(&OP1, &OP2, bufferNum);	
			result = OP1 / OP2;
			break;
		//The operation is multiplication so we will multiply the value in OP1 by the value in OP2 and store the result in 
		//the result register.
		case 4:
			registerForward(&OP1, &OP2, bufferNum);
			result = OP1 * OP2;
			break;
		//The operation is subtraction so we will subtract the value in OP2 from the value in OP1 and store the result in 
		//the result register.
		case 5:
			registerForward(&OP1, &OP2, bufferNum);
			result = OP1 - OP2;
			break;
		//The operation is bitwise OR (|) so we will so we will use the value in OP1 and OP2 to do perform this and 
		//store the result in the result register.
		case 6:
			registerForward(&OP1, &OP2, bufferNum);
			result = OP1 | OP2;
			break;
		//The OPCODE is one of the branch instruction operations. We will have to check the branch type through
		//another switch statement to determine the operation to be executed.
		case 7:
			//If branchTF is true, it means the branch condition is true so we will execute the operation.
			
				switch(fetchedInstruction[0] & mask)
				{	
					//Add the value of br1Words to the program counter for any branch 
					//operations identified by branch type values 0 to 5.
					case 0:
					case 1:
					case 2:
					case 3:
					case 4:
					case 5:	
						if (branchTF)
						{
							result = br1Words;
							break;
						}
						//If branchTF is false, we will not be moving the PC according to the branch operation.
						else
						{		
							result = 0;
						}
						break;		
					//This is the call or jump operation so the number of words we are jumping is stored in the result.
					case 6:
					case 7:
						result = br2Words;
						break;
					default:
						puts("Error in execute branch operation type.");
						puts("Program will terminate.");
						exit(1);
						break;
				}
				break; 
			break; //for the outer case 7 statement
			
		//This is the load operation so OP2 will
		//hold the address in memory that contains value the siaRegister will store.
		case 8:
			result = loadValue(OP2);
			break;
		//This is the store operation so OP1 holds the value we want to store in a certain address.
		case 9:
			result = OP1;
			break;
		//This is the pop operation. OP1 will indicate which of the operations we are doing (pop, push, return). 
		//In the case of push, OP2 will contain the value we are pushing onto the memory stack. In the case of pop,
		//OP1 will contain the siaRegister that will store a value popped from the stack.
		case 10:
			switch(OP1)
			{	
				//Operation is return. Pop the top stack value into the program counter. 
				case 0:
					result = stackPop();
					break;
				//Operation is push so OP2 contains the value we want to push onto the stack.
				case 4:
					result = OP2;
					break;
				//Operation is pop so result contains the value popped off of the stack.
				case 8:
					result = stackPop();					
					break;
				default:
					puts("Error in execution of stack operations.");
					puts("Program will terminate.");
					exit(1);
					break;
			}//end switch for case 10
			break;
		//The operation is move so OP2 contains the value we want to store in one of the siaRegisters.
		case 11:
			result = OP2;
			break;
		//The operation is interrupt so we either print all siaRegisters or print the memory of the VM.
		case 12:
			switch(OP1)
			{	
				//Print the registers.
				case 0:
					for(size_t i = 0; i < 16; i++)
					{
						printf("R%ld: %d\n",i,siaRegister[i]);
					}
					break;
				//Print out the memory.
				case 1:
					puts("VM Memory:");
					for(size_t i = 0; i < 1000; i++)
					{
						printf("%d  ", memory[i]);
						//Print a blank space every 20 entries
						if((i+1) % 20 == 0)
							puts("");
					}
					break;
				default:
					puts("Error in execution of interrupt.");
					exit(1);
					break;
			}//end switch
			break;

	}//end main switch of execute

	//Store the result in the appropriate buffer and switch the appropriate boolean values.
	if(executeResult1) {
		result1 = result;
		executeFirstOP = false;
		executeResult1 = false;
		storeResult1 = true;
	}
	else if (executeResult2) {
		result2 = result;
		executeSecondOP = false;
		executeResult2 = false;
		storeResult2 = true;
	}
}//end execute

//Store the results back in the main memory of the VM, update registers, and update the PC if applicable.
void store()
{	
	//Create the appropriate temporary variables for the corresponding buffer variables
	char fetchedInstruction[4] = {0};
	int OP1 = 0;
	int OP2 = 0;
	int result = 0;

	//Load up the temporary variables with the correct value from the appropriate buffer variables
	if(storeResult1) {
		memcpy(fetchedInstruction, fetchedInstruction1, 4);
		result = result1;
		OP1 = firstOP1;
		OP2 = firstOP2;
		storeResult1 = false;
	}
	else if(storeResult2) {
		memcpy(fetchedInstruction, fetchedInstruction2, 4);
		result = result2;
		OP1 = secondOP1;
		OP2 = secondOP2;
		storeResult2 = false;
	}
	//There is nothing to be stored yet so the VM does nothing.
	else {
		return;
	}

	switch(fetchedInstruction[0]>>4 & mask)
	{	
		//store the result of the 3R operation in the specified siaRegister
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
			siaRegister[ fetchedInstruction[1] & mask] = result;
			lastRegisterUsed = (fetchedInstruction[1] & mask);
			//add 2 to the current PC since this instruction is worth 2 bytes
			PC += 2;
			break;
 
		case 7:
			switch (fetchedInstruction[0] & mask)
			{
					//This is one of the br1 operations
					case 0:
					case 1:
					case 2:
					case 3:
					case 4:
					case 5:
						//If the condition under which the operation takes place is true, we jump to the specified address
						if(branchTF)
						{	
							PC += result;
						}
						//Otherwise we simply add the number of bytes of the branch instruction. 
						else
						{
							PC += 4;
						}
						break;

					//This is the call operation so first we push the address of the next instruction onto the stack and
					//then jump the specified address.
					case 6:
						stackPush(PC+4);
						PC = result;
						break;
					//This is the jump operation so we just jump to the specified address.
					case 7:
						PC = result;
						break;
			}
			break;

		//The value at the specified address in the memory will be loaded into the specified register. 
		case 8:
			siaRegister[OP1] = result;
			lastRegisterUsed = OP1;
			//Add 2 to the current PC since load/store instructions are worth 2 bytes.
			PC += 2;
			break;

		//The value of a specified register (stored in result) will be stored at a specified address in the memory (stored in OP2).
		case 9:
			storeInMemory(result, OP2);
			//Add 2 to the curren PC since load/store instructions are worth 2 bytes.
			PC += 2;
			break;

		//Updates for the stack instructions
		case 10:
			switch(fetchedInstruction[1]>>4 & mask)
			{
				//Operation is return so the result register contains the address we are jumping to.
				case 0:
					PC = result;
					break;
				//Operation is push so we will push the value in result onto the stack 
				case 4:
					stackPush(result);
					//Stack instructions are worth 2 bytes so add 2 to the PC
					PC += 2;
					break;
				//Operation is pop so we will store a value (contained in the result register) into the specified register
				case 8:
					siaRegister[OP2] = result;
					//Stack instructions are worth 2 bytes so add 2 to the PC
					PC += 2;
					break;
			}
			break;
		//Stored the desired number (represented by the result register) into the specified register (represented by OP1)
		case 11:
			siaRegister[OP1] = result;
			lastRegisterUsed = OP1;
			//Add two to the PC because move instructions are worth 2 bytes.
			PC += 2;
			break;
		//Don't have to store anything to memory since this is the interrupt operation and we are only printing values out.
		case 12:
			//Add two to the PC because move instructions are worth 2 bytes.
			PC += 2;
			break;
	}//end switch
}//end store

int main(int argc, char *argv[])
{

	if(argc != 2)
	{
		puts("Error in number of command line arguments.");
		puts("Program will terminate.");
		exit(1);
	}
	
	int overFlow = load(argv[1]);

	if(overFlow == 1)
	{
		puts("Memory overflow error.");
		exit(1);
	}
	//The VM will run until it has executed all the appropriate instructions or the halt operation is called
	else
	{
		//Set R15 to point to the top of the stack;
		siaRegister[15] = 999;
		while(PC != memIndex)
		{
			store();
			decode();
			execute();
			fetch();	
		}
	}
	return 0;
}//end main



