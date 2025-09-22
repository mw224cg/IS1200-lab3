# analyze.S
# Written 2015 by F Lundevall
# Ported 2024/07 by W Szczerek (from MIPS to RISC-V)
# Copyright abandonded - this file is in the public domain.

	.text
	.globl analyze
analyze:
	li	s0, 0x30			#Load 0x30 = ASCII code '0' to s0
loop:
	mv	a0, s0				# copy from s0 to a0
	
	li	a7, 11				# environment call with a7 = 11 will print out
	ecall					# one byte from a0 to the Run I/O window

	addi	s0, s0, 0x03	# what happens if the constant is changed? OUT = 0369<?BEHKNQTWZ, 
				#adderar 3 till register s0 ist för 1 ==> var tredje tecken skrivs ut
	
	li	t0, 0x5A	
	ble	s0, t0, loop	#Fortsätt loopen så länge som s0 <t0
    	jr 	ra			

###########################################################################
#Which lines of code had to be changed to print every third ASCII symbol?
#Answer: Line 16: addi s0, s0, 0x01 --> 0x03 adds 3 instead of 1 to register s0 ==> every third ASCII symbol gets printed