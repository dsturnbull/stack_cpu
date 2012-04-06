; loop counter
push	0x0
push	0x10
store

; kick things off
push	_main
call
hlt

_main:
	push	0x0		; load loop counter
	load
	push	_print_num	; print the counter
	call

	push	0xa		; newline
	out

	push	0x0		; decrement loop counter
	push	0x0
	load
	push	0x1		; by 1
	sub
	store

	push	_main		; loop until counter == 0
	push	0x0
	load
	jnz
	ret

; number on stack
_print_num:
	dup
	push	_print_num_ret
	swap
	jz

	; divide num by 10
	push	0xa
	div

	; push remainder
	push	0x30
	add

	; loop until quo == 0
	swap
	push	_print_num
	call

_print_num_ret:
	out
	ret

