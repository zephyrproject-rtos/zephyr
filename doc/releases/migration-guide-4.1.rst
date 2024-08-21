:orphan:

.. _migration_4.1:

Migration guide to Zephyr v4.1.0 (Working Draft)
################################################

This document describes the changes required when migrating your application from Zephyr v4.0.0 to
Zephyr v4.1.0.

Any other changes (not directly related to migrating applications) can be found in
the :ref:`release notes<zephyr_4.1>`.

.. contents::
    :local:
    :depth: 2

Build System
************

BOSSA Runner
============

The ``bossac`` runner has been changed to no longer do a full erase by default when flashing. To
perform a full erase, pass the ``--erase`` option when executing ``west flash``.

Kernel
******

Security
********

* New options for stack canaries have been added, providing users with finer control over stack
  protection. With this change, :kconfig:option:`CONFIG_STACK_CANARIES` no longer enables the
  compiler option ``-fstack-protector-all``. Users who wish to use this option must now enable
  :kconfig:option:`CONFIG_STACK_CANARIES_ALL`.

Boards
******

* Shield ``mikroe_weather_click`` now supports both I2C and SPI interfaces. Users should select
  the required configuration by using ``mikroe_weather_click_i2c`` or ``mikroe_weather_click_spi``
  instead of ``mikroe_weather_click``.

Devicetree
**********

* The :dtcompatible:`microchip,cap1203` driver has changed its compatible to
  :dtcompatible:`microchip,cap12xx` and has been updated to support multiple
  channels.
  The number of available channels is derived from the length of the devicetree
  array property ``input-codes``.
  The :kconfig:option:`CONFIG_INPUT_CAP1203_POLL` has been removed:
  If the devicetree property ``int-gpios`` is present, interrupt mode is used
  otherwise, polling is used.
  The :kconfig:option:`CONFIG_INPUT_CAP1203_PERIOD` has been replaced with
  the devicetree property ``poll-interval-ms``.
  In interrupt mode, the devicetree property ``repeat`` is supported.

Raspberry Pi
============

* ``CONFIG_SOC_SERIES_RP2XXX`` is renamed to :kconfig:option:`CONFIG_SOC_SERIES_RP2040`.

STM32
=====

* MCO clock source and prescaler are now exclusively configured by the DTS
  as it was introduced earlier.
  The Kconfig method for configuration is now removed.

Modules
*******

Mbed TLS
========

* If a platform has a CSPRNG source available (i.e. :kconfig:option:`CONFIG_CSPRNG_ENABLED`
  is set), then the Kconfig option :kconfig:option:`CONFIG_MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG`
  is the default choice for random number source instead of
  :kconfig:option:`CONFIG_MBEDTLS_PSA_CRYPTO_LEGACY_RNG`. This helps in reducing
  ROM/RAM footprint of the Mbed TLS library.

* The newly-added Kconfig option :kconfig:option:`CONFIG_MBEDTLS_PSA_KEY_SLOT_COUNT`
  allows to specify the number of key slots available in the PSA Crypto core.
  Previously this value was not explicitly set, so Mbed TLS's default value of
  32 was used. The new Kconfig option defaults to 16 instead in order to find
  a reasonable compromise between RAM consumption and most common use cases.
  It can be further trimmed down to reduce RAM consumption if the final
  application doesn't need that many key slots simultaneously.

Trusted Firmware-M
==================

LVGL
====

* The config option :kconfig:option:`CONFIG_LV_Z_FLUSH_THREAD_PRIO` is now called
  :kconfig:option:`CONFIG_LV_Z_FLUSH_THREAD_PRIORITY` and its value is now interpreted as an
  absolute priority instead of a cooperative one.

Device Drivers and Devicetree
*****************************

* Device driver APIs are placed into iterable sections (:github:`71773` and :github:`82102`) to
  allow for runtime checking. See :ref:`device_driver_api` for more details.
  The :c:macro:`DEVICE_API()` macro should be used by out-of-tree driver implementations for
  all the upstream driver classes.

* The :c:func:`video_buffer_alloc` and :c:func:`video_buffer_aligned_alloc` functions in the
  video API now take an additional timeout parameter.

ADC
===

* Renamed the ``compatible`` from ``nxp,kinetis-adc12`` to :dtcompatible:`nxp,adc12`.

Controller Area Network (CAN)
=============================

Display
=======

* Displays using the MIPI DBI driver which set their MIPI DBI mode via the
  ``mipi-mode`` property in devicetree should now use a string property of
  the same name, like so:

  .. code-block:: devicetree

    /* Legacy display definition */

    st7735r: st7735r@0 {
        ...
        mipi-mode = <MIPI_DBI_MODE_SPI_4WIRE>;
        ...
    };

    /* New display definition */

    st7735r: st7735r@0 {
        ...
        mipi-mode = "MIPI_DBI_MODE_SPI_4WIRE";
        ...
    };


Enhanced Serial Peripheral Interface (eSPI)
===========================================

Entropy
=======

* BT HCI based entropy driver now directly sends the HCI command to parse random
  data instead of waiting for BT connection to be ready. This is helpful on
  platforms where the BT controller owns the HW random generator and the application
  processor needs to get random data before BT is fully enabled.
  (:github:`79931`)

GNSS
====

I2C
===

* Renamed the ``compatible`` from ``nxp,imx-lpi2c`` to :dtcompatible:`nxp,lpi2c`.

Input
=====

PWM
===

* Renamed the ``compatible`` from ``renesas,ra8-pwm`` to :dtcompatible:`renesas,ra-pwm`.

Interrupt Controller
====================

LED Strip
=========

MMU/MPU
=======

* Renamed the ``compatible`` from ``nxp,kinetis-mpu`` to :dtcompatible:`nxp,sysmpu` and added
  its corresponding binding.
* Renamed the Kconfig option ``CPU_HAS_NXP_MPU`` to :kconfig:option:`CPU_HAS_NXP_SYSMPU`.

Pin Control
===========

  * Renamed the ``compatible`` from ``nxp,kinetis-pinctrl`` to :dtcompatible:`nxp,port-pinctrl`.
  * Renamed the ``compatible`` from ``nxp,kinetis-pinmux`` to :dtcompatible:`nxp,port-pinmux`.
  * Silabs Series 2 devices now use a new pinctrl driver selected by
    :dtcompatible:`silabs,dbus-pinctrl`. This driver allows the configuration of GPIO properties
    through device tree, rather than having them hard-coded for each supported signal. It also
    supports all possible digital bus signals by including a binding header such as
    :zephyr_file:`include/zephyr/dt-bindings/pinctrl/silabs/xg24-pinctrl.h`.

    Pinctrl should now be configured like this:

    .. code-block:: devicetree

      #include <dt-bindings/pinctrl/silabs/xg24-pinctrl.h>

      &pinctrl {
        i2c0_default: i2c0_default {
          group0 {
            /* Pin selection(s) using bindings included above */
            pins = <I2C0_SDA_PD2>, <I2C0_SCL_PD3>;
            /* Shared properties for the group of pins */
            drive-open-drain;
            bias-pull-up;
          };
        };
      };


PWM
===

* Renamed the ``compatible`` from ``nxp,kinetis-ftm-pwm`` to :dtcompatible:`nxp,ftm-pwm`.

Sensors
=======

Serial
======

* Renamed the ``compatible`` from ``nxp,kinetis-lpuart`` to :dtcompatible:`nxp,lpuart`.

Stepper
=======

  * Renamed the ``compatible`` from ``zephyr,gpio-steppers`` to :dtcompatible:`zephyr,gpio-stepper`.
  * Renamed the ``stepper_set_actual_position`` function to :c:func:`stepper_set_reference_position`.
  * Renamed the ``stepper_enable_constant_velocity_mode`` function to :c:func:`stepper_run`.
  * Renamed the ``stepper_move`` function to :c:func:`stepper_move_by`.
  * Renamed the ``stepper_set_target_position`` function to :c:func:`stepper_move_to`.
  * The :kconfig:option:`STEPPER_ADI_TMC_RAMP_GEN` is now deprecated and is replaced with the new
    :kconfig:option:`STEPPER_ADI_TMC5041_RAMP_GEN` option.

SPI
===

* Renamed the ``compatible`` from ``nxp,imx-lpspi`` to :dtcompatible:`nxp,lpspi`.
* Renamed the ``compatible`` from ``nxp,kinetis-dspi`` to :dtcompatible:`nxp,dspi`.

Regulator
=========

RTC
===

* Renamed the ``compatible`` from ``nxp,kinetis-rtc`` to :dtcompatible:`nxp,rtc`.

Timer
=====

* Renamed the ``compatible`` from ``nxp,kinetis-ftm`` to :dtcompatible:`nxp,ftm` and relocate it
  under ``dts/bindings/timer``.

Video
=====

* The :file:`include/zephyr/drivers/video-controls.h` got updated to have video controls IDs (CIDs)
  matching the definitions in the Linux kernel file ``include/uapi/linux/v4l2-controls.h``.
  In most cases, removing the category prefix is enough: ``VIDEO_CID_CAMERA_GAIN`` becomes
  ``VIDEO_CID_GAIN``.
  The new ``video-controls.h`` source now contains description of each control ID to help
  disambiguating.

Watchdog
========

Wi-Fi
=====

* Renamed the ``compatible`` from ``nxp,kinetis-wdog32`` to :dtcompatible:`nxp,wdog32`.

* The config options :kconfig:option:`CONFIG_NXP_WIFI_BUILD_ONLY_MODE` and
  :kconfig:option:`CONFIG_NRF_WIFI_BUILD_ONLY_MODE` are now unified under
  :kconfig:option:`CONFIG_BUILD_ONLY_NO_BLOBS` making it a common entry point
  for any vendor to enable builds without blobs.

Bluetooth
*********

Bluetooth HCI
=============

* The :kconfig:option:`BT_CTLR` has been deprecated. A new :kconfig:option:`HAS_BT_CTLR` has been
  introduced which should be selected by the respective link layer Kconfig options (e.g. a
  HCI driver option, or the one for the upstream controller). It's recommended that all HCI drivers
  for local link layers select the new option, since that opens up the possibility of indicating
  build-time support for specific features, which e.g. the host stack can take advantage of.

Bluetooth Mesh
==============

* Following the beginnig of the deprecation process for the TinyCrypt crypto
  library, Kconfig symbol :kconfig:option:`CONFIG_BT_MESH_USES_TINYCRYPT` was
  set as deprecated. Default option for platforms that do not support TF-M
  is :kconfig:option:`CONFIG_BT_MESH_USES_MBEDTLS_PSA`.

Bluetooth Audio
===============

* The following Kconfig options are not longer automatically enabled by the LE Audio Kconfig
  options and may need to be enabled manually (:github:`81328`):

    * :kconfig:option:`CONFIG_BT_GATT_CLIENT`
    * :kconfig:option:`CONFIG_BT_GATT_AUTO_DISCOVER_CCC`
    * :kconfig:option:`CONFIG_BT_GATT_AUTO_UPDATE_MTU`
    * :kconfig:option:`CONFIG_BT_EXT_ADV`
    * :kconfig:option:`CONFIG_BT_PER_ADV_SYNC`
    * :kconfig:option:`CONFIG_BT_ISO_BROADCASTER`
    * :kconfig:option:`CONFIG_BT_ISO_SYNC_RECEIVER`
    * :kconfig:option:`CONFIG_BT_PAC_SNK`
    * :kconfig:option:`CONFIG_BT_PAC_SRC`

Bluetooth Classic
=================

Bluetooth Host
==============

* :kconfig:option:`CONFIG_BT_BUF_ACL_RX_COUNT` has been deprecated. The number of ACL RX buffers is
  now computed internally and is equal to :kconfig:option:`CONFIG_BT_MAX_CONN` + 1. If an application
  needs more buffers, it can use the new :kconfig:option:`CONFIG_BT_BUF_ACL_RX_COUNT_EXTRA` to add
  additional ones.

  e.g. if :kconfig:option:`CONFIG_BT_MAX_CONN` was ``3`` and
  :kconfig:option:`CONFIG_BT_BUF_ACL_RX_COUNT` was ``7`` then
  :kconfig:option:`CONFIG_BT_BUF_ACL_RX_COUNT_EXTRA` should be set to ``7 - (3 + 1) = 3``.

  .. warning::

   The default value of :kconfig:option:`CONFIG_BT_BUF_ACL_RX_COUNT` has been set to 0.

* LE legacy pairing is no longer enabled by default since it's not secure. Leaving it enabled
  makes a device vulnerable for downgrade attacks. If an application still needs to use LE legacy
  pairing, it should disable :kconfig:option:`CONFIG_BT_SMP_SC_PAIR_ONLY` manually.

Bluetooth Crypto
================

Networking
**********

* The Prometheus metric creation has changed as user does not need to have a separate
  struct :c:struct:`prometheus_metric` any more. This means that the Prometheus macros
  :c:macro:`PROMETHEUS_COUNTER_DEFINE`, :c:macro:`PROMETHEUS_GAUGE_DEFINE`,
  :c:macro:`PROMETHEUS_HISTOGRAM_DEFINE` and :c:macro:`PROMETHEUS_SUMMARY_DEFINE`
  prototypes have changed. (:github:`81712`)

* The default subnet mask on newly added IPv4 addresses is now specified with
  :kconfig:option:`CONFIG_NET_IPV4_DEFAULT_NETMASK` option instead of being left
  empty. Applications can still specify a custom netmask for an address with
  :c:func:`net_if_ipv4_set_netmask_by_addr` function if needed.

* The HTTP server public API function signature for the :c:type:`http_resource_dynamic_cb_t` has
  changed, the data is now passed in a :c:struct:`http_request_ctx` which holds the data, data
  length and request header information. Request headers should be accessed via this parameter
  rather than directly in the :c:struct:`http_client_ctx` to correctly handle concurrent requests
  on different HTTP/2 streams.

* The :kconfig:option:`CONFIG_NET_L2_OPENTHREAD` symbol no longer implies the
  :kconfig:option:`CONFIG_NVS` Kconfig option. Platforms using OpenThread must explicitly enable
  either the :kconfig:option:`CONFIG_NVS` or :kconfig:option:`CONFIG_ZMS` Kconfig option.

Other Subsystems
****************

Flash map
=========

hawkBit
=======

MCUmgr
======

Modem
=====

LoRa
====

* The function :c:func:`lora_recv_async` and callback ``lora_recv_cb`` now include an
  additional ``user_data`` parameter, which is a void pointer. This parameter can be used to reference
  any user-defined data structure. To maintain the current behavior, set this parameter to ``NULL``.

Architectures
*************

* Common

  * ``_current`` is deprecated, used :c:func:`arch_current_thread` instead.

* native/POSIX

  * :kconfig:option:`CONFIG_NATIVE_APPLICATION` has been deprecated. Out-of-tree boards using this
    option should migrate to the native_simulator runner (:github:`81232`).
    For an example of how this was done with a board in-tree check :github:`61481`.
  * For the native_sim target :kconfig:option:`CONFIG_NATIVE_SIM_NATIVE_POSIX_COMPAT` has been
    switched to ``n`` by default, and this option has been deprecated. Ensure your code does not
    use the :kconfig:option:`CONFIG_BOARD_NATIVE_POSIX` option anymore (:github:`81232`).

* x86

  * Kconfigs ``CONFIG_DISABLE_SSBD`` and ``CONFIG_ENABLE_EXTENDED_IBRS`` have been deprecated
    since v3.7. These were removed.  Use :kconfig:option:`CONFIG_X86_DISABLE_SSBD` and
    :kconfig:option:`CONFIG_X86_ENABLE_EXTENDED_IBRS` instead.
