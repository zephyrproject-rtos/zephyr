# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_ZVFS_FDTABLE
  zvfs_fdtable.c
  zvfs_rw.c
  zvfs_file_locking.c
  zvfs_close.c
  zvfs_lseek.c
  zvfs_fstat.c
  zvfs_fsync.c
  zvfs_ftruncate.c
  zvfs_fcntl.c
  zvfs_ioctl.c
)
zephyr_library_sources_ifdef(CONFIG_ZVFS_DEFAULT_FILE_VMETHODS zvfs_file_vmethods.c)
zephyr_library_sources_ifdef(CONFIG_ZVFS_FS
  zvfs_cwd.c
  zvfs_dirent.c
  zvfs_exe.c
  zvfs_fs.c
  zvfs_futime.c
  zvfs_path.c
  zvfs_record_lock.c
)
zephyr_library_sources_ifdef(CONFIG_ZVFS_EVENTFD zvfs_eventfd.c)
zephyr_library_sources_ifdef(CONFIG_ZVFS_MMAP zvfs_mmap.c)
zephyr_library_sources_ifdef(CONFIG_ZVFS_SHM zvfs_shm.c)
zephyr_library_include_directories_ifdef(CONFIG_ZVFS_MMAP
  ${ZEPHYR_BASE}/kernel/include
  ${ARCH_DIR}/${ARCH}/include
)
zephyr_library_sources_ifdef(CONFIG_ZVFS_POLL zvfs_poll.c)
zephyr_library_sources_ifdef(CONFIG_ZVFS_SELECT zvfs_select.c)
