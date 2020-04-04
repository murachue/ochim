#include <stdint.h>
#include <string.h> // bzero

#include "regs.h"

#define BUFADDR 0x03FF8000

enum STATUS {
	STATUS_IE  = 1 << 0,
	STATUS_IM2 = 1 << 10,
	STATUS_IM4 = 1 << 12,
	STATUS_CU1 = 1 << 29,
};

static inline uint32_t getsr(void) {
	register uint32_t r;
	__asm volatile("mfc0 %0, $12" : "=r"(r));
	return r;
}

static inline void setsr(uint32_t v) {
	__asm volatile("mtc0 %0, $12" :: "r"(v));
}

// new rom must be loaded into romarea
// TODO: should check rom checksum1/2
// TODO: should set LAT/PWD/PGS/RLS
// TODO: support other than 6102?
static void hotload_go(void) {
	// don't interrupt until IPL3/nextprogram allows.
	setsr(getsr() & ~STATUS_IE);

	// load IPL3 into DMEM area
	for(uint32_t i = 0; i < 0x0FC0; i += 4) {
		*(uint32_t*)(0xA4000040 + i) = *(uint32_t*)(0xB0000040 + i);
	}

	// prepare "arguments" for IPL3
	asm volatile ("li $s3, 0"); // is64ddboot
	asm volatile ("lw $s4, 0x80000300"); // tvtype saved by 6102 bootcode
	asm volatile ("li $s5, 0"); // resettype(iscoldboot)
	asm volatile ("li $s6, 0x3F"); // cicseed(6102)
	asm volatile ("lw $s7, 0x80000314"); // "osversion" saved by 6102 bootcode
	asm volatile ("li $sp, 0xA4001FF0");

	// I hope compiler does not put code that changes $s3-$s7 here...

	// pass control to IPL3
	// TODO: ensure we are kernel mode...
	((void(*)(void))0xA4000040)();
}

// PI must not busy before call
static uint32_t ed64_dma(uint32_t addr, uint32_t size, uint32_t cfg) {
	ED64->dmalen = size/512 - 1;
	ED64->cfg; // dummy read ensures not PI_busy
	ED64->dmaaddr = addr/2048;
	ED64->cfg; // dummy read ensures not PI_busy
	ED64->dmacfg = cfg;

	uint32_t status;
	while((status = ED64->status) & ED64_STATUS_DMABUSY) /*busyloop*/ ;

	return status;
}

// PI must not busy before call
static uint32_t ed64_rspk(void) {
	// buf[3] must be 'k', others are ignored (including 'RSP' "ReSPonse"?)
	*(uint32_t*)(0xB0000000 + BUFADDR) = 0x5253506B/*bigendian 'RSPk'*/;
	ED64->cfg; // dummy read ensures not PI_busy
	return ed64_dma(BUFADDR, 512, ED64_DMACFG_RAM_TO_FIFO);
}

// #RXF must be active
static void hotload_ed64cmd(void) {
	// read command into very end of rom area
	// last PI I/O must be read... (ED64->status read)
	uint32_t status = ed64_dma(BUFADDR, 512, ED64_DMACFG_FIFO_TO_RAM);
	if(status & ED64_STATUS_DMATIMEOUT) {
		return;
	}

	// parse OS64 USB Protocol (directly!)
	switch(*(uint32_t*)(0xB0000000 + BUFADDR)) {
	case 0x434D4454: // bigendian 'CMDT'
		// test
		ed64_rspk(); // last cmd read ensures not PI_busy
		break;
	case 0x434D4446: // bigendian 'CMDF'
		// fill2M (for smaller ROM)
		{
			static uint8_t __attribute((aligned(8))) zerobuf[2048];
			bzero(zerobuf, sizeof(zerobuf));
			for(int32_t i = 0; i < 1024; i++) {
				PI->dramaddr = (uint32_t)zerobuf & 0x00FFffff;
				PI->cartaddr = 0x10000000 | (i * 2048); // dom1addr2
				PI->dram2cart = 2048 - 1;
				while(PI->status & PI_STATUS_DMABUSY) /*busyloop*/ ;
			}
			ed64_rspk(); // last cmd read ensures not PI_busy
		}
		break;
	case 0x434D4457: // bigendian 'CMDW'
		// write (from host)
		{
			uint32_t addrsize = *(uint32_t*)(0xB0000000 + BUFADDR + 4);
			// last addrsize read ensures not PI_busy
			ed64_dma((addrsize >> 16) * 2048, (addrsize & 0xFFFF) * 512, ED64_DMACFG_FIFO_TO_RAM);
		}
		break;
	case 0x434D4453: // bigendian 'CMDS'
		// start
		// disable ED64regs before Go
		ED64->key = 0; // last cmd read ensures not PI_busy TODO what if hotload_go failed? ED64regs still disabled!!
		ED64->cfg; // unnecessary for PI_busy but necessary for ED64 to work after warm-reboot (or ED64 crashes with fade-blink internal LED)
		// then, Go!
		hotload_go(); // noreturn
		break;
	}
}

static void hotload_ed64test(void) {
	uint32_t status = ED64->status;
	uint32_t orgstatus = status;

	// it seems ED64->status bit15 is "ED64 regs enabled"... really?
	if(!(status & 0x8000)) {
		// if not ED64regs is enabled, let it be enabled.
		ED64->key = 0x1234; // last ED64->status read ensures not PI_busy
		ED64->cfg; // unnecessary for PI_busy but necessary for ED64 to return valid status.
		status = ED64->status;
	}

	// TODO: perpetual disable & return if it does not look like ED64 (ex. 64drive or wild flashcart)

	if(!(status & ED64_STATUS_RXF)) {
		// #RXF is active!
		hotload_ed64cmd();
	}

	// turn ED64regs off if that was off.
	if(!(orgstatus & 0x8000)) {
		ED64->cfg; // dummy read ensures not PI_busy
		ED64->key = 0;
		ED64->cfg; // unnecessary for PI_busy but necessary for ED64...
	}
}

void hotload_rx(void) {
	hotload_ed64test();
	// hotload_64dtest();
}
