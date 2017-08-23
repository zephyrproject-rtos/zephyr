# QEMU_INSTANCE is a command line argument to make. By appending the
# instance name to the pid file we can easily run more instances of
# the same sample.

set(QEMU_FLAGS -serial)
if(QEMU_PTY)
  list(APPEND QEMU_FLAGS pty)
else()
  if(QEMU_PIPE)
    # Send console output to a pipe, used for running automated sanity tests
    list(APPEND QEMU_FLAGS pipe:${QEMU_PIPE})
  else()
    list(APPEND QEMU_FLAGS mon:stdio)
  endif()
endif()

#TODO To exit from QEMU enter

if(CONFIG_X86_IAMCU)
  set(IAMCU_HACK COMMAND ${PYTHON_EXECUTABLE} $ENV{ZEPHYR_BASE}/scripts/qemu-machine-hack.py $<TARGET_FILE:${logical_target_for_zephyr_elf}>)
endif()

if(NOT QEMU_PIPE)
  set(QEMU_PIPE_COMMENT "\nTo exit from QEMU enter: 'CTRL+a, x'\n")
endif()

add_custom_target(run
  ${IAMCU_HACK}
  COMMAND
  ${QEMU}
  ${QEMU_FLAGS_${ARCH}}
  -pidfile qemu${QEMU_INSTANCE}.pid
  ${QEMU_FLAGS}
  ${QEMU_EXTRA_FLAGS}
  -kernel $<TARGET_FILE:${logical_target_for_zephyr_elf}>
  WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
  COMMENT "${QEMU_PIPE_COMMENT}[QEMU] CPU: ${QEMU_CPU_TYPE_${ARCH}}"
  USES_TERMINAL
)
