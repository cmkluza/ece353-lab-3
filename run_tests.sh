#!/bin/bash

# the directory with the test files
TEST_DIR=./tests
# number of tests in the testing directory
declare -r NUM_TESTS=4

if [[ -d "$TEST_DIR" ]]; then
    echo -e "\e[38;5;27mStarting tests...\e[0m"
    for ((i=1; i<=$NUM_TESTS; i++)); do
        if [[ -f "$TEST_DIR/test$i" ]]; then
            echo ""
            echo -e "\e[38;5;27m========== Test ${i} ==========\e[0m"

            TEST_FILE=${TEST_DIR}/test${i} #test{i}, the command to run
            TEST_ASM=${TEST_DIR}/asm/test${i}.s
            TEST_SOL=${TEST_DIR}/sols/test${i}.sol #test{i}.sol, the expected solution
            TEST_OUT=${TEST_DIR}/out/test${i}.out #test{i}.out, the program output

            # stop if we're missing files
            if [[ ! -f ${TEST_SOL} ]]; then
                echo -e "\e[38;5;160mCouldn't find solution: \e[38;5;226m${TEST_SOL}\e[0m"
                echo -e "\e[38;5;160mExiting...\e[0m"
                exit 1
            fi

            COMMAND=(`cat ${TEST_FILE}`)
            "${COMMAND[@]}" # run the script
            # build the output
            MIDDLE_LINE=`cat ${TEST_OUT} | awk 'FNR==2{ print $0 }'`
            REG_VALS=`cat ${TEST_SOL}`
            OUT=$(printf "program name: %s\n%s\nregister values %s" "${TEST_ASM}" "${MIDDLE_LINE}" "${REG_VALS}")

            # compare the output
            RESULT=`diff ${TEST_OUT} <(echo "${OUT}")`
            if [[ "${RESULT}" != "" ]]; then
                echo -e "\e[38;5;160mThere's differences in your output:\e[0m"
                echo -e "  \e[38;5;226m${RESULT}\e[0m"
            else
                echo -e "\e[1m\e[38;5;40mPassed test${i}!\e[0m"
            fi
        else
            echo -e "\e[38;5;160mCouldn't find test file: \e[38;5;226m${TEST_DIR}/test$i\e[0m"
            echo -e "\e[38;5;160mExiting...\e[0m"
            exit 1
        fi
    done
else
    echo -e "\e[38;5;160mCouldn't find testing directory: \e[38;5;226m${TEST_DIR}\e[0m"
fi