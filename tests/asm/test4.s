# this program will test 'BEQ' with no errors, and with
# m = c = n = 1

# beq $t, $s, offset

# load initial values into temp registers
addi $t0, $0, 0x1 # $t0 (8) = 0x1
addi $t1, $0, 0x2 # $t1 (9) = 0x2
addi $t2, $0, 0x3 # $t2 (10) = 0x3
addi $t3, $0, 0x4 # $t3 (11) = 0x4
addi $t4, $0, 0x5 # $t4 (12) = 0x5
addi $t5, $0, 0x3 # $t5 (13) = 0x3

# load value into register we'll be watching
addi $s0, $0, 0xA # $s0 (16) = 0xA

# this will be false, so it shouldn't skip the next instruction
# and we should see $s0 == 0xF
beq $t0, $t1, 1
addi $s0, $s0, 0x5 # $s0 = 0xF

# load next register
addi $s1, $0, 0xA # $s1 (17) = 0xA

# this will be true, so it should skip the next instruction and
# $s1 should remain 0xA
beq $t2, $t5, 2
addi $s1, $s1, 0x5
# the branch should skip this one too
addi $s2, $0, 0xFF
# should NOT skip this one
addi $s3, $0, 0xAA # $s3 (19) = 0xAA (170)

haltSimulation