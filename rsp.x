OUTPUT_FORMAT("elf32-bigmips")
OUTPUT_ARCH(mips)

MEMORY {
	DMEM (x)  : ORIGIN = 0x00000000, LENGTH = 0x00001000
	IMEM (rw) : ORIGIN = 0x00001000, LENGTH = 0x00001000
}

SECTIONS {
	. = 0x00000000;

	.text : {
		FILL (0x00)
		*(.text .text.*)
		. = ALIGN(8);
	} >IMEM

	.data : {
		FILL (0xaa)
		*(.rodata .rodata.*)
		*(.data .data.*)
		. = ALIGN(8);
	} >DMEM

	.bss (NOLOAD) : {
		*(.scommon)
		*(.sbss .sbss.*)
		*(COMMON)
		*(.bss .bss.*)
	} >DMEM

	/DISCARD/ : {
		*(.MIPS.abiflags)
	}
}
