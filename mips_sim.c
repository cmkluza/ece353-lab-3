/**
 * Authors: Laura DeBurgo, Dametreuss Francois, Cameron Kluza, Kyle McWherter
 */

/* ======================== Preprocessor Directives ======================== */
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SINGLE 1
#define BATCH 0
#define REG_NUM 32

// helper macro that fills in redundant information for an error call
#define PARSER_ERR(msg, inst, col, ...) parserErr(__FUNCTION__, __LINE__, msg, \
    inst, col, ##__VA_ARGS__)

/**
 * There were errors with the submission environment's string functions
 * so these simple implementations are used to get around that.
 */
void Strcpy(char *dest, char *src) {
    while (*src != '\0') *(dest++) = *(src++);
    *dest = '\0';
}

void Strcat(char *dest, char *src) {
    Strcpy(dest + strlen(dest), src);
}

char *Strchr(const char *str, char c) {
    while (*str != '\0' && *str != c) ++str;
    if (*str == c) return (char *) str;
    return NULL;
}

// very simple implementation of Strstr
char *Strstr(const char *haystack, const char *needle) {
    haystack = Strchr(haystack, needle[0]);
    if (haystack == NULL) return (char *) haystack;

    if (strlen(haystack) < strlen(needle)) return NULL;

    int i;
    for (i = 0; needle[i] != '\0'; ++i) {
        if (needle[i] != haystack[i]) break;
    }
    if (needle[i] == '\0') return (char *) haystack;
    return Strstr(haystack, needle);
}

/* ============================ Structs and Enums =========================== */
/**
 * Represents an instruction operation.
 */
enum inst_op {
    ERR,
    ADD,
    ADDI,
    BEQ,
    DEADBEQ, //Used in ID to ensure BEQ is terminated structurally after its resolved
    LW,
    MUL,
    SUB,
    SW,
    HALT, // "haltSimulation"
    COMMENT // # comments
};

/**
 * The type of the instruction
 */
enum inst_type {
    NA, // not applicable (i.e. for haltSimulation)
    R_TYPE, // arithmetic
    I_TYPE // immediate/branch
};

/**
 * Represents a single instruction to the processor.
 */
struct inst {
    enum inst_op op;
    enum inst_type type;
    // source registers - hold either the index of a register or the contents of a register
    int32_t rs;
    int32_t rt;
    // destination register - only holds the index of a register
    uint8_t rd;
    // the result from an operation to be written to rd
    int32_t EX_result;
    // immediate or offset value for I-Type instructions
    int16_t immediate;
};

/* =========================== Function Prototypes ========================== */
/**
 * Reads input from a text file and returns the string as a list of space
 * separated tokens.
 * Treats consecutive commas as a single comma.
 * Asserts proper parenthesis format for loads and stores.
 */
char *progScanner(FILE *inputFile, char *inputLine);

/**
 * Takes as input the output of progScanner and returns a string with registers
 * converted to integers.
 * This function is called and handled from within parser, and should not be
 * called by anything else.
 * Asserts that register names are valid.
 */
char *regNumberConverter(char *instruction);

/**
 * Takes as input the output of progScanner and returns a properly filled
 * instruction struct.
 * Asserts the input instruction is legal (checks opcode, range of immediate,
 * instruction argument format, and memory access alignment)
 */
struct inst parser(char *instruction);

/**
 * Fetches from instruction memory.
 * Freezes if there's an unresolved branch.
 */
void IF(void);

/**
 * Decodes instructions (takes 1 cycle)
 * Checks for data (RAW) and control hazards, and halts until these are
 * resolved.
 * Provides operands to EX.
 */
void ID(void);

/**
 * Executes specified operation on operands from ID.
 * Can take multiple cycles to execute (m or n cycles).
 */
void EX(void);

/**
 * Performs memory read/write operations, used for lw and sw instructions.
 * Can take multiple cycles to execute (c cycles).
 */
void MEM(void);

/**
 * Writes back into the register file (takes 1 cycle).
 */
void WB(void);

// populates the instruction memory
static void populateIM(FILE *input);

// progScanner helper functions
static void validateParens(const char *instruction);

// regNumberConverter helper functions
static char *getRegNumber(char *token, char *base, char *original);

// parser helper functions
static enum inst_op getOp(char *instruction);

static enum inst_type getInstType(enum inst_op op);

static void
parseRType(struct inst *inst, char *converted, char *remainingTokens);

static void
parseIType(struct inst *inst, char *converted, char *remainingTokens);

static void
parseAddi(struct inst *inst, char *converted, char *remainingTokens);

static void
parseBeq(struct inst *inst, char *converted, char *remainingTokens);

static void
parseLwSw(struct inst *inst, char *converted, char *remainingTokens);

static long Strtol(char **numStr, int min, int max, char *inst, long col);

static void parserErr(const char *function, int line, const char *msg,
                      const char *inst, long col, ...);

// validation helper functions
static void validate(const char *instruction, enum inst_op op,
                     enum inst_type type);

static void validateRType(const char *instruction);

static void validateIType(const char *instruction, enum inst_op op);

static void validateAddiBeq(const char *instruction);

static void validateLwSw(const char *instruction);

/* ============================= Global Variables =========================== */
/**
 * Instruction memory - 512 x 1-word instructions.
 * Word-addressable, so accesses would be something like IM[PC >> 2].
 */
static struct inst IM[512];

//  but I'm pretty sure it's actually supposed to be word addressable. should be
//  a few simple changes that don't affect your guys' code at all.
/**
 * Data memory - 512 x 1-word data.
 * Word-addressable, so access should be DM[addr >> 2]
 */
static int32_t DM[512];

/**
 * Program counter
 */
static long PC;

/**
 * Latches
 */
static struct inst IF_ID_latch, ID_EX_latch, EX_MEM_latch, MEM_WB_latch;

/**
* Flags
*/
static int IF_ID_Flag, ID_EX_Flag, EX_MEM_Flag, MEM_WB_Flag, WB_HALT_Flag;

/**
* Registers
*/
static int32_t Registers[REG_NUM];

/**
* Useful cycle counters
*/
static long IF_WorkCycles, ID_WorkCycles, EX_WorkCycles, MEM_WorkCycles,
        WB_WorkCycles;

/**
* IF and EX instruction cycle counter
*/
static long IF_Inst_Cycles, EX_Inst_Cycles;

/**
 * Cycle variables
 */
static int m; // number of cycles for multiply
static int n; // number of cycles for all other EX operations
static int c; // number of cycles for memory access

/* ============================== Main Function ============================= */
int main(int argc, char *argv[]) {
    // given variables
    int sim_mode = BATCH; // mode flag, 1 for single-cycle, 0 for batch

    int i; // for loop counter
    long sim_cycle = 0; // simulation cycle counter

    FILE *input = NULL;
    FILE *output = NULL;

    /* ========== Provided Startup Code ========== */
    printf("The arguments are:");
    for (i = 1; i < argc; i++) {
        printf("%s ", argv[i]);
    }
    printf("\n");

    if (argc == 7) {
        if (strcmp("-s", argv[1]) == 0) {
            sim_mode = SINGLE;
        } else if (strcmp("-b", argv[1]) == 0) {
            sim_mode = BATCH;
        } else {
            printf("Wrong sim mode chosen\n");
            exit(0);
        }

        m = atoi(argv[2]);
        n = atoi(argv[3]);
        c = atoi(argv[4]);
        input = fopen(argv[5], "r");
        output = fopen(argv[6], "w");
    } else {
        printf("Usage: ./sim-mips -s m n c input_name output_name "
               "(single-cycle mode)\n or \n ./sim-mips -b m n c input_name  "
               "output_name(batch mode)\n");
        printf("m,n,c stand for number of cycles needed by multiplication, "
               "other operation, and memory access, respectively\n");
        exit(0);
    }
    if (input == NULL) {
        printf("Unable to open input or output file\n");
        exit(0);
    }
    if (output == NULL) {
        printf("Cannot create output file\n");
        exit(0);
    }

    /* ========== IM Initialization ========== */
    populateIM(input);

    /* ========== Main Program Loop ========== */
    while (1) {
        // stop once halt has passed through every stage
        if (WB_HALT_Flag) break;

        // call each stage in reverse
        WB();
        MEM();
        EX();
        ID();
        IF();

        /* ========== code fragment 2 ========== */
        if (sim_mode == SINGLE) {
            printf("cycle: %ld register value: ", sim_cycle);
            for (i = 1; i < REG_NUM; i++) {
                printf("%d  ", Registers[i]);
            }
            printf("\nprogram counter: %ld\n", PC);
            printf("press ENTER to continue\n");
            while (getchar() != '\n');
        }

        sim_cycle += 1;
    }

    // calculate utilization of each stage
    double ifUtil = (double) IF_WorkCycles / sim_cycle;
    double idUtil = (double) ID_WorkCycles / sim_cycle;
    double exUtil = (double) EX_WorkCycles / sim_cycle;
    double memUtil = (double) MEM_WorkCycles / sim_cycle;
    double wbUtil = (double) WB_WorkCycles / sim_cycle;

    /* ========== code fragment 3 ========== */
    if (sim_mode == BATCH) {
        fprintf(output, "program name: %s\n", argv[5]);
        fprintf(output, "stage utilization: %f  %f  %f  %f  %f \n",
                ifUtil, idUtil, exUtil, memUtil, wbUtil);

        fprintf(output, "register values ");
        for (i = 1; i < REG_NUM; i++) {
            fprintf(output, "%d  ", Registers[i]);
        }
        fprintf(output, "%ld\n", PC);
    }

    // TODO - figure out what this is supposed to say if it's even supposed to be here
    printf("Program name: %s\n"
           "Stage utilization: %f  %f  %f  %f  %f\n"
           "Total CPU Cycles: %ld\n",
           argv[5], ifUtil, idUtil, exUtil, memUtil, wbUtil, sim_cycle);

    //close input and output files at the end of the simulation
    fclose(input);
    fclose(output);
    return 0;
}

/* ======================== Function Implementations ======================== */
char *progScanner(FILE *inputFile, char *inputLine){
    fgets(inputLine, 100, inputFile);

    if (*inputLine == '\n') return "\n"; // blank liness

    // if the instruction is sw/lw, this validates parens by the offset
    validateParens(inputLine);

    char *givenLine;
    givenLine = (char*)malloc(100*sizeof(char ));
    Strcpy(givenLine, inputLine); //strtok is destructive so im using a copy instead

    int i;
    char delimiters[]={"," "(" ")" ";" "\n" " "};
    char ** instructionFields;

    instructionFields = (char **)malloc(100*sizeof(char *));
//    for (i=0; i<4; i++)
//      *(instructionFields+i) = (char *) malloc(20*sizeof(char));

    instructionFields[0] = strtok(inputLine, delimiters);

    if(strcmp(instructionFields[0],"haltSimulation")== 0)
    {

        for(i=1;i<4;i++)
            instructionFields[i] = "0";

    }
    else
    {

        for(i=1;i<4;i++)
            instructionFields[i]=strtok(NULL,delimiters);

    }

    char *result = malloc(100*sizeof(char *));
    Strcpy(result, instructionFields[0]);
    Strcat(result, " ");

    for (i = 1; i < 4; ++i) {
        if (instructionFields[i] != NULL)
            Strcat(result, instructionFields[i]);
        if (i != 3) Strcat(result, " ");
    }

//    strcat(result, instructionFields[1]);
//    strcat(result, " ");
//    strcat(result, instructionFields[2]);
//    strcat(result, " ");
//    strcat(result, instructionFields[3]);

    // free memory we're done with
    free(givenLine);
    free(instructionFields);

    return result;
}

char *regNumberConverter(char *instruction) {
    // local copies of data to prevent modification of external data
    char *copy = malloc(strlen(instruction) + 1);
    char *base = copy;
    Strcpy(copy, instruction);

    if (*base == '#') {
        return copy;
    }

    // data used to store the result of conversion
    char *buffer = malloc(256);
    int bufferPointer = 0;
    char *curToken = strtok(copy, " ");
    size_t len;

    do {
        // see if the token is a register
        if (*curToken == '$') {
            // convert the register name into a decimal number
            char *regNum = getRegNumber(curToken, base, instruction);
            // add the result into the buffer
            len = strlen(regNum);
            Strcpy(buffer + bufferPointer, regNum);
            bufferPointer += len;
            buffer[bufferPointer++] = ' ';
            // we don't need regNum anymore
            free(regNum);
        } else { // not a register
            // check for buffer overflow - if there is any, it indicates an error
            len = strlen(curToken);
            if (bufferPointer + len >= 255) {
                PARSER_ERR("invalid instruction", copy, 0);
            }
            // copy the token into buffer and add the space afterwards
            Strcpy(buffer + bufferPointer, curToken);
            bufferPointer += len;
            buffer[bufferPointer++] = ' ';
        }
    } while ((curToken = strtok(NULL, " ")) != NULL);

    free(copy);

    // if there's a trailing space, remove it; terminate the string regardless
    if (buffer[bufferPointer - 1] == ' ') --bufferPointer;
    buffer[bufferPointer] = '\0';
    return buffer;
}

struct inst parser(char *instruction) {
    struct inst inst;
    if (*instruction == '#') {
        inst.op = COMMENT;
        return inst;
    }
    char *converted = regNumberConverter(instruction);

    // set the op
    if ((inst.op = getOp(converted)) == ERR) {
        PARSER_ERR("unrecognized op in instruction", instruction, 0);
    }
    // set the type
    inst.type = getInstType(inst.op);
    // validate
    validate(instruction, inst.op, inst.type);

    // set the instruction type and parse appropriately
    if (inst.type == R_TYPE) {
        parseRType(&inst, converted, Strchr(converted, ' ') + 1);
    } else if (inst.type == I_TYPE) {
        parseIType(&inst, converted, Strchr(converted, ' ') + 1);
    } else {
        // halt instruction, no further processing needed
        free(converted);
        return inst;
    }

    free(converted);
    return inst;
}

void IF() {

    IF_Inst_Cycles++;
    struct inst curr_inst;
    curr_inst= IM[PC >> 2];                                       // create local copy of the instruction to be executed

    if (IF_ID_Flag==0){                                           // check if latch is empty
//        if (curr_inst.op==HALT){
//            IF_ID_latch= curr_inst;                               // send the halt instruction to the next stage
//            IF_ID_Flag=1;
//        }
        if (IF_Inst_Cycles>=c){
            IF_ID_latch= curr_inst;                               // send the instruction to the next stage
            PC= PC + 4;                                           // change PC to the next instruction
            IF_ID_Flag=1;                                         // set flag IF/ID latch not empty
            IF_Inst_Cycles=0;
            IF_WorkCycles=IF_WorkCycles+c;                        // updates count of useful cycles
        }
    }

}

void ID()
{
    static int BeqTimer = 0;                                              //holds a timer for BEQ hazard avoidance

    if(IF_ID_latch.op == LW || IF_ID_latch.op == ADDI)
    {                                                                      //This doesn't change any reg data, just holds reg #
        IF_ID_latch.rd = IF_ID_latch.rt;                               //makes RAW check simpler and rd isn't being used anyways, exists in both fields
    }                                                                     //this will redunantly copy if pipe is stalled but won't hurt anything


    if((IF_ID_latch.rs != ID_EX_latch.rd || ID_EX_Flag == 0) &&           //This looks complicated but It just checks
       (IF_ID_latch.rt != ID_EX_latch.rd || ID_EX_Flag == 0) &&          //that I'm not about to read a reg that is a
       (IF_ID_latch.rs != EX_MEM_latch.rd || EX_MEM_Flag == 0) &&        //destination reg for a instruction further
       (IF_ID_latch.rt != EX_MEM_latch.rd || EX_MEM_Flag == 0) &&        //down the pipe, IF A FLAG == 0 the assoicated latch is stale and not "in the pipe"
       (IF_ID_latch.rs != MEM_WB_latch.rd || MEM_WB_Flag == 0) &&         //rd must be set to 32 <= rd <= -1 in previous latch after its copied forward out of EX and MEM
       (IF_ID_latch.rt != MEM_WB_latch.rd || MEM_WB_Flag == 0))           //If Flag=0 then Write has happened and r/w can happen same cycle
    {
        if(IF_ID_latch.op == BEQ && ID_EX_latch.op != BEQ && ID_EX_Flag == 0 && IF_ID_Flag == 1) //If Its a branch, yet to move forward, EX is ready,IF fetched
        {
            ++ID_WorkCycles;                                                 //1 cycle of work
            ID_EX_latch = IF_ID_latch;                                       //Copy to ex recieve latch
            ID_EX_latch.rs = Registers[ID_EX_latch.rs];
            ID_EX_latch.rt = Registers[ID_EX_latch.rt];                       //fill struct with reg data for ex
            ID_EX_Flag = 1;                                                  //Tell EX its a go on the next cycle
            BeqTimer = n + 1;                                                //Cycles EX reqires to resolve BEQ, could be off - 1 or -2
        }
        else if (BeqTimer != 0)   //Its a BEQ that has been moved forward but IF is/was waiting on proper PC
        {
            if(BeqTimer != 1)
            {
                --BeqTimer; 	                                  //keep counting down
            }
            else
            {
                IF_ID_Flag = 0;                //Program counter is set, so IF can fetch
                --BeqTimer;                    //Set back to 0
                ID_EX_latch.op = DEADBEQ; //Needed incase of 2+ BEQ in a row, but since BEQ terminates in EX it won't go anywhere
            }
        }
        else if(IF_ID_latch.op == ADD && ID_EX_Flag == 0 && IF_ID_Flag == 1) //If its an add and Ex is ready and IF fetched last cycle
        {
            ++ID_WorkCycles;                               //Doing work
            ID_EX_latch = IF_ID_latch;                     //Copy it forward
            ID_EX_latch.rs = Registers[ID_EX_latch.rs];    //replace register #s with the data inside them
            ID_EX_latch.rt = Registers[ID_EX_latch.rt];
            ID_EX_Flag = 1;                                //Tell EX that next cycle is greenlight
            IF_ID_Flag = 0;                                //Tell IF the latch is clear to be overwritten
        }
        else if(IF_ID_latch.op == ADDI && ID_EX_Flag == 0 && IF_ID_Flag == 1)
        {
            ++ID_WorkCycles;                               //Record work done
            ID_EX_latch = IF_ID_latch;                     //Copy it forward
            ID_EX_latch.rs = Registers[ID_EX_latch.rs];    //Only the rs reg, both rd and rt are the destination reg for addi
            ID_EX_Flag = 1;                                //Tell EX that next cycle it can start
            IF_ID_Flag = 0;                                //Tell IF the latch is clear to be overwritten
        }
        else if(IF_ID_latch.op == SUB && ID_EX_Flag == 0 && IF_ID_Flag == 1)
        {
            ++ID_WorkCycles;                               //Doing work
            ID_EX_latch = IF_ID_latch;                     //Copy it forward
            ID_EX_latch.rs = Registers[ID_EX_latch.rs];    //replace register #s with the data inside them
            ID_EX_latch.rt = Registers[ID_EX_latch.rt];
            ID_EX_Flag = 1;                                //Tell EX that next cycle is greenlight
            IF_ID_Flag = 0;                                //Tell IF the latch is clear to be overwritten
        }
        else if(IF_ID_latch.op == LW && ID_EX_Flag == 0 && IF_ID_Flag == 1)
        {
            ++ID_WorkCycles;                               //Record work done
            ID_EX_latch = IF_ID_latch;                     //Copy it forward
            ID_EX_latch.rs = Registers[ID_EX_latch.rs];    //Only the rs reg, used with immediate by EX and MEM
            //use rt or rd for destination
            ID_EX_Flag = 1;                                //Tell EX that next cycle it can start
            IF_ID_Flag = 0;                                //Tell IF the latch is clear to be overwritten
        }
        else if(IF_ID_latch.op == SW && ID_EX_Flag == 0 && IF_ID_Flag == 1)
        {
            ++ID_WorkCycles;                               //Record work done
            ID_EX_latch = IF_ID_latch;                     //Copy it forward
            ID_EX_latch.rs = Registers[ID_EX_latch.rs];    //rs is used by EX along with immediate to get memory address
            ID_EX_latch.rt = Registers[ID_EX_latch.rt];    //rt is use by MEM as it is the data to be written into memory
            ID_EX_Flag = 1;                                //Tell EX that next cycle it can start
            IF_ID_Flag = 0;                                //Tell IF the latch is clear to be overwritten
        }
        else if(IF_ID_latch.op == MUL && ID_EX_Flag == 0 && IF_ID_Flag == 1)
        {
            ++ID_WorkCycles;                               //Doing work
            ID_EX_latch = IF_ID_latch;                     //Copy it forward
            ID_EX_latch.rs = Registers[ID_EX_latch.rs];    //replace register #s with the data inside them
            ID_EX_latch.rt = Registers[ID_EX_latch.rt];
            ID_EX_Flag = 1;                                //Tell EX that next cycle it can start
            IF_ID_Flag = 0;                                //Tell IF the latch is clear to be overwritten
        }
        else if(IF_ID_latch.op == HALT && ID_EX_Flag == 0 && IF_ID_Flag == 1)
        {
            ID_EX_latch = IF_ID_latch;                     //Copy it forward
            ID_EX_Flag = 1;                                //Tell EX that next cycle it can recieve
        }
        //If none of the above conditions are met other stages are busy or not a valid opcode (wait)
    }
    //If theres a RAW hazard do nothing (wait)

}

void EX() {
    // work directly with the ID_EX_latch
    // keep ID_EX_flag = 1 while working with it
    if (ID_EX_Flag != 1) // stop method if we don't have data to work on
        return;

    // increment number of cycles
    EX_Inst_Cycles++;

    // ADD operation
    if (ID_EX_latch.op==ADD){
        if (EX_Inst_Cycles>=n){
            ID_EX_latch.EX_result= ID_EX_latch.rs + ID_EX_latch.rt;
        }
    }

    // ADDI operation
    if (ID_EX_latch.op==ADDI){
        if (EX_Inst_Cycles>=n){
            ID_EX_latch.EX_result= ID_EX_latch.rs + ID_EX_latch.immediate;
        }
    }

    //BEQ operation
    if (ID_EX_latch.op==BEQ){
        if (EX_Inst_Cycles>=n){
            ID_EX_latch.EX_result= ID_EX_latch.rt - ID_EX_latch.rs;
            if (ID_EX_latch.EX_result==0) PC = PC + 4 * (ID_EX_latch.immediate);
        }
    }

    //LW and SW operation, not sure how to do this
    if (ID_EX_latch.op==LW || ID_EX_latch.op==SW){
        if (EX_Inst_Cycles>=n){
            ID_EX_latch.EX_result= ID_EX_latch.rs + ID_EX_latch.immediate;
        }
    }

    //SUB operation
    if (ID_EX_latch.op==SUB){
        if (EX_Inst_Cycles>=n){
            ID_EX_latch.EX_result= ID_EX_latch.rs - ID_EX_latch.rt;
        }
    }

    //MUL operation
    if (ID_EX_latch.op==MUL){
        if (EX_Inst_Cycles>=m){
            ID_EX_latch.EX_result= ID_EX_latch.rs * ID_EX_latch.rt;
        }
    }

    //send instruction to MEM
    if (EX_MEM_Flag==0 &&
        ((ID_EX_latch.op == MUL && EX_Inst_Cycles >= m) ||
         (ID_EX_latch.op != MUL && EX_Inst_Cycles >= n))) {
        EX_MEM_latch=ID_EX_latch; // move data into the next stage
        EX_MEM_Flag=1; // tell MEM that there's new data
        ID_EX_Flag = 0; // tell ID that we're done
        EX_Inst_Cycles=0; // reset cycle counter
        if (ID_EX_latch.op==MUL) EX_WorkCycles=EX_WorkCycles+m;
        else if (ID_EX_latch.op!=HALT && ID_EX_latch.op!=MUL) EX_WorkCycles=EX_WorkCycles+n;
    }
}

void MEM()
{
    static int MEM_Timer = 0;
    if(EX_MEM_Flag == 1)                      //If latch is ready i can start working, *WRITEBACK* can never hold this up as WB only takes 1 Cycle
    {
        if(EX_MEM_latch.op == SW)          //if its a store word
        {
            if(MEM_Timer == 0)                    //first cycle of a SW
            {
                MEM_Timer = c; //start timer        //This is how many cycles it will take
                ++MEM_WorkCycles;                //I've started so first cycle of work
            }
            if(MEM_Timer != 0 && MEM_Timer != 1)  //Mid operation, not done, but working
            {
                --MEM_Timer;
                ++MEM_WorkCycles;
            }
            if(MEM_Timer == 1)                   //Last cycle
            {
                --MEM_Timer;                       //timer back to zero
                DM[EX_MEM_latch.EX_result >> 2] = EX_MEM_latch.rt;   //store rt where EX has calculated it should go
                // no need to pass it on, it finishes here.

                EX_MEM_Flag = 0;                   // EX can fill it again
            }
        }
        if(EX_MEM_latch.op == LW)         //if its a Load word
        {
            if(MEM_Timer == 0)                    //first cycle of a LW
            {
                MEM_Timer = c; //start timer        //This is how many cycles it will take
                ++MEM_WorkCycles;                //I've started so first cycle of work
            }
            if(MEM_Timer != 0 && MEM_Timer != 1)  //Mid operation, not done, but working
            {
                --MEM_Timer;
                ++MEM_WorkCycles;
            }
            if(MEM_Timer == 1)                   //Last cycle
            {
                --MEM_Timer;                       //timer back to zero
                MEM_WB_latch = EX_MEM_latch;       // no need to check the flag, WB cannot delay the pipe EVER (nice!:))
                MEM_WB_latch.EX_result = DM[EX_MEM_latch.EX_result >> 2];   //go to location calculated by EX and copy the value found into rt
                //could leave it in rd if it make WB any easier
                MEM_WB_Flag = 1;                   //Still i think ID uses the flag for RAW checks
                EX_MEM_Flag = 0;                   // EX can fill it again
            }
        }
        else                                   //something is in the latch but i don't care what, i'll pass it on
        {
            MEM_WB_latch = EX_MEM_latch;         //move intruction forward (do nothing to it)
            EX_MEM_Flag = 0;                     //EX can fill it again
            MEM_WB_Flag = 1;                     //WB has work now
        }
    }
}

void WB()
{
    //Note: SW is done in MEM I think so it's not included
    if(MEM_WB_Flag == 1) //If there is new info...
    {
        if(MEM_WB_latch.op == HALT) //If HALT,
        {
            WB_HALT_Flag = 1; //set halt flag and do nothing else
        }
        else if(MEM_WB_latch.op == ADD) //if ADD use [RD]
        {
            // don't overwrite $0
            if (MEM_WB_latch.rd != 0)
                Registers[MEM_WB_latch.rd] = MEM_WB_latch.EX_result;
            WB_WorkCycles++; //increase useful counter
        }
        else if(MEM_WB_latch.op == ADDI) //if ADDI use [RT]
        {
            // don't overwrite $0
            if (MEM_WB_latch.rt != 0)
                Registers[MEM_WB_latch.rt] = MEM_WB_latch.EX_result;
            WB_WorkCycles++;
        }
        else if(MEM_WB_latch.op == SUB) //if SUB use [RD]
        {
            // don't overwrite $0
            if (MEM_WB_latch.rd != 0)
                Registers[MEM_WB_latch.rd] = MEM_WB_latch.EX_result;
            WB_WorkCycles++;
        }
        else if(MEM_WB_latch.op == MUL) //if MULT use [RD]
        {
            // don't overwrite $0
            if (MEM_WB_latch.rd != 0)
                Registers[MEM_WB_latch.rd] = MEM_WB_latch.EX_result;
            WB_WorkCycles++;
        }
        else if(MEM_WB_latch.op == LW) //if LW use [RT]
        {
            // don't overwrite $0
            if (MEM_WB_latch.rt != 0)
                Registers[MEM_WB_latch.rt] = MEM_WB_latch.EX_result;
            WB_WorkCycles++;
        }
        //SW is done in MEM
    }

    MEM_WB_Flag = 0; //WB work is done

}

/* ==================== Helper Function Implementations ===================== */
// reads in all the instruction in a file
// NOTE: if the input file doesn't end with "haltSimulation", this will loop
// infinitely
static void populateIM(FILE *input) {
    int tempPc = 0; // temporary "program counter" to fill
    char buffer[100]; // buffer to read instructions into
    char *result;
    struct inst inst;

    while (1) {
        // ignore blank lines
        if (*(result = progScanner(input, buffer)) == '\n') continue;
        // parse and free
        inst = parser(result);
        free(result);
        // ignore comments
        if (inst.op == COMMENT) {
            continue;
        }
        // update IM
        IM[tempPc++] = inst;
        if (IM[tempPc - 1].op == HALT) break; // stop after we've stored halt
    }
}

// validates parentheses for lw/sw: lw/sw register offset(register)
static void validateParens(const char *instruction) {
    // make sure it's a load/store
    if (*instruction != 'l' && *instruction != 's') return;
    if (*(instruction + 1) != 'w' ||
        (*(instruction + 2) != ' ' && *(instruction + 2) != '\0')) return;

    // if there's a comment with parentheses, don't want to count those
    char *comment = Strchr(instruction, '#');
    char *temp;

    // find the first parenthesis
    char *result = Strchr(instruction, '(');

    if (result == NULL || (comment && result > comment)) { // no opening parenthesis
        PARSER_ERR("malformed load/store - no opening parenthesis", instruction, 0);
    }
    // there should be no additional opening parentheses
    if ((temp = Strchr(result + 1, '(')) != NULL &&
        (!comment || temp < comment)) {
        PARSER_ERR("malformed load/store - extra opening parentheses",
                instruction, temp - instruction);
    }

    // next should be a register ($ + digits/letters) then a closing paren
    if (*(++result) != '$') {
        PARSER_ERR("malformed load/store - misplaced opening parenthesis",
                instruction, result - instruction);
    }

    // now look for closing parenthesis
    if ((result = Strchr(instruction, ')')) == NULL ||
            (comment && result > comment)) {
        // no closing paren
        PARSER_ERR("malformed load/store - no closing parenthesis", instruction, 0);
    }

    // there should be no additional closing parentheses
    if ((temp = Strchr(result + 1, ')')) != NULL &&
            (!comment || temp < comment)) {
        PARSER_ERR("malformed load/store - extra closing parentheses",
                   instruction, temp - instruction);
    }
}

// converts a string of a register name to a register token
static char *getRegNumber(char *token, char *base, char *original) {
    ++token; // skip the "$"
    char *result = malloc(3); // largest register number is 2 digits
    result[0] = '\0'; result[1] = '\0'; result[2] = '\0';
    // is it a numerical register?
    if (isdigit(*token)) {
        // no processing necessary
        Strcpy(result, token);
        return result;
//        sprintf(result, "%s", token);
    } else {
        // need to convert register name to number
        switch (*token) {
            case 'z': // "zero"
                if (*(token + 1) == 'e' && *(token + 2) == 'r' &&
                    *(token + 3) == 'o' && *(token + 4) == '\0') {
                Strcpy(result, "0");
                    //sprintf(result, "%d", 0);
                } else {
                    PARSER_ERR("invalid register number: %s", original,
                               token - base, token);
                }
                break;
            case 'a': // "at" or "a0-a3"
                ++token;
                if (*token == 't' && *(token + 1) == '\0') {// "at"
                    Strcpy(result, "1");
                    //sprintf(result, "%d", 1);
                } else if ('0' <= *token && *token <= '3'
                           && *(token + 1) == '\0') { // "a0-a3"
                    result[0] = '4' + (*token - '0');
                    //sprintf(result, "%d", 4 + (*token - '0'));
                } else {
                    PARSER_ERR("invalid register number: %s", original,
                               token - base - 1, token - 1);
                }
                break;
            case 'v': // "v0 or v1"
                ++token;
                if ('0' <= *token && *token <= '1'
                    && *(token + 1) == '\0') {
                    result[0] = '2' + (*token - '0');
                    //sprintf(result, "%d", 2 + (*token - '0'));
                } else {
                    PARSER_ERR("invalid register number: %s", original,
                               token - base - 1, token - 1);
                }
                break;
            case 't': // "t0-t7" or "t8-t9"
                ++token;
                if ('8' <= *token && *token <= '9'
                    && *(token + 1) == '\0') { // "t8-t9"
                    result[0] = '2';
                    result[1] = '4' + (*token - '8');
                    //sprintf(result, "%d", 16 + (*token - '0'));
                } else if ('0' <= *token && *token <= '7'
                           && *(token + 1) == '\0') { // "t0-t7"
                    if (*token <= '1') {
                        result[0] = '8' + (*token - '0');
                    } else {
                        result[0] = '1';
                        result[1] = *token - 2;
                    }
                    //sprintf(result, "%d", 8 + (*token - '0'));
                } else {
                    PARSER_ERR("invalid register number: %s", original,
                               token - base - 1, token - 1);
                }
                break;
            case 's': // "s0-s7" or "sp"
                ++token;
                if (*token == 'p' && *(token + 1) == '\0') { // "sp"
                    result[0] = '2';
                    result[1] = '9';
                    //sprintf(result, "%d", 29);
                } else if ('0' <= *token && *token <= '7'
                           && *(token + 1) == '\0') { // "s0-s7"
                    if (*token <= '3') {
                        result[0] = '1';
                        result[1] = '6' + (*token - '0');
                    } else {
                        result[0] = '2';
                        result[1] = *token - 4;
                    }
                    //sprintf(result, "%d", 16 + (*token - '0'));
                } else {
                    PARSER_ERR("invalid register number: %s", original,
                               token - base - 1, token - 1);
                }
                break;
            case 'k': // "k0-k1"
                ++token;
                if ('0' <= *token && *token <= '1'
                    && *(token + 1) == '\0') {
                    result[0] = '2';
                    result[1] = '6' + (*token - '0');
                    //sprintf(result, "%d", 26 + (*token - '0'));
                } else {
                    PARSER_ERR("invalid register number: %s", original,
                               token - base - 1, token - 1);
                }
                break;
            case 'g': // "gp"
                if (*(token + 1) == 'p' && *(token + 2) == '\0') {
                    result[0] = '2';
                    result[1] = '8';
                    //sprintf(result, "%d", 28);
                } else {
                    PARSER_ERR("invalid register number: %s", original,
                               token - base - 1, token - 1);
                }
                break;
            case 'f': // "fp"
                if (*(token + 1) == 'p' && *(token + 2) == '\0') {
                    result[0] = '3';
                    result[1] = '0';
                    //sprintf(result, "%d", 30);
                } else {
                    PARSER_ERR("invalid register number: %s", original,
                               token - base, token);
                }
                break;
            case 'r': // "ra"
                if (*(token + 1) == 'a' && *(token + 2) == '\0') {
                    result[0] = '3';
                    result[1] = '1';
                    //sprintf(result, "%d", 31);
                } else {
                    PARSER_ERR("invalid register number: %s", original,
                               token - base, token);
                }
                break;
            default:
                PARSER_ERR("invalid register number: %s", original,
                           token - base, token);
        }

        return result;
    }
}

// determines the op of the given instruction
static enum inst_op getOp(char *instruction) {
    switch (*instruction) {
        case 'h': // "haltSimulation"
            if (Strstr(instruction, "haltSimulation") == NULL) {
                return ERR;
            } else {
                return HALT;
            }
        case 'a': // "add" or "addi"
            if (*(instruction + 1) != 'd' || *(instruction + 2) != 'd') {
                return ERR; // operation is invalid
            }
            if (*(instruction + 3) == 'i' &&
                    (*(instruction + 4) == ' ' || *(instruction + 4) == '\0')) {
                return ADDI;
            }
            if (*(instruction + 3) == ' ' || *(instruction + 3) == '\0') {
                return ADD;
            } else return ERR;
        case 'b': // "beq"
            if (*(instruction + 1) != 'e' || *(instruction + 2) != 'q') {
                return ERR;
            }
            if (*(instruction + 3) == ' ' || *(instruction + 3) == '\0') {
                return BEQ;
            } else return ERR;
        case 'l': // "lw"
            if (*(instruction + 1) != 'w') {
                return ERR;
            }
            if (*(instruction + 2) == ' ' || *(instruction + 2) == '\0') {
                return LW;
            } else return ERR;
        case 'm': // "mul"
            if (*(instruction + 1) != 'u' || *(instruction + 2) != 'l') {
                return ERR;
            }
            if (*(instruction + 3) == ' ' || *(instruction + 3) == '\0') {
                return MUL;
            } else return ERR;
        case 's': // "sub" or "sw"
            if (*(instruction + 1) == 'w' &&
                (*(instruction + 2) == ' ' || *(instruction + 2) == '\0')) {
                return SW;
            }
            if (*(instruction + 1) == 'u' && *(instruction + 2) == 'b'
                && (*(instruction + 3) == ' ' || *(instruction + 3) == '\0')) {
                return SUB;
            }
            return ERR;
        case '#': // # a comment
            return COMMENT;
        default:
            return ERR;
    }
}

// determines the type of the given op
static enum inst_type getInstType(enum inst_op op) {
    switch (op) {
        case ADD:
        case MUL:
        case SUB:
            return R_TYPE;
        case ADDI:
        case BEQ:
        case LW:
        case SW:
            return I_TYPE;
        case HALT:
        default:
            return NA;
    }
}

// parses an R-Type instruction of the form (op rd rs rt), exiting upon error
static void parseRType(struct inst *inst, char *converted,
                       char *remainingTokens) {
    if (!isdigit(*remainingTokens)) {
        PARSER_ERR("expected a digit for rd, found: %s", converted,
                   remainingTokens - converted, strtok(remainingTokens, " "));
    }
    inst->rd = (uint8_t) Strtol(&remainingTokens, 0, 31, converted,
                                remainingTokens - converted);
    ++remainingTokens; // skip the space

    if (!isdigit(*remainingTokens)) {
        PARSER_ERR("expected a digit for rs, found: %s", converted,
                   remainingTokens - converted, strtok(remainingTokens, " "));
    }
    inst->rs = (uint16_t) Strtol(&remainingTokens, 0, 31, converted,
                                 remainingTokens - converted);
    ++remainingTokens;

    if (!isdigit(*remainingTokens)) {
        PARSER_ERR("expected a digit for rt, found: %s", converted,
                   remainingTokens - converted, remainingTokens);
    }
    inst->rt = (uint16_t) Strtol(&remainingTokens, 0, 31, converted,
                                 remainingTokens - converted);
}

// delegates to an appropriate method for the given instruction
static void parseIType(struct inst *inst, char *converted,
                       char *remainingTokens) {
    switch (inst->op) {
        case ADDI:
            parseAddi(inst, converted, remainingTokens);
            break;
        case BEQ:
            parseBeq(inst, converted, remainingTokens);
            break;
        case LW:
        case SW:
            parseLwSw(inst, converted, remainingTokens);
            break;
        default:
            PARSER_ERR("unrecognized instruction", converted,
                       remainingTokens - converted);
    }
}

// parses an addi instruction of the form (addi rd rs imm), exiting upon error
static void parseAddi(struct inst *inst, char *converted,
                      char *remainingTokens) {
    if (!isdigit(*remainingTokens)) {
        PARSER_ERR("expected a digit for rt, found: %s", converted,
                   remainingTokens - converted, strtok(remainingTokens, " "));
    }
    inst->rt = (uint8_t) Strtol(&remainingTokens, 0, 31, converted,
                                remainingTokens - converted);
    ++remainingTokens;

    if (!isdigit(*remainingTokens)) {
        PARSER_ERR("expected a digit for rs, found: %s", converted,
                   remainingTokens - converted, strtok(remainingTokens, " "));
    }
    inst->rs = (uint16_t) Strtol(&remainingTokens, 0, 31, converted,
                                 remainingTokens - converted);
    ++remainingTokens;

    if (!isdigit(*remainingTokens) && *remainingTokens != '-') {
        PARSER_ERR("expected a digit for the immediate, found: %s", converted,
                   remainingTokens - converted, remainingTokens);
    }
    inst->immediate = (int16_t) Strtol(&remainingTokens, INT16_MIN, INT16_MAX,
                                       converted, remainingTokens - converted);
}

// parses a beq instruction of the form (beq rt rs offset), exiting upon error
static void parseBeq(struct inst *inst, char *converted,
                     char *remainingTokens) {
    if (!isdigit(*remainingTokens)) {
        PARSER_ERR("expected a digit for rt, found: %s", converted,
                   remainingTokens - converted, strtok(remainingTokens, " "));
    }
    inst->rt = (uint16_t) Strtol(&remainingTokens, 0, 31, converted,
                                 remainingTokens - converted);
    ++remainingTokens;

    if (!isdigit(*remainingTokens)) {
        PARSER_ERR("expected a digit for rs, found: %s", converted,
                   remainingTokens - converted, strtok(remainingTokens, " "));
    }
    inst->rs = (uint16_t) Strtol(&remainingTokens, 0, 31, converted,
                                 remainingTokens - converted);
    ++remainingTokens;

    if (!isdigit(*remainingTokens) && *remainingTokens != '-') {
        PARSER_ERR("expected a digit for the immediate, found: %s", converted,
                   remainingTokens - converted, remainingTokens);
    }
    inst->immediate = (int16_t) Strtol(&remainingTokens, INT16_MIN, INT16_MAX,
                                       converted, remainingTokens - converted);
}

// parses a lw/sw instruction of the form (lw/sw rt offset rs), exiting upon error
static void parseLwSw(struct inst *inst, char *converted,
                      char *remainingTokens) {
    if (!isdigit(*remainingTokens)) {
        PARSER_ERR("expected a digit for rt, found: %s", converted,
                   remainingTokens - converted, strtok(remainingTokens, " "));
    }
    inst->rt = (uint16_t) Strtol(&remainingTokens, 0, 31, converted,
                                 remainingTokens - converted);
    ++remainingTokens;

    if (!isdigit(*remainingTokens) && *remainingTokens != '-') {
        PARSER_ERR("expected a digit for the immediate, found: %s", converted,
                   remainingTokens - converted, strtok(remainingTokens, " "));
    }
    inst->immediate = (int16_t) Strtol(&remainingTokens, INT16_MIN, INT16_MAX,
                                       converted, remainingTokens - converted);
    ++remainingTokens;

    if (!isdigit(*remainingTokens)) {
        PARSER_ERR("expected a digit for rs, found: %s", converted,
                   remainingTokens - converted, remainingTokens);
    }
    inst->rs = (uint16_t) Strtol(&remainingTokens, 0, 31, converted,
                                 remainingTokens - converted);

    // assert that memory access is aligned to 4
    if ((inst->immediate) & 0x3) {
        PARSER_ERR("misaligned memory access", converted, 0);
    }
}

// wrapper for strtol tht handles errors and range checking
static long Strtol(char **numStr, int min, int max, char *inst, long col) {
    int origErrno = errno;
    long num = strtol(*numStr, numStr, 0);

    if (errno == ERANGE || num == LONG_MAX || num == LONG_MIN) {
        PARSER_ERR("couldn't parse number: %s", inst, col,
                   strtok(*numStr, " "));
    }

    // range check
    if (num > max || num < min) {
        PARSER_ERR("%d is out of bounds for this field - "
                   "please use a number between [%d, %d]",
                   inst, col, num, min, max);
    }

    errno = origErrno;
    return num;
}

// Logs an error and exits the program
static void parserErr(const char *function, int line, const char *msg,
                      const char *inst, long col, ...) {
    va_list args;
    va_start(args, col);

    fprintf(stderr, "[ERROR - %s#%d] ", function, line);
    vfprintf(stderr, msg, args);
    fprintf(stderr, "\n%s\n", inst);

    if (col > 0) {
        int i;
        for (i = 0; i < col; ++i) fprintf(stderr, " ");
        fprintf(stderr, "^\n");
    }

    va_end(args);
    exit(EXIT_FAILURE);
}

// delegates to an appropriate validation method, which exits if instruction is invalid
static void validate(const char *instruction, enum inst_op op,
                     enum inst_type type) {
    if (type == R_TYPE) validateRType(instruction);
    else if (type == I_TYPE) validateIType(instruction, op);
}

// validates R-Type instructions of the form (op register register register)
static void validateRType(const char *instruction) {
    // op_name has already been validated, skip it
    char *cur = Strchr(instruction, ' ');
    if (!cur) {
        PARSER_ERR("too few arguments to instruction: missing rd, rs, and rt",
                   instruction, 0);
    }

    // each of the next three operands should be registers (i.e. start with '$')
    if (*(cur + 1) != '$') {
        PARSER_ERR("malformed register for rd", instruction,
                   cur + 1 - instruction);
    }

    cur = Strchr(cur + 1, ' ');
    if (!cur) {
        PARSER_ERR("too few arguments to instruction: missing rs and rt",
                   instruction, 0);
    }

    if (*(cur + 1) != '$') {
        PARSER_ERR("malformed register for rs", instruction,
                   cur + 1 - instruction);
    }

    cur = Strchr(cur + 1, ' ');
    if (!cur) {
        PARSER_ERR("too few arguments to instruction: missing rt", instruction,
                   0);
    }

    if (*(cur + 1) != '$') {
        PARSER_ERR("malformed register for rt", instruction,
                   cur + 1 - instruction);
    }

    // check for lingering tokens after the entire instruction is validated
    cur = Strchr(cur + 1, ' ');
    while (cur && isspace(*(cur))) ++cur;
    if (cur && *cur != '\0') {
        PARSER_ERR("malformed instruction, unexpected tokens: %s",
                   instruction, cur - instruction, cur);
    }
}

// delegates to an appropriate validation function for the instruction
static void validateIType(const char *instruction, enum inst_op op) {
    if (op == ADDI || op == BEQ) {
        validateAddiBeq(instruction);
    } else if (op == LW || op == SW) {
        validateLwSw(instruction);
    }
}

// validates addi and beq, of the form (op register register imm/offset)
static void validateAddiBeq(const char *instruction) {
    // op_name has already been validated, skip it
    char *cur = Strchr(instruction, ' ');
    if (!cur) {
        PARSER_ERR("too few arguments to instruction: missing rd/rt, rs, "
                   "and immediate/offset", instruction, 0);
    }

    if (*(cur + 1) != '$') {
        PARSER_ERR("malformed register for rd/rs", instruction,
                   cur + 1 - instruction);
    }

    cur = Strchr(cur + 1, ' ');
    if (!cur) {
        PARSER_ERR("too few arguments to instruction: missing rs and "
                   "immediate/offset", instruction, 0);
    }

    if (*(cur + 1) != '$') {
        PARSER_ERR("malformed register for rs", instruction,
                   cur + 1 - instruction);
    }

    cur = Strchr(cur + 1, ' ');
    if (!cur) {
        PARSER_ERR("too few arguments to instruction: missing immediate/offset",
                   instruction, 0);
    }

    if (!isdigit(*(cur + 1)) && *(cur + 1) != '-') {
        PARSER_ERR("malformed number for the immediate/offset", instruction,
                   cur + 1 - instruction);
    }

    // check for lingering tokens after the entire instruction is validated
    cur = Strchr(cur + 1, ' ');
    while (cur && isspace(*(cur))) ++cur;
    if (cur && *cur != '\0') {
        PARSER_ERR("malformed instruction, unexpected tokens: %s",
                   instruction, cur - instruction, cur);
    }
}

// validates lw/sw of the form (op register offset register)
static void validateLwSw(const char *instruction) {
    // op_name has already been validated, skip it
    char *cur = Strchr(instruction, ' ');
    if (!cur) {
        PARSER_ERR("too few arguments to instruction: missing rt, offset, "
                   "and rs", instruction, 0);
    }

    if (*(cur + 1) != '$') {
        PARSER_ERR("malformed register for rt", instruction,
                   cur + 1 - instruction);
    }

    cur = Strchr(cur + 1, ' ');
    if (!cur) {
        PARSER_ERR("too few arguments to instruction: missing offset and rs",
                   instruction, 0);
    }

    if (!isdigit(*(cur + 1)) && *(cur + 1) != '-') {
        PARSER_ERR("malformed number for the offset", instruction,
                   cur + 1 - instruction);
    }

    cur = Strchr(cur + 1, ' ');
    if (!cur) {
        PARSER_ERR("too few arguments to instruction: missing rs",
                   instruction, 0);
    }

    if (*(cur + 1) != '$') {
        PARSER_ERR("malformed register for rs", instruction,
                   cur + 1 - instruction);
    }

    // check for lingering tokens after the entire instruction is validated
    cur = Strchr(cur + 1, ' ');
    while (cur && isspace(*(cur))) ++cur;
    if (cur && *cur != '\0') {
        PARSER_ERR("malformed instruction, unexpected tokens: %s",
                   instruction, cur - instruction, cur);
    }
}
