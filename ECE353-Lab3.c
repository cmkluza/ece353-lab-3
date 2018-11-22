

void* IF() {

    IF_Inst_cycles++;
	struct inst curr_inst;
	curr_inst= IM[PC];                                            // create local copy of the instruction to be executed

	if (IF_ID_Flag==0){                                           // check if latch is empty
		if (curr_inst.op==HALT){                  
	    	IF_ID_latch= curr_inst;                               // send the halt instruction to the next stage
			IF_ID_Flag=1;
		}
		if (IF_Inst_cycles>=c){                
			IF_ID_latch= curr_inst;                               // send the instruction to the next stage
			PC= PC + 4;                                           // change PC to the next instruction
			IF_ID_Flag=1;                                         // set flag IF/ID latch not empty
			IF_Inst_cycles=0;
			IF_WorkCycles=IF_WorkCycles+c;                        // updates count of useful cycles
		}
	}
	
}


 void* EX() {
 
 	if(ID_EX_Flag==1 && EX_Inst_Cycles==0){
 		struct curr_inst= ID_EX_latch;
 		ID_EX_Flag=0;
 	}
 
 	EX_Inst_Cycles++;

    // ADD operation
	if (curr_inst.op==ADD){
		if (EX_Inst_Cycles>=n){
			curr_inst.EX_result= (int16_t) curr_inst.rs + curr_inst.rt;
		}
	}
	
	// ADDI operation
	if (curr_inst.op==ADDI){
		if (EX_Inst_Cycles>=n){
			curr_inst.EX_result= (int16_t) curr_inst.rs + curr_inst.immediate;
		}
	}
	
	//BEQ operation
	if (curr_inst.op==BEQ){
		if (EX_Inst_Cycles>=n){
			curr_inst.EX_result= (int16_t) curr_inst.rt - curr_inst.rs;
			if (curr_inst.EX_result==0) PC = PC + 4 + 4 * (curr_inst.immediate);
		}
	}
	
	//LW and SW operation, not sure how to do this
	if (curr_inst.op==LW || curr_inst..op==SW){
		if (EX_Inst_Cycles>=n){
			curr_inst.EX_result= (int16_t) curr_inst.rs + curr_inst.immediate; 
		}
	}
	
	//SUB operation
	if (curr_inst.op==SUB){
		if (EX_Inst_Cycles>=n){
			curr_inst.EX_result= (int16_t) curr_inst.rs - curr_inst.rt;
		}
	}
	
	//MUL operation
	if (curr_inst.op==MUL){
		if (EX_Inst_Cycles>=m){
			curr_inst.EX_result= (int16_t) curr_inst.rs * curr_inst.rt;
		}
	}
	
	//send instruction to MEM
	if (EX_MEM_Flag==0){
		EX_MEM_latch=curr.inst;
		EX_MEM_Flag=1;
		EX_Inst_Cycles=0;
		if (curr_inst.op==MUL) EX_WorkCycles=EX_WorkCycles+m;
		else if (curr_inst.op!=HALT && curr_inst.op!=MUL) EX_WorkCycles=EX_WorkCycles+n;
	}
		
}
	
	