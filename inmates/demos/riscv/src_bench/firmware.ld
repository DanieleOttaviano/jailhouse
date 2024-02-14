/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.
*/

MEMORY {
	/* the memory in the testbench is 512M in size; */
	mem (rwx) : ORIGIN = 0x78000000, LENGTH = 0x8000000
}

SECTIONS {
	.memory : {
		. = ORIGIN(mem);
		start*(.text);
		*(.text);
		*(*);
		__stack_start = ORIGIN(mem) + LENGTH(mem);
		end = .;
	} > mem
}

ENTRY(_start)
