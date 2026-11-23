# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# Inter-VM shared memory device, backed either by a doorbell server socket or
# by a plain shared memory file.

if(CONFIG_IVSHMEM)
  if(CONFIG_IVSHMEM_DOORBELL)
    qemu_append_flags(
      -device ivshmem-doorbell,vectors=${CONFIG_IVSHMEM_MSI_X_VECTORS},chardev=ivshmem
      -chardev socket,path=/tmp/ivshmem_socket,id=ivshmem
    )
  else()
    qemu_append_flags(
      -device ivshmem-plain,memdev=hostmem
      -object memory-backend-file,size=${CONFIG_QEMU_IVSHMEM_PLAIN_MEM_SIZE}M,share,mem-path=/dev/shm/ivshmem,id=hostmem
    )
  endif()
endif()
