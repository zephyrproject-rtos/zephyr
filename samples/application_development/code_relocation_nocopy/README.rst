.. _code_relocation_nocopy:

Code relocation nocopy
######################

Overview
********
A simple example that demonstrates how relocation of code, data or bss sections
using a custom linker script.

Differently from the code relocation sample, this sample is relocating the
content of the ext_code.c file to a different FLASH section and the code is XIP
directly from there without the need to copy / relocate the code.
