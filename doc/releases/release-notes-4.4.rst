:orphan:

..
  What goes here: removed/deprecated apis, new boards, new drivers, notable
  features. If you feel like something new can be useful to a user, put it
  under "Other Enhancements" in the first paragraph, if you feel like something
  is worth mentioning in the project media (release blog post, release
  livestream) put it under "Major enhancement".
..
  If you are describing a feature or functionality, consider adding it to the
  actual project documentation rather than the release notes, so that the
  information does not get lost in time.
..
  No list of bugfixes, minor changes, those are already in the git log, this is
  not a changelog.
..
  Does the entry have a link that contains the details? Just add the link, if
  you think it needs more details, put them in the content that shows up on the
  link.
..
  Are you thinking about generating this? Don't put anything at all.
..
  Does the thing require the user to change their application? Put it on the
  migration guide instead. (TODO: move the removed APIs section in the
  migration guide)

.. _zephyr_4.4:

Zephyr 4.4.0 (Working Draft)
############################

We are pleased to announce the release of Zephyr version 4.4.0.

Major enhancements with this release include:

An overview of the changes required or recommended when migrating your application from Zephyr
v4.3.0 to Zephyr v4.4.0 can be found in the separate :ref:`migration guide<migration_4.4>`.

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************

The following CVEs are addressed by this release:

* :cve:`2025-53022` `(TF-M) FWU does not check the length of the TLV’s payload
  <https://trustedfirmware-m.readthedocs.io/en/latest/security/security_advisories/fwu_tlv_payload_out_of_bounds_vulnerability.html>`_

API Changes
***********

..
  Only removed, deprecated and new APIs. Changes go in migration guide.

* Architectures

  * Xtensa

    * Removed as these are architectural features:

      * :kconfig:option:`CONFIG_XTENSA_MMU_DOUBLE_MAP`
      * :kconfig:option:`CONFIG_XTENSA_RPO_CACHE`
      * :kconfig:option:`CONFIG_XTENSA_CACHED_REGION`
      * :kconfig:option:`CONFIG_XTENSA_UNCACHED_REGION`

Removed APIs and options
========================

Deprecated APIs and options
===========================

* Bluetooth

  * Mesh

    * The function :c:func:`bt_mesh_input_number` was deprecated. Applications should use
      :c:func:`bt_mesh_input_numeric` instead.
    * The callback :c:member:`output_number` in :c:struct:`bt_mesh_prov` structure was deprecated.
      Applications should use :c:member:`output_numeric` callback instead.
    * The :kconfig:option:`CONFIG_BT_MESH_MODEL_VND_MSG_CID_FORCE` option has been deprecated.

  * Host

    * :c:member:`bt_conn_le_info.interval` has been deprecated. Use
      :c:member:`bt_conn_le_info.interval_us` instead. Note that the units have changed:
      ``interval`` was in units of 1.25 milliseconds, while ``interval_us`` is in microseconds.

* Sensors

  * NXP

    * Deprecated the ``mcux_lpcmp`` driver (:zephyr_file:`drivers/sensor/nxp/mcux_lpcmp/mcux_lpcmp.c`). It is
      currently scheduled to be removed in Zephyr 4.6, along with the ``mcux_lpcmp`` sample. (:github:`100998`).

New APIs and options
====================
..
  Link to new APIs here, in a group if you think it's necessary, no need to get
  fancy just list the link, that should contain the documentation. If you feel
  like you need to add more details, add them in the API documentation code
  instead.

.. zephyr-keep-sorted-start re(^\* \w)

* Architectures

  * Xtensa

    * :kconfig:option:`CONFIG_XTENSA_MMU_USE_DEFAULT_MAPPINGS`

* Audio

  * :c:macro:`PDM_DT_IO_CFG_GET`
  * :c:macro:`PDM_DT_HAS_LEFT_CHANNEL`
  * :c:macro:`PDM_DT_HAS_RIGHT_CHANNEL`

* Bluetooth

  * Audio

    * :c:func:`bt_bap_ep_get_conn`

  * Host

    * :c:func:`bt_gatt_cb_unregister` Added an API to unregister GATT callback handlers.

  * Mesh

    * :c:func:`bt_mesh_input_numeric` to provide provisioning numeric input OOB value.
    * :c:member:`output_numeric` callback in :c:struct:`bt_mesh_prov` structure to
      output numeric values during provisioning.
    * :kconfig:option:`CONFIG_BT_MESH_CDB_KEY_SYNC` to enable key synchronization between
      the Configuration Database (CDB) and the local Subnet and AppKey storages when keys are
      added, deleted, or updated during key refresh procedure.
      The option is enabled by default.

  * Services

    * Introduced Alert Notification Service (ANS) :kconfig:option:`CONFIG_BT_ANS`

* Ethernet

  * Driver MAC address configuration with support for NVMEM cell.

    * :c:func:`net_eth_mac_load`
    * :c:struct:`net_eth_mac_config`
    * :c:macro:`NET_ETH_MAC_DT_CONFIG_INIT` and :c:macro:`NET_ETH_MAC_DT_INST_CONFIG_INIT`

  * Added :c:enum:`ethernet_stats_type` and optional ``get_stats_type`` callback in
    :c:struct:`ethernet_api` to allow filtering of ethernet statistics by type
    (common, vendor, or all). Drivers that support vendor-specific statistics can
    implement ``get_stats_type`` to skip expensive FW queries when only common stats
    are requested. The existing ``get_stats`` API remains unchanged for backward
    compatibility.

* Flash

  * :dtcompatible:`jedec,mspi-nor` now allows MSPI configuration of read, write and
    control commands separately via devicetree.

* IPM

  * IPM callbacks for the mailbox backend now correctly handle signal-only mailbox
    mailbox usage. Applications should be prepared to receive a NULL payload pointer
    in IPM callbacks when no data buffer is provided by the mailbox.

* Modem

  * :kconfig:option:`CONFIG_MODEM_HL78XX_AT_SHELL`
  * :kconfig:option:`CONFIG_MODEM_HL78XX_AIRVANTAGE`

* NVMEM

  * Flash device support

    * :kconfig:option:`CONFIG_NVMEM_FLASH`
    * :kconfig:option:`CONFIG_NVMEM_FLASH_WRITE`

* PWM

  * Extended API with PWM events

    * :c:struct:`pwm_event_callback` to hold a pwm event callback
    * :c:func:`pwm_init_event_callback` to help initialize a :c:struct:`pwm_event_callback` object
    * :c:func:`pwm_add_event_callback` to add a callback
    * :c:func:`pwm_remove_event_callback` to remove a callback
    * :c:member:`manage_event_callback` in :c:struct:`pwm_driver_api` to manage pwm events
    * :kconfig:option:`CONFIG_PWM_EVENT`

* Power

  * The new ``voltage-scale`` property of :dtcompatible:`st,stm32u5-pwr` can be used to
    select the voltage scale manually on STM32U5 series via Devicetree. This notably
    enables usage of the USB controller at lower system clock frequencies.

* Settings

  * :kconfig:option:`CONFIG_SETTINGS_SAVE_SINGLE_SUBTREE_WITHOUT_MODIFICATION`
  * :kconfig:option:`CONFIG_SETTINGS_SAVE_SINGLE_SUBTREE_WITHOUT_MODIFICATION_VALUE_SIZE`

* Sys

  * :c:macro:`COND_CASE_1`

* Timeutil

  * :kconfig:option:`CONFIG_TIMEUTIL_APPLY_SKEW`

* Video

  * :kconfig:option:`CONFIG_VIDEO_BUFFER_POOL_HEAP_SIZE`
  * :kconfig:option:`CONFIG_VIDEO_BUFFER_POOL_ZEPHYR_REGION`
  * :kconfig:option:`CONFIG_VIDEO_BUFFER_POOL_ZEPHYR_REGION_NAME`

.. zephyr-keep-sorted-stop

New Boards
**********

..
  You may update this list as you contribute a new board during the release cycle, in order to make
  it visible to people who might be looking at the working draft of the release notes. However, note
  that this list will be recomputed at the time of the release, so you don't *have* to update it.
  In any case, just link the board, further details go in the board description.

* Ai-Thinker Co., Ltd.

   * :zephyr:board:`ai_m61_32s_kit` (``ai_m61_32s_kit``)
   * Rename ai_m62_12f and ai_wb2_12f to ai_m62_12f_kit and ai_wb2_12f_kit

New Shields
***********

..
  Same as above, this will also be recomputed at the time of the release.

New Drivers
***********

..
  Same as above, this will also be recomputed at the time of the release.
  Just link the driver, further details go in the binding description

* Comparator

    * Added NXP low power comparator driver (:dtcompatible:`nxp,lpcmp`). (:github:`100998`)

* Radio

  * :dtcompatible:`radio-fem-two-ctrl-pins` (renamed from ``generic-fem-two-ctrl-pins``)
  * :dtcompatible:`radio-gpio-coex` (renamed from ``gpio-radio-coex``)

New Samples
***********

* :zephyr:code-sample:`ble_peripheral_ans`

..
  Same as above, this will also be recomputed at the time of the release.
 Just link the sample, further details go in the sample documentation itself.

DeviceTree
**********

* :c:macro:`DT_CHILD_BY_UNIT_ADDR_INT`
* :c:macro:`DT_INST_CHILD_BY_UNIT_ADDR_INT`

Libraries / Subsystems
**********************

* LoRa/LoRaWAN

   * :c:func:`lora_airtime`

Other notable changes
*********************

* TF-M was updated to version 2.2.2 (from 2.2.0). The release notes can be found at:

  * https://trustedfirmware-m.readthedocs.io/en/tf-mv2.2.2/releases/2.2.1.html
  * https://trustedfirmware-m.readthedocs.io/en/tf-mv2.2.2/releases/2.2.2.html

..
  Any more descriptive subsystem or driver changes. Do you really want to write
  a paragraph or is it enough to link to the api/driver/Kconfig/board page above?
