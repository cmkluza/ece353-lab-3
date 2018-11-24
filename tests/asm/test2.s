# this program will test 'ADD', 'SUB', and 'MUL' with no errors, and with
# m = c = n = 1

# add $d, $s, $t -> $d = $s + $t
# mul $d, $s, $t -> $d = $s * $t
# sub $d, $s, $t -> $d = $s - $t

# load initial values into temp registers
addi $t0, $0, 0x1 # $t0 (8) = 0x1
addi $t1, $0, 0x2 # $t1 (9) = 0x2
addi $t2, $0, 0x3 # $t2 (10) = 0x3
addi $t3, $0, 0xFF # $t3 (11) = 0xFF (255)
addi $t4, $0, 0x100 # $t4 (12) = 0x100 (256)
# max positive 16-bit signed number:
addi $t5, $0, 0x7FFF # $t5 (13) = 0x7FFF (32767)
# do some math
add $s0, $t0, $t1 # $s0 (16) = 1 + 2 = 0x3
add $s1, $s0, $t2 # $s1 (17) = 3 + 3 = 0x6
add $s2, $t5, $t5 # $s2 (18) = 0x7FFF + 0x7FFF = 0xFFFE (65534)
add $s3, $t1, $t1 # $s3 (19) = 2 + 2 = 0x4
mul $s4, $t1, $t2 # $s4 (20) = 2 * 3 = 0x6
mul $s5, $t1, $t4 # $s5 (21) = 2 * 256 = 0x200 (512)
# this should overflow and become negative
mul $s6, $s2, $s2 # $s6 (22) = 0xFFFE * 0xFFFE = 0xFFFC0004 (-262140)
sub $23, $t2, $t1 # $23 = 3 - 2 = 0x1
sub $24, $t0, $t3 # $24 = 1 - 255 = 0xFF01 (-254)
sub $25, $t1, $t0 # $25 = 2 - 1 = 0x1
haltSimulation