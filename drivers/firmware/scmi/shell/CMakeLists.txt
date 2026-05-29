# SPDX-License-Identifier: Apache-2.0
#
# Dependency on CONFIG_ARM_SCMI_SHELL implied by the root SCMI
# CMakeLists.txt, which includes this conditionally.
zephyr_library()

# core module - implements root scmi command
zephyr_library_sources(core.c)

# protocol module - implements protocol-related sub-commands
zephyr_library_sources_ifdef(CONFIG_ARM_SCMI_CLK_HELPERS clk.c)
