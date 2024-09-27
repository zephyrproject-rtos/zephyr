# SPDX-License-Identifier: Apache-2.0

if("${ARCH}" STREQUAL "x86")
  set_ifndef(QEMU_binary_suffix i386)
elseif("${ARCH}" STREQUAL "mips")
  if(CONFIG_BIG_ENDIAN)
    set_ifndef(QEMU_binary_suffix mips)
  else()
    set_ifndef(QEMU_binary_suffix mipsel)
  endif()
elseif(DEFINED QEMU_ARCH)
  set_ifndef(QEMU_binary_suffix ${QEMU_ARCH})
else()
  set_ifndef(QEMU_binary_suffix ${ARCH})
endif()

set(qemu_alternate_path $ENV{QEMU_BIN_PATH})
if(qemu_alternate_path)
find_program(
  QEMU
  PATHS ${qemu_alternate_path}
  NO_DEFAULT_PATH
  NAMES qemu-system-${QEMU_binary_suffix}
  )
else()
find_program(
  QEMU
  qemu-system-${QEMU_binary_suffix}
  )
endif()

# We need to set up uefi-run and OVMF environment
# for testing UEFI method on qemu platforms
if(CONFIG_QEMU_UEFI_BOOT)
  find_program(UEFI NAMES uefi-run REQUIRED)
  if(DEFINED ENV{OVMF_FD_PATH})
    set(OVMF_FD_PATH $ENV{OVMF_FD_PATH})
  else()
    message(FATAL_ERROR "Couldn't find an valid OVMF_FD_PATH.")
  endif()
  list(APPEND UEFI -b ${OVMF_FD_PATH} -q ${QEMU})
  set(QEMU ${UEFI})
endif()

set(qemu_targets
  run_qemu
  debugserver_qemu
  )

set(QEMU_FLAGS -pidfile)
if(${CMAKE_GENERATOR} STREQUAL "Unix Makefiles")
  list(APPEND QEMU_FLAGS qemu\${QEMU_INSTANCE}.pid)
else()
  list(APPEND QEMU_FLAGS qemu${QEMU_INSTANCE}.pid)
endif()

# If running with sysbuild, we need to ensure this variable is populated
zephyr_get(QEMU_PIPE)
# Set up chardev for console.
if(QEMU_PTY)
  # Redirect console to a pseudo-tty, used for running automated tests.
  list(APPEND QEMU_FLAGS -chardev pty,id=con,mux=on)
elseif(QEMU_PIPE)
  # Redirect console to a pipe, used for running automated tests.
  list(APPEND QEMU_FLAGS -chardev pipe,id=con,mux=on,path=${QEMU_PIPE})
  # Create the pipe file before passing the path to QEMU.
  foreach(target ${qemu_targets})
    list(APPEND PRE_QEMU_COMMANDS_FOR_${target} COMMAND ${CMAKE_COMMAND} -E touch ${QEMU_PIPE})
  endforeach()
else()
  # Redirect console to stdio, used for manual debugging.
  list(APPEND QEMU_FLAGS -chardev stdio,id=con,mux=on)
endif()

# Connect main serial port to the console chardev.
list(APPEND QEMU_FLAGS -serial chardev:con)

# Connect semihosting console to the console chardev if configured.
if(CONFIG_SEMIHOST)
  list(APPEND QEMU_FLAGS
    -semihosting-config enable=on,target=auto,chardev=con
    )
endif()

# Connect monitor to the console chardev.
list(APPEND QEMU_FLAGS -mon chardev=con,mode=readline)

if(CONFIG_QEMU_ICOUNT)
  if(CONFIG_QEMU_ICOUNT_SLEEP)
    list(APPEND QEMU_FLAGS
	  -icount shift=${CONFIG_QEMU_ICOUNT_SHIFT},align=off,sleep=on
	  -rtc clock=vm)
  else()
    list(APPEND QEMU_FLAGS
	  -icount shift=${CONFIG_QEMU_ICOUNT_SHIFT},align=off,sleep=off
	  -rtc clock=vm)
  endif()
endif()

# Add a BT serial device when building for bluetooth, unless the
# application explicitly opts out with NO_QEMU_SERIAL_BT_SERVER.
if(CONFIG_BT)
  if(NOT CONFIG_BT_UART)
      set(NO_QEMU_SERIAL_BT_SERVER 1)
  endif()
  if(NOT NO_QEMU_SERIAL_BT_SERVER)
    list(APPEND QEMU_FLAGS -serial unix:/tmp/bt-server-bredr)
  endif()
endif()

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
    else()
      # QEMU_INSTANCE is a command line argument to *make* (not cmake). By
      # appending the instance name to the pid file we can easily run more
      # instances of the same sample.

      if(CONFIG_NET_QEMU_PPP)
	if(${CMAKE_GENERATOR} STREQUAL "Unix Makefiles")
	  set(ppp_path unix:/tmp/ppp\${QEMU_INSTANCE})
	else()
	  set(ppp_path unix:/tmp/ppp${QEMU_INSTANCE})
	endif()

	list(APPEND MORE_FLAGS_FOR_${target}
          -serial ${ppp_path}
          )
      else()
	if(${CMAKE_GENERATOR} STREQUAL "Unix Makefiles")
          set(tmp_file unix:/tmp/slip.sock\${QEMU_INSTANCE})
	else()
          set(tmp_file unix:/tmp/slip.sock${QEMU_INSTANCE})
	endif()

	list(APPEND MORE_FLAGS_FOR_${target}
          -serial ${tmp_file}
          )
      endif()

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
endif(QEMU_PIPE_STACK)

if(CONFIG_CAN AND NOT (CONFIG_NIOS2 OR CONFIG_SOC_LEON3))
  # Add CAN bus 0
  list(APPEND QEMU_FLAGS -object can-bus,id=canbus0)

  if(NOT "${CONFIG_CAN_QEMU_IFACE_NAME}" STREQUAL "")
    # Connect CAN bus 0 to host SocketCAN interface
    list(APPEND QEMU_FLAGS
      -object can-host-socketcan,id=canhost0,if=${CONFIG_CAN_QEMU_IFACE_NAME},canbus=canbus0)
  endif()

  if(CONFIG_CAN_KVASER_PCI)
    # Emulate a single-channel Kvaser PCIcan card connected to CAN bus 0
    list(APPEND QEMU_FLAGS -device kvaser_pci,canbus=canbus0)
  endif()
endif()

if(CONFIG_X86_64 AND NOT CONFIG_QEMU_UEFI_BOOT)
  # QEMU doesn't like 64-bit ELF files. Since we don't use any >4GB
  # addresses, converting it to 32-bit is safe enough for emulation.
  add_custom_target(qemu_image_target
    COMMAND
    ${CMAKE_OBJCOPY}
    -O elf32-i386
    $<TARGET_FILE:${logical_target_for_zephyr_elf}>
    ${ZEPHYR_BINARY_DIR}/zephyr-qemu.elf
    DEPENDS ${logical_target_for_zephyr_elf}
    )

  # Split the 'locore' and 'main' memory regions into separate executable
  # images and specify the 'locore' as the boot kernel, in order to prevent
  # the QEMU direct multiboot kernel loader from overwriting the BIOS and
  # option ROM areas located in between the two memory regions.
  # (for more details, refer to the issue zephyrproject-rtos/sdk-ng#168)
  add_custom_target(qemu_locore_image_target
    COMMAND
    ${CMAKE_OBJCOPY}
    -j .locore
    ${ZEPHYR_BINARY_DIR}/zephyr-qemu.elf
    ${ZEPHYR_BINARY_DIR}/zephyr-qemu-locore.elf
    2>&1 | grep -iv \"empty loadable segment detected\" || true
    DEPENDS qemu_image_target
    )

  add_custom_target(qemu_main_image_target
    COMMAND
    ${CMAKE_OBJCOPY}
    -R .locore
    ${ZEPHYR_BINARY_DIR}/zephyr-qemu.elf
    ${ZEPHYR_BINARY_DIR}/zephyr-qemu-main.elf
    2>&1 | grep -iv \"empty loadable segment detected\" || true
    DEPENDS qemu_image_target
    )

  add_custom_target(
    qemu_kernel_target
    DEPENDS qemu_locore_image_target qemu_main_image_target
    )

  set(QEMU_KERNEL_FILE "${ZEPHYR_BINARY_DIR}/zephyr-qemu-locore.elf")

  list(APPEND QEMU_EXTRA_FLAGS
    "-device;loader,file=${ZEPHYR_BINARY_DIR}/zephyr-qemu-main.elf"
    )
endif()

if(CONFIG_IVSHMEM)
  if(CONFIG_IVSHMEM_DOORBELL)
    list(APPEND QEMU_FLAGS
      -device ivshmem-doorbell,vectors=${CONFIG_IVSHMEM_MSI_X_VECTORS},chardev=ivshmem
      -chardev socket,path=/tmp/ivshmem_socket,id=ivshmem
    )
  else()
    list(APPEND QEMU_FLAGS
      -device ivshmem-plain,memdev=hostmem
      -object memory-backend-file,size=${CONFIG_QEMU_IVSHMEM_PLAIN_MEM_SIZE}M,share,mem-path=/dev/shm/ivshmem,id=hostmem
    )
  endif()
endif()

if(CONFIG_NVME)
  if(qemu_alternate_path)
    find_program(
      QEMU_IMG
      PATHS ${qemu_alternate_path}
      NO_DEFAULT_PATH
      NAMES qemu-img
    )
  else()
    find_program(
      QEMU_IMG
      qemu-img
    )
  endif()

  list(APPEND QEMU_EXTRA_FLAGS
    -drive file=${ZEPHYR_BINARY_DIR}/nvme_disk.img,if=none,id=nvm1
    -device nvme,serial=deadbeef,drive=nvm1
  )

  add_custom_target(qemu_nvme_disk
    COMMAND
    ${QEMU_IMG}
    create
    ${ZEPHYR_BINARY_DIR}/nvme_disk.img
    1M
  )
else()
  add_custom_target(qemu_nvme_disk)
endif()

if(CONFIG_FLASH_SIMULATOR_PROVISION)
  list(APPEND QEMU_EXTRA_FLAGS
    -device loader,file=${CMAKE_CURRENT_BINARY_DIR}/soc-nv-flash-image.hex
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
list(APPEND QEMU_EXTRA_FLAGS ${env_qemu})

# Also append QEMU flags from config
if(NOT CONFIG_QEMU_EXTRA_FLAGS STREQUAL "")
  set(config_qemu_flags ${CONFIG_QEMU_EXTRA_FLAGS})
  separate_arguments(config_qemu_flags)
  list(APPEND QEMU_EXTRA_FLAGS "${config_qemu_flags}")
endif()

list(APPEND MORE_FLAGS_FOR_debugserver_qemu -S)

if(NOT CONFIG_QEMU_GDBSERVER_LISTEN_DEV STREQUAL "")
  list(APPEND MORE_FLAGS_FOR_debugserver_qemu -gdb "${CONFIG_QEMU_GDBSERVER_LISTEN_DEV}")
endif()

# Architectures can define QEMU_KERNEL_FILE to use a specific output
# file to pass to qemu (and a "qemu_kernel_target" target to generate
# it), or set QEMU_KERNEL_OPTION if they want to replace the "-kernel
# ..." option entirely.
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

foreach(target ${qemu_targets})
  add_custom_target(${target}
    ${PRE_QEMU_COMMANDS}
    ${PRE_QEMU_COMMANDS_FOR_${target}}
    COMMAND
    ${QEMU}
    ${QEMU_UEFI_OPTION}
    ${QEMU_FLAGS_${ARCH}}
    ${QEMU_FLAGS}
    ${QEMU_EXTRA_FLAGS}
    ${MORE_FLAGS_FOR_${target}}
    ${QEMU_SMP_FLAGS}
    ${QEMU_KERNEL_OPTION}
    ${POST_QEMU_COMMANDS_FOR_${target}}
    DEPENDS ${logical_target_for_zephyr_elf}
    WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
    COMMENT "${QEMU_PIPE_COMMENT}[QEMU] CPU: ${QEMU_CPU_TYPE_${ARCH}}"
    USES_TERMINAL
    )
  if(DEFINED QEMU_KERNEL_FILE)
    add_dependencies(${target} qemu_nvme_disk qemu_kernel_target)
  endif()
endforeach()
