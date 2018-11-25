# this program will test 'LW' and 'SW' with no errors
# note: memory accessses *must* be word-aligned
# lw $t, offset($s) -> $t = MEM[$s + offset]
# sw $t, offset($s) -> MEM[$s + offset] = $t

# load initial values into temp registers
addi $t0, $0, 0x1 # $t0 (8) = 0x1
addi $t1, $0, 0x28 # $t1 (9) = 0x28 (40)
addi $t2, $0, 0xFA # $t2 (10) = 0xFA (280)
addi $t3, $0, 0xA1F # $t3 (11) = 0xA1F (2591)
addi $t4, $0, 0x100 # $t4 (12) = 0x100 (256)
# max positive 16-bit signed number:
addi $t5, $0, 0x7FFF # $t5 (13) = 0x7FFF (32767)

# store words
sw $t4, 0x0($t1) # MEM[40] = 256
sw $t5, 0x4($t2) # MEM[284] = 32767
sw $t3, 0x8($t1) # MEM[48] = 2591

# load words
lw $s0, 0x0($t1) # $s0 (16) = (MEM[40] = 256)
lw $s1, 0x4($t2) # $s1 (17) = (MEM[284] = 32767)
lw $s2, 0x8($t1) # $s2 (18) = (MEM[48] = 2591)
haltSimulation