bits 64

section .rdata

global <name>_start
global <name>_end
global <name>_size

<name>_start: incbin "<bin>"
<name>_end:
<name>_size: dd $ - <name>_start
