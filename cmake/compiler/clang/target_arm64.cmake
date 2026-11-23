# SPDX-License-Identifier: Apache-2.0

# Add SVE support if enabled, or explicitly disable it for ARMv9-A
if(CONFIG_ARM64_SVE)
  if(DEFINED GCC_M_ARCH)
    set(GCC_M_ARCH "${GCC_M_ARCH}+sve")
  else()
    set(GCC_M_ARCH "armv9-a+sve")
  endif()
elseif(CONFIG_ARMV9_A)
  # ARMv9-A includes SVE by default, so explicitly disable it when not configured
  if(DEFINED GCC_M_ARCH)
    set(GCC_M_ARCH "${GCC_M_ARCH}+nosve")
  else()
    set(GCC_M_ARCH "armv9-a+nosve")
  endif()
endif()

if(DEFINED GCC_M_CPU)
  list(APPEND TOOLCHAIN_C_FLAGS   -mcpu=${GCC_M_CPU})
  list(APPEND TOOLCHAIN_LD_FLAGS  -mcpu=${GCC_M_CPU})
endif()

if(DEFINED GCC_M_ARCH)
  list(APPEND TOOLCHAIN_C_FLAGS   -march=${GCC_M_ARCH})
  list(APPEND TOOLCHAIN_LD_FLAGS  -march=${GCC_M_ARCH})
endif()

if(DEFINED GCC_M_TUNE)
  list(APPEND TOOLCHAIN_C_FLAGS   -mtune=${GCC_M_TUNE})
  list(APPEND TOOLCHAIN_LD_FLAGS  -mtune=${GCC_M_TUNE})
endif()

# Branch protection (PAC/BTI) compiler flags
if(CONFIG_ARM_PACBTI_STANDARD)
  list(APPEND TOOLCHAIN_C_FLAGS -mbranch-protection=standard)
elseif(CONFIG_ARM_PACBTI_PACRET)
  list(APPEND TOOLCHAIN_C_FLAGS -mbranch-protection=pac-ret)
elseif(CONFIG_ARM_PACBTI_PACRET_LEAF)
  list(APPEND TOOLCHAIN_C_FLAGS -mbranch-protection=pac-ret+leaf)
elseif(CONFIG_ARM_PACBTI_BTI)
  list(APPEND TOOLCHAIN_C_FLAGS -mbranch-protection=bti)
elseif(CONFIG_ARM_PACBTI_PACRET_BTI)
  list(APPEND TOOLCHAIN_C_FLAGS -mbranch-protection=pac-ret+bti)
elseif(CONFIG_ARM_PACBTI_PACRET_LEAF_BTI)
  list(APPEND TOOLCHAIN_C_FLAGS -mbranch-protection=pac-ret+leaf+bti)
endif()
