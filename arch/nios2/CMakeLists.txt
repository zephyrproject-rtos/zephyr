# SPDX-License-Identifier: Apache-2.0

if(CONFIG_GP_NONE)
set(gpopt none)
elseif(CONFIG_GP_LOCAL)
set(gpopt local)
elseif(CONFIG_GP_GLOBAL)
set(gpopt global)
elseif(CONFIG_GP_ALL_DATA)
set(gpopt data)
endif()

# Set Global Pointer option based on Kconfig.
zephyr_cc_option(-mgpopt=${gpopt})

# TODO Find a way to pull this out of system.h somehow
# instead of having Kconfig for it

if(CONFIG_HAS_MUL_INSTRUCTION)
zephyr_cc_option(-mhw-mul)
else()
zephyr_cc_option(-mno-hw-mul)
endif()

if(CONFIG_HAS_MULX_INSTRUCTION)
zephyr_cc_option(-mhw-mulx)
else()
zephyr_cc_option(-mno-hw-mulx)
endif()

if(CONFIG_HAS_DIV_INSTRUCTION)
zephyr_cc_option(-mhw-div)
else()
zephyr_cc_option(-mno-hw-div)
endif()

add_subdirectory(core)
