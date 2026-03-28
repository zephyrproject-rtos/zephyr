# SPDX-License-Identifier: Apache-2.0
# The contents of this file is based on include/zephyr/linker/kobject-priv-stacks.ld
# Please keep in sync

if(CONFIG_USERSPACE)
  if(CONFIG_GEN_PRIV_STACKS)
    # Padding is needed to preserve kobject addresses
    # if we have reserved more space than needed.
    zephyr_linker_section(NAME .priv_stacks_noinit GROUP NOINIT_REGION NOINPUT NOINIT
                MIN_SIZE @KOBJECT_PRIV_STACKS_SZ,undef:0@ MAX_SIZE @KOBJECT_PRIV_STACKS_SZ,undef:0@)

    zephyr_linker_section_configure(
      SECTION .priv_stacks_noinit
      SYMBOLS z_priv_stacks_ram_start
    )

    # During LINKER_KOBJECT_PREBUILT and LINKER_ZEPHYR_PREBUILT,
    # space needs to be reserved for the rodata that will be
    # produced by gperf during the final stages of linking.
    # The alignment and size are produced by
    # scripts/build/gen_kobject_placeholders.py. These are here
    # so the addresses to kobjects would remain the same
    # during the final stages of linking (LINKER_ZEPHYR_FINAL).
    zephyr_linker_section_configure(
      SECTION .priv_stacks_noinit
      ALIGN @KOBJECT_PRIV_STACKS_ALIGN,undef:0@
      INPUT ".priv_stacks.noinit"
      KEEP
      PASS LINKER_ZEPHYR_PREBUILT LINKER_ZEPHYR_FINAL
      SYMBOLS z_priv_stacks_ram_aligned_start z_priv_stacks_ram_end
    )

    if(KOBJECT_PRIV_STACKS_ALIGN)
      zephyr_linker_symbol(
        SYMBOL z_priv_stacks_ram_used
        EXPR "(@z_priv_stacks_ram_end@ - @z_priv_stacks_ram_start@)"
        PASS LINKER_ZEPHYR_FINAL
      )
    endif()
  endif()
endif()
