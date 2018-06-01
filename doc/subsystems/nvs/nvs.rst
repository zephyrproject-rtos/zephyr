.. _nvs:

Non-volatile storage (NVS) for Zephyr
#####################################

Elements, represented as id-data pairs, are stored in flash using a
FIFO-managed circular buffer. The flash area is divided into sectors. Elements
are appended to a sector until storage space in the sector is exhausted. Then a
new sector in the flash area is prepared for use (erased). Before erasing the
sector it is checked that identifier - data pairs exist in the sectors in use,
if not the id-data pair is copied.

The id is a 16-bit unsigned number, where two values are reserved:

- ``0xFFFF`` is used to determine free locations in flash
- ``0x0000`` is used to determine jumps or to correct errors

NVS ensures that for each id there is at least one id-data pair stored in flash
at all time.

NVS allows storage of binary blobs, strings, integers, longs, and any
combination of these.

Each element is stored in flash as: ``length-data-id-length``. The length
parameter is the size in byte of the data. It is added at the start and the end
of a data block for two purposes:

- It allows the verification of a data block, in case of a incomplete write
  the length at the end is missing.
- It allows NVS to travel forward and backward over the flash data blocks.


To reduce flash usage the size of the length is dependent on the size of the
data. For data size < 128 byte length occupies 1 byte in flash, for data size
>= 128 byte length occupies 2 bytes.

If an error is detected in the flash storage (during writing) NVS is placed in
a locked state and becomes a read-only storage system. This allows the user
application to read all variables up to the error location to recover as much
as possible. A call to nvs_reinit() when in locked state erases the flash
storage and reopens NVS for storage.

NVS will also be placed in a locked state when it is full meaning there is no
extra space to store data. A call to nvs_reinit() when in locked state erases
the flash  storage and reopens NVS for storage.

During initialization NVS will verify the data stored in flash, if it
encounters an error it will correct the error and restore the data as much as
possible. All data that is found before the error is restored.

NVS has a configuration option to enable extra flash protection
(``CONFIG_NVS_FLASH_PROTECTION=y``), when selected NVS does a extra check
before writing data to flash. If the id-data pair is unchanged no write to
flash is performed. The down-side of this protection is that NVS needs to read
the storage system to check if the id-data pair is unchanged. If you are
already performing such a check you can disable the extra flash protection
(``CONFIG_NVS_FLASH_PROTECTION=n``).

To protect the storage system against frequent sector erases the size of
id-data pairs is limited to ``CONFIG_NVS_MAX_ELEM_SIZE``. This limit is
by default set to 1/4 of the sector size.


For NVS the file system is declared as:

.. code-block:: c

	static struct nvs_fs fs = {
	.sector_size = NVS_SECTOR_SIZE,
	.sector_count = NVS_SECTOR_COUNT,
	.offset = NVS_STORAGE_OFFSET,
	.max_len = NVS_MAX_ELEM_SIZE,
	};

where

- ``NVS_SECTOR_SIZE`` is the sector size, it has to be a multiple of
  the flash erase page size and a power of 2.
- ``NVS_SECTOR_COUNT`` is the number of sectors, it is at least 2, one
  sector is always kept empty to allow copying of existing data.
- ``NVS_STORAGE_OFFSET`` is the offset of the storage area in flash.
- ``NVS_MAX_ELEM_SIZE`` is the maximum item size.


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

Each write of the state variable requires 8 bytes of flash storage (length: 1
byte, id: 2 bytes, data: 4 bytes, length: 1 byte). When storing the data the
first sector will be full after 1024/8 = 128 minutes. After another 128
minutes, the second sector is full.  When this happens, because we're using
only two sectors, the first sector will be used for storage and will be erased
after 256 minutes of system time.  With the expected device life of 20,000
writes, with two sectors writing every 256 minutes, the device should last
about 256 * 20,000 minutes, or about 9.75 years.

More generally then, with

- ``NS`` as the number of storage requests per minute,
- ``DS`` as the data size in bytes,
- ``SECTOR_SIZE`` in bytes, and
- ``PAGE_ERASES`` as the number of times the page can be erased,

for DS < 128 byte, the expected device life (in minutes) can be calculated as::

   SECTOR_COUNT * SECTOR_SIZE * PAGE_ERASES / (NS * (DS+4)) minutes

for DS >= 128 byte, the expected device life (in minutes) can be calculated
as::

   SECTOR_COUNT * SECTOR_SIZE * PAGE_ERASES / (NS * (DS+8)) minutes

From this formula it is also clear what to do in case the expected life is too
short: increase ``SECTOR_COUNT`` or ``SECTOR_SIZE``.

Sample
******

A sample of how NVS can be used is supplied in ``samples/subsys/nvs``.

API
**************

The NVS subsystem APIs are provided by ``nvs.h``:

.. doxygengroup:: nvs_data_structures
   :project: Zephyr

.. doxygengroup:: nvs_high_level_api
   :project: Zephyr
