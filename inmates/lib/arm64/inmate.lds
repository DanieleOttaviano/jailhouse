SECTIONS {
 . = 0x0;
 .boot : { *(.boot) }
 . = 0x0 + 0x1000;
 .cmdline : {
  *(.cmdline)
  BYTE(0);
 }
 bss_start = .;
 .bss : {
  *(.bss)
 }
 . = ALIGN(4);
 .text : {
  *(.text)
 }
 .rodata : {
  *(.rodata)
 }
 .data : {
  *(.data)
 }
 . = ALIGN(4096);
 . = . + 0x1000;
 stack_top = .;
}
ENTRY(__reset_entry)
