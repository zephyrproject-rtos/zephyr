# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# Select the host display backend, or run headless.

if(CONFIG_DISPLAY)
  if(CMAKE_HOST_APPLE)
    set(QEMU_DISPLAY_BACKEND cocoa)
  else()
    set(QEMU_DISPLAY_BACKEND sdl)
  endif()
  if(CONFIG_INPUT_VIRTIO)
    # An absolute pointing device makes QEMU hide the host cursor over the
    # guest display, but the guest does not draw one
    string(APPEND QEMU_DISPLAY_BACKEND ",show-cursor=on")
  endif()
  if(CONFIG_QEMU_RAMFB_DISPLAY)
    qemu_append_flags(-device ramfb -vga none -display ${QEMU_DISPLAY_BACKEND})
  else()
    qemu_append_flags(-display ${QEMU_DISPLAY_BACKEND})
  endif()
else()
  qemu_append_flags(-nographic)
endif()
