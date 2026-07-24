# SPDX-License-Identifier: Apache-2.0

if(CONFIG_LLVM_USE_LD)
  set(LINKER ld)
elseif(CONFIG_LLVM_USE_LLD)
  set(LINKER lld)
endif()

if(NOT "${ARCH}" STREQUAL "posix")
  # Locate the Zephyr SDK directory
  if(DEFINED ZEPHYR_SDK_INSTALL_DIR)
      set(SDK_DIR "${ZEPHYR_SDK_INSTALL_DIR}")
  elseif(DEFINED ENV{ZEPHYR_SDK_INSTALL_DIR})
      set(SDK_DIR "$ENV{ZEPHYR_SDK_INSTALL_DIR}")
  elseif(DEFINED ZEPHYR_TOOLCHAIN_PATH)
      set(SDK_DIR "${ZEPHYR_TOOLCHAIN_PATH}")
  elseif(DEFINED ENV{ZEPHYR_TOOLCHAIN_PATH})
      set(SDK_DIR "$ENV{ZEPHYR_TOOLCHAIN_PATH}")
  else()
      # Try finding Zephyr-sdk package using find_package
      find_package(Zephyr-sdk QUIET CONFIG)
      if(ZEPHYR_SDK_INSTALL_DIR)
          set(SDK_DIR "${ZEPHYR_SDK_INSTALL_DIR}")
      elseif(Zephyr-sdk_FOUND AND Zephyr-sdk_DIR)
          set(SDK_DIR "${Zephyr-sdk_DIR}")
      else()
          # Fallback search in standard directories
          file(GLOB sdk_candidates
              LIST_DIRECTORIES true
              "$ENV{HOME}/zephyr-sdk-*"
              "/opt/zephyr-sdk-*"
              "$ENV{HOME}/.local/zephyr-sdk-*"
              "/usr/local/zephyr-sdk-*"
          )
          if(sdk_candidates)
              list(SORT sdk_candidates COMPARE NATURAL ORDER DESCENDING)
              list(GET sdk_candidates 0 SDK_DIR)
          endif()
      endif()
  endif()

  if(NOT SDK_DIR OR NOT EXISTS "${SDK_DIR}")
      message(FATAL_ERROR "Zephyr SDK directory not found! Please set ZEPHYR_SDK_INSTALL_DIR or ZEPHYR_TOOLCHAIN_PATH environment variable.")
  endif()

  # Set ZEPHYR_SDK_INSTALL_DIR so that SDK cmake files can find it
  set(ZEPHYR_SDK_INSTALL_DIR "${SDK_DIR}")

  # Include target-specific triple settings from Zephyr SDK's LLVM directory
  if(EXISTS "${SDK_DIR}/cmake/zephyr/llvm/target.cmake")
      include("${SDK_DIR}/cmake/zephyr/llvm/target.cmake")
  else()
      message(FATAL_ERROR "Zephyr SDK LLVM target configuration not found at ${SDK_DIR}/cmake/zephyr/llvm/target.cmake")
  endif()

  # Include Zephyr SDK's GNU/general target mappings to determine the sysroot target names (e.g. riscv64-zephyr-elf, arm-zephyr-eabi, etc.)
  if(EXISTS "${SDK_DIR}/cmake/zephyr/gnu/target.cmake")
      include("${SDK_DIR}/cmake/zephyr/gnu/target.cmake")
  elseif(EXISTS "${SDK_DIR}/cmake/zephyr/target.cmake")
      include("${SDK_DIR}/cmake/zephyr/target.cmake")
  endif()

  # Fallback to the root-level target folder if the nested gnu subdirectory is missing
  if(NOT EXISTS "${SYSROOT_DIR}" AND DEFINED SYSROOT_TARGET)
      if(EXISTS "${SDK_DIR}/${SYSROOT_TARGET}/${SYSROOT_TARGET}")
          set(SYSROOT_DIR "${SDK_DIR}/${SYSROOT_TARGET}/${SYSROOT_TARGET}")
      endif()
  endif()

  if(SYSROOT_DIR AND EXISTS "${SYSROOT_DIR}")
      list(APPEND TOOLCHAIN_C_FLAGS --sysroot=${SYSROOT_DIR})
      list(APPEND TOOLCHAIN_LD_FLAGS --sysroot=${SYSROOT_DIR})
  else()
      message(FATAL_ERROR "Could not resolve sysroot directory for target '${SYSROOT_TARGET}' in Zephyr SDK at '${SDK_DIR}'")
  endif()

  # Instruct custom LLVM Clang to use the Zephyr SDK's LLVM resource directory.
  # The resource-dir contains embedded compiler-rt and clang builtins.
  # We dynamically locate the resource-dir (which is versioned, e.g. lib/clang/20) using file(GLOB).
  if(NOT "${ARCH}" STREQUAL "xtensa")
    file(GLOB CLANG_RESOURCE_DIR LIST_DIRECTORIES true "${SDK_DIR}/llvm/lib/clang/[0-9]*")
    if(CLANG_RESOURCE_DIR)
        list(GET CLANG_RESOURCE_DIR 0 SDK_CLANG_RESOURCE_DIR)
        list(APPEND TOOLCHAIN_C_FLAGS -resource-dir=${SDK_CLANG_RESOURCE_DIR})
        list(APPEND TOOLCHAIN_LD_FLAGS -resource-dir=${SDK_CLANG_RESOURCE_DIR})
    else()
        message(FATAL_ERROR "Clang resource directory not found in ${SDK_DIR}/llvm/lib/clang/")
    endif()
  endif()

  # Override compiler_set_linker_properties to query the SDK compiler for runtime libraries.
  set(COMPILER_SET_LINKER_PROPERTIES_DEFINED TRUE)
  function(compiler_set_linker_properties)
    # Find the SDK directory. We prefer the CMake variable, then environment variable.
    if(DEFINED ZEPHYR_SDK_INSTALL_DIR)
      set(SDK_DIR "${ZEPHYR_SDK_INSTALL_DIR}")
    elseif(DEFINED ENV{ZEPHYR_SDK_INSTALL_DIR})
      set(SDK_DIR "$ENV{ZEPHYR_SDK_INSTALL_DIR}")
    elseif(DEFINED ZEPHYR_TOOLCHAIN_PATH)
      set(SDK_DIR "${ZEPHYR_TOOLCHAIN_PATH}")
    elseif(DEFINED ENV{ZEPHYR_TOOLCHAIN_PATH})
      set(SDK_DIR "$ENV{ZEPHYR_TOOLCHAIN_PATH}")
    else()
      # Try finding Zephyr-sdk package using find_package
      find_package(Zephyr-sdk QUIET CONFIG)
      if(ZEPHYR_SDK_INSTALL_DIR)
        set(SDK_DIR "${ZEPHYR_SDK_INSTALL_DIR}")
      elseif(Zephyr-sdk_FOUND AND Zephyr-sdk_DIR)
        set(SDK_DIR "${Zephyr-sdk_DIR}")
      else()
        # Fallback search in standard directories
        file(GLOB sdk_candidates
          LIST_DIRECTORIES true
          "$ENV{HOME}/zephyr-sdk-*"
          "/opt/zephyr-sdk-*"
          "$ENV{HOME}/.local/zephyr-sdk-*"
          "/usr/local/zephyr-sdk-*"
        )
        if(sdk_candidates)
          list(SORT sdk_candidates COMPARE NATURAL ORDER DESCENDING)
          list(GET sdk_candidates 0 SDK_DIR)
        endif()
      endif()
    endif()

    if(NOT SDK_DIR OR NOT EXISTS "${SDK_DIR}")
      message(FATAL_ERROR "Zephyr SDK directory not found! Please define ZEPHYR_SDK_INSTALL_DIR or ZEPHYR_TOOLCHAIN_PATH environment variable.")
    endif()

    set(SDK_CLANG "${SDK_DIR}/llvm/bin/clang")

    if(EXISTS "${SDK_CLANG}" AND NOT "${ARCH}" STREQUAL "xtensa")
      # We query the SDK clang for the multilib builtins path.
      # We need to filter out -resource-dir and --sysroot from TOOLCHAIN_C_FLAGS
      # to let the SDK Clang query its own default multilib mappings.
      set(query_compiler "${SDK_CLANG}")
      set(filtered_flags "")
      foreach(flag ${TOOLCHAIN_C_FLAGS})
        if(NOT flag MATCHES "^-resource-dir=" AND NOT flag MATCHES "^--sysroot=")
          list(APPEND filtered_flags ${flag})
        endif()
      endforeach()
    else()
      # Fallback to the host compiler if SDK clang doesn't exist (or for xtensa)
      set(query_compiler "${CMAKE_C_COMPILER}")
      set(filtered_flags "")
      foreach(flag ${TOOLCHAIN_C_FLAGS})
        if(NOT flag MATCHES "^-resource-dir=" AND NOT flag MATCHES "^--sysroot=")
          list(APPEND filtered_flags ${flag})
        endif()
      endforeach()
    endif()

    # Get compile options without generator expressions
    get_property(flags TARGET zephyr_interface PROPERTY INTERFACE_COMPILE_OPTIONS)
    set(simple_options "")
    foreach(flag ${flags})
      string(GENEX_STRIP "${flag}" sflag)
      if(flag STREQUAL sflag)
        if(flag MATCHES "^SHELL:[ ]*(.*)")
          separate_arguments(flag UNIX_COMMAND ${CMAKE_MATCH_1})
        endif()
        if(NOT flag MATCHES "^-resource-dir=" AND NOT flag MATCHES "^--sysroot=")
          list(APPEND simple_options ${flag})
        endif()
      endif()
    endforeach()

    if(DEFINED CMAKE_C_COMPILER_TARGET)
      set(target_flag "--target=${CMAKE_C_COMPILER_TARGET}")
    endif()
    if("${ARCH}" STREQUAL "xtensa" AND DEFINED CONFIG_SOC)
      list(APPEND target_flag "-mcpu=${CONFIG_SOC}")
    endif()

    # Query for the libgcc/compiler-rt library path
    execute_process(
      COMMAND ${query_compiler} ${filtered_flags} ${COMPILER_OPTIMIZATION_FLAG} ${simple_options}
      ${target_flag}
      --print-libgcc-file-name
      OUTPUT_VARIABLE library_path
      OUTPUT_STRIP_TRAILING_WHITESPACE
      )

    if(library_path)
      set_linker_property(PROPERTY lib_include_dir "")
      set_linker_property(PROPERTY rt_library "${library_path}")
    endif()
  endfunction()

else()
  # Native builds (e.g. native_sim on posix) use the host sysroot and default configs.
  if(CONFIG_LIBGCC_RTLIB)
    set(runtime_lib "libgcc")
  elseif(CONFIG_COMPILER_RT_RTLIB)
    set(runtime_lib "compiler_rt")
  endif()

  if(DEFINED runtime_lib)
    list(APPEND TOOLCHAIN_C_FLAGS
         --config=${ZEPHYR_BASE}/cmake/toolchain/host/llvm/clang_${runtime_lib}.cfg)
    list(APPEND TOOLCHAIN_LD_FLAGS
         --config=${ZEPHYR_BASE}/cmake/toolchain/host/llvm/clang_${runtime_lib}.cfg)
  endif()
endif()
