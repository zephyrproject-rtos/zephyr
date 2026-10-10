.. _zvfs:

Zephyr Virtual File System (ZVFS)
#################################

ZVFS is the common waypoint for every file descriptor in Zephyr. A descriptor
is a slot in the file descriptor table that carries an I/O object and a
vtable; ``zvfs_read()``, ``zvfs_write()``, ``zvfs_close()``, ``zvfs_ioctl()``
and the other entry points in :file:`include/zephyr/sys/zvfs.h` dispatch
through that vtable, so a regular file, a socket, an eventfd or a pipe are
reached through the same calls. The POSIX and C library file interfaces sit on
top of these entry points.

Memory Mappings
***************

:kconfig:option:`CONFIG_ZVFS_MMAP` adds ``zvfs_mmap()``, ``zvfs_munmap()``,
``zvfs_msync()`` and ``zvfs_mprotect()``: file descriptors and anonymous memory
mapped into the address space, at the granularity of
:kconfig:option:`CONFIG_ZVFS_MMAP_PAGE_SIZE`.

Storage
   With an MMU a mapping owns pinned anonymous pages from :c:func:`k_mem_map`,
   with guard pages on both sides. Without one it owns a block of the kernel
   heap; under :kconfig:option:`CONFIG_USERSPACE` the block is a power of two
   aligned to its size, so that it can be a memory-protection region.

File contents
   A file-backed mapping is a copy: the pages are read from the file when the
   mapping is created and, for a ``ZVFS_MAP_SHARED`` mapping that has been
   writable, written back on ``zvfs_msync()``, on ``zvfs_munmap()`` and when
   the owner exits, clipped to the file's size at that time. ``ZVFS_MS_INVALIDATE``
   re-reads the pages. An object that owns its pages answers ``ZFD_IOCTL_MMAP``
   with their address and is mapped in place, so every mapping of it aliases
   the same memory; as they share the address, unmapping it releases them all.

Modified pages
   With :kconfig:option:`CONFIG_ZVFS_MMAP_DIRTY_TRACK`, on an MMU that offers
   write faults to the kernel, the pages of a shared writable file mapping
   start write-protected. The first write to a page records it as modified and
   makes it writable, and ``zvfs_msync()`` writes back only the modified pages
   before protecting them again. Without it the whole range is written back.

Protection
   A user thread reaches a mapping through a memory-domain partition on its
   pages, added to the caller's domain when a user thread creates the mapping
   and removed when it is unmapped. The kernel's view of the pages it owns
   follows the protection as well.

References and lifetime
   A mapping counts as a reference on its open file object alongside the
   descriptors, so the file stays open after the last descriptor is closed
   until the last mapping of it is released.

Ownership
   A mapping belongs to the thread that created it or, with
   :kconfig:option:`CONFIG_PROCESS`, to that thread's process, and is released
   when the owner exits: in the exiting process's own context, so a shared file
   mapping is written back by the time the process can be reaped, and otherwise
   by the next mapping operation. A process created with a copy of its parent's
   address space (:kconfig:option:`CONFIG_PROCESS_VM`) inherits the parent's
   mappings: shared mappings keep aliasing the same pages, private ones are
   copied along with the rest of the writable memory. A process started on a
   new image has no mappings. Mapping operations only reach the caller's own
   process's mappings.

``zvfs_munmap()`` of part of a mapping trims it or splits it into two pieces
that share the original's storage, which is released with the last piece.
A mapping shared with forked copies keeps its shape, as does a mapping granted
to user mode on a board without an MMU, whose protection region covers it whole:
a partial ``zvfs_munmap()`` of those fails with ``EINVAL``. ``zvfs_mprotect()``
operates on whole mappings; a range that covers part of one is rejected.

Memory Locking
**************

:kconfig:option:`CONFIG_ZVFS_MLOCK` adds ``zvfs_mlock()`` and ``zvfs_munlock()``.
A locked page is resident and exempt from eviction, a distinction that only
exists under :kconfig:option:`CONFIG_DEMAND_PAGING`; without it every page is
resident already and both calls succeed without effect. Called with no scope
flag they lock or unlock the pages of a range; from user mode a range the
caller cannot read is ``ENOMEM``.
``ZVFS_MLOCK_CURRENT`` is the caller's mappings and, from user mode, the
partitions of its memory domain; ``ZVFS_MLOCK_FUTURE`` records, per process,
that every mapping created from then on is locked, until an unlock with the
same flag clears it. The pages of a mapping are pinned for its lifetime, as
Storage describes, so an unlock passes over them.

Shared Memory Objects
*********************

:kconfig:option:`CONFIG_ZVFS_SHM` adds ``zvfs_shm_open()`` and ``zvfs_shm_unlink()``:
named, system-wide memory objects reached through file descriptors. An object
is created with ``ZVFS_O_CREAT`` (``ZVFS_O_EXCL`` insists on creating it), has
size zero until ``zvfs_ftruncate()`` sizes it, and is then read and written at
offsets like a file. It owns its pages, pinned :c:func:`k_mem_map` pages with an
MMU and a kernel-heap block otherwise, and answers ``ZFD_IOCTL_MMAP`` with
their address, so ``zvfs_mmap()`` maps it in place and every mapping of it
aliases the same memory.

An object is sized once: its pages may already be mapped, so a second
``zvfs_ftruncate()`` to another size and ``ZVFS_O_TRUNC`` on a sized object
fail with ``EBUSY``. The name is only a lookup key: ``zvfs_shm_unlink()``
removes it at once, a later ``zvfs_shm_open()`` with ``ZVFS_O_CREAT`` makes a
new object under the same name, and the unlinked object is freed when its last
descriptor and its last mapping are gone. Objects come from a pool of
:kconfig:option:`CONFIG_ZVFS_SHM_MAX`, with names shorter than
:kconfig:option:`CONFIG_ZVFS_SHM_NAME_MAX`.
