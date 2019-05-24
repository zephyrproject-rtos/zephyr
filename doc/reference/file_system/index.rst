.. _file_system:

File Systems
############

Zephyr RTOS Virtual Filesystem Switch (VFS) allows applications to mount multiple
file systems at different mount points (e.g., ``/fatfs`` and ``/nffs``). The
mount point data structure contains all the necessary information required
to instantiate, mount, and operate on a file system. The File system Switch
decouples the applications from directly accessing an individual file system's
specific API or internal functions by introducing file system registration
mechanisms.

In Zephyr, any file system implementation or library can be plugged into or
pulled out through a file system registration API.

.. code-block:: c

        int fs_register(enum fs_type type, struct fs_file_system_t *fs);

        int fs_unregister(enum fs_type type, struct fs_file_system_t *fs);

Zephyr RTOS supports multiple instances of a file system by making use of
the mount point as the disk volume name, which is used by the file system library
while formatting or mounting a disk.

A file system is declared as:

.. code-block:: c

	static struct fs_mount_t mp = {
	.type = FS_FATFS,
	.mnt_point = FATFS_MNTP,
	.fs_data = &fat_fs,
	};

where

- ``FS_FATFS`` is the file system type like FATFS or NFFS.
- ``FATFS_MNTP`` is the mount point where the file system will be mounted.
- ``fat_fs`` is the file system data which will be used by fs_mount() API.

Known Limitations
*****************

NFFS supports only one instance of file system due to the library's internal
implementation limitation.


Sample
******

A sample of how the file system can be used is supplied in ``samples/subsys/fs``.

API Reference
*************

.. doxygengroup:: file_system_api
   :project: Zephyr
