.. _disk_partition_layout:

Partition layout scan (MBR/GPT)
###############################

:c:func:`disk_partition_walk` in :file:`subsys/disk/disk_partition.c` parses
MBR (including extended and logical partitions) and GPT layouts. The caller
provides a read callback for sectors; the walk reports
:c:struct:`disk_partition_entry` records (start LBA, length, and type). The
parser does not choose which slice to mount or detect filesystem formats.
Entries with a zero sector count are omitted from the result table.

Enable :kconfig:option:`CONFIG_DISK_PARTITION`. Logical block size must be
512–4096 bytes (:c:macro:`DISK_PARTITION_MAX_BLOCK_SIZE`).

:c:func:`disk_partition_set_whole_disk` can fill a single synthetic entry
spanning the device when :c:enum:`DISK_PARTITION_SCHEME_NONE` indicates
partitionless media.

Callers above this layer select a slice and register a block device or mount a
filesystem. Tests live in :zephyr_file:`tests/subsys/disk/partition`.
