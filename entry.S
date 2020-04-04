	/* TODO: use 3264 */

	.section .boot, "ax"
	.type _start, @function
	.global _start
_start:
	lw $t0, 0x80000318 /* memory size */
	/* init sp */
	li $t1, 0x80000000 - 0x10
	addu $sp, $t0, $t1
	la $gp, _gp /* init gp */

	/* magic N64 hardware init */
	li $v0, 8
	sw $v0, 0xbfc007fc

	/* a bit from libgloss so we start at a known state...? */
	li $v0, 0x20000000 /* CU1 */
	mtc0 $v0, $12 /* Status register */
	mtc0 $0, $13 /* Cause */

	/* just for safety */
	mtc0 $0, $18 /* WatchLo */
	mtc0 $0, $19 /* WatchHi */

	/* copy code and data */
codeinit:
	la $s0, _start
	la $s1, __data_end
	subu $s1, $s0
	li $t0, 0x00100000
	subu $s1, $t0

	/* if data_end < start+1M, IPL3 already DMA'd, so do nothing. */
	bltz $s1, 1f

	addu $s0, $t0

	/* invalidate dcache before DMA */
	jal dwbinvalall

	/* PI DMA read .text and .data */
	addiu $s1, -1

	li $t2, 0x00FFffff /* PJ64 2.0 requires physical address */
	and $t2, $s0

	la $t0, 0xA4600000
	li $t1, 0x10001000 + 0x00100000
	sw $t2, 0x00($t0) /* dram=start+1M */
	sw $t1, 0x04($t0) /* cart=lma_start+1M */
	sw $s1, 0x0C($t0) /* cart2dram */
2:
	lw $t1, 0x10($t0)
	andi $t1, 1
	bnez $t1, 2b

	/* invalidate icache after DMA... may not be required. */
	/*jal iinvalall*/

1:

	/* fill .bss with 0s; aligned by 4 are ensured by linker script */
bssinit:
	la $a0, __bss_start
	la $a1, __bss_end
1:
	sw $0, ($a0)
	addiu $a0,4
	bltu $a0, $a1, 1b

	/* make sure .bss is actually written */
	/*
	la $a0,__bss_start
	la $a1,__bss_end
	sub $a1, $a0
	jal data_cache_hit_writeback_invalidate
	*/

	/* load exception handlers */
loadeh:
	/* TODO: some emulator glitches on uncached access...? */
	la $t0, eh
	la $t1, 0xa0000000
	li $t2, eh_end - eh
1:
	lw $t3, ($t0)
	sw $t3, 0($t1)
	sw $t3, 0x80($t1)
	sw $t3, 0x100($t1)
	sw $t3, 0x180($t1)
	/* sync */
	cache (4<<2) | 0, 0($t1) /* HitInvalidateI */
	cache (4<<2) | 0,0x80($t1)
	cache (4<<2) | 0,0x100($t1)
	cache (4<<2) | 0,0x180($t1)
	addi $t0, 4
	addi $t1, 4
	addiu $t2, -1
	bnez $t2, 1b

	/* make sure all data are written to memory (potentially hardware buffer, and for cache error handler) */
	jal dwbinvalall

callmain:
	jal main /* call main app */

	j . /* die */

/* this is copied to exception handler */
eh:
	/*
	la $k1, ehentry
	jalr $k1
	*/
	j ehentry /* assumes ehentry in kseg0 */
eh_end:

.globl ehentry /* DO NOT MERGE */
ehentry:
	mfc0 $k0, $13 /* cause */
	andi $k0, 0xFC
	bnez $k0, fatal /* other than interrupt exception are... fatal. */

	/* save temps... s*,sp,fp(s8) are preserved by callee, no need to save at here */
	.set push
	.set noat
	la $k0, tempregs
	sw $at, 8*0($k0)
	sw $v0, 8*1($k0)
	sw $v1, 8*2($k0)
	sw $a0, 8*3($k0)
	sw $a1, 8*4($k0)
	sw $a2, 8*5($k0)
	sw $a3, 8*6($k0)
	sw $t0, 8*7($k0)
	sw $t1, 8*8($k0)
	sw $t2, 8*9($k0)
	sw $t3, 8*10($k0)
	sw $t4, 8*11($k0)
	sw $t5, 8*12($k0)
	sw $t6, 8*13($k0)
	sw $t7, 8*14($k0)
	sw $t8, 8*15($k0)
	sw $t9, 8*16($k0)
	sw $ra, 8*17($k0)
	#sw $gp, 8*18($k0)
	.set pop
	/*
	la $k1, inthandler
	jalr $k1
	*/
	jal inthandler
	.set push
	.set noat
	la $k0, tempregs
	lw $at, 8*0($k0)
	lw $v0, 8*1($k0)
	lw $v1, 8*2($k0)
	lw $a0, 8*3($k0)
	lw $a1, 8*4($k0)
	lw $a2, 8*5($k0)
	lw $a3, 8*6($k0)
	lw $t0, 8*7($k0)
	lw $t1, 8*8($k0)
	lw $t2, 8*9($k0)
	lw $t3, 8*10($k0)
	lw $t4, 8*11($k0)
	lw $t5, 8*12($k0)
	lw $t6, 8*13($k0)
	lw $t7, 8*14($k0)
	lw $t8, 8*15($k0)
	lw $t9, 8*16($k0)
	lw $ra, 8*17($k0)
	#lw $gp, 8*18($k0)
	.set pop
	eret

fatal:
	jal fatalhandler
	j . /* die */

	.global halt
halt:
	j halt
	jr $ra

	.section .bss
tempregs: .space 8*19 /* r1-r15,r24,r25,r31(,r28 if PIC) */
