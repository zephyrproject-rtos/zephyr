:orphan:

Migration guide to Zephyr v4.6.0
################################

Modem
*****

Sierra Wireless HL78xx
======================

* The ``CONFIG_HL78XX_EVT_MONITOR_HEAP_SIZE`` option has been replaced by
  :kconfig:option:`CONFIG_HL78XX_EVT_MONITOR_QUEUE_DEPTH`. The queue of notifications waiting for
  the deferred event monitors is now sized as a number of notifications instead of a heap size in
  bytes. Applications that set the old option must remove it, and set the new one if the default
  of 16 notifications is not enough. A notification that does not fit is dropped and counted; the
  count is available from :c:func:`hl78xx_evt_monitor_dropped_count`. The slots are reserved
  statically, one queued notification each, so the default of 16 uses more RAM than the 256-byte
  heap it replaces.
