#include <mem.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>

#include "mips_sim.h"

// helper macro that fills in fatalErr call with redundant information
#define FATAL_ERR(msg, inst, col, ...) fatalErr(__FUNCTION__, __LINE__, msg, inst, col, ##__VA_ARGS__)

/* ====== Prototypes ====== */
// regNumberConverter helpers
static char *getRegNumber(char *token, char *base, char *original);

// parser helpers
static enum inst_op getOp(char *instruction);

static enum inst_type getInstType(enum inst_op op);

static void parseRType(struct inst *inst, char *converted, char *remainingTokens);

static void parseIType(struct inst *inst, char *converted, char *remainingTokens);

static void parseAddi(struct inst *inst, char *converted, char *remainingTokens);

static void parseBeq(struct inst *inst, char *converted, char *remainingTokens);

static void parseLwSw(struct inst *inst, char *converted, char *remainingTokens);

static long Strtol(char **numStr, int min, int max, char *inst, long col);

static void fatalErr(const char *function, int line, const char *msg,
                     const char *inst, long col, ...);

// validate helpers
static void validate(const char *instruction, enum inst_op op, enum inst_type type);

static void validateRType(const char *instruction);

static void validateIType(const char *instruction, enum inst_op op);

static void validateAddiBeq(const char *instruction);

static void validateLwSw(const char *instruction);

/* ====== Primary Functions ====== */

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
            if (bufferPointer + len >= 256) {
                FATAL_ERR("invalid instruction", copy, 0);
            }
            // copy the token into buffer and add the space afterwards
            strncpy(buffer + bufferPointer, curToken, len);
            bufferPointer += len;
            buffer[bufferPointer++] = ' ';
        }
    } while ((curToken = strtok(NULL, " ")) != NULL);

    free(copy);

    // null-terminate and return
    buffer[bufferPointer] = '\0';
    return buffer;
}

struct inst parser(char *instruction) {
    struct inst inst;
    char *converted = regNumberConverter(instruction);

    // set the op
    if ((inst.op = getOp(converted)) == ERR) {
        FATAL_ERR("unrecognized op in instruction", instruction, 0);
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

/* ====== regNumberConverter helpers ====== */

/**
 * Converts the given token into a number representing its register.
 * The return value from this method must be freed by the caller.
 *
 * @param token string representing the token
 * @return string representing the register number
 */
static char *getRegNumber(char *token, char *base, char *original) {
    token++; // skip the "$"
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
                    FATAL_ERR("invalid register number: %s", original, token - base, token);
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
                    FATAL_ERR("invalid register number: %s", original, token - base - 1, token - 1);
                }
                break;
            case 'v': // "v0 or v1"
                ++token;
                if ('0' <= *token && *token <= '1'
                    && *(token + 1) == '\0') {
                    sprintf(result, "%d", 2 + (*token - '0'));
                } else {
                    FATAL_ERR("invalid register number: %s", original, token - base - 1, token - 1);
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
                    FATAL_ERR("invalid register number: %s", original, token - base - 1, token - 1);
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
                    FATAL_ERR("invalid register number: %s", original, token - base - 1, token - 1);
                }
                break;
            case 'k': // "k0-k1"
                ++token;
                if ('0' <= *token && *token <= '1'
                    && *(token + 1) == '\0') {
                    sprintf(result, "%d", 26 + (*token - '0'));
                } else {
                    FATAL_ERR("invalid register number: %s", original, token - base - 1, token - 1);
                }
                break;
            case 'g': // "gp"
                if (*(token + 1) == 'p' && *(token + 2) == '\0') {
                    sprintf(result, "%d", 28);
                } else {
                    FATAL_ERR("invalid register number: %s", original, token - base - 1, token - 1);
                }
                break;
            case 'f': // "fp"
                if (*(token + 1) == 'p' && *(token + 2) == '\0') {
                    sprintf(result, "%d", 30);
                } else {
                    FATAL_ERR("invalid register number: %s", original, token - base, token);
                }
                break;
            case 'r': // "ra"
                if (*(token + 1) == 'a' && *(token + 2) == '\0') {
                    sprintf(result, "%d", 31);
                } else {
                    FATAL_ERR("invalid register number: %s", original, token - base, token);
                }
                break;
            default:
                FATAL_ERR("invalid register number: %s", original, token - base, token);
        }

        return result;
    }
}

/* ====== parser helpers ====== */

/**
 * Determines the operation from the given operation token.
 * @param instruction the instruction string pointing to the start of the instruction
 * @return the parsed op (equal to ERR if the op is unrecognized)
 */
static enum inst_op getOp(char *instruction) {
    switch (*instruction) {
        case 'h': // "haltSimulation"
            return HALT;
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

/**
 * Determines what kind of instruction the given operation is.
 * @param op the operation to get the instruction type of
 * @return the corresponding instruction type
 */
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

/**
 * Parses an R-Type instruction and stores the appropriate info in the passed instruction.
 * The R-Type instructions we're working with all have the following format:
 * - op rd rs rt
 * Exits the program upon seeing an error.
 */
static void parseRType(struct inst *inst, char *converted, char *remainingTokens) {
    if (!isdigit(*remainingTokens))
        FATAL_ERR("expected a digit for rd, found: %s", converted,
                  remainingTokens - converted, strtok(remainingTokens, " "));
    inst->rd = (uint8_t) Strtol(&remainingTokens, 0, 31, converted, remainingTokens - converted);
    ++remainingTokens; // skip the space

    if (!isdigit(*remainingTokens))
        FATAL_ERR("expected a digit for rs, found: %s", converted,
                  remainingTokens - converted, strtok(remainingTokens, " "));
    inst->rs = (uint16_t) Strtol(&remainingTokens, 0, 31, converted, remainingTokens - converted);
    ++remainingTokens;

    if (!isdigit(*remainingTokens))
        FATAL_ERR("expected a digit for rt, found: %s", converted,
                  remainingTokens - converted, remainingTokens);
    inst->rt = (uint16_t) Strtol(&remainingTokens, 0, 31, converted, remainingTokens - converted);
}

/**
 * Delegates parsing to an appropriate method for the given instruction.
 */
static void parseIType(struct inst *inst, char *converted, char *remainingTokens) {
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
            FATAL_ERR("unrecognized instruction", converted, remainingTokens - converted);
    }
}

/**
 * Parses the given add immediate instruction:
 * - addi rd rs imm
 * Exits the program upon seeing an error.
 */
static void parseAddi(struct inst *inst, char *converted, char *remainingTokens) {
    if (!isdigit(*remainingTokens))
        FATAL_ERR("expected a digit for rd, found: %s", converted,
                  remainingTokens - converted, strtok(remainingTokens, " "));
    inst->rd = (uint8_t) Strtol(&remainingTokens, 0, 31, converted, remainingTokens - converted);
    ++remainingTokens;

    if (!isdigit(*remainingTokens))
        FATAL_ERR("expected a digit for rs, found: %s", converted,
                  remainingTokens - converted, strtok(remainingTokens, " "));
    inst->rs = (uint16_t) Strtol(&remainingTokens, 0, 31, converted, remainingTokens - converted);
    ++remainingTokens;

    if (!isdigit(*remainingTokens) || *remainingTokens == '-')
        FATAL_ERR("expected a digit for the immediate, found: %s", converted,
                  remainingTokens - converted, remainingTokens);
    inst->immediate = (int16_t) Strtol(&remainingTokens, INT16_MIN, INT16_MAX, converted, remainingTokens - converted);
}

/**
 * Parses the given branch on equal instruction.
 * - beq rt rs offset
 * Exits the program upon seeing an error.
 */
static void parseBeq(struct inst *inst, char *converted, char *remainingTokens) {
    if (!isdigit(*remainingTokens))
        FATAL_ERR("expected a digit for rt, found: %s", converted,
                  remainingTokens - converted, strtok(remainingTokens, " "));
    inst->rt = (uint16_t) Strtol(&remainingTokens, 0, 31, converted, remainingTokens - converted);
    ++remainingTokens;

    if (!isdigit(*remainingTokens))
        FATAL_ERR("expected a digit for rs, found: %s", converted,
                  remainingTokens - converted, strtok(remainingTokens, " "));
    inst->rs = (uint16_t) Strtol(&remainingTokens, 0, 31, converted, remainingTokens - converted);
    ++remainingTokens;

    if (!isdigit(*remainingTokens) || *remainingTokens == '-')
        FATAL_ERR("expected a digit for the immediate, found: %s", converted,
                  remainingTokens - converted, remainingTokens);
    inst->immediate = (int16_t) Strtol(&remainingTokens, INT16_MIN, INT16_MAX, converted, remainingTokens - converted);
}

/**
 * Parses the given load or store word instruction.
 * - lw/sw rt offset rs
 * Exits the program upon seeing an error.
 */
static void parseLwSw(struct inst *inst, char *converted, char *remainingTokens) {
    if (!isdigit(*remainingTokens))
        FATAL_ERR("expected a digit for rt, found: %s", converted,
                  remainingTokens - converted, strtok(remainingTokens, " "));
    inst->rt = (uint16_t) Strtol(&remainingTokens, 0, 31, converted, remainingTokens - converted);
    ++remainingTokens;

    if (!isdigit(*remainingTokens))
        FATAL_ERR("expected a digit for the immediate, found: %s", converted,
                  remainingTokens - converted, strtok(remainingTokens, " "));
    inst->immediate = (int16_t) Strtol(&remainingTokens, INT16_MIN, INT16_MAX, converted, remainingTokens - converted);
    // assert that memory access is aligned to 4
    if (inst->immediate & 0x3) {
        FATAL_ERR("misaligned memory access", converted, 0);
    }
    ++remainingTokens;

    if (!isdigit(*remainingTokens))
        FATAL_ERR("expected a digit for rs, found: %s", converted,
                  remainingTokens - converted, remainingTokens);
    inst->rs = (uint16_t) Strtol(&remainingTokens, 0, 31, converted, remainingTokens - converted);
}

/**
 * Wrapper for strtol that handles errors.
 * Moves the passed pointer to after the parsed number.
 */
static long Strtol(char **numStr, int min, int max, char *inst, long col) {
    int origErrno = errno;
    long num = strtol(*numStr, numStr, 0);

    if (errno == ERANGE || num == LONG_MAX || num == LONG_MIN) {
        FATAL_ERR("couldn't parse number: %s", inst, col, strtok(*numStr, " "));
    }

    // range check
    if (num > max || num < min) {
        FATAL_ERR("%d is out of bounds for this field - please use a number between [%d, %d]",
                  inst, col, num, min, max);
    }

    errno = origErrno;
    return num;
}

/**
 * Logs a fatal error and quits the program.
 */
static void fatalErr(const char *function, int line, const char *msg,
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

/*  ====== parser and helpers ====== */

/**
 * Delegates to the appropriate validation method for the given op type.
 *
 * This function and its delegates ensure instructions are formatted properly
 * and will exit the program if they aren't.
 *
 * This function should be called with the output of progScanner
 * (i.e. single space-separated tokens)
 */
static void validate(const char *instruction, enum inst_op op, enum inst_type type) {
    if (type == R_TYPE) validateRType(instruction);
    else if (type == I_TYPE) validateIType(instruction, op);
}

/**
 * Exits the program if the given instruction doesn't follow the following R-Type format:
 * op_name register(rd) register(rs) register(rt)
 */
static void validateRType(const char *instruction) {
    // op_name has already been validated, skip it
    char *cur = strchr(instruction, ' ');
    if (!cur) {
        FATAL_ERR("too few arguments to instruction: missing rd, rs, and rt", instruction, 0);
    }

    // each of the next three operands should be registers (i.e. start with '$')
    if (*(cur + 1) != '$') {
        FATAL_ERR("malformed register for rd", instruction,
                  cur + 1 - instruction);
    }

    cur = strchr(cur + 1, ' ');
    if (!cur) {
        FATAL_ERR("too few arguments to instruction: missing rs and rt", instruction, 0);
    }

    if (*(cur + 1) != '$') {
        FATAL_ERR("malformed register for rs", instruction,
                  cur + 1 - instruction);
    }

    cur = strchr(cur + 1, ' ');
    if (!cur) {
        FATAL_ERR("too few arguments to instruction: missing rt", instruction, 0);
    }

    if (*(cur + 1) != '$') {
        FATAL_ERR("malformed register for rt", instruction,
                  cur + 1 - instruction);
    }

    // check for lingering tokens after the entire instruction is validated
    cur = strchr(cur + 1, ' ');
    while (cur && isspace(*(cur))) ++cur;
    if (cur && *cur != '\0') {
        FATAL_ERR("malformed instruction, unexpected tokens: %s",
                  instruction, cur - instruction, cur);
    }
}

/**
 * Delegates to the appropriate I-Type validation function.
 */
static void validateIType(const char *instruction, enum inst_op op) {
    if (op == ADDI || op == BEQ) {
        validateAddiBeq(instruction);
    } else if (op == LW || op == SW) {
        validateLwSw(instruction);
    }
}

/**
 * Exits the program if the given instruction doesn't follow the following format:
 * op_name register(rd/rt) register(rs) immediate/offset(imm)
 */
static void validateAddiBeq(const char *instruction) {
    // op_name has already been validated, skip it
    char *cur = strchr(instruction, ' ');
    if (!cur) {
        FATAL_ERR("too few arguments to instruction: missing rd/rt, rs, and immediate/offset",
                  instruction, 0);
    }

    if (*(cur + 1) != '$') {
        FATAL_ERR("malformed register for rd/rs", instruction, cur + 1 - instruction);
    }

    cur = strchr(cur + 1, ' ');
    if (!cur) {
        FATAL_ERR("too few arguments to instruction: missing rs and immediate/offset",
                  instruction, 0);
    }

    if (*(cur + 1) != '$') {
        FATAL_ERR("malformed register for rs", instruction, cur + 1 - instruction);
    }

    cur = strchr(cur + 1, ' ');
    if (!cur) {
        FATAL_ERR("too few arguments to instruction: missing immediate/offset",
                  instruction, 0);
    }

    if (!isdigit(*(cur + 1))) {
        FATAL_ERR("malformed number for the immediate/offset", instruction,
                  cur + 1 - instruction);
    }

    // check for lingering tokens after the entire instruction is validated
    cur = strchr(cur + 1, ' ');
    while (cur && isspace(*(cur))) ++cur;
    if (cur && *cur != '\0') {
        FATAL_ERR("malformed instruction, unexpected tokens: %s",
                  instruction, cur - instruction, cur);
    }
}

/**
 * Exits the program if the given instruction doesn't follow the following format:
 * op_name register(rt) offset register(rs)
 */
static void validateLwSw(const char *instruction) {
    // op_name has already been validated, skip it
    char *cur = strchr(instruction, ' ');
    if (!cur) {
        FATAL_ERR("too few arguments to instruction: missing rt, offset, and rs",
                  instruction, 0);
    }

    if (*(cur + 1) != '$') {
        FATAL_ERR("malformed register for rt", instruction,
                  cur + 1 - instruction);
    }

    cur = strchr(cur + 1, ' ');
    if (!cur) {
        FATAL_ERR("too few arguments to instruction: missing offset and rs",
                  instruction, 0);
    }

    if (!isdigit(*(cur + 1))) {
        FATAL_ERR("malformed number for the offset", instruction,
                  cur + 1 - instruction);
    }

    cur = strchr(cur + 1, ' ');
    if (!cur) {
        FATAL_ERR("too few arguments to instruction: missing rs",
                  instruction, 0);
    }

    if (*(cur + 1) != '$') {
        FATAL_ERR("malformed register for rs", instruction,
                  cur + 1 - instruction);
    }

    // check for lingering tokens after the entire instruction is validated
    cur = strchr(cur + 1, ' ');
    while (cur && isspace(*(cur))) ++cur;
    if (cur && *cur != '\0') {
        FATAL_ERR("malformed instruction, unexpected tokens: %s",
                  instruction, cur - instruction, cur);
    }
}