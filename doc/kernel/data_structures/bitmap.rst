.. SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
.. SPDX-License-Identifier: Apache-2.0

.. _sys_bitmap_api:

Bitmap
######

A bitmap is a byte array in which each bit is one flag or the state of one numbered
item. :c:macro:`SYS_BITMAP_DEFINE` defines one with room for a given number of bits,
at file scope, inside a function or as a structure member; the bitmap has no other
data.

Bit ``n`` is bit ``n % 8`` of byte ``n / 8``, where bit 0 of a byte is its least
significant bit (``BIT(0)``). This is how protocols and on-disk formats commonly lay out
bit masks, so the functions also work on such byte arrays in received messages.

Concurrency
***********

The functions take no lock and are not atomic. Changing a bit reads and writes its
whole byte, which loses a concurrent change of another bit in the same byte. The
owner of the bitmap serializes all access to it, for example by touching it from one
thread only or under a lock it already holds.

Which bit API to use:

* Bitmap: bits that one context owns, or that a lock of the caller protects.
* :ref:`Atomic bit operations <atomic_v2>` (:c:macro:`ATOMIC_DEFINE`): bits that
  several contexts change or test-and-set without a lock.
* :c:type:`sys_bitarray_t`: bit arrays that need internal locking or bounds checks,
  and allocation of bit regions.
* ``sys_bitfield_*()``: bits that must be accessed as volatile 32-bit words, such as
  register bits.

Usage
*****

.. code-block:: c

   #include <zephyr/sys/bitmap.h>

   #define NUM_SLOTS 40

   static SYS_BITMAP_DEFINE(slots_used, NUM_SLOTS);

   static int slot_alloc(void)
   {
           size_t slot = sys_bitmap_find_next_clear(slots_used, NUM_SLOTS, 0);

           if (slot == NUM_SLOTS) {
                   return -ENOMEM;
           }

           sys_bitmap_set_bit(slots_used, slot);

           return slot;
   }

The single-bit functions do not check the bit number against the size of the bitmap;
the caller passes them only bit numbers below it. The search and count functions take
the number of bits, accept any start bit and ignore the unused bits of the last byte.

API Reference
*************

.. doxygengroup:: sys_bitmap
