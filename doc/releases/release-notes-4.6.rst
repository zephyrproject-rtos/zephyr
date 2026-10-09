:orphan:

Zephyr 4.6.0
############

API Changes
***********

Removed APIs and options
========================

* Modem

    * ``CONFIG_HL78XX_EVT_MONITOR_HEAP_SIZE``, replaced by
      :kconfig:option:`CONFIG_HL78XX_EVT_MONITOR_QUEUE_DEPTH`

New APIs and options
====================

* Modem

  * Sierra Wireless HL78xx:

    * :c:func:`hl78xx_at_monitor_dropped_count`
    * :c:func:`hl78xx_evt_monitor_dispatch`
    * :c:func:`hl78xx_evt_monitor_dropped_count`
    * :kconfig:option:`CONFIG_HL78XX_EVT_MONITOR_QUEUE_DEPTH`
    * :kconfig:option:`CONFIG_MODEM_HL78XX_NTN_PDP_FAMILY`
