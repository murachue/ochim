OUTPUT_FORMAT("elf32-bigmips")
OUTPUT_ARCH(mips)
EXTERN(_start)
ENTRY(_start)

MEMORY {
	RAM (rwx) : ORIGIN = 0x80000400, LENGTH = 0x00400000-0x0400 /* memory without expand */
	ROM (rx)  : ORIGIN = 0x10001000, LENGTH = 0x04000000-0x1000 /* 64MiB ROM */
}

SECTIONS {
	. = 0x80000400;

	.text : {
		FILL (0)

		*(.boot)
		. = ALIGN(8); /* 64bit align required by PI */
		__text_start = .;
		*(.text .text.*)
		*(.ctors)
		*(.dtors)
		*(.rodata .rodata.*)
		*(.init)
		*(.fini)
		__text_end  = .;

		/* I don't know why, but section are forcefully aligned by 8 (even specifying ALIGN(1) at section command!!),
		   and it cause vacuumed area, and it causes symbol-offsetting when converted to "binary".
		   It's very regrettable, but solve it by fill vacuum by .text, here. */
		. = ALIGN(8);
	} >RAM AT>ROM

	.data : {
		FILL (0xaa)

		__data_start = .;
		*(.data .data.*)
		_gp = . + 0x7ff0;
		*(.lit8)
		*(.lit4)
		*(.sdata .sdata.*)
		*(.eh_frame)
		. = ALIGN(8); /* 64bit align required by PI */
		__data_end = . ;

		/* there is also vacuum area between .data and .bss, but it's trivial. do nothing. */
	} >RAM AT>ROM

	.bss (NOLOAD) : {
		. = ALIGN(4);
		__bss_start = . ;
		*(.scommon)
		*(.sbss .sbss.*)
		*(COMMON)
		*(.bss .bss.*)
		. = ALIGN(4);
		__bss_end = . ;
		end = . ;
	} >RAM

	/DISCARD/ : {
		*(.MIPS.abiflags)
	}
}
