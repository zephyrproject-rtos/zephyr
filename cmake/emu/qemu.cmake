# SPDX-License-Identifier: Apache-2.0

# Assemble the QEMU command line and emit the run/debugserver targets.
#
# Fragments contribute to the command line through the qemu_append_*() API in
# cmake/emu/qemu/flags.cmake rather than by appending to variables by name. The
# pieces are concatenated in this order when the run targets are created below:
#
#   QEMU_UEFI_OPTION     boot through uefi-run instead of -kernel
#   QEMU_BOARD_FLAGS     machine and CPU selection, set by the board
#   FLAGS_DEVICE slot    emulated devices and host plumbing
#   FLAGS_EXTRA slot     late additions, after every device
#   TARGET_FLAGS_<t>     flags specific to one run target
#   QEMU_SMP_FLAGS       -smp
#   QEMU_KERNEL_OPTION   how to load the image
#
# Within a slot, flags keep the order in which they were contributed, so the
# include order of the fragments below is significant.

if(DEFINED QEMU_CPU_TYPE_${ARCH})
  message(FATAL_ERROR "QEMU_CPU_TYPE_\${ARCH} has been renamed to QEMU_CPU_TYPE.")
endif()

if(DEFINED QEMU_FLAGS_${ARCH})
  message(FATAL_ERROR "QEMU_FLAGS_\${ARCH} has been renamed to QEMU_BOARD_FLAGS.")
endif()

# QEMU_CPU_TYPE names the emulated CPU for the build system's benefit: it is
# reported in the run target's progress message. It is not passed to QEMU from
# here. Boards that need a -cpu argument reference it when composing
# QEMU_BOARD_FLAGS, and boards whose -machine already implies a CPU set it
# without passing -cpu at all.

include(${ZEPHYR_BASE}/cmake/emu/qemu/flags.cmake)
include(${ZEPHYR_BASE}/cmake/emu/qemu/binary.cmake)

# Boards contribute to the command line before this file runs, by appending to
# QEMU_EXTRA_FLAGS. Seed the EXTRA slot with what they left behind.
qemu_append_extra_flags(${QEMU_EXTRA_FLAGS})

# QEMU_INSTANCE is a command line argument to *make* (not cmake). With the
# Makefiles generator the reference must survive CMake expansion and reach the
# generated makefile verbatim, so that appending the instance name to the pid
# file lets several instances of the same sample run side by side.
if(CMAKE_GENERATOR STREQUAL "Unix Makefiles")
  set(qemu_instance "\${QEMU_INSTANCE}")
else()
  set(qemu_instance "${QEMU_INSTANCE}")
endif()

qemu_append_flags(-pidfile qemu${qemu_instance}.pid)

if(CONFIG_VIRTIO_MMIO)
  qemu_append_flags(-global virtio-mmio.force-legacy=false)
endif()

include(${ZEPHYR_BASE}/cmake/emu/qemu/console.cmake)
include(${ZEPHYR_BASE}/cmake/emu/qemu/display.cmake)
include(${ZEPHYR_BASE}/cmake/emu/qemu/icount.cmake)
include(${ZEPHYR_BASE}/cmake/emu/qemu/bluetooth.cmake)
include(${ZEPHYR_BASE}/cmake/emu/qemu/net_serial.cmake)
include(${ZEPHYR_BASE}/cmake/emu/qemu/can.cmake)
include(${ZEPHYR_BASE}/cmake/emu/qemu/x86_locore.cmake)
include(${ZEPHYR_BASE}/cmake/emu/qemu/ivshmem.cmake)
include(${ZEPHYR_BASE}/cmake/emu/qemu/nvme.cmake)
include(${ZEPHYR_BASE}/cmake/emu/qemu/net_nic.cmake)

if(CONFIG_FLASH_INTEL_PFLASH_CFI01)
  if(CONFIG_X86)
    # X86 Needs an initial bios file in pflash0 slot
    list(APPEND QEMU_EXTRA_FLAGS
      -drive file=${HOST_TOOLS_HOME}/usr/share/qemu/bios-256k.bin,if=pflash,format=raw,unit=0
    )
  endif()

  list(APPEND QEMU_EXTRA_FLAGS
    -drive file=${ZEPHYR_BINARY_DIR}/pflash.img,if=pflash,format=raw,unit=1
  )

  if(CONFIG_RISCV)
    set(PFLASH_SIZE 32768)
  else()
    set(PFLASH_SIZE 4096)
  endif()
  execute_process(COMMAND ${PYTHON_EXECUTABLE} -c
    "open('${ZEPHYR_BINARY_DIR}/pflash.img', 'wb').write(bytes([0])*${PFLASH_SIZE}*1024)"
    COMMAND_ERROR_IS_FATAL ANY
  )
endif()

if(NOT QEMU_PIPE)
  set(QEMU_PIPE_COMMENT "\nTo exit from QEMU enter: 'CTRL+a, x'\n")
endif()

# Don't just test CONFIG_SMP, there is at least one test of the lower
# level multiprocessor API that wants an auxiliary CPU but doesn't
# want SMP using it.
if(NOT CONFIG_MP_MAX_NUM_CPUS MATCHES "1")
  list(APPEND QEMU_SMP_FLAGS -smp cpus=${CONFIG_MP_MAX_NUM_CPUS})
endif()

# Use flags passed in from the environment
set(env_qemu $ENV{QEMU_EXTRA_FLAGS})
separate_arguments(env_qemu)
qemu_append_extra_flags(${env_qemu})

# Also append QEMU flags from config
if(NOT CONFIG_QEMU_EXTRA_FLAGS STREQUAL "")
  set(config_qemu_flags ${CONFIG_QEMU_EXTRA_FLAGS})
  separate_arguments(config_qemu_flags)
  qemu_append_extra_flags(${config_qemu_flags})
endif()

qemu_append_target_flags(debugserver_qemu -S)

if(NOT CONFIG_QEMU_GDBSERVER_LISTEN_DEV STREQUAL "")
  qemu_append_target_flags(debugserver_qemu -gdb "${CONFIG_QEMU_GDBSERVER_LISTEN_DEV}")
endif()

# Boards and architectures can define QEMU_KERNEL_FILE to use a specific output
# file to pass to qemu; whatever target generates that file must be registered
# with qemu_add_target_depends(). Alternatively they can set QEMU_KERNEL_OPTION
# if they want to replace the "-kernel ..." option entirely.
if(CONFIG_QEMU_UEFI_BOOT)
  set(QEMU_UEFI_OPTION  ${PROJECT_BINARY_DIR}/${CONFIG_KERNEL_BIN_NAME}.efi)
  list(APPEND QEMU_UEFI_OPTION --)
elseif(DEFINED QEMU_KERNEL_FILE)
  set(QEMU_KERNEL_OPTION "-kernel;${QEMU_KERNEL_FILE}")
elseif(NOT DEFINED QEMU_KERNEL_OPTION)
  set(QEMU_KERNEL_OPTION "-kernel;$<TARGET_FILE:${logical_target_for_zephyr_elf}>")
elseif(DEFINED QEMU_KERNEL_OPTION)
  string(CONFIGURE "${QEMU_KERNEL_OPTION}" QEMU_KERNEL_OPTION)
endif()

qemu_get_property(qemu_device_flags   FLAGS_DEVICE)
qemu_get_property(qemu_extra_flags    FLAGS_EXTRA)
qemu_get_property(qemu_target_depends TARGET_DEPENDS)
qemu_get_run_targets(qemu_targets)

foreach(target ${qemu_targets})
  qemu_get_property(pre_commands  PRE_COMMANDS_${target})
  qemu_get_property(post_commands POST_COMMANDS_${target})
  qemu_get_property(target_flags  TARGET_FLAGS_${target})

  add_custom_target(${target}
    ${pre_commands}
    COMMAND
    ${QEMU}
    ${QEMU_UEFI_OPTION}
    ${QEMU_BOARD_FLAGS}
    ${qemu_device_flags}
    ${qemu_extra_flags}
    ${target_flags}
    ${QEMU_SMP_FLAGS}
    ${QEMU_KERNEL_OPTION}
    ${post_commands}
    DEPENDS ${logical_target_for_zephyr_elf}
    WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
    COMMENT "${QEMU_PIPE_COMMENT}[QEMU] CPU: ${QEMU_CPU_TYPE}"
    USES_TERMINAL
  )
  if(qemu_target_depends)
    add_dependencies(${target} ${qemu_target_depends})
  endif()
endforeach()
