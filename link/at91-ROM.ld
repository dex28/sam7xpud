/*---------------------------------------------------------------------------*/
/*-         ATMEL Microcontroller Software Support  -  ROUSSET  -            */
/*---------------------------------------------------------------------------*/
/* The software is delivered "AS IS" without warranty or condition of any    */
/* kind, either express, implied or statutory. This includes without         */
/* limitation any warranty or condition with respect to merchantability or   */
/* fitness for any particular purpose, or against the infringements of       */
/* intellectual property rights of others.                                   */
/*---------------------------------------------------------------------------*/
/*- File source          : AT91SAM7S_sections_ROM.ldh                        */
/*- Object               : Section-definitions for the Flash workspace       */
/*                         (to be included from the linker-script)           */
/*- Compilation flag     : None                                              */
/*-                                                                          */
/*- 1.0 20/Oct/04 JPP    : Creation                                          */
/*                                                                           */
/*  Modifications by Martin Thomas (not Atmel) for WinARM                    */
/*  1.1mt  --.---.----   : ramfunc, C++-support, and more                    */
/*  1.2mt  04.Aug.2006   : split of MEMORY and SECTIONS                      */
/*                         support for function- and data-sections (gcc4)    */
/*  1.3mt  02.Sep.2006   : added NOLOAD at .bss                              */
/*---------------------------------------------------------------------------*/

/*
__FIRST_IN_RAM = ORIGIN(DATA);
__TOP_STACK    = ORIGIN(DATA) + LENGTH(DATA);
*/

/* Section Definitions */

SECTIONS
{
	/* first section is .text which is used for code */
	. = ORIGIN(CODE);
	
	.text :
	{
		KEEP(*(.vectrom))
		. = ALIGN(4);
		KEEP(*(.init))
		*(.text .text.*)                   /* remaining code */
		*(.gnu.linkonce.t.*)
		*(.glue_7)
		*(.glue_7t)
		*(.gcc_except_table)
		*(.rodata)                 /* read-only data (constants) */
		*(.rodata.*)
		*(.gnu.linkonce.r.*)
		. = ALIGN(4);
	} >CODE 
	
	. = ALIGN(4);

	/* .ctors .dtors are used for c++ constructors/destructors */
	
	.ctors :
	{
		PROVIDE(__ctors_start__ = .);
		KEEP(*(SORT(.ctors.*)))
		KEEP(*(.ctors))
		PROVIDE(__ctors_end__ = .);
	} >CODE

	.dtors :
	{
		PROVIDE(__dtors_start__ = .); 
		KEEP(*(SORT(.dtors.*)))
		KEEP(*(.dtors))
		PROVIDE(__dtors_end__ = .);
	} >CODE
	
	. = ALIGN(4);

	_etext = . ;
	PROVIDE (etext = .);

	/* .data section which is used for initialized data */
	.data : AT (_etext)
	{
		_data = . ;
		__data_start = .;
		KEEP(*(.vectmapped))
		. = ALIGN(4);
		*(.fastrun .fastrun.*)
		. = ALIGN(4);
		SORT(CONSTRUCTORS)
		. = ALIGN(4);
		*(.data)
		*(.data.*)
		*(.gnu.linkonce.d.*)
		. = ALIGN(4);
	} >DATA
	
	. = ALIGN(4);
	
	_edata = . ;
	PROVIDE (edata = .);
	
	/* .bss section which is used for uninitialized data */
	.bss (NOLOAD) :
	{
		__bss_start = . ;
		__bss_start__ = . ;
		*(.bss)
		*(.bss.*)
		*(.gnu.linkonce.b.*)
		*(COMMON)
		. = ALIGN(4);
	} >DATA
	
	. = ALIGN(4);
	
	__bss_end__ = . ;
	
	_end = .;
	PROVIDE (end = .);
	
	/* Stabs debugging sections.  */
	.stab          0 : { *(.stab) }
	.stabstr       0 : { *(.stabstr) }
	.stab.excl     0 : { *(.stab.excl) }
	.stab.exclstr  0 : { *(.stab.exclstr) }
	.stab.index    0 : { *(.stab.index) }
	.stab.indexstr 0 : { *(.stab.indexstr) }
	.comment       0 : { *(.comment) }
	/* DWARF debug sections.
	 Symbols in the DWARF debugging sections are relative to the beginning
	 of the section so we begin them at 0.  */
	/* DWARF 1 */
	.debug          0 : { *(.debug) }
	.line           0 : { *(.line) }
	/* GNU DWARF 1 extensions */
	.debug_srcinfo  0 : { *(.debug_srcinfo) }
	.debug_sfnames  0 : { *(.debug_sfnames) }
	/* DWARF 1.1 and DWARF 2 */
	.debug_aranges  0 : { *(.debug_aranges) }
	.debug_pubnames 0 : { *(.debug_pubnames) }
	/* DWARF 2 */
	.debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
	.debug_abbrev   0 : { *(.debug_abbrev) }
	.debug_line     0 : { *(.debug_line) }
	.debug_frame    0 : { *(.debug_frame) }
	.debug_str      0 : { *(.debug_str) }
	.debug_loc      0 : { *(.debug_loc) }
	.debug_macinfo  0 : { *(.debug_macinfo) }
	/* SGI/MIPS DWARF 2 extensions */
	.debug_weaknames 0 : { *(.debug_weaknames) }
	.debug_funcnames 0 : { *(.debug_funcnames) }
	.debug_typenames 0 : { *(.debug_typenames) }
	.debug_varnames  0 : { *(.debug_varnames) }

}
