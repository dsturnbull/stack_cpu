push	_read_key
call
hlt

_read_key:
	push	0x0
	push	_read_key_print
	int

_read_key_print:
	dup
	push	0x71
	sub
	push	_read_key_ret
	swap
	jz

	out

	push	_read_key
	jmp

_read_key_ret:
	pop

	push	0xa
	out

	ret
