#
# Copyright (c) Charles Dias <charlesdias.cd@outlook.com>
#
# SPDX-License-Identifier: Apache-2.0
#

# Add custom linker section to relocate framebuffers to PSRAM
zephyr_linker_sources_ifdef(CONFIG_LV_Z_VDB_CUSTOM_SECTION
  SECTIONS dc_ram.ld)
