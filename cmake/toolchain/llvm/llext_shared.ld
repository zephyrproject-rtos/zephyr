/* llext_shared.ld
 * Linker script overlay for LLEXT shared libraries built with Clang.
 * Places .rodata BEFORE .text so l32r literal references work.
 * Merges all .text.* and .literal.* sections into .text.
 * .rodata must NOT be marked executable.
 */
SECTIONS {
  .rodata : {
    *(.rodata .rodata.*)
    *(._log_const.*)
    *(.module)
    *(.exported_sym)
  }
  .text : {
    *(.literal .literal.*)
    *(.text .text.*)
  }
}
