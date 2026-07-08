# SPDX-License-Identifier: Apache-2.0

if(NOT DEFINED QEMU_IMG OR QEMU_IMG STREQUAL "" OR QEMU_IMG MATCHES "-NOTFOUND$")
  message(FATAL_ERROR "qemu-img not found")
endif()

if(NOT DEFINED DISK_FILE OR DISK_FILE STREQUAL "")
  message(FATAL_ERROR "DISK_FILE is not set")
endif()

if(NOT DEFINED DISK_SIZE OR DISK_SIZE STREQUAL "")
  message(FATAL_ERROR "DISK_SIZE is not set")
endif()

set(stamp_file "${DISK_FILE}.size")
set(create_disk TRUE)

if(EXISTS "${DISK_FILE}")
  set(create_disk FALSE)
  if(EXISTS "${stamp_file}")
    file(READ "${stamp_file}" old_size)
    string(STRIP "${old_size}" old_size)
    if(NOT old_size STREQUAL DISK_SIZE)
      set(create_disk TRUE)
    endif()
  endif()
endif()

if(create_disk)
  execute_process(
    COMMAND
    ${QEMU_IMG}
    create
    -f raw
    ${DISK_FILE}
    ${DISK_SIZE}
    RESULT_VARIABLE ret
  )
  if(ret)
    message(FATAL_ERROR "failed to create QEMU virtio-blk disk image")
  endif()
endif()

file(WRITE "${stamp_file}" "${DISK_SIZE}\n")
