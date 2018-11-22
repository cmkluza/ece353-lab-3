static int wbCount;  //WB useful counter
void WB()
{
	//Note: SW is done in MEM I think so it's not included
	if(MEM_WB_Flag == 1) //If there is new info...
	{
		if(MEM_WB_Latch.op == HALT) //If HALT, 
			{
				WB_Halt_Flag == 1; //set halt flag and do nothing else
			}
		else if(MEM_WB_Latch.op == ADD) //if ADD use [RD] 
			{
				Registers[MEM_WB_Latch.rd] = MEM_WB_Latch.result;
				wbCount+=1; //increase useful counter
			}
		else if(MEM_WB_Latch.op == ADDI) //if ADDI use [RT]
			{
				Registers[MEM_WB_Latch.rt] = MEM_WB_Latch.result;
				wbCount+=1;	
			} 
		else if(MEM_WB_Latch.op == SUB) //if SUB use [RD]
			{
				Registers[MEM_WB_Latch.rd] = MEM_WB_Latch.result;
				wbCount+=1;
			} 
		else if(MEM_WB_Latch.op == MUL) //if MULT use [RD]
			{
				Registers[MEM_WB_Latch.rd] = MEM_WB_Latch.result;
				wbCount+=1;
			} 
		else if(MEM_WB_Latch.op == LW) //if LW use [RT]
			{
				Registers[MEM_WB_Latch.rt] = MEM_WB_Latch.result;
				wbCount+=1;
			} 
										//SW is done in MEM
	}

	MEM_WB_Flag == 0; //WB work is done

}


char *progScanner(FILE *givenFile, char *givenEntry)
{
// NOT DONE YET(sry)
//bad initial brainstorming pseudocode below, if you read it try not to judge me too hard pls





//turn add $s0, $s1, $s2  $s2 into add $s0 $s1 $s2
//turn lw $s0, 8($t0) into lw $s0 8 $t0



//take in line, put into string

 	fgets(givenEntry, 100, givenFile);
 	char * givenLine;
    givenLine = (char *)malloc(100*sizeof(char ));
    givenLine = trim(givenEntry);
    int i = 0;

    //har *token = strtok(givenLine,'');

// remove spaces => add$s0,$s1,$s2 or lw$s0,8($t0)
//remove commas => add$s0$s1$s2 or lw$s08($t0)

// *GROUP ALPHA START*
//check first 3 chars if == (ADD||SUB||BEQ) or (ADDI||MULT)
//if ADD||SUB||BEQ
//first 3 chars go into newInstr[0]
//if ADDI||MULT
//first 4 chars go into newInstr[0]
//then next 3 go into newInstr[1]
//next 3 go into newInstr[2]
//next 3 go into newInstr[3]
//newFinalString =newInstr[0] + " " + newInstr[1] + " " + newInstr[2] + " " newInstr[3]
//*GROUP ALPHA END*

//if (LW||SW)
//remove parenthesis = lw$s08$t0
//put first 2 chars into newInstr[0]
//put next 2 chars into newInstr[1]
//if second char of next 2 chars contain $, put next 1 char into newInstr[2], else put next 2 into newInstr[2]
//put next 2 chars into newInstr[3]
    //newFinal String = 
//newFinalString = newInstruction + " " + newRegOne + " " + newRegTwo + " " + newRegThree
}