# this program will test the 'ADDI' function without errors
# addi $t, $s, imm -> $t = $s + imm

addi $t0, $0, 0xF # $t0 (8) = 0xF
addi $t1, $zero, 10 # $t1 (9) = 0xA
# RAW hazard:
addi $s0, $t1, 0xA # $s0 (16) = $t1 + 0xA = 0x14 (20)
addi $17, $0, 0x7 # $s1 (17) = 0x7
addi $ra, $0, 0x8 # $rs (31) = 0x8
addi $sp, $zero, 0x5 # $sp (29) = 0x5

# try setting zero to somthing else
addi $0, $0, 0xF

haltSimulation