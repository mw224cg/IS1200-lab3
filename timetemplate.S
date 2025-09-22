# timetemplate.S
# Written 2015 by F Lundevall
# Ported 2024/06 by W Szczerek (from MIPS to RISC-V)
# Copyright abandonded - this file is in the public domain.

#############################################################
# Choose the macro syntax for RARS or DTEK-V board.         #
# The syntax for RARS is probably due to its MIPS heritage. #
#############################################################
###################
# Macros for RARS #
###################
#.macro	PUSH (%reg)
#	addi	sp,sp,-4
#	sw	%reg,0(sp) 
#.end_macro

#.macro	POP (%reg)
#	lw	%reg,0(sp)
#	addi	sp,sp,4
#.end_macro
###################
# Macros for DTEK #
###################
.macro	PUSH reg
	addi sp,sp,-4
	sw \reg,0(sp) 
.endm

.macro	POP reg
	lw	\reg,0(sp)
	addi	sp,sp,4
.endm
#############################################################

	.data
	.align 2
mytime:	.word 	0x5957
timstr:	.asciz 	"text more text lots of text\0"
	.text
	.globl timetemplate, tick, time2string, display_string, delay

# Function for displaying a string with a newline at the end	
display_string:	
	li a7,4
	ecall
	li a0, 10
	li a7,11
	ecall
	jr ra
	
timetemplate:
	la	a0, timstr		#sätt addressen i timstr till a0
	jal     display_string		#Skriv ut string sparad i minnet vid adressen som a0 pekar på
	
	# wait a little
	li	a0, 1000		# ms
	jal	delay
	
	# call tick
	la 	a0, mytime		#sätt a0 till adressen där tiden lagras
	jal	tick
	
	# call your function time2string
	la	a0, timstr		#sätt a0 till att peka där ASCII tiden ska lagras i minnet
	la	t0, mytime		#sätt t0 till att peka på adressen där tiden i 16bit sparas
	lw	a1, 0(t0)		#spara tiden i a1 mha pekare 
	jal	time2string		#Konvertera tiden i a1 till ASCII och skriv till minnesadressen i a0
	
	# go back and do it all again
	j	timetemplate

# tick: update time pointed to by $a0
tick:	lw	t0, 0(a0)	# get time
	addi	t0, t0, 1	# increase
	andi	t1, t0, 0xf	# check lowest digit
	sltiu	t2, t1, 0xa	# if digit < a, okay
	bnez	t2, tiend
	addi	t0, t0, 0x6	# adjust lowest digit
	
	andi	t1, t0, 0xf0	# check next digit
	sltiu	t2, t1, 0x60	# if digit < 6, okay
	bnez	t2, tiend
	addi	t0, t0, 0xa0	# adjust digit
	
	li	t3, 0xF
	slli	t3, t3, 0x8
	and	t1, t0, t3	# check minute digit
	addi	t3, x0, 0xA
	slli	t3, t3, 0x8
	slt	t2, t1, t3	# if digit < a, okay
	bnez	t2, tiend
	addi	t0, t0, 0x600	# adjust digit - this one's okay, it's lower than 0x7FF 
	
	li	t3, 0xF
	slli	t3, t3, 0xC
	and	t1, t0, t3	# check last digit
	addi	t3, x0, 0x6
	slli	t3, t3, 0xC
	slt	t2, t1, t3	# if digit < 6, okay
	bnez	t2, tiend
	
	li	t3, 0xA
	slli	t3, t3, 0xC
	add	t0, t0, t3	# adjust last digit
tiend:	sw	t0,0(a0)	# save updated result
	jr	ra		# return

#########################################################
# Place for your functions: time2string, hex2asc, delay.#
#########################################################

#########################################################
# time2string: Skriver ut tiden som MM:SS
# Register a0: Addressen för lagring av output
# Register a1: Innehåller tidsinfo, 16 bitar [Minuter tiotal - Minuter ental - Sekunder tiotal, Sekunder ental]
time2string:

	#Spara register som skall bevaras på stacken
	#ra: jal skriver över ra --> måste sparas
	#a0: spara pekare till strängen
	PUSH ra
	PUSH a0

	#Extrahera tiden från a0 till: t1 = Min 10, t2 = Min 1, t3 = S 10, t4 = S1

	srli t1, a1, 12
	andi t1, t1, 0xF

	srli t2, a1, 8
	andi t2, t2, 0xF

	srli t3, a1, 4
	andi t3, t3, 0xF
	
	andi t4, a1, 0xF
	
	
	#Spara a0 (minnesadress) i t5
	POP t5
	PUSH t5

	#Skriv varje register till a0
	#Load/store: Registers värde fungerar som en pekare till adress i minnet
	mv a0, t1 		#Flytta XXXX till a0
	jal hexasc		#Konvertera till ASCII (Hoppar till hexasc, sparar ra)
	sb a0, 0(t5)		#Sparar minuter tiotal i ASCII till adressen som t5 pekar på
	addi t5, t5, 1		#Addera 1 till t5 så att t5 pekar på nästa byte i minnet
	
	mv a0, t2
	jal hexasc
	sb a0, 0(t5)
	addi t5, t5, 1
	
	li t6, 58	#ASCII 58 = ':'
	sb t6, 0(t5)
	addi t5, t5, 1
	
	mv a0, t3
	jal hexasc
	sb a0, 0(t5)
	addi t5, t5, 1
	
	
	mv a0, t4
	jal hexasc
	sb a0, 0(t5)
	addi t5, t5, 1
	
	
	
	sb x0, 0(t5)	#ASCII 0 = NULL byte
	
	POP a0		#Återställ a0
	POP ra		#Återställ ra
	
	jr ra
	
hexasc:
andi a0, a0, 15	#AND operation with 0000 0000 0000 1111: keeping the last 4 bits

li t0, 10		#store value 10 in t0 to compare with a0
blt a0,t0, digit		#if a0 < 10 then a0 is a digit and 0-9 should be printed --> go to label digit

#annars a0 is A-F

addi a0, a0, 55		#ASCII Code: A = 0x41 = 65, 10 => A --> Add 55 to a0 to get correct ASCII code
jr ra

digit:
addi a0, a0, 48		#ASCII Code: 0 = 0x30 = 48, 0 => 0 --> Add 48 to a0 to get correct ASCII code
jr ra


#Delay: delays (n)ms stored in a0
delay:
	PUSH ra	#Spara retur-address på stacken
	PUSH s0	#Spara värdet i s0 på stacken

	mv s0, a0	#nyttja s0, genom att flytta värdet i a0 till s0

while:
	bge zero, s0, end_delay		#om 0 är större eller lika med s0 ==> avsluta funktionen och återställ register
	addi s0, s0, -1			#minska med 1
	
	
	li t0, 0	# i = 0;
	li t1, 20000	# konstanten 4711 i villkoret

for_loop:
	bge t0, t1, continue	#om i >= 4711, bryt for-loop
	addi t0, t0, 1
	j for_loop		#börja om

continue:
	j while

end_delay:
	POP s0		#Återställ s0 till värdet innan funktionens start
	POP ra		#Återställ returadress till värdet innan funktionens start
	jr ra		#Återgå till anropet

###################################################################
#QUESTIONS:
#Register usage  TIME2STRING:
# Reg a0:  Contains memory adress to store result
# Reg a1:  Contains time information encoded in 16-least significant bits
# Reg t1 - t4:  Used to store each digit in the given time, 1byte each
# Reg t5:  Stores memory adress during loop (Hexasc uses a0 to write ASCII)

#Register usage  DELAY:
# Reg a0:  contains time to delay in ms
# Reg s0:  Original value saved on stack, then used as variable inside while loop
# Reg t0:  Used as i=0
# Reg t1:  Stores variable for amount of times to loop

# S-registers are saved and restored, t-registers are used freely

####################--DELAY--#########################################
# Which lines handle the '5', assuming the time is 16:53?
# Answer: Line 133-134 extracts the '5' from the timestring input from a0
#	  Line 158-161 moves the '5' to a0 and calls hexasc converting to ASCII
#	  The ASCII code gets stored in the memory location (original+3 bytes)

#om a0 <= 0 --> loopen avslutas direkt (rad 198)
