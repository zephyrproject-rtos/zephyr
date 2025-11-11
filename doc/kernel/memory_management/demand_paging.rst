.. _memory_management_api_demand_paging:

Demand Paging
#############

Demand paging provides a mechanism where data is only brought into physical
memory as required by current execution context. The physical memory is
conceptually divided in page-sized page frames as regions to hold data.

* When the processor tries to access data and the data page exists in
  one of the page frames, the execution continues without any interruptions.

* When the processor tries to access the data page that does not exist
  in any page frames, a page fault occurs. The paging code then brings in
  the corresponding data page from backing store into physical memory if
  there is a free page frame. If there is no more free page frames,
  the eviction algorithm is invoked to select a data page to be paged out,
  thus freeing up a page frame for new data to be paged in. If this data
  page has been modified after it is first paged in, the data will be
  written back into the backing store. If no modifications is done or
  after written back into backing store, the data page is now considered
  paged out and the corresponding page frame is now free. The paging code
  then invokes the backing store to page in the data page corresponding to
  the location of the requested data. The backing store copies that data
  page into the free page frame. Now the data page is in physical memory
  and execution can continue.

There are functions where paging in and out can be invoked manually
using :c:func:`k_mem_page_in()` and :c:func:`k_mem_page_out()`.
:c:func:`k_mem_page_in()` can be used to page in data pages
in anticipation that they are required in the near future. This is used to
minimize number of page faults as these data pages are already in physical
memory, and thus minimizing latency. :c:func:`k_mem_page_out()` can be
used to page out data pages where they are not going to be accessed for
a considerable amount of time. This frees up page frames so that the next
page in can be executed faster as the paging code does not need to invoke
the eviction algorithm.

Terminology
***********

Data Page
  A data page is a page-sized region of data. It may exist in a page frame,
  or be paged out to some backing store. Its location can always be looked
  up in the CPU's page tables (or equivalent) by virtual address.
  The data type will always be ``void *`` or in some cases ``uint8_t *``
  when doing pointer arithmetic.

Page Frame
  A page frame is a page-sized physical memory region in RAM. It is a
  container where a data page may be placed. It is always referred to by
  physical address. Zephyr has a convention of using ``uintptr_t`` for physical
  addresses. For every page frame, a ``struct k_mem_page_frame`` is instantiated to
  store metadata. Flags for each page frame:

  * ``K_MEM_PAGE_FRAME_FREE`` indicates a page frame is unused and on the list of
    free page frames. When this flag is set, none of the other flags are
    meaningful and they must not be modified.

  * ``K_MEM_PAGE_FRAME_PINNED`` indicates a page frame is pinned in memory
    and should never be paged out.

  * ``K_MEM_PAGE_FRAME_RESERVED`` indicates a physical page reserved by hardware
    and should not be used at all.

  * ``K_MEM_PAGE_FRAME_MAPPED`` is set when a physical page is mapped to
    virtual memory address.

  * ``K_MEM_PAGE_FRAME_BUSY`` indicates a page frame is currently involved in
    a page-in/out operation.

  * ``K_MEM_PAGE_FRAME_BACKED`` indicates a page frame has a clean copy
    in the backing store.

K_MEM_SCRATCH_PAGE
  The virtual address of a special page provided to the backing store to:
  * Copy a data page from ``k_MEM_SCRATCH_PAGE`` to the specified location; or,
  * Copy a data page from the provided location to ``K_MEM_SCRATCH_PAGE``.
  This is used as an intermediate page for page in/out operations. This
  scratch needs to be mapped read/write for backing store code to access.
  However the data page itself may only be mapped as read-only in virtual
  address space. If this page is provided as-is to backing store,
  the data page must be re-mapped as read/write which has security
  implications as the data page is no longer read-only to other parts of
  the application.

Paging Statistics
*****************

Paging statistics can be obtained via various function calls when
:kconfig:option:`CONFIG_DEMAND_PAGING_TIMING_HISTOGRAM_NUM_BINS` is enabled:

* Overall statistics via :c:func:`k_mem_paging_stats_get()`

* Per-thread statistics via :c:func:`k_mem_paging_thread_stats_get()`
  if :kconfig:option:`CONFIG_DEMAND_PAGING_THREAD_STATS` is enabled

* Execution time histogram can be obtained when
  :kconfig:option:`CONFIG_DEMAND_PAGING_TIMING_HISTOGRAM` is enabled, and
  :kconfig:option:`CONFIG_DEMAND_PAGING_TIMING_HISTOGRAM_NUM_BINS` is defined.
  Note that the timing is highly dependent on the architecture,
  SoC or board. It is highly recommended that
  ``k_mem_paging_eviction_histogram_bounds[]`` and
  ``k_mem_paging_backing_store_histogram_bounds[]``
  be defined for a particular application.

  * Execution time histogram of eviction algorithm via
    :c:func:`k_mem_paging_histogram_eviction_get()`

  * Execution time histogram of backing store doing page-in via
    :c:func:`k_mem_paging_histogram_backing_store_page_in_get()`

  * Execution time histogram of backing store doing page-out via
    :c:func:`k_mem_paging_histogram_backing_store_page_out_get()`

Eviction Algorithm
******************

The eviction algorithm is used to determine which data page and its
corresponding page frame can be paged out to free up a page frame
for the next page in operation. There are four functions which are
called from the kernel paging code:

* :c:func:`k_mem_paging_eviction_init()` is called to initialize
  the eviction algorithm. This is called at ``POST_KERNEL``.

* :c:func:`k_mem_paging_eviction_add()` is called each time a data page becomes
  eligible for future eviction.

* :c:func:`k_mem_paging_eviction_remove()` is called when a data page is no
  longer eligible for eviction. This may happen if the given data page becomes
  pinned, gets unmapped or is about to be evicted.

* :c:func:`k_mem_paging_eviction_select()` is called to select
  a data page to evict. A function argument ``dirty`` is written to
  signal the caller whether the selected data page has been modified
  since it is first paged in. If the ``dirty`` bit is returned
  as set, the paging code signals to the backing store to write
  the data page back into storage (thus updating its content).
  The function returns a pointer to the page frame corresponding to
  the selected data page.

There is one additional function which is called by the architecture's memory
management code to flag data pages when they trigger an access fault:
:c:func:`k_mem_paging_eviction_accessed()`. This is used by the LRU algorithm
to requeue "used" pages.

Two eviction algorithms are currently available:

* An NRU (Not-Recently-Used) eviction algorithm has been implemented as a
  sample. This is a very simple algorithm which ranks data pages on whether
  they have been accessed and modified. The selection is based on this ranking.

* An LRU (Least-Recently-Used) eviction algorithm is also available. It is
  based on a sorted queue of data pages. The LRU code is more complex compared
  to the NRU code but also considerably more efficient. This is recommended for
  production use.

To implement a new eviction algorithm, :c:func:`k_mem_paging_eviction_init()`
and :c:func:`k_mem_paging_eviction_select()` must be implemented.
If :kconfig:option:`CONFIG_EVICTION_TRACKING` is enabled for an algorithm,
these additional functions must also be implemented,
:c:func:`k_mem_paging_eviction_add()`, :c:func:`k_mem_paging_eviction_remove()`,
:c:func:`k_mem_paging_eviction_accessed()`.

Backing Store
*************

Backing store is responsible for paging in/out data page between
their corresponding page frames and storage. These are the functions
which must be implemented:

* :c:func:`k_mem_paging_backing_store_init()` is called to
  initialized the backing store at ``POST_KERNEL``.

* :c:func:`k_mem_paging_backing_store_location_get()` is called to
  reserve a backing store location so a data page can be paged out.
  This ``location`` token is passed to
  :c:func:`k_mem_paging_backing_store_page_out()` to perform actual
  page out operation.

* :c:func:`k_mem_paging_backing_store_location_free()` is called to
  free a backing store location (the ``location`` token) which can
  then be used for subsequent page out operation.

* :c:func:`k_mem_paging_backing_store_location_query()` is called to obtain
  the ``location`` token corresponding to storage content to be virtually
  mapped and paged-in on demand. Most useful with
  :kconfig:option:`CONFIG_DEMAND_MAPPING`.

* :c:func:`k_mem_paging_backing_store_page_in()` copies a data page
  from the backing store location associated with the provided
  ``location`` token to the page pointed by ``K_MEM_SCRATCH_PAGE``.

* :c:func:`k_mem_paging_backing_store_page_out()` copies a data page
  from ``K_MEM_SCRATCH_PAGE`` to the backing store location associated
  with the provided ``location`` token.

* :c:func:`k_mem_paging_backing_store_page_finalize()` is invoked after
  :c:func:`k_mem_paging_backing_store_page_in()` so that the page frame
  struct may be updated for internal accounting. This can be
  a no-op.

To implement a new backing store, the functions mentioned above
must be implemented.
:c:func:`k_mem_paging_backing_store_page_finalize()` can be an empty
function if so desired.

API Reference
*************

.. doxygengroup:: mem-demand-paging

Eviction Algorithm APIs
=======================

.. doxygengroup:: mem-demand-paging-eviction

Backing Store APIs
==================

.. doxygengroup:: mem-demand-paging-backing-store
