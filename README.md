# MIPS Pipeline Simulator

#### To compile:
```
gcc -o sim-mips mips_sim.c 
```

#### To run tests:
```
chmod u+x run_tests.sh
./run_tests.sh
```

#### To add an `i`th test:
* Create `testi` in `./tests/` with three lines:
    1. Description (this is printed out with the test number)
    2. Program arguments, e.g.
        >`./sim-mips -b 1 1 1 ./tests/asm/testi.s ./tests/out/testi.out`
    3. Solution file, e.g.
        >`./tests/sols/testi.sol`
* (Optional) Create a `testi.s` MIPS assembly file in `./tests/asm`
    * This simulator uses a subset of MIPS asm described below
* (Optional) Create a `testi.sol` file in `./tests/sols`
    * This file simply lists the expected state of registers 1-31 and the 
    program counter, with each number separated by two spaces:
        >`Register[1]␣␣Register[2]␣␣Register[3]␣␣...␣␣Register[31]␣␣PC`  
        (where `␣` represents a space)
* Adjust the hard-coded test number variable `NUM_TESTS` in `./run_tests.sh` to 
reflect how many test files there are

#### MIPS Specification
This simulator uses a subset of MIPS assembly instructions, and will not
work with standard MIPS programs. There are no sections, no labels, and no
data declarations - only a list of instructions. **The input program 
must end with the `haltSimulation` directive**. For examples, look at the 
programs in [tests/asm](tests/asm/).

##### Supported Standard Instructions:
* `add`
* `addi`
* `beq`
    * Only supports numerical offsets - no labels
* `lw`
* `sub`
* `sw`
##### Supported Pseudo-Instruction
* `mul`
    * This instruction stores the low-order 32-bits of the multiplication operation:
    >`mul $d, $s, $t` => `$d = $s * $t`