.. _fcb_api:

Flash Circular Buffer (FCB)
###########################

Flash circular buffer provides an abstraction through which you can treat
flash like a FIFO. You append entries to the end, and read data from the
beginning.

.. note::

   As of Zephyr release 2.1 the :ref:`NVS <nvs_api>` storage API is
   recommended over FCB for use as a back-end for the :ref:`settings API
   <settings_api>`.

Description
***********

Entries in the flash contain the length of the entry, the data within
the entry, and checksum over the entry contents.

Storage of entries in flash is done in a FIFO fashion. When you
request space for the next entry, space is located at the end of the
used area. When you start reading, the first entry served is the
oldest entry in flash.

Entries can be appended to the end of the area until storage space is
exhausted. You have control over what happens next; either erase oldest
block of data, thereby freeing up some space, or stop writing new data
until existing data has been collected. FCB treats underlying storage as
an array of flash sectors; when it erases old data, it does this a
sector at a time.

Entries in the flash are checksummed. That is how FCB detects whether
writing entry to flash completed ok. It will skip over entries which
don't have a valid checksum.

Usage
*****

To add an entry to circular buffer:

- Call :c:func:`fcb_append` to get the location where data can be written. If
  this fails due to lack of space, you can call :c:func:`fcb_rotate` to erase
  the oldest sector which will make the space. And then call
  :c:func:`fcb_append` again.
- Use :c:func:`flash_area_write` to write entry contents.
- Call :c:func:`fcb_append_finish` when done. This completes the writing of the
  entry by calculating the checksum.

To read contents of the circular buffer:

- Call :c:func:`fcb_walk` with a pointer to your callback function.
- Within callback function copy in data from the entry using
  :c:func:`flash_area_read`. You can tell when all data from within a sector
  has been read by monitoring the returned entry's area pointer. Then you
  can call :c:func:`fcb_rotate`, if you're done with that data.

Alternatively:

- Call :c:func:`fcb_getnext` with 0 in entry offset to get the pointer to
  the oldest entry.
- Use :c:func:`flash_area_read` to read entry contents.
- Call :c:func:`fcb_getnext` with pointer to current entry to get the next one.
  And so on.

API Reference
*************

The FCB subsystem APIs are provided by ``fcb.h``:

Data structures
===============
.. doxygengroup:: fcb_data_structures

API functions
=============
.. doxygengroup:: fcb_api
