#*++
#	Copyright 2026 Driftless Software Pvt. Ltd
#	
#	NEEV Firmware.
#
#	Module Name:
#
#		atomic.S
#
#	Description:
#
#		RISCV64 atomic operations.
#
#	Author:
#
#		Mayank Pathak (mpathak).
#		30/09/2026
#--*

.section .text
.global rv64_atomic_lock_mp
.global rv64_atomic_free_mp
.global rv64_atomic_lock_up
.global rv64_atomic_free_up

rv64_atomic_lock_up:
	csrci sstatus, 2
	ret

rv64_atomic_free_up:
	csrsi sstatus, 2
	ret

rv64_atomic_lock_mp:
	# pointer in a0
.spin_loop:
	lw t0, 0(a0)
	bnez t0, .spin_loop

	li t1, 1
	# horrendous instruction name..
	amoswap.w.aqrl t0, t1, 0(a0)
	bnez t0, .spin_loop
	ret

rv64_atomic_free_mp:
	amoswap.w.rl zero, zero, 0(a0)
	ret
