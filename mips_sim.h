/**
 * Authors: Laura DeBurgo, Dametreuss Francois, Cameron Kluza, Kyle McWherter
 */
#ifndef LAB_3_TEST_MIPS_SIM_H
#define LAB_3_TEST_MIPS_SIM_H

#include <stdint.h>

/* ========== Structs and enums that might be used in the implementation ========== */

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
    NA, // not applicable (i.e. for HALT)
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

/**
 * Represents a latch that is placed between
 */
struct latch {
    struct inst inst;
    uint8_t is_done;
};

/* ========== Globals that might be used in the implementation ========== */
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
 * Program counter.
 */
static int PC;

/**
 * Latches.
 */
static struct latch IF_ID_latch, ID_EX_latch, EX_MEM_latch, MEM_WB_latch;

/**
*Flags
*/
static int IF_ID_Flag, ID_EX_Flag, EX_MEM_Flag, MEM_WB_Flag, WB_HALT_Flag;

/**
*Registers
*/
static int Registers[32]

// anything else we need

#endif //LAB_3_TEST_MIPS_SIM_H
