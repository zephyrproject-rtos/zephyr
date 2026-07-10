# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# Set up the console chardev and connect the serial port, the semihosting
# console and the QEMU monitor to it.

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
elseif(QEMU_SOCKET)
  # Serve serial console on a TCP/IP port.
  list(APPEND QEMU_FLAGS -chardev socket,id=con,mux=on,server=on,host=127.0.0.1,port=4321)
else()
  # Redirect console to stdio, used for manual debugging.
  list(APPEND QEMU_FLAGS -chardev stdio,id=con,mux=on)
endif()

# Connect main serial port to the console chardev.
if(CONFIG_DT_HAS_VIRTIO_CONSOLE_ENABLED)
  list(APPEND QEMU_FLAGS -serial none -device virtio-serial -device virtconsole,chardev=con)
else()
  list(APPEND QEMU_FLAGS -serial chardev:con)
endif()

# Connect semihosting console to the console chardev if configured.
if(CONFIG_SEMIHOST)
  list(APPEND QEMU_FLAGS
    -semihosting-config enable=on,target=auto,chardev=con
  )
endif()

# Connect monitor to the console chardev.
if(CONFIG_DT_HAS_VIRTIO_CONSOLE_ENABLED)
  list(APPEND QEMU_FLAGS -monitor none)
else()
  list(APPEND QEMU_FLAGS -mon chardev=con,mode=readline)
endif()
