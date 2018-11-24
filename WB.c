void WB()
{
	//Note: SW is done in MEM I think so it's not included
	if(MEM_WB_Flag == 1) //If there is new info...
	{
		if(MEM_WB_latch.inst_op == HALT) //If HALT, 
			{
				WB_Halt_Flag == 1; //set halt flag and do nothing else
			}
		else if(MEM_WB_latch.inst_op == ADD) //if ADD use [RD] 
			{
				Registers[MEM_WB_Latch.rd] = MEM_WB_Latch.result;
				WB_WorkCycles++; //increase useful counter
			}
		else if(MEM_WB_latch.inst_op == ADDI) //if ADDI use [RT]
			{
				Registers[MEM_WB_Latch.rt] = MEM_WB_Latch.result;
				WB_WorkCycles++;	
			} 
		else if(MEM_WB_latch.inst_op == SUB) //if SUB use [RD]
			{
				Registers[MEM_WB_Latch.rd] = MEM_WB_Latch.result;
				WB_WorkCycles++;
			} 
		else if(MEM_WB_latch.inst_op == MUL) //if MULT use [RD]
			{
				Registers[MEM_WB_Latch.rd] = MEM_WB_Latch.result;
				WB_WorkCycles++;
			} 
		else if(MEM_WB_latch.inst_op == LW) //if LW use [RT]
			{
				Registers[MEM_WB_Latch.rt] = MEM_WB_Latch.result;
				WB_WorkCycles++;
			} 
										//SW is done in MEM
	}

	MEM_WB_Flag == 0; //WB work is done

}
