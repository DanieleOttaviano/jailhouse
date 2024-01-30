	.text
	.align	2
	.globl  __stack_start
_start:
	la	sp, __stack_start
	jal	main
_hw_shutdown:
	jal	_hw_shutdown
