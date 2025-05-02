# SPDX-License-Identifier: Apache-2.0
# The contents of this file is based on include/zephyr/linker/kobject-text.ld
# Please keep in sync

if(CONFIG_USERSPACE)
  zephyr_linker_section(NAME _kobject_text_area
                        GROUP TEXT_REGION NOINPUT)

  zephyr_linker_section_configure(
    SECTION
    _kobject_text_area
    INPUT
    ".kobject_data.literal*"
    ".kobject_data.text*"
    MIN_SIZE ${CONFIG_KOBJECT_TEXT_AREA}
    MAX_SIZE ${CONFIG_KOBJECT_TEXT_AREA}
    SYMBOLS
    _kobject_text_area_start
    _kobject_text_area_end
    )
  zephyr_linker_symbol(
    SYMBOL
    _kobject_text_area_used
    EXPR
    "(@_kobject_text_area_end@ - @_kobject_text_area_start@)"
    )
endif()
