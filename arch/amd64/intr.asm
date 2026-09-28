;*++
;	Copyright 2026 Driftless Software Pvt. Ltd
;	
;	NEEV Firmware.
;
;	Module Name:
;
;		intr.asm
;
;	Description:
;
;		AMD64 interrupts setup.
;
;	Author:
;
;		Mayank Pathak (mpathak).
;		28/09/2026
;--*

extern amd64_ehandler
global isr_stub_table
global amd64_load_gdt
global isr_stub_32
global isr_stub_255

;dont question why i put this here
amd64_load_gdt:
	lgdt [rdi]
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov ss, ax
	xor eax, eax
	mov fs, ax
	mov gs, ax
	pop rax
	push 0x08
	push rax
	retfq

%macro isr_stub 2

isr_stub_%1:
%if %2 == 0
	;dummy
	push qword 0
%endif

	push qword %1
	jmp isr_common

%endmacro

isr_common:
	push rax
	push rbx
	push rcx
	push rdx
	push rsi
	push rdi
	push rbp
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15

	mov rdi, rsp
	cld
	call amd64_ehandler

	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rbp
	pop rdi
	pop rsi
	pop rdx
	pop rcx
	pop rbx
	pop rax
	add rsp, 16
	iretq

;our 32 handlers...

isr_stub 0, 0
isr_stub 1, 0
isr_stub 2, 0
isr_stub 3, 0
isr_stub 4, 0
isr_stub 5, 0
isr_stub 6, 0
isr_stub 7, 0
isr_stub 8, 1
isr_stub 9, 0
isr_stub 10, 1
isr_stub 11, 1
isr_stub 12, 1
isr_stub 13, 1
isr_stub 14, 1
isr_stub 15, 0
isr_stub 16, 0
isr_stub 17, 1
isr_stub 18, 0
isr_stub 19, 0
isr_stub 20, 0
isr_stub 21, 1
isr_stub 22, 0
isr_stub 23, 0
isr_stub 24, 0
isr_stub 25, 0
isr_stub 26, 0
isr_stub 27, 0
isr_stub 28, 0
isr_stub 29, 1
isr_stub 30, 1
isr_stub 31, 0

;extras
isr_stub 32, 0
isr_stub 255, 0

section .data
isr_stub_table:
%assign i 0
%rep 32
	dq isr_stub_%+i
%assign i i+1
%endrep
