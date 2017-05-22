bits 64
section .rodata
global _<name>_start
global _<name>_end
global _<name>_size

_<name>_start: incbin "<bin>"
_<name>_end:
_<name>_size: dd $ - _<name>_start
