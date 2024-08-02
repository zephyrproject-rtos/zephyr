.. _file_system_api:

File Systems
############

Zephyr RTOS Virtual Filesystem Switch (VFS) allows applications to mount multiple
file systems at different mount points (e.g., ``/fatfs`` and ``/lfs``). The
mount point data structure contains all the necessary information required
to instantiate, mount, and operate on a file system. The File system Switch
decouples the applications from directly accessing an individual file system's
specific API or internal functions by introducing file system registration
mechanisms.

In Zephyr, any file system implementation or library can be plugged into or
pulled out through a file system registration API.  Each file system
implementation must have a globally unique integer identifier; use
:c:enumerator:`FS_TYPE_EXTERNAL_BASE` to avoid clashes with in-tree identifiers.

.. code-block:: c

        int fs_register(int type, const struct fs_file_system_t *fs);

        int fs_unregister(int type, const struct fs_file_system_t *fs);

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

- ``FS_FATFS`` is the file system type like FATFS or LittleFS.
- ``FATFS_MNTP`` is the mount point where the file system will be mounted.
- ``fat_fs`` is the file system data which will be used by fs_mount() API.



Samples
*******

Samples for the VFS are mainly supplied in ``samples/subsys/fs``, although various examples of the
VFS usage are provided as important functionalities in samples for different subsystems.
Here is the list of samples worth looking at:

- ``samples/subsys/fs/fat_fs`` is an example of FAT file system usage with SDHC media;
- ``samples/subsys/shell/fs`` is an example of Shell fs subsystem, using internal flash partition
	formatted to LittleFS;
- ``samples/subsys/usb/mass/`` example of USB Mass Storage device that uses FAT FS driver with RAM
	or SPI connected FLASH, or LittleFS in flash, depending on the sample configuration.

API Reference
*************

.. doxygengroup:: file_system_api
