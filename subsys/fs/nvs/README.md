# Non volatile storage (NVS) for zephyr

# Overview

Storage of elements in flash in a FIFO manner. Elements are stored as identifier (id) - data pairs in a flash circular buffer. The flash area is divided into sectors. Elements are appended to a sector until storage space in the sector is exhausted. Then a new sector in the flash area is prepared for use (erased). Before erasing the sector it is checked that identifier - data pairs exist in the sectors in use, if not the id-data pair is copied.

The identifier is a 16 bit number of which 2 cannot be used:

  0xFFFF as it is used to determine free locations in flash_map
  0xFFFE as it is used to close a sector


NVS ensures that for each id there is at least one id-data pair stored in flash at all time.

NVS does flash wear levelling as each new write is done at the next available flash location.

NVS allows storage of binary blobs, strings, integers, longs, ... and any combination of these. As each element is stored with a header (containing the id and length) and a footer (containing the crc16) it is less suited to store only integers.

For NVS the file system is declared as:

static struct nvs_fs fs = {
  .sector_size = SECTOR_SIZE,
  .sector_count = SECTOR_COUNT,
  .offset = STORAGE_OFFSET,
};

where

  SECTOR_SIZE is a multiple of the flash erase page size and a power of 2,
  SECTOR_COUNT is at least 2,
  STORAGE_OFFSET is the offset of the storage area in flash.

# High level API

In the high level API the following routines are available:

nvs_init()
  - initialize nvs for a given nvs_fs

nvs_clear()
  - clear the nvs_fs, deleting all Storage

nvs_write()
  - write an entry to flash

nvs_read()
  - read the latest entry from flash

nvs_read_hist()
  - read older entry from flash. This requires first to read the oldest one: nvs_read_hist(fs, entry, data, 0) and then with consecutive nvs_read_hist(fs, entry,data,1) all history items can be read.

To avoid frequent erases of flash sectors the maximum size of the entries that can be written in the high level API is limited to SECTOR_SIZE/4.

# Usage of high level API:

1. Initialize the filesystem with nvs_init
2. To add an element to nvs: call nvs_write() to write an entry to flash
3. To read newest entry in nvs: call nvs_read() to read the latest entry from flash

# Low Level API

The low level API gives an interface that is somewhat similar to FCB. The low level API allows some more detailed control and provides the following routines:

nvs_append()
  - reserve space to store an element - writes header to flash and returns write location
nvs_flash_write()
  - write contents to flash
nvs_append_close()
  - calculate crc16 and write to flash
nvs_get_first_entry()
  - get the first (oldest) entry in flash of specific id: returns read location and length
nvs_get_last_entry()
  - get the last (newest) entry in flash of specific id: returns read location and length
nvs_walk_entry()
  - get the next entry in flash of specific id: returns read location and length (entry needs to be initialised with data location)
nvs_check_crc()
  - check the crc16 of an entry in flash, the calculation is done one the flash data and compared to the footer
nvs_rotate()
  - erase oldest used sector, copy old data before making the sector current
nvs_flash_read()
  - read an entry from flash

# Usage of low level API:

1. Initialize the filesystem with nvs_init
2. To add an element to nvs:
   a. call nvs_append() to get location; if this fails due to lack of space,
   call nvs_rotate()
   b. use nvs_flash_write() to write contents
   c. call nvs_append_close() when done
3. To read newest entry in nvs:
   a. call nvs_get_last_entry to get read location
   b. optional: call nvs_check_crc() to check the crc16
   c. call nvs_flash_read() to read the data
