# SPDX-License-Identifier: Apache-2.0

set_property(GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT "elf32-sparc")
add_subdirectory_ifdef(CONFIG_SPARC core)
