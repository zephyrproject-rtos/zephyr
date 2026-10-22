# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# Set up the console chardev and connect the serial port, the semihosting
# console and the QEMU monitor to it.

# If running with sysbuild, we need to ensure this variable is populated
zephyr_get(QEMU_PIPE)
# Set up chardev for console.
if(QEMU_PTY)
  # Redirect console to a pseudo-tty, used for running automated tests.
  qemu_append_flags(-chardev pty,id=con,mux=on)
elseif(QEMU_PIPE)
  # Redirect console to a pipe, used for running automated tests.
  qemu_append_flags(-chardev pipe,id=con,mux=on,path=${QEMU_PIPE})
  # Create the pipe file before passing the path to QEMU.
  qemu_get_run_targets(qemu_targets)
  foreach(target ${qemu_targets})
    qemu_add_pre_commands(${target} COMMAND ${CMAKE_COMMAND} -E touch ${QEMU_PIPE})
  endforeach()
elseif(QEMU_SOCKET)
  # Serve serial console on a TCP/IP port.
  qemu_append_flags(-chardev socket,id=con,mux=on,server=on,host=127.0.0.1,port=4321)
else()
  # Redirect console to stdio, used for manual debugging.
  qemu_append_flags(-chardev stdio,id=con,mux=on)
endif()

# Connect main serial port to the console chardev.
if(CONFIG_DT_HAS_VIRTIO_CONSOLE_ENABLED)
  qemu_append_flags(-serial none -device virtio-serial -device virtconsole,chardev=con)
else()
  qemu_append_flags(-serial chardev:con)
endif()

# Connect semihosting console to the console chardev if configured.
if(CONFIG_SEMIHOST)
  qemu_append_flags(
    -semihosting-config enable=on,target=auto,chardev=con
  )
endif()

# Connect monitor to the console chardev.
if(CONFIG_DT_HAS_VIRTIO_CONSOLE_ENABLED)
  qemu_append_flags(-monitor none)
else()
  qemu_append_flags(-mon chardev=con,mode=readline)
endif()
