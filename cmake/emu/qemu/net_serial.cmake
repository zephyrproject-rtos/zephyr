# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# Serial transport for networking (SLIP, PPP, IEEE 802.15.4 over UART pipe).
#
# This hooks two QEMU instances together through host FIFOs so they can pass
# packets between each other, and adds the extra "client"/"server"/"node" run
# targets that drive them. The Ethernet NIC model, which is the other half of
# QEMU networking, lives in net_nic.cmake.
#
# TODO: This is POSIX-only; it shells out to mkfifo, stty and pkill.

# If we are running a networking application in QEMU, then set proper
# QEMU variables. This also allows two QEMUs to be hooked together and
# pass data between them. The QEMU flags are not set for standalone
# tests defined by CONFIG_NET_TEST. For PPP, the serial port file is
# not available if we run unit tests which define CONFIG_NET_TEST.
if(CONFIG_NETWORKING)
  if(CONFIG_NET_QEMU_SLIP)
    if((CONFIG_NET_SLIP_TAP) OR (CONFIG_IEEE802154_UPIPE))
      set(QEMU_NET_STACK 1)
    endif()
  elseif((CONFIG_NET_QEMU_PPP) AND NOT (CONFIG_NET_TEST))
    set(QEMU_NET_STACK 1)
  endif()
endif()

# TO create independent pipes for each QEMU application set QEMU_PIPE_STACK
if(QEMU_PIPE_STACK)
  list(APPEND qemu_targets
    node
  )

  if(NOT QEMU_PIPE_ID)
    set(QEMU_PIPE_ID 1)
  endif()

  list(APPEND QEMU_FLAGS
    -serial none
  )

  list(APPEND MORE_FLAGS_FOR_node
    -serial pipe:/tmp/hub/ip-stack-node${QEMU_PIPE_ID}
    -pidfile qemu-node${QEMU_PIPE_ID}.pid
  )

  set(PIPE_NODE_IN  /tmp/hub/ip-stack-node${QEMU_PIPE_ID}.in)
  set(PIPE_NODE_OUT /tmp/hub/ip-stack-node${QEMU_PIPE_ID}.out)

  set(pipes
    ${PIPE_NODE_IN}
    ${PIPE_NODE_OUT}
  )

  set(destroy_pipe_commands
    COMMAND ${CMAKE_COMMAND} -E remove -f ${pipes}
  )

  set(create_pipe_commands
    COMMAND ${CMAKE_COMMAND} -E make_directory /tmp/hub
    COMMAND mkfifo ${PIPE_NODE_IN}
    COMMAND mkfifo ${PIPE_NODE_OUT}
  )

  set(PRE_QEMU_COMMANDS_FOR_node
    ${destroy_pipe_commands}
    ${create_pipe_commands}
  )

elseif(QEMU_NET_STACK)
  list(APPEND qemu_targets
    client
    server
  )

  foreach(target ${qemu_targets})
    if((${target} STREQUAL client) OR (${target} STREQUAL server))
      list(APPEND MORE_FLAGS_FOR_${target}
        -serial pipe:/tmp/ip-stack-${target}
        -pidfile qemu-${target}.pid
      )
    elseif(CONFIG_NET_QEMU_PPP)
      list(APPEND MORE_FLAGS_FOR_${target} -serial unix:/tmp/ppp${qemu_instance})
    else()
      list(APPEND MORE_FLAGS_FOR_${target} -serial unix:/tmp/slip.sock${qemu_instance})
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

    set_ifndef(NET_TOOLS ${ZEPHYR_BASE}/../tools/net-tools) # Default if not set

    list(APPEND PRE_QEMU_COMMANDS_FOR_server
      #Disable Ctrl-C to ensure that users won't accidentally exit
      #w/o killing the monitor.
      COMMAND stty intr ^d

      #This command is run in the background using '&'. This prevents
      #chaining other commands with '&&'. The command is enclosed in '{}'
      #to fix this.
      COMMAND {
        ${NET_TOOLS}/monitor_15_4
        ${PCAP}
        /tmp/ip-stack-server
        /tmp/ip-stack-client
        > /dev/null &
      }
    )
    set(POST_QEMU_COMMANDS_FOR_server
      # Re-enable Ctrl-C.
      COMMAND stty intr ^c

      # Kill the monitor_15_4 sub-process
      COMMAND pkill -P $$$$
    )
  endif()
endif()
