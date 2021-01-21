
.section ".text.startup"

.global _start
_start:
    ldr sp, =0x8000
    bl kernel_main