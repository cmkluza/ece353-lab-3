# MIPS Pipeline Simulator

To compile:
```
gcc -o sim-mips mips_sim.c 
```

To run tests:
```
chmod u+x run_tests.sh
./run_tests.sh
```

To add an `{i}`th test:
* Create `test{i}` in `./tests/`
    * Specify the program command line arguments (e.g. `./sim-mips -b 1 1 1 ./tests/asm/test{i}.s ./tests/out/test{i}.out`)
* Create a `test{i}.s` MIPS assembly file in `./tests/asm`
* Create a `test{i}.sol` file in `./tests/sols`
    * This file simply lists the expected state of register 1-31 and the program counter, with each number separated by two spaces:
    * `Register[1]  Register[2]  Register[3]  ...  Register[31]  PC` 