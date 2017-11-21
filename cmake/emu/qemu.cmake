if(EMU_PLATFORM)
  # TODO: Support more emulators

  if("${ARCH}" STREQUAL "x86")
    set_ifndef(QEMU_binary_suffix i386)
  else()
    set_ifndef(QEMU_binary_suffix ${ARCH})
  endif()
  find_program(
    QEMU
    qemu-system-${QEMU_binary_suffix}
    )

  set(qemu_targets
    run
    debugserver
    )

  # We can set "default" value for QEMU_PTY & QEMU_PIPE on cmake invocation.
  if(QEMU_PTY)
    # Send console output to a pseudo-tty, used for running automated tests
    set(CMAKE_QEMU_SERIAL0 pty)
  else()
    if(QEMU_PIPE)
      # Send console output to a pipe, used for running automated tests
      set(CMAKE_QEMU_SERIAL0 pipe:${QEMU_PIPE})
    else()
      set(CMAKE_QEMU_SERIAL0 mon:stdio)
    endif()
  endif()

  # But also can set QEMU_PTY & QEMU_PIPE on *make* (not cmake) invocation,
  # like it was before cmake.
  set(QEMU_FLAGS -serial \${if \${QEMU_PTY}, pty, \${if \${QEMU_PIPE}, pipe:\${QEMU_PIPE}, ${CMAKE_QEMU_SERIAL0}}})

  # Add a BT serial device when building for bluetooth, unless the
  # application explicitly opts out with NO_QEMU_SERIAL_BT_SERVER.
  if(CONFIG_BT)
    if(NOT NO_QEMU_SERIAL_BT_SERVER)
      list(APPEND QEMU_FLAGS -serial unix:/tmp/bt-server-bredr)
    endif()
  endif()

  # If we are running a networking application in QEMU, then set proper
  # QEMU variables. This also allows two QEMUs to be hooked together and
  # pass data between them. The QEMU flags are not set for standalone
  # tests defined by CONFIG_NET_TEST.
  if(CONFIG_NETWORKING)
    if(CONFIG_NET_SLIP_TAP)
      set(QEMU_NET_STACK 1)
    endif()
  endif()

  if(QEMU_NET_STACK)
    list(APPEND qemu_targets
      client
      server
      )

    list(APPEND QEMU_FLAGS
      -serial none
      )

    foreach(target ${qemu_targets})
      if((${target} STREQUAL client) OR (${target} STREQUAL server))
        list(APPEND MORE_FLAGS_FOR_${target}
          -serial pipe:/tmp/ip-stack-${target}
          -pidfile qemu-${target}.pid
          )
      else()
        # QEMU_INSTANCE is a command line argument to *make* (not cmake). By
        # appending the instance name to the pid file we can easily run more
        # instances of the same sample.
        list(APPEND MORE_FLAGS_FOR_${target}
          -serial unix:/tmp/slip.sock\${QEMU_INSTANCE}
          )
      endif()
    endforeach()


    set(PIPE_SERVER_IN  /tmp/ip-stack-server.in)
    set(PIPE_SERVER_OUT /tmp/ip-stack-server.out)
    set(PIPE_CLIENT_IN  /tmp/ip-stack-client.in)
    set(PIPE_CLIENT_OUT /tmp/ip-stack-client.out)

    set(pipes
      ${PIPE_SERVER_IN}
      ${PIPE_SERVER_OUT}
      ${PIPE_CLIENT_IN}
      ${PIPE_CLIENT_OUT}
      )

    set(destroy_pipe_commands
      COMMAND ${CMAKE_COMMAND} -E remove -f ${pipes}
      )

    # TODO: Port to Windows. Perhaps using python? Or removing the
    # need for mkfifo and create_symlink somehow.
    set(create_pipe_commands
      COMMAND mkfifo ${PIPE_SERVER_IN}
      COMMAND mkfifo ${PIPE_SERVER_OUT}
      )
    if(PCAP)
      list(APPEND create_pipe_commands
        COMMAND mkfifo ${PIPE_CLIENT_IN}
        COMMAND mkfifo ${PIPE_CLIENT_OUT}
        )
    else()
      list(APPEND create_pipe_commands
        COMMAND ${CMAKE_COMMAND} -E create_symlink ${PIPE_SERVER_IN}  ${PIPE_CLIENT_OUT}
        COMMAND ${CMAKE_COMMAND} -E create_symlink ${PIPE_SERVER_OUT} ${PIPE_CLIENT_IN}
        )
    endif()

    set(PRE_QEMU_COMMANDS_FOR_server
      ${destroy_pipe_commands}
      ${create_pipe_commands}
      )
    if(PCAP)
      # Start a monitor application to capture traffic
      #
      # Assumes;
      # PCAP has been set to the file where traffic should be captured
      # NET_TOOLS has been set to the net-tools repo path
      # net-tools/monitor_15_4 has been built beforehand

      set_ifndef(NET_TOOLS $ENV{ZEPHYR_BASE}/../net-tools) # Default if not set

      list(APPEND PRE_QEMU_COMMANDS_FOR_server
        COMMAND ${NET_TOOLS}/monitor_15_4
		${PCAP}
		/tmp/ip-stack-server
        /tmp/ip-stack-client
        > /dev/null &
        # TODO: Support cleanup of the monitor_15_4 process
        )
    endif()
  endif(QEMU_NET_STACK)

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

  list(APPEND MORE_FLAGS_FOR_debugserver -s -S)

  set_ifndef(QEMU_KERNEL_OPTION
    "-kernel;$<TARGET_FILE:${logical_target_for_zephyr_elf}>"
    )

  foreach(target ${qemu_targets})
    add_custom_target(${target}
      ${PRE_QEMU_COMMANDS}
      ${PRE_QEMU_COMMANDS_FOR_${target}}
      COMMAND
      ${QEMU}
      ${QEMU_FLAGS_${ARCH}}
      -pidfile qemu\${QEMU_INSTANCE}.pid
      ${QEMU_FLAGS}
      ${QEMU_EXTRA_FLAGS}
      ${MORE_FLAGS_FOR_${target}}
      ${QEMU_KERNEL_OPTION}
      DEPENDS ${logical_target_for_zephyr_elf}
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
