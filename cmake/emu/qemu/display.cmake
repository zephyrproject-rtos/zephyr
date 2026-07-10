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
    list(APPEND QEMU_FLAGS -device ramfb -vga none -display ${QEMU_DISPLAY_BACKEND})
  else()
    list(APPEND QEMU_FLAGS -display ${QEMU_DISPLAY_BACKEND})
  endif()
else()
  list(APPEND QEMU_FLAGS -nographic)
endif()
