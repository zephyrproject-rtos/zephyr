.. _arm64_developer_guide:

ARM64 Developer Guide
#####################

.. _arm64_mmu_dt_regions:

Devicetree-based MMU Region Mapping
************************************

On ARM64 platforms the MMU page tables can be populated automatically from
devicetree at compile time.  Any node with ``compatible = "zephyr,memory-region"``
that also carries a ``zephyr,memory-attr`` property will be turned into a
static ``arm_mmu_region`` entry by the architecture startup code — no
per-SoC ``mmu_regions.c`` changes are required.

Supported memory attributes
============================

Only **Normal** memory types are accepted.  Device memory (peripheral MMIO)
must be mapped through the ``DEVICE_MMIO`` API (see :ref:`device_model_api`).

The attribute macros are defined in
:zephyr_file:`include/zephyr/dt-bindings/memory-attr/memory-attr-arm64.h`
as composable combinations of the generic ``DT_MEM_CACHEABLE`` flag and
architecture-specific sub-attributes:

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - DT macro
     - Description
   * - ``DT_MEM_ARM64_MMU_NORMAL``
     - Normal write-back cacheable (``DT_MEM_CACHEABLE | ATTR_ARM64_CACHE_WB``)
   * - ``DT_MEM_ARM64_MMU_NORMAL_NC``
     - Normal non-cacheable (``0``)
   * - ``DT_MEM_ARM64_MMU_NORMAL_WT``
     - Normal write-through cacheable (``DT_MEM_CACHEABLE``)

Example devicetree overlay
===========================

.. code-block:: devicetree

   #include <zephyr/dt-bindings/memory-attr/memory-attr.h>
   #include <zephyr/dt-bindings/memory-attr/memory-attr-arm64.h>

   / {
       soc {
           /* Cacheable shared memory pool */
           shm0: memory@42000000 {
               compatible = "zephyr,memory-region";
               reg = <0x0 0x42000000 0x0 0x1000>;
               zephyr,memory-region = "SHM0";
               zephyr,memory-attr = <DT_MEM_ARM64_MMU_NORMAL>;
           };

           /* Non-cacheable DMA buffer */
           dma_buf: memory@43000000 {
               compatible = "zephyr,memory-region";
               reg = <0x0 0x43000000 0x0 0x1000>;
               zephyr,memory-region = "DMA_BUF";
               zephyr,memory-attr = <DT_MEM_ARM64_MMU_NORMAL_NC>;
           };
       };
   };

Each region is mapped with ``MT_P_RW_U_NA | MT_DEFAULT_SECURE_STATE`` combined
with the memory type derived from ``zephyr,memory-attr``.

Translation table sizing
=========================

Every mapped region that falls on a different 2 MB boundary requires an
additional Level 3 page table.  If the boot hangs silently after adding new
regions, increase :kconfig:option:`CONFIG_MAX_XLAT_TABLES` in the board or
test configuration:

.. code-block:: kconfig

   CONFIG_MAX_XLAT_TABLES=16

Supported attribute combinations
=================================

All non-device attribute combinations are valid: the ``DT_MEM_CACHEABLE``
generic bit selects cacheable vs non-cacheable, and the arch-specific
``ATTR_ARM64_CACHE_WB`` sub-bit selects write-back vs write-through
when cacheable.
