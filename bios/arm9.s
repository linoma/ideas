	.code 32
	
b 0
b 0
b swiHandler
b abortHandler
b 0
b 0
b irqHandler
b 0


    .global irqHandler
irqHandler:
	stmdb 	sp!,{r0-r3,r12,lr}
	mrc 	p15,1,r0,c9,c1,0
    mov  	r0, r0, lsr #12
    mov  	r0, r0, lsl #12
    add   	r0, r0, #0x00004000
    add   	lr, pc, #0x00000000
    ldr 	pc, [r0, #-4]
	ldmia 	sp!,{r0-r3,r12,lr}
    subs   	pc, lr,#0x00000004

swiHandler:
	stmdb 	sp!,{r11-r12,lr}
	ldrb 	r12, [lr, #-2]
	stmdb 	sp!,{r12}
	ldr		r11,=swiTable
	ldr 	r12, [r11, r12, lsl #2]
	mrs 	r11,SPSR
	stmdb 	sp!,{r11}
	and   	r11, r11, #0x00000080
	orr   	r11, r11, #0x0000001F	
	msr 	CPSR,r11
	stmdb 	sp!,{r2,lr}
	cmp  	r12, #0
    ldreq 	r12, =emuBios
    cmpeq	r12, #0
    addne 	lr, pc, #0
	bxne 	r12
	ldmia 	sp!,{r2,lr}
	mov  	r12, #0x000000D3
	msr 	CPSR,r12
	ldmia 	sp!,{r11}
	msr 	SPSR,r11
	ldmia 	sp!,{r11}
	ldmia 	sp!,{r11-r12,lr}
	movs  	pc, lr

	.word 0
	.word 0
	.word 0
        .word 0
swiTable:
	.word 0
	.word 0
	.word 0
	.word waitloop
	.word waitirq
	.word waitvblank
	.word stop
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
    mov r0, #0x00000000
	mcr p15,0,r0,c7,c0,4
	bx lr
waitloop:
    subs r0, r0, #0x00000001
    bge waitloop
	bx lr
waitirq:
	stmdb sp!,{lr}
    cmp  r0, #0x00000000
	blne wait_irq
waitirq_1:
	mov lr,#0x0
	mcr p15,0,r14,c7,c0,4
	bl wait_irq
    beq waitirq_1
	ldmia sp!,{lr}
	bx lr
	
waitvblank:
    mov r0, #0x00000001
    mov r1, #0x00000001
	stmdb sp!,{r4,lr}
    cmp  r0, #0x00000000
	blne wait_irq
waitvblank_1:
	mov lr,#0x0
	mcr p15,0,r14,c7,c0,4
	bl wait_irq
    beq waitvblank_1
	ldmia sp!,{r4,lr}
	bx lr
wait_irq:
    mov  r12, #0x04000000
    str r12, [r12, #0x00000208]
	mrc p15,1,r3,c9,c1,0
    mov  r3, r3, lsr #12
    mov  r3, r3, lsl #12
    add   r3, r3, #0x00004000
    ldr r2, [r3, #-8]
	ands   r0, r1, r2
	eorne   r2, r2, r0
    strne r2, [r3, #-8]
    mov  r0, #0x00000001
    str r0, [r12, #0x208]
	bx lr

	
emuBios:
	mov  r12, #0x000000D3
	msr CPSR,r12	
	ldmia sp,{r11-r12}
	mrs r11,SPSR
	msr CPSR,r11
	swi #0xFEFEFE
	bx lr	
	
	.global abortHandler
abortHandler:
	mrs 	sp,CPSR
	orr 	sp,sp,#0xC0
	msr 	CPSR,sp	
	ldr 	sp,=0x027FFD9C
	stmdb 	sp!,{r12,lr}	
	mrs 	lr,SPSR
	;mrc 	p15,1,r12,c1,c0,0
	stmdb 	sp!,{r12,lr}
	;bic 	r12,r12,#0x1
	;mcr 	p15,0,r12,c1,c0,0
	mov 	r12,sp
	ldr 	r12,[r12,#0x10]
	cmp 	r12,#0
	blxne 	r12
	ldmia 	sp!,{r12,lr}
	;mcr 	p15,0,r12,c1,c0,0
	msr 	SPSR,lr
	ldmia 	sp!,{r12,lr}
	subs	pc, lr,#0x00000004
	
