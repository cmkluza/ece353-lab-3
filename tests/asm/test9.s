# this program will test RAW hazards

# just do a long stream of arithmetic with a lot of RAWs
addi $t0, $0, 0xF # $t0 (8) = 15
addi $t1, $t0, 0xA # $t1 (9) = $t0 + 10 = 25
addi $t2, $t1, -0x5 # $t2 (10) = $t1 + (-5) = 20
add $t3, $t2, $t1 # $t3 (11) = $t2 + $t1 = 45
add $t4, $t3, $t1 # $t4 (12) = $t3 + $t1 = 70
mul $t5, $t4, $t0 # $t5 (13) = $t4 * $t0 = 1050
sub $t6, $t5, $t4 # $t6 (14) = $t5 - $t4 = 980
sub $t7, $t5, $t6 # $t7 (15) = $t5 - $t6 = 70

haltSimulation