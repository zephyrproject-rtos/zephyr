:orphan:

.. _migration_3.5:

Migration guide to Zephyr v3.5.0 (Working Draft)
################################################

This document describes the changes required or recommended when migrating your
application from Zephyr v3.4.0 to Zephyr v3.5.0.

Required changes
****************

* The kernel :c:func:`k_mem_slab_free` function has changed its signature, now
  taking a ``void *mem`` pointer instead of a ``void **mem`` double-pointer.
  The new signature will not immediately trigger a compiler error or warning,
  instead likely causing a invalid memory access at runtime. A new ``_ASSERT``
  statement, that you can enable with :kconfig:option:`CONFIG_ASSERT`, will
  detect if you pass the function memory not belonging to the memory blocks in
  the slab.

* The :kconfig:option:`CONFIG_BOOTLOADER_SRAM_SIZE` default value is now ``0`` (was
  ``16``). Bootloaders that use a part of the SRAM should set this value to an
  appropriate size. :github:`60371`

* The Kconfig option ``CONFIG_GPIO_NCT38XX_INTERRUPT`` has been renamed to
  :kconfig:option:`CONFIG_GPIO_NCT38XX_ALERT`.

* MCUmgr SMP version 2 error codes entry has changed due to a collision with an
  existing response in shell_mgmt. Previously, these errors had the entry ``ret``
  but now have the entry ``err``. ``smp_add_cmd_ret()`` is now deprecated and
  :c:func:`smp_add_cmd_err` should be used instead, ``MGMT_CB_ERROR_RET`` is
  now deprecated and :c:enumerator:`MGMT_CB_ERROR_ERR` should be used instead.
  SMP version 2 error code defines for in-tree modules have been updated to
  replace the ``*_RET_RC_*`` parts with ``*_ERR_*``.

Recommended Changes
*******************

* Setting the GIC architecture version by selecting
  :kconfig:option:`CONFIG_GIC_V1`, :kconfig:option:`CONFIG_GIC_V2` and
  :kconfig:option:`CONFIG_GIC_V3` directly in Kconfig has been deprecated.
  The GIC version should now be specified by adding the appropriate compatible, for
  example :dtcompatible:`arm,gic-v2`, to the GIC node in the device tree.
