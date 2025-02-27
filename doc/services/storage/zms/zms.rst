.. _zms_api:

Zephyr Memory Storage (ZMS)
###########################
Zephyr Memory Storage is a new key-value storage system that is designed to work with all types
of non-volatile storage technologies. It supports classical on-chip NOR flash as well as new
technologies like RRAM and MRAM that do not require a separate erase operation at all, that is,
data on these types of devices can be overwritten directly at any time.

General behavior
****************
ZMS divides the memory space into sectors (minimum 2), and each sector is filled with key-value
pairs until it is full.

The key-value pair is divided into two parts:

- The key part is written in an ATE (Allocation Table Entry) called "ID-ATE" which is stored
  starting from the bottom of the sector.
- The value part is defined as "data" and is stored raw starting from the top of the sector.

Additionally, for each sector we store at the last positions header ATEs which are ATEs that
are needed for the sector to describe its status (closed, open) and the current version of ZMS.

When the current sector is full we verify first that the following sector is empty, we garbage
collect the sector N+2 (where N is the current sector number) by moving the valid ATEs to the
N+1 empty sector, we erase the garbage-collected sector and then we close the current sector by
writing a garbage_collect_done ATE and the close ATE (one of the header entries).
Afterwards we move forward to the next sector and start writing entries again.

This behavior is repeated until it reaches the end of the partition. Then it starts again from
the first sector after garbage collecting it and erasing its content.

Composition of a sector
=======================
A sector is organized in this form (example with 3 sectors):

.. list-table::
   :widths: 25 25 25
   :header-rows: 1

   * - Sector 0 (closed)
     - Sector 1 (open)
     - Sector 2 (empty)
   * - Data_a0
     - Data_b0
     - Data_c0
   * - Data_a1
     - Data_b1
     - Data_c1
   * - Data_a2
     - Data_b2
     - Data_c2
   * - GC_done
     -    .
     -    .
   * -    .
     -    .
     -    .
   * -    .
     -    .
     -    .
   * -    .
     - ID ATE_b2
     - ID ATE_c2
   * - ID ATE_a2
     - ID ATE_b1
     - ID ATE_c1
   * - ID ATE_a1
     - ID ATE_b0
     - ID ATE_c0
   * - ID ATE_a0
     - GC_done ATE
     - GC_done ATE
   * - Close ATE (cyc=1)
     - Close ATE (cyc=1)
     - Close ATE (cyc=1)
   * - Empty ATE (cyc=1)
     - Empty ATE (cyc=2)
     - Empty ATE (cyc=2)

Definition of each element in the sector
========================================

``Empty ATE`` is written when erasing a sector (last position of the sector).

``Close ATE`` is written when closing a sector (second to last position of the sector).

``GC_done ATE`` is written to indicate that the next sector has already been garbage-collected.
This ATE could be at any position of the sector.

``ID ATE`` are entries that contain a 32-bit key and describe where the data is stored, its
size and its CRC32.

``Data`` is the actual value associated to the ID-ATE.

How does ZMS work?
******************

Mounting the storage system
===========================

Mounting the storage system starts by getting the flash parameters, checking that the file system
properties are correct (sector_size, sector_count ...) then calling the zms_init function to
make the storage ready.

To mount the filesystem the following elements in the ``zms_fs`` structure must be initialized:

.. code-block:: c

	struct zms_fs {
		/** File system offset in flash **/
		off_t offset;

		/** Storage system is split into sectors, each sector size must be multiple of
		 * erase-blocks if the device has erase capabilities
		 */
		uint32_t sector_size;
		/** Number of sectors in the file system */
		uint32_t sector_count;

		/** Flash device runtime structure */
		const struct device *flash_device;
	};

Initialization
==============

As ZMS has a fast-forward write mechanism, it must find the last sector and the last pointer of
the entry where it stopped the last time.
It must look for a closed sector followed by an open one, then within the open sector, it finds
(recovers) the last written ATE.
After that, it checks that the sector after this one is empty, or it will erase it.

ZMS ID/data write
===================

To avoid rewriting the same data with the same ID again, ZMS must look in all the sectors if the
same ID exists and then compares its data. If the data is identical, no write is performed.
If it must perform a write, then an ATE and the data (if the operation is not a delete) are written
in the sector.
If the sector is full (cannot hold the current data + ATE), ZMS has to move to the next sector,
garbage collect the sector after the newly opened one then erase it.
Data whose size is smaller or equal to 8 bytes are written within the ATE.

ZMS ID/data read (with history)
===============================

By default ZMS looks for the last data with the same ID by browsing through all stored ATEs from
the most recent ones to the oldest ones. If it finds a valid ATE with a matching ID it retrieves
its data and returns the number of bytes that were read.
If a history count is provided and different than 0, older data with same ID is retrieved.

ZMS free space calculation
==========================

ZMS can also return the free space remaining in the partition.
However, this operation is very time-consuming as it needs to browse through all valid ATEs
in all sectors of the partition and for each valid ATE try to find if an older one exists.
It is not recommended for applications to use this function often, as it is time-consuming and
could slow down the calling thread.

The cycle counter
=================

Each sector has a lead cycle counter which is a ``uin8_t`` that is used to validate all the other
ATEs.
The lead cycle counter is stored in the empty ATE.
To become valid, an ATE must have the same cycle counter as the one stored in the empty ATE.
Each time an ATE is moved from a sector to another it must get the cycle counter of the
destination sector.
To erase a sector, the cycle counter of the empty ATE is incremented and a single write of the
empty ATE is done.
All the ATEs in that sector become invalid.

Closing sectors
===============

To close a sector a close ATE is added at the end of the sector and it must have the same cycle
counter as the empty ATE.
When closing a sector, all the remaining space that has not been used is filled with garbage data
to avoid having old ATEs with a valid cycle counter.

Triggering garbage collection
=============================

Some applications need to make sure that storage writes have a maximum defined latency.
When calling ZMS to make a write, the current sector could be almost full such that ZMS needs to
trigger the GC to switch to the next sector.
This operation is time-consuming and will cause some applications to not meet their real time
constraints.
ZMS adds an API for the application to get the current remaining free space in a sector.
The application could then decide when to switch to the next sector if the current one is almost
full. This will of course trigger the garbage collection operation on the next sector.
This will guarantee the application that the next write won't trigger the garbage collection.

ATE (Allocation Table Entry) structure
======================================

An entry has 16 bytes divided between these fields:

See the :c:struct:`zms_ate` structure.

.. note:: The CRC of the data is checked only when a full read of the data is made.
   The CRC of the data is not checked for a partial read, as it is computed for the whole element.

.. warning:: Enabling the CRC feature on previously existing ZMS content that did not have it
   enabled will make all existing data invalid.

Available space for user data (key-value pairs)
***********************************************

ZMS always needs an empty sector to be able to perform the garbage collection (GC).
So, if we suppose that 4 sectors exist in a partition, ZMS will only use 3 sectors to store
key-value pairs and keep one sector empty to be able to perform GC.
The empty sector will rotate between the 4 sectors in the partition.

.. note:: The maximum single data length that can be written at once in a sector is 64K
   (this could change in future versions of ZMS).

Small data values
=================

Values smaller than or equal to 8 bytes will be stored within the entry (ATE) itself, without
writing data at the top of the sector.
ZMS has an entry size of 16 bytes which means that the maximum available space in a partition to
store data is computed in this scenario as:

.. math::

   \small\frac{(NUM\_SECTORS - 1) \times (SECTOR\_SIZE - (5 \times ATE\_SIZE)) \times (DATA\_SIZE)}{ATE\_SIZE}

Where:

``NUM_SECTOR``: Total number of sectors

``SECTOR_SIZE``: Size of the sector

``ATE_SIZE``: 16 bytes

``(5 * ATE_SIZE)``: Reserved ATEs for header and delete items

``DATA_SIZE``: Size of the small data values (range from 1 to 8)

For example for 4 sectors of 1024 bytes, free space for 8-byte length data is :math:`\frac{3 \times 944 \times 8}{16} = 1416 \, \text{ bytes}`.

Large data values
=================

Large data values ( > 8 bytes) are stored separately at the top of the sector.
In this case, it is hard to estimate the free available space, as this depends on the size of
the data. But we can take into account that for N bytes of data (N > 8 bytes) an additional
16 bytes of ATE must be added at the bottom of the sector.

Let's take an example:

For a partition that has 4 sectors of 1024 bytes and for data size of 64 bytes.
Only 3 sectors are available for writes with a capacity of 944 bytes each.
Each key-value pair needs an extra 16 bytes for the ATE, which makes it possible to store 11 pairs
in each sector (:math:`\frac{944}{80}`).
Total data that could be stored in this partition for this case is :math:`11 \times 3 \times 64 = 2112 \text{ bytes}`.

Wear leveling
*************

This storage system is optimized for devices that do not require an erase.
Storage systems that rely on an erase value (NVS as an example) need to emulate the erase with
write operations. This causes a significant decrease in the life expectancy of these devices
as well as more delays for write operations and initialization of the device when it is empty.
ZMS uses a cycle count mechanism that avoids emulating erase operations for these devices.
It also guarantees that every memory location is written only once for each cycle of sector write.

As an example, to erase a 4096-byte sector on devices that do not require an erase operation
using NVS, 256 flash writes must be performed (supposing that ``write-block-size`` = 16 bytes), while
using ZMS, only 1 write of 16 bytes is needed. This operation is 256 times faster in this case.

The garbage collection operation also reduces the memory cell life expectancy as it performs write
operations when moving blocks from one sector to another.
To make the garbage collector not affect the life expectancy of the device it is recommended
to dimension the partition appropriately. Its size should be the double of the maximum size of
data (including headers) that could be written in the storage.

See `Available space for user data <#available-space-for-user-data-key-value-pairs>`_.

Device lifetime calculation
===========================

Storage devices, whether they are classical flash or new technologies like RRAM/MRAM, have a
limited life expectancy which is determined by the number of times memory cells can be
erased/written.
Flash devices are erased one page at a time as part of their functional behavior (otherwise
memory cells cannot be overwritten), and for storage devices that do not require an erase
operation, memory cells can be overwritten directly.

A typical scenario is shown here to calculate the life expectancy of a device:
Let's suppose that we store an 8-byte variable using the same ID but its content changes every
minute. The partition has 4 sectors with 1024 bytes each.
Each write of the variable requires 16 bytes of storage.
As we have 944 bytes available for ATEs for each sector, and because ZMS is a fast-forward
storage system, we are going to rewrite the first location of the first sector after
:math:`\frac{(944 \times 4)}{16} = 236 \text{ minutes}`.

In addition to the normal writes, the garbage collector will move the data that is still valid
from old sectors to new ones.
As we are using the same ID and a big partition size, no data will be moved by the garbage
collector in this case.
For storage devices that can be written 20 000 times, the storage will last about
4 720 000 minutes (~9 years).

To make a more general formula we must first compute the effective used size in ZMS by our
typical set of data.
For ID/data pairs with data <= 8 bytes, ``effective_size`` is 16 bytes.
For ID/data pairs with data > 8 bytes, ``effective_size`` is ``16 + sizeof(data)`` bytes.
Let's suppose that ``total_effective_size`` is the total size of the data that is written in
the storage and that the partition is sized appropriately (double of the effective size) to avoid
having the garbage collector moving blocks all the time.

The expected lifetime of the device in minutes is computed as:

.. math::

   \small\frac{(SECTOR\_EFFECTIVE\_SIZE \times SECTOR\_NUMBER \times MAX\_NUM\_WRITES)}{(TOTAL\_EFFECTIVE\_SIZE \times WR\_MIN)}

Where:

``SECTOR_EFFECTIVE_SIZE``: The sector size - header size (80 bytes)

``SECTOR_NUMBER``: The number of sectors

``MAX_NUM_WRITES``: The life expectancy of the storage device in number of writes

``TOTAL_EFFECTIVE_SIZE``: Total effective size of the set of written data

``WR_MIN``: Number of writes of the set of data per minute

Features
********
ZMS has introduced many features compared to existing storage system like NVS and will evolve
from its initial version to include more features that satisfies new technologies requirements
such as low latency and bigger storage space.

Existing features
=================
Version 1
---------
- Supports storage devices that do not require an erase operation (only one write operation
  to invalidate a sector)
- Supports large partition and sector sizes (64-bit address space)
- Supports 32-bit IDs
- Small-sized data (<= 8 bytes) are stored in the ATE itself
- Built-in data CRC32 (included in the ATE)
- Versioning of ZMS (to handle future evolutions)
- Supports large ``write-block-size`` (only for platforms that need it)

Future features
===============

- Add multiple format ATE support to be able to use ZMS with different ATE formats that satisfies
  requirements from application
- Add the possibility to skip garbage collector for some application usage where ID/value pairs
  are written periodically and do not exceed half of the partition size (there is always an old
  entry with the same ID).
- Divide IDs into namespaces and allocate IDs on demand from application to handle collisions
  between IDs used by different subsystems or samples.
- Add the possibility to retrieve the wear out value of the device based on the cycle count value
- Add a recovery function that can recover a storage partition if something went wrong
- Add a library/application to allow migration from NVS entries to ZMS entries
- Add the possibility to force formatting the storage partition to the ZMS format if something
  went wrong when mounting the storage.

ZMS and other storage systems in Zephyr
=======================================
This section describes ZMS in the wider context of storage systems in Zephyr (not full filesystems,
but simpler, non-hierarchical ones).
Today Zephyr includes at least two other systems that are somewhat comparable in scope and
functionality: :ref:`NVS <nvs_api>` and :ref:`FCB <fcb_api>`.
Which one to use in your application will depend on your needs and the hardware you are using,
and this section provides information to help make a choice.

- If you are using devices that do not require an erase operation like RRAM or MRAM, :ref:`ZMS <zms_api>` is definitely the
  best fit for your storage subsystem as it is designed to avoid emulating erase operation using
  large block writes for these devices and replaces it with a single write call.
- For devices that have a large ``write_block_size`` and/or need a sector size that is different than the
  classical flash page size (equal to erase_block_size), :ref:`ZMS <zms_api>` is also the best fit as there is
  the possibility to customize these parameters and add the support of these devices in ZMS.
- For classical flash technology devices, :ref:`NVS <nvs_api>` is recommended as it has low footprint (smaller
  ATEs and smaller header ATEs). Erasing flash in NVS is also very fast and do not require an
  additional write operation compared to ZMS.
  For these devices, NVS reads/writes will be faster as well than ZMS as it has smaller ATE size.
- If your application needs more than 64K IDs for storage, :ref:`ZMS <zms_api>` is recommended here as it
  has a 32-bit ID field.
- If your application is working in a FIFO mode (First-in First-out) then :ref:`FCB <fcb_api>` is
  the best storage solution for this use case.

More generally to make the right choice between NVS and ZMS, all the blockers should be first
verified to make sure that the application could work with one subsystem or the other, then if
both solutions could be implemented, the best choice should be based on the calculations of the
life expectancy of the device described in this section: `Wear leveling <#wear-leveling>`_.

Recommendations to increase performance
***************************************

Sector size and count
=====================

- The total size of the storage partition should be set appropriately to achieve the best
  performance with ZMS.
  All the information regarding the effectively available free space in ZMS can be found
  in the documentation. See `Available space for user data <#available-space-for-user-data-key-value-pairs>`_.
  It's recommended to choose a storage partition size that is double the size of the key-value pairs
  that will be written in the storage.
- The sector size needs to be set such that a sector can fit the maximum data size that will be
  stored.
  Increasing the sector size will slow down the garbage collection operation and make it occur
  less frequently.
  Decreasing its size, on the opposite, will make the garbage collection operation faster but also
  occur more frequently.
- For some subsystems like :ref:`Settings <settings_api>`, all path-value pairs are split into two ZMS entries (ATEs).
  The headers needed by the two entries should be accounted for when computing the needed storage
  space.
- Storing small data (<= 8 bytes) in ZMS entries can increase the performance, as this data is
  written within the entry.
  For example, for the :ref:`Settings <settings_api>` subsystem, choosing a path name that is
  less than or equal to 8 bytes can make reads and writes faster.

Cache size
==========

- When using the ZMS API directly, the recommendation for the cache size is to make it at least
  equal to the number of different entries that will be written in the storage.
- Each additional cache entry will add 8 bytes to your RAM usage. Cache size should be carefully
  chosen.
- If you use ZMS through :ref:`Settings <settings_api>`, you have to take into account that each Settings entry is
  divided into two ZMS entries. The recommendation for the cache size is to make it at least
  twice the number of Settings entries.

API Reference
*************

The ZMS API is provided by ``zms.h``:

.. doxygengroup:: zms_data_structures

.. doxygengroup:: zms_high_level_api

.. comment
   not documenting .. doxygengroup:: zms
