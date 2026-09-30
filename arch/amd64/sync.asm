;*++
;	Copyright 2026 Driftless Software Pvt. Ltd
;	
;	NEEV Firmware.
;
;	Module Name:
;
;		sync.asm
;
;	Description:
;
;		AMD64 atomic operations.
;
;	Author:
;
;		Mayank Pathak (mpathak).
;		30/09/2026
;--*

global amd64_save_and_disable_int
global amd64_restore_and_enable_int
global amd64_atomic_lock
global amd64_atomic_release

;
; This is for uniprocessors only! it is much more efficient to simply disable interrupts
; as a way of "locking" and more dangerous to use actual spinlocks on UP systems.
;

amd64_save_and_disable_int:
	pushfq
	cli
	ret

amd64_restore_and_enable_int:
	popfq
	ret

;
; Multiprocessing systems only
; currently using TTAS technique
;

amd64_atomic_lock:
.lock_loop:
	; First ptr arg is in RDI
	mov eax, [rdi]
	test eax, 1
	jnz .lock_loop
	
	lock bts dword [rdi], 0
	jc .lock_loop
	ret

amd64_atomic_release:
	mov dword [rdi], 0
	ret
