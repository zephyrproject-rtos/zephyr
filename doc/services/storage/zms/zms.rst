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
  starting from the bottom of the sector
- The value part is defined as "DATA" and is stored raw starting from the top of the sector

Additionally, for each sector we store at the last positions Header-ATEs which are ATEs that
are needed for the sector to describe its status (closed, open) and the current version of ZMS.

When the current sector is full we verify first that the following sector is empty, we garbage
collect the N+2 sector (where N is the current sector number) by moving the valid ATEs to the
N+1 empty sector, we erase the garbage collected sector and then we close the current sector by
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
     - ATE_b2
     - ATE_c2
   * - ATE_a2
     - ATE_b1
     - ATE_c1
   * - ATE_a1
     - ATE_b0
     - ATE_c0
   * - ATE_a0
     - GC_done
     - GC_done
   * - Close (cyc=1)
     - Close (cyc=1)
     - Close (cyc=1)
   * - Empty (cyc=1)
     - Empty (cyc=2)
     - Empty (cyc=2)

Definition of each element in the sector
========================================

``Empty ATE:`` is written when erasing a sector (last position of the sector).

``Close ATE:`` is written when closing a sector (second to last position of the sector).

``GC_done ATE:`` is written to indicate that the next sector has been already garbage
collected. This ATE could be in any position of the sector.

``ID-ATE:`` are entries that contain a 32 bits Key and describe where the data is stored, its
size and its crc32

``Data:`` is the actual value associated to the ID-ATE

How does ZMS work?
******************

Mounting the Storage system
===========================

Mounting the storage starts by getting the flash parameters, checking that the file system
properties are correct (sector_size, sector_count ...) then calling the zms_init function to
make the storage ready.

To mount the filesystem some elements in the zms_fs structure must be initialized.

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

As ZMS has a fast-forward write mechanism, we must find the last sector and the last pointer of
the entry where it stopped the last time.
It must look for a closed sector followed by an open one, then within the open sector, it finds
(recover) the last written ATE (Allocation Table Entry).
After that, it checks that the sector after this one is empty, or it will erase it.

ZMS ID-Data write
===================

To avoid rewriting the same data with the same ID again, it must look in all the sectors if the
same ID exist then compares its data, if the data is identical no write is performed.
If we must perform a write, then an ATE and Data (if not a delete) are written in the sector.
If the sector is full (cannot hold the current data + ATE) we have to move to the next sector,
garbage collect the sector after the newly opened one then erase it.
Data size that is smaller or equal to 8 bytes are written within the ATE.

ZMS ID/data read (with history)
===============================

By default it looks for the last data with the same ID by browsing through all stored ATEs from
the most recent ones to the oldest ones. If it finds a valid ATE with a matching ID it retrieves
its data and returns the number of bytes that were read.
If history count is provided that is different than 0, older data with same ID is retrieved.

ZMS free space calculation
==========================

ZMS can also return the free space remaining in the partition.
However, this operation is very time consuming and needs to browse all valid ATEs in all sectors
of the partition and for each valid ATE try to find if an older one exist.
It is not recommended for application to use this function often, as it is time consuming and
could slow down the calling thread.

The cycle counter
=================

Each sector has a lead cycle counter which is a uin8_t that is used to validate all the other
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

Triggering Garbage collection
=============================

Some applications need to make sure that storage writes have a maximum defined latency.
When calling a ZMS write, the current sector could be almost full and we need to trigger the GC
to switch to the next sector.
This operation is time consuming and it will cause some applications to not meet their real time
constraints.
ZMS adds an API for the application to get the current remaining free space in a sector.
The application could then decide when needed to switch to the next sector if the current one is
almost full and of course it will trigger the garbage collection on the next sector.
This will guarantee the application that the next write won't trigger the garbage collection.

ATE (Allocation Table Entry) structure
======================================

An entry has 16 bytes divided between these variables :

.. code-block:: c

	struct zms_ate {
		uint8_t crc8;      /* crc8 check of the entry */
		uint8_t cycle_cnt; /* cycle counter for non erasable devices */
		uint32_t id;       /* data id */
		uint16_t len;      /* data len within sector */
		union {
			uint8_t data[8]; /* used to store small size data */
			struct {
				uint32_t offset; /* data offset within sector */
				union {
					uint32_t data_crc; /* crc for data */
					uint32_t metadata; /* Used to store metadata information
							    * such as storage version.
							    */
				};
			};
		};
	} __packed;

.. note:: The data CRC is checked only when the whole data of the element is read.
   The data CRC is not checked for a partial read, as it is computed for the complete set of data.

.. note:: Enabling the data CRC feature on a previously existing ZMS content without
   data CRC will make all existing data invalid.

.. _free-space:

Available space for user data (key-value pairs)
***********************************************

For both scenarios ZMS should have always an empty sector to be able to perform the garbage
collection.
So if we suppose that 4 sectors exist in a partition, ZMS will only use 3 sectors to store
Key-value pairs and keep always one (rotating sector) empty to be able to launch GC.

.. note:: The maximum single data length that could be written at once in a sector is 64K
   (This could change in future versions of ZMS)

Small data values
=================

For small data values (<= 8 bytes), the data is stored within the entry (ATE) itself and no data
is written at the top of the sector.
ZMS has an entry size of 16 bytes which means that the maximum available space in a partition to
store data is computed in this scenario as :

.. math::

   \small\frac{(NUM\_SECTORS - 1) \times (SECTOR\_SIZE - (5 \times ATE\_SIZE))}{2}

Where:

``NUM_SECTOR:`` Total number of sectors

``SECTOR_SIZE:`` Size of the sector

``ATE_SIZE:`` 16 bytes

``(5 * ATE_SIZE):`` Reserved ATEs for header and delete items

For example for 4 sectors of 1024 bytes, free space for data is :math:`\frac{3 \times 944}{2} = 1416 \, \text{ bytes}`.

Large data values
=================

Large data values ( > 8 bytes) are stored separately at the top of the sector.
In this case it is hard to estimate the free available space as this depends on the size of
the data. But we can take into account that for N bytes of data (N > 8 bytes) an additional
16 bytes of ATE must be added at the bottom of the sector.

Let's take an example:

For a partition that has 4 sectors of 1024 bytes and for data size of 64 bytes.
Only 3 sectors are available for writes with a capacity of 944 bytes each.
Each Key-value pair needs an extra 16 bytes for ATE which makes it possible to store 11 pairs
in each sectors (:math:`\frac{944}{80}`).
Total data that could be stored in this partition for this case is :math:`11 \times 3 \times 64 = 2112 \text{ bytes}`

.. _wear-leveling:

Wear leveling
*************

This storage system is optimized for devices that do not require an erase.
Using storage systems that rely on an erase-value (NVS as an example) will need to emulate the
erase with write operations. This will cause a significant decrease in the life expectancy of
these devices and will cause more delays for write operations and for initialization.
ZMS introduces a cycle count mechanism that avoids emulating erase operation for these devices.
It also guarantees that every memory location is written only once for each cycle of sector write.

As an example, to erase a 4096 bytes sector on a non erasable device using NVS, 256 flash writes
must be performed (supposing that write-block-size=16 bytes), while using ZMS only 1 write of
16 bytes is needed. This operation is 256 times faster in this case.

Garbage collection operation is also adding some writes to the memory cell life expectancy as it
is moving some blocks from one sector to another.
To make the garbage collector not affect the life expectancy of the device it is recommended
to dimension correctly the partition size. Its size should be the double of the maximum size of
data (including extra headers) that could be written in the storage.

See :ref:`free-space`.

Device lifetime calculation
===========================

Storage devices whether they are classical Flash or new technologies like RRAM/MRAM has a limited
life expectancy which is determined by the number of times memory cells can be erased/written.
Flash devices are erased one page at a time as part of their functional behavior (otherwise
memory cells cannot be overwritten) and for non erasable storage devices memory cells can be
overwritten directly.

A typical scenario is shown here to calculate the life expectancy of a device.
Let's suppose that we store an 8 bytes variable using the same ID but its content changes every
minute. The partition has 4 sectors with 1024 bytes each.
Each write of the variable requires 16 bytes of storage.
As we have 944 bytes available for ATEs for each sector, and because ZMS is a fast-forward
storage system, we are going to rewrite the first location of the first sector after
:math:`\frac{(944 \times 4)}{16} = 236 \text{ minutes}`.

In addition to the normal writes, garbage collector will move the still valid data from old
sectors to new ones.
As we are using the same ID and a big partition size, no data will be moved by the garbage
collector in this case.
For storage devices that could be written 20000 times, the storage will last about
4.720.000 minutes (~9 years).

To make a more general formula we must first compute the effective used size in ZMS by our
typical set of data.
For id/data pair with data <= 8 bytes, effective_size is 16 bytes
For id/data pair with data > 8 bytes, effective_size is 16 bytes + sizeof(data)
Let's suppose that total_effective_size is the total size of the set of data that is written in
the storage and that the partition is well dimensioned (double of the effective size) to avoid
having the garbage collector moving blocks all the time.

The expected life of the device in minutes is computed as :

.. math::

   \small\frac{(SECTOR\_EFFECTIVE\_SIZE \times SECTOR\_NUMBER \times MAX\_NUM\_WRITES)}{(TOTAL\_EFFECTIVE\_SIZE \times WR\_MIN)}

Where:

``SECTOR_EFFECTIVE_SIZE``: is the size sector - header_size(80 bytes)

``SECTOR_NUMBER``: is the number of sectors

``MAX_NUM_WRITES``: is the life expectancy of the storage device in number of writes

``TOTAL_EFFECTIVE_SIZE``: Total effective size of the set of written data

``WR_MIN``: Number of writes of the set of data per minute

Features
********
ZMS has introduced many features compared to existing storage system like NVS and will evolve
from its initial version to include more features that satisfies new technologies requirements
such as low latency and bigger storage space.

Existing features
=================
Version1
--------
- Supports non erasable devices (only one write operation to erase a sector)
- Supports large partition size and sector size (64 bits address space)
- Supports large IDs width (32 bits) to store ID/Value pairs
- Small sized data ( <= 8 bytes) are stored in the ATE itself
- Built-in Data CRC32 (included in the ATE)
- Versionning of ZMS (to handle future evolution)
- Supports large write-block-size (Only for platforms that need this)

Future features
===============

- Add multiple format ATE support to be able to use ZMS with different ATE formats that satisfies
  requirements from application
- Add the possibility to skip garbage collector for some application usage where ID/value pairs
  are written periodically and do not exceed half of the partition size (ther is always an old
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

- If you are using a non erasable technology device like RRAM or MRAM, :ref:`ZMS <zms_api>` is definitely the
  best fit for your storage subsystem as it is designed very well to avoid emulating erase for
  these devices and replace it by a single write call.
- For devices with large write_block_size and/or needs a sector size that is different than the
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
life expectancy of the device described in this section: :ref:`wear-leveling`.

Sample
******

A sample of how ZMS can be used is supplied in :zephyr:code-sample:`zms`.

API Reference
*************

The ZMS subsystem APIs are provided by ``zms.h``:

.. doxygengroup:: zms_data_structures

.. doxygengroup:: zms_high_level_api

.. comment
   not documenting .. doxygengroup:: zms
