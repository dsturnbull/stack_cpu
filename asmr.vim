" Vim syntax file
" Language:	Microsoft Macro Assembler (80x86)
" Orig Author:	Rob Brady <robb@datatone.com>
" Maintainer:	Wu Yongwei <wuyongwei@gmail.com>
" Last Change:	$Date: 2007/04/21 13:20:15 $
" $Revision: 1.44 $

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

syn case ignore


syn match asmrIdentifier	"[@a-z_$?][@a-z0-9_$?]*"
syn match asmrLabel		"_[a-z_]*"

syn match asmrHexadecimal	"[-+]\?0x[0-9a-f]*"

syn match asmrComment		";.*" contains=@Spell
syn region asmrString		start=+'+ end=+'+ oneline contains=@Spell
syn region asmrString		start=+"+ end=+"+ oneline contains=@Spell

syn match asmrTitle		"[^\t ;]\([^;]*[^\t ;]\)\?" contained contains=@Spell
syn match asmrText		"[^\t ;]\([^;]*[^\t ;]\)\?" contained

"syn match asmrConstant		"[A-Z0-9]*"

" opcodes
syn keyword asmrOpcode		NOP HLT OUT LOAD STORE ADD SUB MUL DIV
syn keyword asmrOpcode		JMP JZ JNZ CALL RET DUP POP SWAP INT DEBUG
syn keyword asmrOpcode		PUSH

command -nargs=+ HiLink hi def link <args>

" The default methods for highlighting.  Can be overridden later
HiLink asmrLabel	PreProc
HiLink asmrComment	Comment
HiLink asmrString	String
HiLink asmrOpcode	Statement

HiLink asmrHexadecimal	Number

HiLink asmrIdentifier	Identifier

"HiLink asmrConstant	Constant

syntax sync minlines=50

delcommand HiLink

let b:current_syntax = "asmr"

" vim: ts=8
