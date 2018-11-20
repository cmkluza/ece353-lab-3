

void* IF() {

  struct inst curr_inst;
	
	if (IF_ID_Flag==0){                    // check if latch is empty
		curr_inst= IM[PC];             // create local copy of the instruction to be executed
		IF_ID_latch= curr_inst;        // send the instruction to the next stage
		PC= PC + 4;                    // change PC to the next instruction
		IF_ID_Flag=1;                  // set flag IF/ID latch not empty
	}
}

 void* EX() {
 
 	struct curr_inst;
 
 	if(ID_EX_Flag==1 && EX_Cycle_Counter==0){
 		struct curr_inst= ID_EX_latch;
 		ID_EX_Flag=0;
 	}
 
 	EX_Cycle_Counter=EX_Cycle_Counter+1;
 
        // ADD operation
	if (curr_inst.op==ADD){
		if (EX_Cycle_Counter>= ){
			curr_inst.EX_result= (int16_t) curr_inst.rs + curr_inst.rt;
			EX_Cycle_Counter=0;
		}
	}
	
	// ADDI operation
	if (curr_inst.op==ADDI){
		if (EX_Cycle_Counter>= ){
			curr_inst.EX_result= (int16_t) curr_inst.rt + curr_inst.immediate;
			EX_Cycle_Counter=0;
		}
	}
	
	//BEQ operation
	if (curr_inst.op==BEQ){
	
		if (EX_Cycle_Counter>= ){
			curr_inst.EX_result= (int16_t) curr_inst.rt - curr_inst.rs;
			if (curr_inst.EX_result==0) PC = PC + 4 + 4 * (curr_inst.immediate);
			EX_Cycle_Counter=0;
		}
	}
	
	//LW and SW operation, not sure how to do this
	if (curr_inst.op==LW || curr_inst..op==SW){
		if (EX_Cycle_Counter>= ){
			//curr_inst.EX_result= (int16_t) 
			EX_Cycle_Counter=0;
		}
	}
	
	//MUL operation
	if (curr_inst.op==MUL){
		if (EX_Cycle_Counter>= ){
			curr_inst.EX_result= (int16_t) curr_inst.rs * curr_inst.rt;
			EX_Cycle_Counter=0;
		}
	}
	
	//SUB operation
	if (curr_inst.op==SUB){
		if (EX_Cycle_Counter>= ){
			curr_inst.EX_result= (int16_t) curr_inst.rs - curr_inst.rt;
			EX_Cycle_Counter=0;
		}
	}
	
	//send instruction to MEM
	if (EX_MEM_Flag==0){
		EX_MEM_latch=curr.inst;
		EX_MEM_Flag=1;
	}
		
}
	
	
