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

.. _zephyr_4.5:

Zephyr 4.5.0 (Working Draft)
############################

We are pleased to announce the release of Zephyr version 4.5.0.

Major enhancements with this release include:

An overview of the changes required or recommended when migrating your application from Zephyr
v4.4.0 to Zephyr v4.5.0 can be found in the separate :ref:`migration guide<migration_4.5>`.

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************

The following CVEs are addressed by this release:

* :cve:`2026-8718` Under embargo until 2026-08-08

* :cve:`2026-9263` Under embargo until 2026-06-28

API Changes
***********

..
  Only removed, deprecated and new APIs. Changes go in migration guide.

Removed APIs and options
========================

* Networking

    * ``CONFIG_NET_TC_SKIP_FOR_HIGH_PRIO``
    * ``CONFIG_NET_SOCKETS_POLL_MAX``
    * ``net_ipv6_set_hop_limit()``
    * ``net_if_ipv4_get_netmask()``
    * ``net_if_ipv4_set_netmask()``
    * ``net_if_ipv4_set_netmask_by_index()``
    * ``openthread_state_changed_cb_register()``
    * ``openthread_state_changed_cb_unregister()``
    * ``openthread_start()``
    * ``openthread_api_mutex_lock()``
    * ``openthread_api_mutex_try_lock()``
    * ``openthread_api_mutex_unlock()``
    * ``struct openthread_state_changed_cb``
    * ``TLS_CREDENTIAL_SERVER_CERTIFICATE``

* Random

    * ``CONFIG_CTR_DRBG_CSPRNG_GENERATOR``
    * ``CONFIG_CS_CTR_DRBG_PERSONALIZATION``

* West sign support for imgtool, which was deprecated in Zephyr 4.0, has been removed.

Deprecated APIs and options
===========================

* Audio Codec

  * The :c:struct:`audio_codec_api` struct has been deprecated. Audio codec drivers are now
    expected to use the :c:macro:`DEVICE_API` macro to declare their driver API.

* :abbr:`DMIC (Digital Microphone Interface)`

  * The :c:struct:`_dmic_ops` struct has been deprecated. DMIC drivers are now expected to use the
    :c:macro:`DEVICE_API` macro to declare their driver API.

* Fuel Gauge

  * Deprecated various fuel gauge property enums and union fields in favor of
    new versions with explicit unit suffixes.

* LoRa

  * Renamed :c:func:`lora_recv_duty_cycle` to :c:func:`lora_recv_duty_cycle_async`
    to be consistent with the existing sync/async naming convention.

* Nordic

  * The internal SoC platform Kconfig symbols ``NRF_PLATFORM_HALTIUM`` and
    ``NRF_PLATFORM_LUMOS`` have been deprecated. Use specific SOC_SERIES_* Kconfig options instead.

  * The sysbuild Kconfig option ``SB_CONFIG_NRF_HALTIUM_GENERATE_UICR`` has
    been renamed to :kconfig:option:`SB_CONFIG_NRF_GENERATE_UICR`.

  * The Nordic SoC headers :file:`<haltium_power.h>` and :file:`<haltium_pm_s2ram.h>`
    have been renamed to :file:`<soc_power.h>` and :file:`<soc_pm_s2ram.h>` respectively.

* Ring buffer

  * The ring buffer item API (:c:func:`ring_buf_item_init`, :c:func:`ring_buf_item_put`,
    :c:func:`ring_buf_item_get`, :c:func:`ring_buf_item_space_get`) has been deprecated in favor of
    :c:struct:`sys_ringq` (see :ref:`fixed_size_ringq_api`).

New APIs and options
====================
..
  Link to new APIs here, in a group if you think it's necessary, no need to get
  fancy just list the link, that should contain the documentation. If you feel
  like you need to add more details, add them in the API documentation code
  instead.

.. zephyr-keep-sorted-start re(^\* \w) ignorecase

* Audio

  * :c:member:`pcm_stream_cfg.gain_db`

* Bluetooth

  * Audio

    * :c:func:`bt_ascs_register`
    * :c:func:`bt_ascs_unregister`

  * Host

    * :c:func:`bt_conn_take`
    * :c:func:`bt_conn_drop`

  * Mesh

    * :c:struct:`bt_mesh_lpn_timing`
    * :c:func:`bt_mesh_stat_lpn_timing_get`
    * :c:func:`bt_mesh_stat_lpn_timing_reset`

* Devicetree

  * :c:macro:`DT_IRQN_BY_NAME`
  * :c:macro:`DT_INST_IRQN_BY_NAME`

* Haptics

  * :c:enumerator:`haptics_monitor`
  * :c:enumerator:`haptics_monitor_type`
  * :c:enumerator:`haptics_source`
  * :c:union:`haptics_config`
  * :c:func:`haptics_calibrate`
  * :c:func:`haptics_monitor_get`
  * :c:func:`haptics_monitor_set`
  * :c:func:`haptics_select_source`
  * :c:func:`haptics_set_level`
  * :c:func:`haptics_stream_samples`

* Kernel

  * :c:func:`k_thread_runtime_stats_is_enabled`
  * :c:func:`atomic_test_and_set_bit_to`

* LoRa

  * :c:func:`lora_recv_duty_cycle`
  * :c:func:`lora_recv_duty_cycle_async`

* :c:struct:`sys_ringq` (see :ref:`fixed_size_ringq_api`)

* Network

  * Add :c:func:`net_eth_set_if_type_wifi` to set the ethernet interface type to Wi-Fi.

.. zephyr-keep-sorted-stop

New Boards
**********

..
  You may update this list as you contribute a new board during the release cycle, in order to make
  it visible to people who might be looking at the working draft of the release notes. However, note
  that this list will be recomputed at the time of the release, so you don't *have* to update it.
  In any case, just link the board, further details go in the board description.

* Arduino

  * :zephyr:board:`Arduino Nesso N1 <arduino_nesso_n1>` (``arduino_nesso_n1``)

* Seeed

  * :zephyr:board:`Seeed Wio Tracker L1 <wio_tracker_l1>` (``wio_tracker_l1``)

New Shields
***********

..
  Same as above, this will also be recomputed at the time of the release.


New Drivers
***********

..
  Same as above, this will also be recomputed at the time of the release.
  Just link the driver, further details go in the binding description

* GPIO

  * Diodes/Pericom PI4IOE5V6408 8-bit I2C-bus I/O expander
    (:dtcompatible:`diodes,pi4ioe5v6408`).

* Input

  * VIRTIO input device (:dtcompatible:`virtio,input`).

New Samples
***********

..
  Same as above, this will also be recomputed at the time of the release.
 Just link the sample, further details go in the sample documentation itself.

* :zephyr:code-sample:`mctp_i2c_bus_host` (renamed from ``mctp_i2c_bus_owner``)
* :zephyr:code-sample:`mctp_i3c_bus_host` (renamed from ``mctp_i3c_bus_owner``)

Libraries / Subsystems
**********************

* DFU

  * Added :kconfig:option:`CONFIG_IMG_CUSTOM_SECTOR_SIZE` to allow MCUboot to use a different
    sector size for reducing the swap-using-offset status area size.

* LoRa / LoRaWAN

  * Added a native LoRaWAN backend
    (:kconfig:option:`CONFIG_LORA_MODULE_BACKEND_NATIVE`) that implements
    LoRaWAN 1.0.x Class A directly on top of the LoRa radio driver, without
    the Semtech LoRaMac-node dependency.  Currently supports the EU868 region.
  * :c:member:`lora_modem_config.sync_word`

Devicetree
**********

  * :c:macro:`DT_NODELABEL_C_TOKEN`
  * :c:macro:`DT_NODELABEL_C_TOKEN_BY_IDX`


* TF-M

  * TF-M was updated from version 2.2.2 to version 2.3.0. Release notes can be
    found at:
    https://trustedfirmware-m.readthedocs.io/en/tf-mv2.3.0/releases/2.3.0.html

Other notable changes
*********************

* Kernel

  * :kconfig:option:`CONFIG_SCHED_CPU_MASK` no longer depends on
    :kconfig:option:`CONFIG_SCHED_SIMPLE`.  CPU affinity masks are now
    supported on all three scheduler backends: ``SCHED_SIMPLE`` (O(N) list
    scan), ``SCHED_SCALABLE`` (O(N) red/black tree walk), and ``SCHED_MULTIQ``
    (O(P·N) per-priority bucket scan).  See the updated
    :ref:`SMP documentation<smp_cpu_mask>` for per-backend performance notes.

  * :kconfig:option:`CONFIG_SCHED_CPU_MASK_PIN_ONLY` now enforces the
    one-CPU-bit invariant at both the API boundary (``cpu_mask_mod()``) and at
    queue time (``thread_runq()``).  Calling :c:func:`k_thread_cpu_mask_clear`,
    :c:func:`k_thread_cpu_mask_enable_all`, or
    :c:func:`k_thread_cpu_mask_disable` in PIN_ONLY mode triggers an assertion
    failure.  Use :c:func:`k_thread_cpu_pin` to reassign a thread to a
    different CPU.

* Wi-Fi

  * Removed the ``samples/net/wifi/test_certs/rsa2k`` enterprise test
    certificates (DES-encrypted private keys). Use ``rsa2k_no_des`` instead.

..
  Any more descriptive subsystem or driver changes. Do you really want to write
  a paragraph or is it enough to link to the api/driver/Kconfig/board page above?
