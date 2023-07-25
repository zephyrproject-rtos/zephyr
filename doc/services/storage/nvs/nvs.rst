.. _nvs_api:

Non-Volatile Storage (NVS)
##########################

Elements, represented as id-data pairs, are stored in flash using a
FIFO-managed circular buffer. The flash area is divided into sectors. Elements
are appended to a sector until storage space in the sector is exhausted. Then a
new sector in the flash area is prepared for use (erased). Before erasing the
sector it is checked that identifier - data pairs exist in the sectors in use,
if not the id-data pair is copied.

The id is a 16-bit unsigned number. NVS ensures that for each used id there is
at least one id-data pair stored in flash at all time.

NVS allows storage of binary blobs, strings, integers, longs, and any
combination of these.

Each element is stored in flash as metadata (8 byte) and data. The metadata is
written in a table starting from the end of a nvs sector, the data is
written one after the other from the start of the sector. The metadata consists
of: id, data offset in sector, data length, part (unused) and a crc.

A write of data to nvs always starts with writing the data, followed by a write
of the metadata. Data that is written in flash without metadata is ignored
during initialization.

During initialization NVS will verify the data stored in flash, if it
encounters an error it will ignore any data with missing/incorrect metadata.

NVS checks the id-data pair before writing data to flash. If the id-data pair
is unchanged no write to flash is performed.

To protect the flash area against frequent erases it is important that there is
sufficient free space. NVS has a protection mechanism to avoid getting in a
endless loop of flash page erases when there is limited free space. When such
a loop is detected NVS returns that there is no more space available.

For NVS the file system is declared as:

.. code-block:: c

	static struct nvs_fs fs = {
	.flash_device = NVS_FLASH_DEVICE,
	.sector_size = NVS_SECTOR_SIZE,
	.sector_count = NVS_SECTOR_COUNT,
	.offset = NVS_STORAGE_OFFSET,
	};

where

- ``NVS_FLASH_DEVICE`` is a reference to the flash device that will be used. The
  device needs to be operational.
- ``NVS_SECTOR_SIZE`` is the sector size, it has to be a multiple of
  the flash erase page size and a power of 2.
- ``NVS_SECTOR_COUNT`` is the number of sectors, it is at least 2, one
  sector is always kept empty to allow copying of existing data.
- ``NVS_STORAGE_OFFSET`` is the offset of the storage area in flash.


Flash wear
**********

When writing data to flash a study of the flash wear is important. Flash has a
limited life which is determined by the number of times flash can be erased.
Flash is erased one page at a time and the pagesize is determined by the
hardware. As an example a nRF51822 device has a pagesize of 1024 bytes and each
page can be erased about 20,000 times.

Calculating expected device lifetime
====================================

Suppose we use a 4 bytes state variable that is changed every minute and
needs to be restored after reboot. NVS has been defined with a sector_size
equal to the pagesize (1024 bytes) and 2 sectors have been defined.

Each write of the state variable requires 12 bytes of flash storage: 8 bytes
for the metadata and 4 bytes for the data. When storing the data the
first sector will be full after 1024/12 = 85.33 minutes. After another 85.33
minutes, the second sector is full.  When this happens, because we're using
only two sectors, the first sector will be used for storage and will be erased
after 171 minutes of system time.  With the expected device life of 20,000
writes, with two sectors writing every 171 minutes, the device should last
about 171 * 20,000 minutes, or about 6.5 years.

More generally then, with

- ``NS`` as the number of storage requests per minute,
- ``DS`` as the data size in bytes,
- ``SECTOR_SIZE`` in bytes, and
- ``PAGE_ERASES`` as the number of times the page can be erased,

the expected device life (in minutes) can be calculated as::

   SECTOR_COUNT * SECTOR_SIZE * PAGE_ERASES / (NS * (DS+8)) minutes

From this formula it is also clear what to do in case the expected life is too
short: increase ``SECTOR_COUNT`` or ``SECTOR_SIZE``.

Flash write block size migration
********************************
It is possible that during a DFU process, the flash driver used by the NVS
changes the supported minimal write block size.
The NVS in-flash image will stay compatible unless the
physical ATE size changes.
Especially, migration between 1,2,4,8-bytes write block sizes is allowed.

Sample
******

A sample of how NVS can be used is supplied in ``samples/subsys/nvs``.

Troubleshooting
***************

MPU fault while using NVS, or ``-ETIMEDOUT`` error returned
   NVS can use the internal flash of the SoC.  While the MPU is enabled,
   the flash driver requires MPU RWX access to flash memory, configured
   using :kconfig:option:`CONFIG_MPU_ALLOW_FLASH_WRITE`.  If this option is
   disabled, the NVS application will get an MPU fault if it references
   the internal SoC flash and it's the only thread running.  In a
   multi-threaded application, another thread might intercept the fault
   and the NVS API will return an ``-ETIMEDOUT`` error.


API Reference
*************

The NVS subsystem APIs are provided by ``nvs.h``:

.. doxygengroup:: nvs_data_structures

.. doxygengroup:: nvs_high_level_api

.. comment
   not documenting
   .. doxygengroup:: nvs
