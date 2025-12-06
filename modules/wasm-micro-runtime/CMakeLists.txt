# Copyright (c) 2021 Intel Corporation
# SPDX-Licensedentifier: Apache-2.0

if(CONFIG_WAMR)

  set(WAMR_DIR ${ZEPHYR_CURRENT_MODULE_DIR})

  zephyr_library()

  zephyr_include_directories(
    ${WAMR_DIR}/core
    ${WAMR_DIR}/core/iwasm/include
    ${WAMR_DIR}/core/iwasm/common
    ${WAMR_DIR}/core/iwasm/interpreter
    ${WAMR_DIR}/core/shared/platform/common/libc-util
    ${WAMR_DIR}/core/shared/platform/include
    ${WAMR_DIR}/core/shared/platform/zephyr
    ${WAMR_DIR}/core/shared/utils
    ${WAMR_DIR}/core/shared/mem-alloc
  )

  zephyr_library_compile_options(-Wno-unused-variable)

  zephyr_compile_definitions(
    BH_PLATFORM_ZEPHYR
    BH_MALLOC=wasm_runtime_malloc
    BH_FREE=wasm_runtime_free
  )

  # General files
  zephyr_library_sources(
    ${WAMR_DIR}/core/shared/platform/zephyr/zephyr_platform.c
    ${WAMR_DIR}/core/shared/platform/zephyr/zephyr_thread.c
    ${WAMR_DIR}/core/shared/platform/zephyr/zephyr_time.c
    ${WAMR_DIR}/core/shared/platform/common/math/math.c
    ${WAMR_DIR}/core/shared/platform/common/libc-util/libc_errno.c
    ${WAMR_DIR}/core/shared/mem-alloc/mem_alloc.c
    ${WAMR_DIR}/core/shared/mem-alloc/ems/ems_kfc.c
    ${WAMR_DIR}/core/shared/mem-alloc/ems/ems_gc.c
    ${WAMR_DIR}/core/shared/mem-alloc/ems/ems_hmu.c
    ${WAMR_DIR}/core/shared/mem-alloc/ems/ems_alloc.c
    ${WAMR_DIR}/core/shared/utils/bh_assert.c
    ${WAMR_DIR}/core/shared/utils/bh_bitmap.c
    ${WAMR_DIR}/core/shared/utils/bh_common.c
    ${WAMR_DIR}/core/shared/utils/bh_hashmap.c
    ${WAMR_DIR}/core/shared/utils/bh_leb128.c
    ${WAMR_DIR}/core/shared/utils/bh_list.c
    ${WAMR_DIR}/core/shared/utils/bh_log.c
    ${WAMR_DIR}/core/shared/utils/bh_queue.c
    ${WAMR_DIR}/core/shared/utils/bh_vector.c
    ${WAMR_DIR}/core/shared/utils/runtime_timer.c
    ${WAMR_DIR}/core/shared/utils/uncommon/bh_getopt.c
    ${WAMR_DIR}/core/shared/utils/uncommon/bh_read_file.c
    ${WAMR_DIR}/core/iwasm/common/wasm_runtime_common.c
    ${WAMR_DIR}/core/iwasm/common/wasm_exec_env.c
    ${WAMR_DIR}/core/iwasm/common/wasm_native.c
    ${WAMR_DIR}/core/iwasm/common/wasm_memory.c
    ${WAMR_DIR}/core/iwasm/common/wasm_shared_memory.c
    ${WAMR_DIR}/core/iwasm/common/wasm_c_api.c
    ${WAMR_DIR}/core/iwasm/common/wasm_application.c
    ${WAMR_DIR}/core/iwasm/common/wasm_loader_common.c
  )

  zephyr_library_sources_ifdef(CONFIG_WAMR_LIBC_WASI
    ${WAMR_DIR}/core/shared/platform/zephyr/zephyr_socket.c
    ${WAMR_DIR}/core/shared/platform/zephyr/zephyr_file.c
    ${WAMR_DIR}/core/shared/platform/zephyr/zephyr_clock.c
  )

  # Build target macros and source files
  if(CONFIG_WAMR_BUILD_TARGET)
    string(TOUPPER ${CONFIG_WAMR_BUILD_TARGET} CONFIG_WAMR_BUILD_TARGET)
  endif()
  if((CONFIG_WAMR_BUILD_TARGET STREQUAL "X86_64") OR CONFIG_X86_64 OR
     (CONFIG_ARCH_POSIX AND (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "x86_64")))
    zephyr_compile_definitions(
      BUILD_TARGET_X86_64
      BUILD_TARGET="X86_64"
    )
    if(NOT ${CONFIG_WAMR_SIMD})
      zephyr_library_sources(
        ${WAMR_DIR}/core/iwasm/common/arch/invokeNative_em64.s
      )
    else()
      zephyr_library_sources(
        ${WAMR_DIR}/core/iwasm/common/arch/invokeNative_em64_simd.s
      )
    endif()
    if(${CONFIG_WAMR_AOT})
      zephyr_library_sources(
        ${WAMR_DIR}/core/iwasm/aot/arch/aot_reloc_x86_64.c
      )
    endif()
  elseif((CONFIG_WAMR_BUILD_TARGET STREQUAL "X86_32") OR CONFIG_X86 OR
	  (CONFIG_ARCH_POSIX AND (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "x86")))
    zephyr_compile_definitions(
      BUILD_TARGET_X86_32
      BUILD_TARGET="X86_32"
    )
    zephyr_library_sources(
      ${WAMR_DIR}/core/iwasm/common/arch/invokeNative_ia32.s
    )
    if(${CONFIG_WAMR_AOT})
      zephyr_library_sources(
        ${WAMR_DIR}/core/iwasm/aot/arch/aot_reloc_x86_32.c
      )
    endif()
  elseif((CONFIG_WAMR_BUILD_TARGET STREQUAL "XTENSA") OR
         (${CONFIG_XTENSA}))
    zephyr_compile_definitions(
      BUILD_TARGET_XTENSA
      BUILD_TARGET="XTENSA"
    )
    zephyr_library_sources(
      ${WAMR_DIR}/core/iwasm/common/arch/invokeNative_xtensa.s
    )
    if(${CONFIG_WAMR_AOT})
      zephyr_library_sources(
        ${WAMR_DIR}/core/iwasm/aot/arch/aot_reloc_xtensa.c
      )
    endif()
  elseif((CONFIG_WAMR_BUILD_TARGET STREQUAL "RISCV64") OR
         (CONFIG_WAMR_BUILD_TARGET STREQUAL "RISCV64_LP64D"))
    zephyr_compile_definitions(
      BUILD_TARGET_RISCV64_LP64D
      BUILD_TARGET="RISCV64_LP64D"
    )
    zephyr_library_sources(
      ${WAMR_DIR}/core/iwasm/common/arch/invokeNative_riscv.S
    )
    if(${CONFIG_WAMR_AOT})
      zephyr_library_sources(
        ${WAMR_DIR}/core/iwasm/aot/arch/aot_reloc_riscv.c
      )
    endif()
  elseif((CONFIG_WAMR_BUILD_TARGET STREQUAL "RISCV64_LP64") OR
         ((${CONFIG_RISCV}) AND (${CONFIG_64BIT})))
    zephyr_compile_definitions(
      BUILD_TARGET_RISCV64_LP64
      BUILD_TARGET="RISCV64_LP64"
    )
    zephyr_library_sources(
      ${WAMR_DIR}/core/iwasm/common/arch/invokeNative_riscv.S
    )
    if(${CONFIG_WAMR_AOT})
      zephyr_library_sources(
        ${WAMR_DIR}/core/iwasm/aot/arch/aot_reloc_riscv.c
      )
    endif()
  elseif((CONFIG_WAMR_BUILD_TARGET STREQUAL "RISCV32") OR
         (CONFIG_WAMR_BUILD_TARGET STREQUAL "RISCV32_ILP32D"))
    zephyr_compile_definitions(
      BUILD_TARGET_RISCV32_ILP32D
      BUILD_TARGET="RISCV32_ILP32D"
    )
    zephyr_library_sources(
      ${WAMR_DIR}/core/iwasm/common/arch/invokeNative_riscv.S
    )
    if(${CONFIG_WAMR_AOT})
      zephyr_library_sources(
        ${WAMR_DIR}/core/iwasm/aot/arch/aot_reloc_riscv.c
      )
    endif()
  elseif((CONFIG_WAMR_BUILD_TARGET STREQUAL "RISCV32_ILP32") OR
         ((${CONFIG_RISCV}) AND (NOT (${CONFIG_64BIT}))))
    zephyr_compile_definitions(
      BUILD_TARGET_RISCV32_ILP32
      BUILD_TARGET="RISCV32_ILP32"
    )
    zephyr_library_sources(
      ${WAMR_DIR}/core/iwasm/common/arch/invokeNative_riscv.S
    )
    if(${CONFIG_WAMR_AOT})
      zephyr_library_sources(
        ${WAMR_DIR}/core/iwasm/aot/arch/aot_reloc_riscv.c
      )
    endif()
      zephyr_library_sources(
        ${WAMR_DIR}/core/iwasm/aot/arch/aot_reloc_riscv.c
      )
  elseif((CONFIG_WAMR_BUILD_TARGET STREQUAL "ARC") OR (${CONFIG_ARC}))
    zephyr_compile_definitions(
      BUILD_TARGET_ARC
      BUILD_TARGET="ARC"
    )
    zephyr_library_sources(
      ${WAMR_DIR}/core/iwasm/common/arch/invokeNative_arc.s
    )
    if(${CONFIG_WAMR_AOT})
      zephyr_library_sources(
        ${WAMR_DIR}/core/iwasm/aot/arch/aot_reloc_arc.c
      )
    endif()
  elseif(CONFIG_WAMR_BUILD_TARGET MATCHES "ARM.*")
    if(CONFIG_WAMR_BUILD_TARGET MATCHES "(ARM.*)_VFP")
      add_definitions(-DBUILD_TARGET_ARM_VFP)
      add_definitions(-DBUILD_TARGET="${CMAKE_MATCH_1}")
      zephyr_library_sources(
        ${WAMR_DIR}/core/iwasm/common/arch/invokeNative_arm_vfp.s
      )
      if(${CONFIG_WAMR_AOT})
        zephyr_library_sources(
          ${WAMR_DIR}/core/iwasm/aot/arch/aot_reloc_arm.c
        )
      endif()
    else()
      add_definitions(-DBUILD_TARGET_ARM)
      add_definitions(-DBUILD_TARGET="${CONFIG_WAMR_BUILD_TARGET}")
      zephyr_library_sources(
        ${WAMR_DIR}/core/iwasm/common/arch/invokeNative_arm.s
      )
      if(${CONFIG_WAMR_AOT})
        zephyr_library_sources(
          ${WAMR_DIR}/core/iwasm/aot/arch/aot_reloc_arm.c
        )
      endif()
    endif()
  elseif(CONFIG_WAMR_BUILD_TARGET MATCHES "THUMB.*")
    if(CONFIG_WAMR_BUILD_TARGET MATCHES "(THUMB.*)_VFP")
      add_definitions(-DBUILD_TARGET_THUMB_VFP)
      add_definitions(-DBUILD_TARGET="${CMAKE_MATCH_1}")
      zephyr_library_sources(
        ${WAMR_DIR}/core/iwasm/common/arch/invokeNative_thumb_vfp.s
      )
      if(${CONFIG_WAMR_AOT})
        zephyr_library_sources(
          ${WAMR_DIR}/core/iwasm/aot/arch/aot_reloc_thumb.c
        )
      endif()
    else()
      add_definitions(-DBUILD_TARGET_THUMB)
      add_definitions(-DBUILD_TARGET="${CONFIG_WAMR_BUILD_TARGET}")
      zephyr_library_sources(
        ${WAMR_DIR}/core/iwasm/common/arch/invokeNative_thumb.s
      )
      if(${CONFIG_WAMR_AOT})
        zephyr_library_sources(
          ${WAMR_DIR}/core/iwasm/aot/arch/aot_reloc_thumb.c
        )
      endif()
    endif()
  elseif(CONFIG_WAMR_BUILD_TARGET MATCHES "AARCH64.*")
    add_definitions(-DBUILD_TARGET_AARCH64)
    add_definitions(-DBUILD_TARGET="${CONFIG_WAMR_BUILD_TARGET}")
    if(NOT ${CONFIG_WAMR_SIMD})
      zephyr_library_sources(
        ${WAMR_DIR}/core/iwasm/common/arch/invokeNative_aarch64.s
      )
    else()
      zephyr_library_sources(
        ${WAMR_DIR}/core/iwasm/common/arch/invokeNative_aarch64_simd.s
      )
    endif()
    if(${CONFIG_WAMR_AOT})
      zephyr_library_sources(
        ${WAMR_DIR}/core/iwasm/aot/arch/aot_reloc_aarch64.c
      )
    endif()
  elseif(${CONFIG_ARM})
    if(${CONFIG_ISA_THUMB2})
      zephyr_compile_definitions(
        BUILD_TARGET_THUMB
        BUILD_TARGET="THUMB"
      )
      zephyr_library_sources(
        ${WAMR_DIR}/core/iwasm/common/arch/invokeNative_thumb.s
      )
      if(${CONFIG_WAMR_AOT})
        zephyr_library_sources(
          ${WAMR_DIR}/core/iwasm/aot/arch/aot_reloc_thumb.c
        )
      endif()
    else()
      zephyr_compile_definitions(
        BUILD_TARGET_ARM
        BUILD_TARGET="ARM"
      )
      zephyr_library_sources(
        ${WAMR_DIR}/core/iwasm/common/arch/invokeNative_arm.s
      )
      if(${CONFIG_WAMR_AOT})
        zephyr_library_sources(
          ${WAMR_DIR}/core/iwasm/aot/arch/aot_reloc_arm.c
        )
      endif()
    endif()
  elseif(${CONFIG_ARM64})
    zephyr_compile_definitions(
      BUILD_TARGET_AARCH64
      BUILD_TARGET="AARCH64"
    )
    if(NOT ${CONFIG_WAMR_SIMD})
      zephyr_library_sources(
        ${WAMR_DIR}/core/iwasm/common/arch/invokeNative_aarch64.s
      )
    else()
      zephyr_library_sources(
        ${WAMR_DIR}/core/iwasm/common/arch/invokeNative_aarch64_simd.s
      )
    endif()
    if(${CONFIG_WAMR_AOT})
      zephyr_library_sources(
        ${WAMR_DIR}/core/iwasm/aot/arch/aot_reloc_aarch64.c
      )
    endif()
  else()
    message(FATAL_ERROR "Unsupported CPU architecture")
  endif()

  # Interpreter macros and source files
  zephyr_compile_definitions_ifdef(CONFIG_WAMR_INTERP
    WASM_ENABLE_INTERP=1
  )

  if(${CONFIG_WAMR_INTERP})
    zephyr_library_sources(
      ${WAMR_DIR}/core/iwasm/interpreter/wasm_loader.c
      ${WAMR_DIR}/core/iwasm/interpreter/wasm_runtime.c
    )
    if(${CONFIG_WAMR_FAST_INTERP})
      zephyr_compile_definitions(
        WASM_ENABLE_FAST_INTERP=1
      )
      zephyr_library_sources(
        ${WAMR_DIR}/core/iwasm/interpreter/wasm_interp_fast.c
      )
    else()
      zephyr_library_sources(
        ${WAMR_DIR}/core/iwasm/interpreter/wasm_interp_classic.c
      )
    endif()
  endif()

  # AOT macros and source files
  zephyr_compile_definitions_ifdef(CONFIG_WAMR_AOT
    WASM_ENABLE_AOT=1
  )

  zephyr_include_directories_ifdef(CONFIG_WAMR_AOT
    ${WAMR_DIR}/core/iwasm/aot
  )

  zephyr_library_sources_ifdef(CONFIG_WAMR_AOT
    ${WAMR_DIR}/core/iwasm/aot/aot_loader.c
    ${WAMR_DIR}/core/iwasm/aot/aot_runtime.c
    ${WAMR_DIR}/core/iwasm/aot/aot_intrinsic.c
  )

  # libc builtin
  zephyr_compile_definitions_ifdef(CONFIG_WAMR_LIBC_BUILTIN
    WASM_ENABLE_LIBC_BUILTIN=1
  )

  zephyr_library_sources_ifdef(CONFIG_WAMR_LIBC_BUILTIN
    ${WAMR_DIR}/core/iwasm/libraries/libc-builtin/libc_builtin_wrapper.c
  )

  # libc WASI
  zephyr_compile_definitions_ifdef(CONFIG_WAMR_LIBC_WASI
    WASM_ENABLE_LIBC_WASI=1
  )

  zephyr_include_directories_ifdef(CONFIG_WAMR_LIBC_WASI
    ${WAMR_DIR}/core/iwasm/libraries/libc-wasi/sandboxed-system-primitives/include
    ${WAMR_DIR}/core/iwasm/libraries/libc-wasi/sandboxed-system-primitives/src
  )

  zephyr_library_sources_ifdef(CONFIG_WAMR_LIBC_WASI
    ${WAMR_DIR}/core/iwasm/libraries/libc-wasi/libc_wasi_wrapper.c
    ${WAMR_DIR}/core/iwasm/libraries/libc-wasi/sandboxed-system-primitives/src/blocking_op.c
    ${WAMR_DIR}/core/iwasm/libraries/libc-wasi/sandboxed-system-primitives/src/posix.c
    ${WAMR_DIR}/core/iwasm/libraries/libc-wasi/sandboxed-system-primitives/src/random.c
    ${WAMR_DIR}/core/iwasm/libraries/libc-wasi/sandboxed-system-primitives/src/str.c
  )

  # lib pthread
  zephyr_compile_definitions_ifdef(CONFIG_WAMR_LIB_PTHREAD
      WASM_ENABLE_LIB_PTHREAD=1
      WASM_ENABLE_THREAD_MGR=1
      WASM_ENABLE_BULK_MEMORY=1
      WASM_ENABLE_SHARED_MEMORY=1
  )

  zephyr_include_directories_ifdef(CONFIG_WAMR_LIB_PTHREAD
    ${WAMR_DIR}/core/iwasm/libraries/thread-mgr
  )

  zephyr_library_sources_ifdef(CONFIG_WAMR_LIB_PTHREAD
    ${WAMR_DIR}/core/iwasm/libraries/lib-pthread/lib_pthread_wrapper.c
    ${WAMR_DIR}/core/iwasm/libraries/thread-mgr/thread_manager.c
  )

  # SIMD
  zephyr_compile_definitions_ifdef(CONFIG_WAMR_SIMD
    WASM_ENABLE_SIMD=1
  )

  # Reference types
  zephyr_compile_definitions_ifdef(CONFIG_WAMR_REF_TYPES
      WASM_ENABLE_REF_TYPES=1
  )

  # Memory profiling
  zephyr_compile_definitions_ifdef(CONFIG_WAMR_MEMORY_PROFILING
    WASM_ENABLE_MEMORY_PROFILING=1
  )

  # Performance profiling
  zephyr_compile_definitions_ifdef(CONFIG_WAMR_PERF_PROFILING
    WASM_ENABLE_PERF_PROFILING=1
  )

  # Dump call stack
  zephyr_compile_definitions_ifdef(CONFIG_WAMR_DUMP_CALL_STACK
    WASM_ENABLE_DUMP_CALL_STACK=1
  )

  # MPU stack count and size
  zephyr_compile_definitions_ifdef(CONFIG_WAMR_MPU_STACK_COUNT
    BH_ZEPHYR_MPU_STACK_COUNT=${CONFIG_WAMR_MPU_STACK_COUNT}
  )

  zephyr_compile_definitions_ifdef(CONFIG_WAMR_MPU_STACK_SIZE
      BH_ZEPHYR_MPU_STACK_SIZE=${CONFIG_WAMR_MPU_STACK_SIZE}
  )

endif()
