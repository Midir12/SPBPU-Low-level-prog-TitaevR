start:
li t0, 0x00
li t1, 0xFF
la t2, LED_MATRIX_0_BASE
la a0, SWITCHES_0_BASE
li t3, 10
li t4, 0
li s3, 16
li s2, -1
1:
beq t3, t4 4f
li a1, 0
    2:
    lw t5, 0(a0)
    beq a1, t5 3f
    addi a1, a1, 1 
    bge t5, s3 4f
    sw t1, 0(t2)
    sw t0, 0(t2)
    j 2b
3:
sw t1, 0(t2)
addi t4, t4, 1
addi t2, t2, 4
j 1b
4:
sw t0, 0(t2)
addi t2, t2, -4
addi t4, t4, -1
beq t4, s2 5f
j 4b

5:
lw t5, 0(a0)
bge t5, s3 end
j 1b

end:
    j end