  # hexmain.S
  # Written 2015-09-04 by F Lundevall
  # Copyright abandonded - this file is in the public domain.

	.text
	.globl hex2asc

hex2asc:
	li	a0, 15		# test number (from 0 to 15) if a0 = 17 what is the result and why?
	
	addi    sp,sp,-4	#Store return adress on stack (PUSH)
	sw      ra,0(sp)
	
	jal	hexasc		# call hexasc
	
	li	a7, 11	# write a0 to stdout
	ecall

	lw      ra,0(sp)	#Load original return adress to ra (POP to ra)
	addi    sp,sp,4
	jr      ra	

  # You can write your own code for hexasc here
  # In a0 = 0-15 --> OUT = ASCII 0-9 & A-F
hexasc:
andi a0, a0, 15	#AND operation with 0000 0000 0000 1111: keeping the last 4 bits

li t0, 10		#store value 10 in t0 to compare with a0
blt a0,t0, digit		#if a0 < 10 then a0 is a digit and 0-9 should be printed --> go to label digit


addi a0, a0, 55		#ASCII Code: A = 0x41 = 65, 10 => A --> Add 55 to a0 to get correct ASCII code
jr ra

digit:
addi a0, a0, 48		#ASCII Code: 0 = 0x30 = 48, 0 => 0 --> Add 48 to a0 to get correct ASCII code
jr ra

##################################################################################################
#QUESTIONS:
#Are s-registers used? : NO
#Are registers saved?  : NO 
#What happens if register a0 = 17? : Then the output is 1.
#This is because 17(dec) = 0000 0000 0001 0001(bin) and the function only reads the last four bits (0001) which equals to 1
#Does the function use conditional branch statements?  : YES, Line 29
#If a0 is less than t0   ==>  Go to branch "digit"


