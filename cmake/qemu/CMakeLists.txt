if(EMU_PLATFORM)
  # TODO: Support more emulators

  # QEMU_INSTANCE is a command line argument to make. By appending the
  # instance name to the pid file we can easily run more instances of
  # the same sample.

  set(qemu_targets
    run
    debugserver
    )

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

  if(QEMU_NET_STACK)
    list(APPEND QEMU_FLAGS
      -serial none
      -serial unix:/tmp/slip.sock${QEMU_INSTANCE}
      )
  endif()


  if(CONFIG_X86_IAMCU)
    list(APPEND PRE_QEMU_COMMANDS
      COMMAND
      ${PYTHON_EXECUTABLE}
      $ENV{ZEPHYR_BASE}/scripts/qemu-machine-hack.py
      $<TARGET_FILE:${logical_target_for_zephyr_elf}>
      )
  endif()

  if(NOT QEMU_PIPE)
    set(QEMU_PIPE_COMMENT "\nTo exit from QEMU enter: 'CTRL+a, x'\n")
  endif()

  # Use flags passed in from the environment
  set(env_qemu $ENV{QEMU_EXTRA_FLAGS})
  separate_arguments(env_qemu)
  list(APPEND QEMU_EXTRA_FLAGS ${env_qemu})

  set(MORE_FLAGS_FOR_debugserver -s -S)

  foreach(target ${qemu_targets})
    add_custom_target(${target}
      ${PRE_QEMU_COMMANDS}
      COMMAND
      ${QEMU}
      ${QEMU_FLAGS_${ARCH}}
      -pidfile qemu${QEMU_INSTANCE}.pid
      ${QEMU_FLAGS}
      ${QEMU_EXTRA_FLAGS}
      ${MORE_FLAGS_FOR_${target}}
      -kernel $<TARGET_FILE:${logical_target_for_zephyr_elf}>
      WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
      COMMENT "${QEMU_PIPE_COMMENT}[QEMU] CPU: ${QEMU_CPU_TYPE_${ARCH}}"
      USES_TERMINAL
      )
  endforeach()
else()
  add_custom_target(run
    COMMAND
    ${CMAKE_COMMAND} -E echo
    "==================================================="
	"Emulation/Simulation not supported with this board."
    "==================================================="
    )
endif()
