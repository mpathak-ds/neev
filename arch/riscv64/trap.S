#*++
#	Copyright 2026 Driftless Software Pvt. Ltd
#	
#	NEEV Firmware.
#
#	Module Name:
#
#		trap.S
#
#	Description:
#
#		RISCV64 interrupts setup.
#
#	Author:
#
#		Mayank Pathak (mpathak).
#		30/09/2026
#--*

.section .text
.global trap_entry
.align 4

trap_entry:
	addi sp, sp, -256
	sd ra,  0(sp)
	sd t0,  8(sp)
	sd t1, 16(sp)
	sd t2, 24(sp)
	sd a0, 32(sp)
	sd a1, 40(sp)
	sd a2, 48(sp)
	sd a3, 56(sp)
	sd a4, 64(sp)
	sd a5, 72(sp)
	sd a6, 80(sp)
	sd a7, 88(sp)

	call trap_dispatch

	ld ra,  0(sp)
	ld t0,  8(sp)
	ld t1, 16(sp)
	ld t2, 24(sp)
	ld a0, 32(sp)
	ld a1, 40(sp)
	ld a2, 48(sp)
	ld a3, 56(sp)
	ld a4, 64(sp)
	ld a5, 72(sp)
	ld a6, 80(sp)
	ld a7, 88(sp)
	addi sp, sp, 256
	sret
