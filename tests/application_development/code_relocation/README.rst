.. _code_relocation:

Code relocation
#################

Overview
********
A simple example that demonstrates how relocation of code, data or bss sections
using a custom linker script.

The build also checks that relocation directives do not appear as source files in
the CMake File API codemodel and that generator expressions forwarded through
multiple target properties resolve to the expected file list.
