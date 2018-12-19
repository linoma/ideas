	.code 32
b 0x00000000
b 0x00000000
b swiHandler
b 0x00000000
b 0x00000000
b 0x00000000
b irqHandler
b 0x00000000

	.global irqHandler	
irqHandler:	
	stmdb sp!,{r0-r3,r12,lr}
	mov r0, #0x04000000
	add   lr, pc, #0
	ldr pc, [r0,#-4]
	ldmia sp!,{r0-r3,r12,lr}
	subs   pc, lr, #4
	
swiHandler:
	stmdb sp!,{r11-r12,lr}
	ldrb r12, [lr, #-2]
	stmdb sp!,{r12}
	ldr	r11,=swiTable
	ldr r12, [r11, r12, lsl #2]
	mrs r11,SPSR
	stmdb sp!,{r11}
	and   r11, r11, #0x00000080
	orr   r11, r11, #0x0000001F	
	msr CPSR,r11
	stmdb sp!,{r2,lr}
	cmp   r12, #0
    ldreq r12, =emuBios
    cmpeq r12, #0
    addne lr, pc, #0
	bxne r12
	ldmia sp!,{r2,lr}
	mov  r12, #0x000000D3
	msr CPSR,r12
	ldmia sp!,{r11}
	msr SPSR,r11
	ldmia sp!,{r11}
	ldmia sp!,{r11-r12,lr}
	movs  pc, lr

swiTable:	
	.word 0
	.word 0
	.word stop
	.word 0
	.word waitirq
	.word waitvblank
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0 @andeq   r4, r0, 0x00064000
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0

stop:
	mov  r0, #0x04000000
	mov  r1, #0x00000000
	strb r1, [r0, #0x00000301]
	bx lr
waitloop:
	subs   r0, r0, #0x00000001
	bge waitloop
	bx lr
waitvblank:
	mov  r0, #0x00000001
	mov  r1, #0x00000001
	stmdb sp!,{lr}	
	cmp   r0, #0x00000000
	blne waitirq
waitvblank_1:
	mov  r12, #0x04000000
	mov  r2, #0x00000000
	strb r2, [r12, #0x00000301]
	bl waitirq
	beq waitvblank_1
	ldmia sp!,{lr}
	bx lr
waitirq:
	mrs r2,CPSR
	orr   r2, r2, #0x00000080
	msr CPSR,r2
	mov r12, #0x04000000
	ldrh   r2, [r12,#-8]
	ands   r0, r1, r2
	eorne   r2, r2, r0
	strneh  r2, [r12,#-8]
	mrs r2,CPSR
	bic   r2, r2, #0x00000080
	msr CPSR,r2
	bx lr
	

emuBios:
	mov  r12, #0x000000D3
	msr CPSR,r12	
	ldmia sp,{r11-r12}
	mrs r11,SPSR
	msr CPSR,r11
	swi #0xFEFEFE
	bx lr	
