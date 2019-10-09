# SPDX-License-Identifier: Apache-2.0

macro(toolchain_cc_asan)

zephyr_compile_options(-fsanitize=address)
zephyr_link_libraries(-lasan)
zephyr_ld_options(-fsanitize=address)

endmacro()

macro(toolchain_cc_ubsan)

zephyr_compile_options(-fsanitize=undefined)
zephyr_ld_options(-fsanitize=undefined)

endmacro()
