  # labwork.S
  # Written 2015-2017 by F Lundevall
  # Skeleton file for IS1200/IS1500 lab 1.
  # The macros PUSH and POP are in the public domain.
  # Please add your own code at the end of the file.

  #
  # Please keep the two macros PUSH and POP unchanged
  #


.macro	PUSH reg
	addi	$sp,$sp,-4
	sw	\reg,0($sp)
.endm

.macro	POP reg
	lw	\reg,0($sp)
	addi	$sp,$sp,4
.endm

.data
.align 2

.text
  #
  # Please add your own code below this line
  #
  # Modified by Erik Martines Sanches
.global hexasc
hexasc:
	andi 	$a0, $a0, 0xF	# extracting the 4 least sign. bits
	addi 	$t0, $0, 9 	# nine, the upper bound we want to test against in the next line
	ble 	$a0, $t0, lte 	# if $a0 is lte $t0, go to lte
	addi 	$v0, $a0, 0x37	# 0x30 is the offset to get from n to n + 0x30 (ascii 0)
	jr 	$ra
	nop
	
lte:
	addi 	$v0, $a0, 0x30	# 0x30 is the offset to get from n to n + 0x30 (ascii 0)
	jr 	$ra
	nop

.global delay
delay:
	#PUSH ($s0)
	#PUSH ($s1)
	addiu $t1, $0, 4450	# 4711 stored in $t1
while:
	beq $a0, $0, equal0	# branch if ms == 0
	nop 
	addi $a0, $a0, -1	# ms = ms – 1;
	
	addiu $t0, $0, 0x0	# i = 0
for:
	beq	$t0, $t1, equal1# branch if i == 4711
	nop
	addiu	$t0, $t0, 1	# i = i + 1
	j	for
	nop
equal1:				#exiting the for loop	
	j while
	nop
	
equal0:				#we skip the entire while loop
	#POP ($s1)
	#POP ($s0)
	jr $ra
	nop

.global time2string
time2string:
	PUSH $s0		#saving save registers on stack
	PUSH $s1
	
	move $s0, $a0		# saving address into a saved register
	move $s1, $a1		# moving the time info into a saved register 
		
	# put the ASCII null byte at the LSB in $s1
	andi	$t0, $s1, 0x0	# make $t2 contain all zeros.
	sb	$t0, 5($s0)	# storing LSB from $t2 at location pointed to by $s0
	
	# fourth digit
	move 	$a0, $s1	# moving over into the argument register
	PUSH $ra
	jal	hexasc		# The LSB in $v0 should contain the ascii code
	nop
	POP $ra
	sb 	$v0, 4($s0)	# storing at given location with an offset
	
	# third digit
	srl 	$a0, $s1, 4	# shifting right by 4 bits
	PUSH $ra
	jal	hexasc		# The LSB in $v0 should contain the ascii code
	nop
	POP $ra
	sb	$v0, 3($s0)	# storing at given location with an offset
	
	# Put a colon character in LSB of $t2
	addi 	$t0, $0, 0x003A	# putting in the colon character
	sb 	$t0, 2($s0)	# storing at given location with an offset
	
	# second digit
	srl 	$a0, $s1, 8	# shifting right by 8 bits
	PUSH $ra
	jal	hexasc		# The LSB in $v0 should contain the ascii code
	nop
	POP $ra
	sb	$v0, 1($s0)	# storing at given location with an offset
	
	# first digit
	srl 	$a0, $s1, 12	# shifting right by 12 bits
	PUSH $ra	
	jal	hexasc		# The LSB in $v0 should contain the ascii code
	nop
	POP $ra	
	sb	$v0, 0($s0)	# storing at given location with an offset
	
	POP $s1		#restoring save registers
	POP $s0
	
	jr	$ra
	nop
  

  # Erik Martines Sanches
  # Allocate and deallocate memory
#.global malloc
#malloc:
#	PUSH 
