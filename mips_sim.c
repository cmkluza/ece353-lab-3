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
    HALT // "haltSimulation"
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
    int16_t rs;
    int16_t rt;
    // destination register - only holds the index of a register
    uint8_t rd;
    // the result from an operation to be written to rd
    int16_t EX_result;
    // immediate or offset value for I-Type instructions
    int16_t immediate;
};

/* ======================= Parsing Function Prototypes ====================== */
/**
 * Reads input from a text file and returns the string as a list of space
 * separated tokens.
 * Treats consecutive commas as a single comma.
 * Asserts proper parenthesis format for loads and stores.
 */
char *progScanner();

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

// regNumberConverter helper functions
static char *getRegNumber(char *token, char *base, char *original);

// parser helper functions
static enum inst_op getOp(char *instruction);
static enum inst_type getInstType(enum inst_op op);
static void parseRType(struct inst *inst, char *converted, char *remainingTokens);
static void parseIType(struct inst *inst, char *converted, char *remainingTokens);
static void parseAddi(struct inst *inst, char *converted, char *remainingTokens);
static void parseBeq(struct inst *inst, char *converted, char *remainingTokens);
static void parseLwSw(struct inst *inst, char *converted, char *remainingTokens);
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

/**
 * Data memory - 2kB.
 * Byte-addressable.
 */
static uint8_t DM[2048];

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
static long Registers[REG_NUM];

/**
* Useful cycle counters
*/
static long IF_WorkCycles, ID_WorkCycles, EX_WorkCycles, MEM_WorkCycles,
    WB_WorkCycles;

/**
* IF and EX instruction cycle counter
*/
static long IF_Inst_Cycles, EX_Inst_Cycles;

// TODO - is this okay?
static int haltPassedWB;

/* ============================== Main Function ============================= */
int main(int argc, char *argv[]) {
    // given variables
    int sim_mode = BATCH; // mode flag, 1 for single-cycle, 0 for batch

    int c; // number of cycles for memory access
    int m; // number of cycles for multiply
    int n; // number of cycles for all other EX operations

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
    // TODO

    /* ========== Main Program Loop ========== */
    while (1) {
        // stop once halt has passed through every stage
        if (haltPassedWB) break;

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
                printf("%ld  ", Registers[i]);
            }
            printf("program counter: %ld\n", PC);
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
            fprintf(output, "%ld  ", Registers[i]);
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
// TODO
char *progScanner() {}

char *regNumberConverter(char *instruction) {
    // local copies of data to prevent modification of external data
    char *copy = malloc(strlen(instruction) + 1);
    char *base = copy;
    strcpy(copy, instruction);

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
            strncpy(buffer + bufferPointer, regNum, len);
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
            strncpy(buffer + bufferPointer, curToken, len);
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
        parseRType(&inst, converted, strchr(converted, ' ') + 1);
    } else if (inst.type == I_TYPE) {
        parseIType(&inst, converted, strchr(converted, ' ') + 1);
    } else {
        // halt instruction, no further processing needed
        free(converted);
        return inst;
    }

    free(converted);
    return inst;
}

// TODO
void IF(void) {
    IF_Inst_Cycles++;
	struct inst curr_inst;
	curr_inst= IM[PC];                                            // create local copy of the instruction to be executed

	if (IF_ID_Flag==0){                                           // check if latch is empty
		if (curr_inst.op==HALT){                  
	    	IF_ID_latch= curr_inst;                               // send the halt instruction to the next stage
			IF_ID_Flag=1;
		}
		if (IF_Inst_Cycles>=c){                
			IF_ID_latch= curr_inst;                               // send the instruction to the next stage
			PC= PC + 4;                                           // change PC to the next instruction
			IF_ID_Flag=1;                                         // set flag IF/ID latch not empty
			IF_Inst_Cycles=0;
			IF_WorkCycles=IF_WorkCycles+c;                        // updates count of useful cycles
		}
	}
}

// TODO
void ID(void) {}

// TODO
void EX(void) {}

// TODO
void MEM(void) {}

// TODO
void WB(void) {}

/* ==================== Helper Function Implementations ===================== */
// converts a string of a register name to a register token
static char *getRegNumber(char *token, char *base, char *original) {
    ++token; // skip the "$"
    char *result = malloc(3); // largest register number is 2 digits
    // is it a numerical register?
    if (isdigit(*token)) {
        // no processing necessary
        sprintf(result, "%s", token);
    } else {
        // need to convert register name to number
        switch (*token) {
            case 'z': // "zero"
                if (*(token + 1) == 'e' && *(token + 2) == 'r' &&
                    *(token + 3) == 'o' && *(token + 4) == '\0') {
                    sprintf(result, "%d", 0);
                } else {
                    PARSER_ERR("invalid register number: %s", original,
                            token - base, token);
                }
                break;
            case 'a': // "at" or "a0-a3"
                ++token;
                if (*token == 't' && *(token + 1) == '\0') {// "at"
                    sprintf(result, "%d", 1);
                } else if ('0' <= *token && *token <= '3'
                           && *(token + 1) == '\0') { // "a0-a3"
                    sprintf(result, "%d", 4 + (*token - '0'));
                } else {
                    PARSER_ERR("invalid register number: %s", original,
                            token - base - 1, token - 1);
                }
                break;
            case 'v': // "v0 or v1"
                ++token;
                if ('0' <= *token && *token <= '1'
                    && *(token + 1) == '\0') {
                    sprintf(result, "%d", 2 + (*token - '0'));
                } else {
                    PARSER_ERR("invalid register number: %s", original,
                            token - base - 1, token - 1);
                }
                break;
            case 't': // "t0-t7" or "t8-t9"
                ++token;
                if ('8' <= *token && *token <= '9'
                    && *(token + 1) == '\0') { // "t8-t9"
                    sprintf(result, "%d", 16 + (*token - '0'));
                } else if ('0' <= *token && *token <= '7'
                           && *(token + 1) == '\0') { // "t0-t7"
                    sprintf(result, "%d", 8 + (*token - '0'));
                } else {
                    PARSER_ERR("invalid register number: %s", original,
                            token - base - 1, token - 1);
                }
                break;
            case 's': // "s0-s7" or "sp"
                ++token;
                if (*token == 'p' && *(token + 1) == '\0') { // "sp"
                    sprintf(result, "%d", 29);
                } else if ('0' <= *token && *token <= '7'
                           && *(token + 1) == '\0') { // "s0-s7"
                    sprintf(result, "%d", 16 + (*token - '0'));
                } else {
                    PARSER_ERR("invalid register number: %s", original,
                            token - base - 1, token - 1);
                }
                break;
            case 'k': // "k0-k1"
                ++token;
                if ('0' <= *token && *token <= '1'
                    && *(token + 1) == '\0') {
                    sprintf(result, "%d", 26 + (*token - '0'));
                } else {
                    PARSER_ERR("invalid register number: %s", original,
                            token - base - 1, token - 1);
                }
                break;
            case 'g': // "gp"
                if (*(token + 1) == 'p' && *(token + 2) == '\0') {
                    sprintf(result, "%d", 28);
                } else {
                    PARSER_ERR("invalid register number: %s", original,
                            token - base - 1, token - 1);
                }
                break;
            case 'f': // "fp"
                if (*(token + 1) == 'p' && *(token + 2) == '\0') {
                    sprintf(result, "%d", 30);
                } else {
                    PARSER_ERR("invalid register number: %s", original,
                            token - base, token);
                }
                break;
            case 'r': // "ra"
                if (*(token + 1) == 'a' && *(token + 2) == '\0') {
                    sprintf(result, "%d", 31);
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
            if (strcmp("haltSimulation", instruction) != 0) {
                return ERR;
            } else {
                return HALT;
            }
        case 'a': // "add" or "addi"
            if (*(instruction + 1) != 'd' || *(instruction + 2) != 'd') {
                return ERR; // operation is invalid
            }
            if (*(instruction + 3) && *(instruction + 3) == 'i') {
                return ADDI;
            }
            return ADD;
        case 'b': // "beq"
            if (*(instruction + 1) != 'e' || *(instruction + 2) != 'q') {
                return ERR;
            }
            return BEQ;
        case 'l': // "lw"
            if (*(instruction + 1) != 'w') {
                return ERR;
            }
            return LW;
        case 'm': // "mul"
            if (*(instruction + 1) != 'u' || *(instruction + 2) != 'l') {
                return ERR;
            }
            return MUL;
        case 's': // "sub" or "sw"
            if (*(instruction + 1) == 'w') {
                return SW;
            }
            if (*(instruction + 1) == 'u' && *(instruction + 2) == 'b') {
                return SUB;
            }
            return ERR;
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
        PARSER_ERR("expected a digit for rd, found: %s", converted,
                   remainingTokens - converted, strtok(remainingTokens, " "));
    }
    inst->rd = (uint8_t) Strtol(&remainingTokens, 0, 31, converted,
                                remainingTokens - converted);
    ++remainingTokens;

    if (!isdigit(*remainingTokens)) {
        PARSER_ERR("expected a digit for rs, found: %s", converted,
                   remainingTokens - converted, strtok(remainingTokens, " "));
    }
    inst->rs = (uint16_t) Strtol(&remainingTokens, 0, 31, converted,
            remainingTokens - converted);
    ++remainingTokens;

    if (!isdigit(*remainingTokens) || *remainingTokens == '-') {
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

    if (!isdigit(*remainingTokens) || *remainingTokens == '-') {
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

    if (!isdigit(*remainingTokens)) {
        PARSER_ERR("expected a digit for the immediate, found: %s", converted,
                   remainingTokens - converted, strtok(remainingTokens, " "));
    }
    inst->immediate = (int16_t) Strtol(&remainingTokens, INT16_MIN, INT16_MAX,
            converted, remainingTokens - converted);
    // assert that memory access is aligned to 4
    if (inst->immediate & 0x3) {
        PARSER_ERR("misaligned memory access", converted, 0);
    }
    ++remainingTokens;

    if (!isdigit(*remainingTokens)) {
        PARSER_ERR("expected a digit for rs, found: %s", converted,
                   remainingTokens - converted, remainingTokens);
    }
    inst->rs = (uint16_t) Strtol(&remainingTokens, 0, 31, converted,
            remainingTokens - converted);
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
        for (int i = 0; i < col; ++i) fprintf(stderr, " ");
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
    char *cur = strchr(instruction, ' ');
    if (!cur) {
        PARSER_ERR("too few arguments to instruction: missing rd, rs, and rt",
                instruction, 0);
    }

    // each of the next three operands should be registers (i.e. start with '$')
    if (*(cur + 1) != '$') {
        PARSER_ERR("malformed register for rd", instruction,
                   cur + 1 - instruction);
    }

    cur = strchr(cur + 1, ' ');
    if (!cur) {
        PARSER_ERR("too few arguments to instruction: missing rs and rt",
                instruction, 0);
    }

    if (*(cur + 1) != '$') {
        PARSER_ERR("malformed register for rs", instruction,
                   cur + 1 - instruction);
    }

    cur = strchr(cur + 1, ' ');
    if (!cur) {
        PARSER_ERR("too few arguments to instruction: missing rt", instruction,
                0);
    }

    if (*(cur + 1) != '$') {
        PARSER_ERR("malformed register for rt", instruction,
                   cur + 1 - instruction);
    }

    // check for lingering tokens after the entire instruction is validated
    cur = strchr(cur + 1, ' ');
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
    char *cur = strchr(instruction, ' ');
    if (!cur) {
        PARSER_ERR("too few arguments to instruction: missing rd/rt, rs, "
                   "and immediate/offset", instruction, 0);
    }

    if (*(cur + 1) != '$') {
        PARSER_ERR("malformed register for rd/rs", instruction,
                cur + 1 - instruction);
    }

    cur = strchr(cur + 1, ' ');
    if (!cur) {
        PARSER_ERR("too few arguments to instruction: missing rs and "
                   "immediate/offset", instruction, 0);
    }

    if (*(cur + 1) != '$') {
        PARSER_ERR("malformed register for rs", instruction,
                cur + 1 - instruction);
    }

    cur = strchr(cur + 1, ' ');
    if (!cur) {
        PARSER_ERR("too few arguments to instruction: missing immediate/offset",
                   instruction, 0);
    }

    if (!isdigit(*(cur + 1))) {
        PARSER_ERR("malformed number for the immediate/offset", instruction,
                   cur + 1 - instruction);
    }

    // check for lingering tokens after the entire instruction is validated
    cur = strchr(cur + 1, ' ');
    while (cur && isspace(*(cur))) ++cur;
    if (cur && *cur != '\0') {
        PARSER_ERR("malformed instruction, unexpected tokens: %s",
                   instruction, cur - instruction, cur);
    }
}

// validates lw/sw of the form (op register offset register)
static void validateLwSw(const char *instruction) {
    // op_name has already been validated, skip it
    char *cur = strchr(instruction, ' ');
    if (!cur) {
        PARSER_ERR("too few arguments to instruction: missing rt, offset, "
                   "and rs", instruction, 0);
    }

    if (*(cur + 1) != '$') {
        PARSER_ERR("malformed register for rt", instruction,
                   cur + 1 - instruction);
    }

    cur = strchr(cur + 1, ' ');
    if (!cur) {
        PARSER_ERR("too few arguments to instruction: missing offset and rs",
                   instruction, 0);
    }

    if (!isdigit(*(cur + 1))) {
        PARSER_ERR("malformed number for the offset", instruction,
                   cur + 1 - instruction);
    }

    cur = strchr(cur + 1, ' ');
    if (!cur) {
        PARSER_ERR("too few arguments to instruction: missing rs",
                   instruction, 0);
    }

    if (*(cur + 1) != '$') {
        PARSER_ERR("malformed register for rs", instruction,
                   cur + 1 - instruction);
    }

    // check for lingering tokens after the entire instruction is validated
    cur = strchr(cur + 1, ' ');
    while (cur && isspace(*(cur))) ++cur;
    if (cur && *cur != '\0') {
        PARSER_ERR("malformed instruction, unexpected tokens: %s",
                   instruction, cur - instruction, cur);
    }
}
