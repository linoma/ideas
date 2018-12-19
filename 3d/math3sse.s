	SEGMENT .data USE32 class=code
	
	global vet4_mtx4x4sse
	global mtx4x4_multsse
	global vet3_mtx4x4sse
	global clearsse
	
clearsse:
	push 	eax
	stmxcsr [esp]
	and		dword [esp],-40
	ldmxcsr	[esp]
	pop 	eax
	ret	
	
mtx4x4_multsse:	
	push    ebp
	mov     ebp,esp
	add     esp,-120
	push    esi
	push    edi
	
	lea     eax,[ebp-72]
	test    al,15
	je      short .1
	and     eax,-16
.1:
	mov		[ebp-4],eax		;pm2
	lea     eax,[ebp-104]
	test    al,15
	je      short .2
	and     eax,-16
.2:
	mov		[ebp-8],eax	;pf
	mov     esi,[ebp+8]
	mov     edi,[ebp-4]
	mov		ecx,16
	rep   	movsd

	mov		eax,[ebp-8]
	mov		ecx,[ebp+12]	;m1
	
	mov		edx,[ecx]
	mov		[eax],edx
	mov		[eax+4],edx
	mov		[eax+8],edx
	mov		[eax+12],edx	
	movaps	xmm0,[eax]
	mov		esi,[ebp-4]		;pm2
	mulps	xmm0,[esi]
	
	mov		edx,[ecx+4]
	mov		[eax],edx
	mov		[eax+4],edx
	mov		[eax+8],edx
	mov		[eax+12],edx
	movaps	xmm1,[eax]
	mulps	xmm1,[esi+16]
	addps	xmm0,xmm1

	mov		edx,[ecx+8]
	mov		[eax],edx
	mov		[eax+4],edx
	mov		[eax+8],edx
	mov		[eax+12],edx
	movaps	xmm1,[eax]
	mulps	xmm1,[esi+32]
	addps	xmm0,xmm1	
	
	mov		edx,[ecx+12]
	mov		[eax],edx
	mov		[eax+4],edx
	mov		[eax+8],edx
	mov		[eax+12],edx
	movaps	xmm1,[eax]
	mulps	xmm1,[esi+48]
	addps	xmm0,xmm1	
	
	mov		edx,[ecx+16]
	mov		[eax],edx
	mov		[eax+4],edx
	mov		[eax+8],edx
	mov		[eax+12],edx
	movaps	xmm2,[eax]
	mulps	xmm2,[esi]

	mov		edx,[ecx+20]
	mov		[eax],edx
	mov		[eax+4],edx
	mov		[eax+8],edx
	mov		[eax+12],edx
	movaps	xmm1,[eax]
	mulps	xmm1,[esi+16]
	addps	xmm2,xmm1

	mov		edx,[ecx+24]
	mov		[eax],edx
	mov		[eax+4],edx
	mov		[eax+8],edx
	mov		[eax+12],edx
	movaps	xmm1,[eax]
	mulps	xmm1,[esi+32]
	addps	xmm2,xmm1
	
	mov		edx,[ecx+28]
	mov		[eax],edx
	mov		[eax+4],edx
	mov		[eax+8],edx
	mov		[eax+12],edx
	movaps	xmm1,[eax]
	mulps	xmm1,[esi+48]
	addps	xmm2,xmm1

	mov		edx,[ecx+32]
	mov		[eax],edx
	mov		[eax+4],edx
	mov		[eax+8],edx
	mov		[eax+12],edx
	movaps	xmm3,[eax]
	mulps	xmm3,[esi]

	mov		edx,[ecx+36]
	mov		[eax],edx
	mov		[eax+4],edx
	mov		[eax+8],edx
	mov		[eax+12],edx
	movaps	xmm1,[eax]
	mulps	xmm1,[esi+16]
	addps	xmm3,xmm1

	mov		edx,[ecx+40]
	mov		[eax],edx
	mov		[eax+4],edx
	mov		[eax+8],edx
	mov		[eax+12],edx
	movaps	xmm1,[eax]
	mulps	xmm1,[esi+32]
	addps	xmm3,xmm1
	
	mov		edx,[ecx+44]
	mov		[eax],edx
	mov		[eax+4],edx
	mov		[eax+8],edx
	mov		[eax+12],edx
	movaps	xmm1,[eax]
	mulps	xmm1,[esi+48]
	addps	xmm3,xmm1	
	
	mov		edx,[ecx+48]
	mov		[eax],edx
	mov		[eax+4],edx
	mov		[eax+8],edx
	mov		[eax+12],edx
	movaps	xmm4,[eax]
	mulps	xmm4,[esi]

	mov		edx,[ecx+52]
	mov		[eax],edx
	mov		[eax+4],edx
	mov		[eax+8],edx
	mov		[eax+12],edx
	movaps	xmm1,[eax]
	mulps	xmm1,[esi+16]
	addps	xmm4,xmm1
	
	mov		edx,[ecx+56]
	mov		[eax],edx
	mov		[eax+4],edx
	mov		[eax+8],edx
	mov		[eax+12],edx
	movaps	xmm1,[eax]
	mulps	xmm1,[esi+32]
	addps	xmm4,xmm1
	
	mov		edx,[ecx+60]
	mov		[eax],edx
	mov		[eax+4],edx
	mov		[eax+8],edx
	mov		[eax+12],edx
	movaps	xmm1,[eax]
	mulps	xmm1,[esi+48]
	addps	xmm4,xmm1		
	
	mov		eax,[ebp+8]
	test	al,15
	je		short .3
	movups	[eax],xmm0
	movups	[eax+16],xmm2
	movups	[eax+32],xmm3
	movups	[eax+48],xmm4
	pop edi
	pop esi
	leave
	ret
.3:			
	movaps	[eax],xmm0
	movaps	[eax+16],xmm2
	movaps	[eax+32],xmm3
	movaps	[eax+48],xmm4
	pop		edi
	pop     esi
	leave
	ret
		
vet4_mtx4x4sse:
	push	ebp
	mov     ebp,esp
	add     esp,-36

	lea     eax,[ebp-20]
	test    al,15
	je      short .3
	and     eax,-16
.3:
	mov     [ebp-4],eax
	mov 	eax,[ebp + 8]
	mov		ecx,[ebp + 12]	
	lea		edx,[ecx]
	test	edx,15
	jz		short .1
	movups	xmm2,[ecx]
	movups	xmm3,[ecx+16]
	movups	xmm4,[ecx+32]
	movups	xmm5,[ecx+48]
	jmp 	short .2
.1:
	movaps	xmm2,[ecx]
	movaps	xmm3,[ecx+16]
	movaps	xmm4,[ecx+32]
	movaps	xmm5,[ecx+48]
.2:	
	mov		edx,[eax]
	mov		ecx,[ebp-4]
	mov		[ecx],edx
	mov		[ecx+4],edx
	mov		[ecx+8],edx
	mov		[ecx+12],edx	
	movaps	xmm0,[ecx]
	mulps	xmm0,xmm2
	mov		edx,[eax+4]
	mov		[ecx],edx
	mov		[ecx+4],edx
	mov		[ecx+8],edx
	mov		[ecx+12],edx	
	movaps	xmm1,[ecx]
	mulps	xmm1,xmm3
	addps	xmm0,xmm1
	mov		edx,[eax+8]
	mov		[ecx],edx
	mov		[ecx+4],edx
	mov		[ecx+8],edx
	mov		[ecx+12],edx	
	movaps	xmm1,[ecx]
	mulps	xmm1,xmm4
	addps	xmm0,xmm1
	mov		edx,[eax+12]
	mov		[ecx],edx
	mov		[ecx+4],edx
	mov		[ecx+8],edx
	mov		[ecx+12],edx	
	movaps	xmm1,[ecx]		
	mulps	xmm1,xmm5
	addps	xmm0,xmm1	
	lea		edx,[eax]
	test	edx,15
	jnz		short .5
	movaps	[eax],xmm0
	leave
	ret
.5:
	movups	[eax],xmm0	
	leave
	ret 

vet3_mtx4x4sse:
	push	ebp
	mov     ebp,esp
	add     esp,-36

	lea     eax,[ebp-20]
	test    al,15
	je      short .3
	and     eax,-16
.3:
	mov     [ebp-4],eax
	mov 	eax,[ebp + 8]
	mov		ecx,[ebp + 12]	
	lea		edx,[ecx]
	test	edx,15
	jz		short .1
	movups	xmm2,[ecx]
	movups	xmm3,[ecx+16]
	movups	xmm4,[ecx+32]
	jmp 	short .2
.1:
	movaps	xmm2,[ecx]
	movaps	xmm3,[ecx+16]
	movaps	xmm4,[ecx+32]
.2:	
	mov		edx,[eax]
	mov		ecx,[ebp-4]
	mov		[ecx],edx
	mov		[ecx+4],edx
	mov		[ecx+8],edx
	mov		dword [ecx+12],0
	movaps	xmm0,[ecx]
	mulps	xmm0,xmm2
	mov		edx,[eax+4]
	mov		[ecx],edx
	mov		[ecx+4],edx
	mov		[ecx+8],edx
	movaps	xmm1,[ecx]
	mulps	xmm1,xmm3
	addps	xmm0,xmm1
	mov		edx,[eax+8]
	mov		[ecx],edx
	mov		[ecx+4],edx
	mov		[ecx+8],edx
	movaps	xmm1,[ecx]
	mulps	xmm1,xmm4
	addps	xmm0,xmm1
	lea		edx,[eax]
	test	edx,15
	jnz		short .5
	movaps	[eax],xmm0
	leave 
	ret
.5:
	movups	[eax],xmm0	
	leave
	ret 
