.. _cache_guide:

Caching Basics
##############

This section discusses the basics of cache coherency and under what situations a
user needs to explicitly deal with caching. For more detailed info on Zephyr's
caching tools, see :ref:`cache_config` for Zephyr Kconfig options or
:ref:`cache_api` for the API reference. This section primarily focuses on the
data cache though there is typically also an instruction cache for systems with
cache support.

.. note::

  The information here assumes that the architecture-specific MPU support is
  enabled. See the architecture-specific documentation for details.

.. note::

  While cache coherence can be a concern for data shared between SMP cores, Zephyr
  in general ensures that memory will be seen in a coherent state from multiple
  cores. Most applications will only need to use the cache APIs for interaction
  with external hardware like DMA controllers or foreign CPUs running a
  different OS image. For more information on cache coherence between SMP cores,
  see :kconfig:option:`CONFIG_KERNEL_COHERENCE`.

When dealing with memory shared between a processor core and other bus masters,
cache coherency needs to be considered. Typically processor caches exist as
close to each processor core as possible to maximize performance gain. Because
of this, data moved into and out of memory by DMA engines will be stale in the
processor's cache, resulting in what appears to be corrupt data. If you are
moving data using DMA and the processor doesn't see the data you expect, cache
coherency may be the issue.

There are multiple approaches to ensuring that the data seen by the processor
core and peripherals is coherent. The simplest is just to disable caching, but
this defeats the purpose of having a hardware cache in the first place and
results in a significant performance hit. Many architectures provide methods for
disabling caching for only a portion of memory. This can be useful when cache
coherence is more important than performance, such as when using DMA with SPI.
Finally, there is the option to flush or invalidate the cache for regions of
memory at runtime.

Globally Disabling the Data Cache
---------------------------------

As mentioned above, globally disabling data caching can have a significant
performance impact but can be useful for debugging.

Requirements:

* :kconfig:option:`CONFIG_DCACHE`: DCACHE control enabled in Zephyr.

* :kconfig:option:`CONFIG_CACHE_MANAGEMENT`: cache API enabled.

* Call :c:func:`sys_cache_data_disable()` to globally disable the data cache.

Disabling Caching for a Memory Region
-------------------------------------

Disabling caching for only a portion of memory can be a good performance
compromise if performance on the uncached memory is not critical to the
application. This is a good option if the application requires many small
unrelated buffers that are smaller than a cache line.

Requirements:

* :kconfig:option:`CONFIG_DCACHE`: DCACHE control enabled in Zephyr.

* :kconfig:option:`CONFIG_MEM_ATTR`: enable the ``mem-attr`` library for
  handling memory attributes in the device tree.

* Annotate your device tree according to :ref:`mem_mgmt_api`.

Assuming the MPU driver is enabled, it will configure the specified regions
according to the memory attributes specified during kernel initialization. When
using a dedicated uncached region of memory, the linker needs to be instructed
to place buffers into that region. This can be accomplished by specifying the
memory region explicitly using ``Z_GENERIC_SECTION``:

.. code-block:: c

  /* SRAM4 marked as uncached in device tree */
  uint8_t buffer[BUF_SIZE] Z_GENERIC_SECTION("SRAM4");

.. note::

  Configuring a distinct memory region with separate caching rules requires the
  use of an MPU region which may be a limited resource on some architectures.
  MPU regions may be needed by other memory protection features such as
  :ref:`userspace <mpu_userspace>`, :ref:`stack protection <mpu_stack_objects>`,
  or :ref:`memory domains<memory_domain>`.

Automatically Disabling Caching by Variable
-------------------------------------------

Zephyr has the ability to automatically define an uncached region in memory and
allocate variables to it using ``__nocache``. Any variables marked with this
attribute will be placed in a special ``nocache`` linker region in memory. This
region will be configured as uncached by the MPU driver during initialization.
This is a simpler option than explicitly declaring a region of memory uncached
but provides less control over the placement of these variables, as the linker
may allocate this region anywhere in RAM.

Requirements:

* :kconfig:option:`CONFIG_DCACHE`: DCACHE control enabled in Zephyr.

* :kconfig:option:`CONFIG_NOCACHE_MEMORY`: enable allocation of the ``nocache``
  linker region and configure it as uncached.

* Add the ``__nocache`` attribute at the end of any uncached buffer definition:

.. code-block:: c

  uint8_t buffer[BUF_SIZE] __nocache;

.. note::

  See note above regarding possible limitations on MPU regions. The ``nocache``
  region is still a distinct MPU region even though it is automatically created
  by Zephyr instead of being explicitly defined by the user.

Runtime Cache Control
---------------------

The most performant but most complex option is to control data caching at
runtime. The two most relevant cache operations in this case are **flushing**
and **invalidating**. Both of these operations operate on the smallest unit of
cacheable memory, the cache line. Data cache lines are typically 16 to 128
bytes. See :kconfig:option:`CONFIG_DCACHE_LINE_SIZE`. Cache line sizes are
typically fixed in hardware and not configurable, but Zephyr does need to know
the size of cache lines in order to correctly and efficiently manage the cache.
If the buffers in question are smaller than the data cache line size, it may be
more efficient to place them in an uncached region, as unrelated data packed
into the same cache line may be destroyed when invalidating.

Flushing the cache involves writing all modified cache lines in a specified
region back to shared memory. Flush the cache associated with a buffer after the
processor has written to it and before a remote bus master reads from that
region.

.. note::

  Some architectures support a cache configuration called **write-through**
  caching in which data writes from the processor core propagate through to
  shared memory. While this solves the cache coherence problem for CPU writes,
  it also results in more traffic to main memory which may result in performance
  degradation.

Invalidating the cache works similarly but in the other direction. It marks
cache lines in the specified region as stale, ensuring that the cache line will
be refreshed from main memory when the processor next reads from the specified
region. Invalidate the data cache of a buffer that a peripheral has written to
before reading from that region.

In some cases, the same buffer may be reused for e.g. DMA reads and DMA writes.
In that case it is possible to first flush the cache associated with a buffer
and then invalidate it, ensuring that the cache will be refreshed the next time
the processor reads from the buffer.

Requirements:

* :kconfig:option:`CONFIG_DCACHE`: DCACHE control enabled in Zephyr.

* :kconfig:option:`CONFIG_CACHE_MANAGEMENT`: cache API enabled.

* Call :c:func:`sys_cache_data_flush_range()` to flush a memory region.

* Call :c:func:`sys_cache_data_invd_range()` to invalidate a memory region.

* Call :c:func:`sys_cache_data_flush_and_invd_range()` to flush and invalidate.

Alignment
---------

As mentioned in :c:func:`sys_cache_data_invd_range()` and associated functions,
buffers should be aligned to the cache line size. This can be accomplished by
using ``__aligned``:

.. code-block:: c

  uint8_t buffer[BUF_SIZE] __aligned(CONFIG_DCACHE_LINE_SIZE);
