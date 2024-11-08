:orphan:

.. _zephyr_4.0:

Zephyr 4.0.0 (Working Draft)
############################

We are pleased to announce the release of Zephyr version 4.0.0.

Major enhancements with this release include:

* The introduction of the :ref:`secure storage<secure_storage>` subsystem. It allows the use of the
  PSA Secure Storage API and of persistent keys in the PSA Crypto API on all board targets. It
  is now the standard way to provide device-specific protection to data at rest. (:github:`76222`)

An overview of the changes required or recommended when migrating your application from Zephyr
v3.7.0 to Zephyr v4.0.0 can be found in the separate :ref:`migration guide<migration_4.0>`.

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************
The following CVEs are addressed by this release:

More detailed information can be found in:
https://docs.zephyrproject.org/latest/security/vulnerabilities.html

* :cve:`2024-8798`: Under embargo until 2024-11-22

API Changes
***********

* Removed deprecated arch-level CMSIS header files
  ``include/zephyr/arch/arm/cortex_a_r/cmsis.h`` and
  ``include/zephyr/arch/arm/cortex_m/cmsis.h``. ``cmsis_core.h`` needs to be
  included now.

* Removed deprecated ``ceiling_fraction`` macro. :c:macro:`DIV_ROUND_UP` needs
  to be used now.

* Deprecated ``EARLY``, ``APPLICATION`` and ``SMP`` init levels can no longer be
  used for devices.

Removed APIs in this release
============================

* Macro ``K_THREAD_STACK_MEMBER``, deprecated since v3.5.0, has been removed.
  Use :c:macro:`K_KERNEL_STACK_MEMBER` instead.
* ``CBPRINTF_PACKAGE_COPY_*`` macros, deprecated since Zephyr 3.5.0, have been removed.
* ``_ENUM_TOKEN`` and ``_ENUM_UPPER_TOKEN`` macros, deprecated since Zephyr 2.7.0,
  are no longer generated.

Deprecated in this release
==========================

* Deprecated the :c:func:`net_buf_put` and :c:func:`net_buf_get` API functions in favor of
  :c:func:`k_fifo_put` and :c:func:`k_fifo_get`.

* The :ref:`kscan_api` subsystem has been marked as deprecated.

* The TinyCrypt library was marked as deprecated (:github:`79566`). The reasons
  for this are (:github:`43712``):

  * the upstream version of this library is unmaintained.

  * to reduce the number of crypto libraries available in Zephyr (currently there are
    3 different implementations: TinyCrypt, MbedTLS and PSA Crypto APIs).

  The PSA Crypto API is now the de-facto standard to perform crypto operations.

Architectures
*************

* ARC

* ARM

* ARM64

  * Added initial support for :c:func:`arch_stack_walk` that supports unwinding via esf only

  * Added support for demand paging.

  * Added support for Linkable Loadable Extensions (LLEXT).

* RISC-V

  * The stack traces upon fatal exception now prints the address of stack pointer (sp) or frame
    pointer (fp) depending on the build configuration.

  * When :kconfig:option:`CONFIG_EXTRA_EXCEPTION_INFO` is enabled, the exception stack frame (arch_esf)
    has an additional field ``csf`` that points to the callee-saved-registers upon an fatal error,
    which can be accessed in :c:func:`k_sys_fatal_error_handler` by ``esf->csf``.

    * For SoCs that select ``RISCV_SOC_HAS_ISR_STACKING``, the ``SOC_ISR_STACKING_ESF_DECLARE`` has to
      include the ``csf`` member, otherwise the build would fail.

* Xtensa

* x86

  * Added initial support for :c:func:`arch_stack_walk` that supports unwinding via esf only

Kernel
******

* Devicetree devices are now exported to :ref:`llext`.

Bluetooth
*********

* Audio

  * :c:func:`bt_tbs_client_register_cb` now supports multiple listeners and may now return an error.

  * Added APIs for getting and setting the assisted listening stream values in codec capabilities
    and codec configuration:

    * :c:func:`bt_audio_codec_cfg_meta_get_assisted_listening_stream`
    * :c:func:`bt_audio_codec_cfg_meta_set_assisted_listening_stream`
    * :c:func:`bt_audio_codec_cap_meta_get_assisted_listening_stream`
    * :c:func:`bt_audio_codec_cap_meta_set_assisted_listening_stream`

  * Added APIs for getting and setting the broadcast name in codec capabilities
    and codec configuration:

    * :c:func:`bt_audio_codec_cfg_meta_get_broadcast_name`
    * :c:func:`bt_audio_codec_cfg_meta_set_broadcast_name`
    * :c:func:`bt_audio_codec_cap_meta_get_broadcast_name`
    * :c:func:`bt_audio_codec_cap_meta_set_broadcast_name`

* Host

  * Added API :c:func:`bt_gatt_get_uatt_mtu` to get current Unenhanced ATT MTU of a given
    connection (experimental).
  * Added :kconfig:option:`CONFIG_BT_CONN_TX_NOTIFY_WQ`.
    The option allows using a separate workqueue for connection TX notify processing
    (:c:func:`bt_conn_tx_notify`) to make Bluetooth stack more independent from the system workqueue.

  * The host now disconnects from the peer upon ATT timeout.

  * Added a warning to :c:func:`bt_conn_le_create` and :c:func:`bt_conn_le_create_synced` if
    the connection pointer passed as an argument is not NULL.

  * Added Kconfig option :kconfig:option:`CONFIG_BT_CONN_CHECK_NULL_BEFORE_CREATE` to enforce
    :c:func:`bt_conn_le_create` and :c:func:`bt_conn_le_create_synced` return an error if the
    connection pointer passed as an argument is not NULL.

* Controller

  * Added Periodic Advertising Sync Transfer (PAST) support with support for both sending and receiving roles.
    The option can be enabled by :kconfig:option:`CONFIG_BT_CTLR_SYNC_TRANSFER_SENDER` and
    :kconfig:option:`CONFIG_BT_CTLR_SYNC_TRANSFER_RECEIVER`.

* HCI Drivers

* Mesh

  * Introduced a mesh-specific workqueue to increase reliability of the mesh messages
    transmission. To get the old behavior enable :kconfig:option:`CONFIG_BT_MESH_WORKQ_SYS`.

Boards & SoC Support
********************

* Added support for these SoC series:

  * Added ESP32-C2 and ESP8684 SoC support.

* Made these changes in other SoC series:

  * NXP S32Z270: Added support for the new silicon cut version 2.0. Note that the previous
    versions (1.0 and 1.1) are no longer supported.
  * Added ESP32 WROVER-E-N16R4 variant.

* Added support for these boards:

   * :zephyr:board:`01space ESP32C3 0.42 OLED <esp32c3_042_oled>` (``esp32c3_042_oled``)
   * :zephyr:board:`ADI MAX32662EVKIT <max32662evkit>` (``max32662evkit``)
   * :zephyr:board:`ADI MAX32666EVKIT <max32666evkit>` (``max32666evkit``)
   * :zephyr:board:`ADI MAX32666FTHR <max32666fthr>` (``max32666fthr``)
   * :zephyr:board:`ADI MAX32675EVKIT <max32675evkit>` (``max32675evkit``)
   * :zephyr:board:`ADI MAX32690FTHR <max32690fthr>` (``max32690fthr``)
   * :ref:`Arduino Nicla Vision <arduino_nicla_vision_board>` (``arduino_nicla_vision``)
   * :zephyr:board:`BeagleBone AI-64 <beaglebone_ai64>` (``beaglebone_ai64``)
   * :zephyr:board:`BeaglePlay (CC1352) <beagleplay>` (``beagleplay``)
   * :zephyr:board:`DPTechnics Walter <walter>` (``walter``)
   * :zephyr:board:`Espressif ESP32-C3-DevKitC <esp32c3_devkitc>` (``esp32c3_devkitc``)
   * :zephyr:board:`Espressif ESP32-C3-DevKit-RUST <esp32c3_rust>` (``esp32c3_rust``)
   * :zephyr:board:`Espressif ESP32-S3-EYE <esp32s3_eye>` (``esp32s3_eye``)
   * :zephyr:board:`Espressif ESP8684-DevKitM <esp8684_devkitm>` (``esp8684_devkitm``)
   * :zephyr:board:`Gardena Smart Garden Radio Module <sgrm>` (``sgrm``)
   * :zephyr:board:`mikroe STM32 M4 Clicker <mikroe_stm32_m4_clicker>` (``mikroe_stm32_m4_clicker``)
   * :ref:`Nordic Semiconductor nRF54L15 DK <nrf54l15dk_nrf54l15>` (``nrf54l15dk``)
   * :ref:`Nordic Semiconductor nRF54L20 PDK <nrf54l20pdk_nrf54l20>` (``nrf54l20pdk``)
   * :ref:`Nordic Semiconductor nRF7002 DK <nrf7002dk_nrf5340>` (``nrf7002dk``)
   * :zephyr:board:`Nuvoton NPCM400_EVB <npcm400_evb>` (``npcm400_evb``)
   * :zephyr:board:`NXP FRDM-MCXA156 <frdm_mcxa156>` (``frdm_mcxa156``)
   * :zephyr:board:`NXP FRDM-MCXC242 <frdm_mcxc242>` (``frdm_mcxc242``)
   * :zephyr:board:`NXP FRDM-MCXC444 <frdm_mcxc444>` (``frdm_mcxc444``)
   * :zephyr:board:`NXP FRDM-MCXN236 <frdm_mcxn236>` (``frdm_mcxn236``)
   * :zephyr:board:`NXP FRDM-MCXW71 <frdm_mcxw71>` (``frdm_mcxw71``)
   * :zephyr:board:`NXP i.MX95 EVK <imx95_evk>` (``imx95_evk``)
   * :zephyr:board:`NXP MIMXRT1180-EVK <mimxrt1180_evk>` (``mimxrt1180_evk``)
   * :ref:`PHYTEC phyBOARD-Nash i.MX93 <phyboard_nash>` (``phyboard_nash``)
   * :ref:`Renesas RA2A1 Evaluation Kit <ek_ra2a1>` (``ek_ra2a1``)
   * :ref:`Renesas RA4E2 Evaluation Kit <ek_ra4e2>` (``ek_ra4e2``)
   * :ref:`Renesas RA4M2 Evaluation Kit <ek_ra4m2>` (``ek_ra4m2``)
   * :ref:`Renesas RA4M3 Evaluation Kit <ek_ra4m3>` (``ek_ra4m3``)
   * :ref:`Renesas RA4W1 Evaluation Kit <ek_ra4w1>` (``ek_ra4w1``)
   * :ref:`Renesas RA6E2 Evaluation Kit <ek_ra6e2>` (``ek_ra6e2``)
   * :ref:`Renesas RA6M1 Evaluation Kit <ek_ra6m1>` (``ek_ra6m1``)
   * :ref:`Renesas RA6M2 Evaluation Kit <ek_ra6m2>` (``ek_ra6m2``)
   * :ref:`Renesas RA6M3 Evaluation Kit <ek_ra6m3>` (``ek_ra6m3``)
   * :ref:`Renesas RA6M4 Evaluation Kit <ek_ra6m4>` (``ek_ra6m4``)
   * :ref:`Renesas RA6M5 Evaluation Kit <ek_ra6m5>` (``ek_ra6m5``)
   * :ref:`Renesas RA8D1 Evaluation Kit <ek_ra8d1>` (``ek_ra8d1``)
   * :ref:`Renesas RA6E1 Fast Prototyping Board <fpb_ra6e1>` (``fpb_ra6e1``)
   * :ref:`Renesas RA6E2 Fast Prototyping Board <fpb_ra6e2>` (``fpb_ra6e2``)
   * :ref:`Renesas RA8T1 Evaluation Kit <mcb_ra8t1>` (``mck_ra8t1``)
   * :zephyr:board:`Renode Cortex-R8 Virtual <cortex_r8_virtual>` (``cortex_r8_virtual``)
   * :zephyr:board:`Seeed XIAO ESP32-S3 Sense Variant <xiao_esp32s3>`: ``xiao_esp32s3``.
   * :ref:`sensry.io Ganymed Break-Out-Board (BOB) <ganymed_bob>` (``ganymed_bob``)
   * :zephyr:board:`SiLabs SiM3U1xx 32-bit MCU USB Development Kit <sim3u1xx_dk>` (``sim3u1xx_dk``)
   * :ref:`SparkFun Thing Plus Matter <sparkfun_thing_plus_mgm240p>` (``sparkfun_thing_plus_matter_mgm240p``)
   * :zephyr:board:`ST Nucleo G431KB <nucleo_g431kb>` (``nucleo_g431kb``)
   * :zephyr:board:`ST Nucleo H503RB <nucleo_h503rb>` (``nucleo_h503rb``)
   * :zephyr:board:`ST Nucleo H755ZI-Q <nucleo_h755zi_q>` (``nucleo_h755zi_q``)
   * :zephyr:board:`ST Nucleo U031R8 <nucleo_u031r8>` (``nucleo_u031r8``)
   * :zephyr:board:`ST Nucleo U083RC <nucleo_u083rc>` (``nucleo_u083rc``)
   * :zephyr:board:`ST Nucleo WB05KZ <nucleo_wb05kz>` (``nucleo_wb05kz``)
   * :zephyr:board:`ST Nucleo WB09KE <nucleo_wb09ke>` (``nucleo_wb09ke``)
   * :zephyr:board:`ST STM32U083C-DK <stm32u083c_dk>` (``stm32u083c_dk``)
   * :zephyr:board:`TI CC1352P7 LaunchPad <cc1352p7_lp>` (``cc1352p7_lp``)
   * :zephyr:board:`vcc-gnd YD-STM32H750VB <yd_stm32h750vb>` (``yd_stm32h750vb``)
   * :zephyr:board:`WeAct Studio STM32F405 Core Board V1.0 <weact_stm32f405_core>` (``weact_stm32f405_core``)
   * :zephyr:board:`WeAct Studio USB2CANFDV1 <usb2canfdv1>` (``usb2canfdv1``)
   * :zephyr:board:`Witte Technology Linum Board <linum>` (``linum``)

* Made these board changes:

  * :ref:`native_posix<native_posix>` has been deprecated in favour of
    :ref:`native_sim<native_sim>`.
  * The nrf54l15bsim target now includes models of the AAR, CCM and ECB peripherals, and many
    other improvements.
  * Support for Google Kukui EC board (``google_kukui``) has been dropped.
  * STM32: Deprecated MCO configuration via Kconfig in favour of setting it through devicetree.
    See ``samples/boards/stm32/mco`` sample.
  * Removed the ``nrf54l15pdk`` board, use :ref:`nrf54l15dk_nrf54l15` instead.
  * PHYTEC: ``mimx8mp_phyboard_pollux`` has been renamed to :ref:`phyboard_pollux<phyboard_pollux>`,
    with the old name marked as deprecated.
  * PHYTEC: ``mimx8mm_phyboard_polis`` has been renamed to :ref:`phyboard_polis<phyboard_polis>`,
    with the old name marked as deprecated.
  * The board qualifier for MPS3/AN547 is changed from:

    * ``mps3/an547`` to ``mps3/corstone300/an547`` for secure and
    * ``mps3/an547/ns`` to ``mps3/corstone300/an547/ns`` for non-secure.

  * Added Thingy53 forwarding of network core pins to network core for SPI peripheral (disabled
    by default) including pin mappings.

* Added support for the following shields:

  * :ref:`ADI EVAL-ADXL362-ARDZ <eval_adxl362_ardz>`
  * :ref:`ADI EVAL-ADXL372-ARDZ <eval_adxl372_ardz>`
  * :ref:`Digilent Pmod ACL <pmod_acl>`
  * :ref:`MikroElektronika BLE TINY Click <mikroe_ble_tiny_click_shield>`
  * :ref:`Nordic SemiConductor nRF7002 EB <nrf7002eb>`
  * :ref:`Nordic SemiConductor nRF7002 EK <nrf7002ek>`
  * :ref:`ST X-NUCLEO-WB05KN1: BLE expansion board <x-nucleo-wb05kn1>`
  * :ref:`WeAct Studio MiniSTM32H7xx OV2640 Camera Sensor <weact_ov2640_cam_module>`

Build system and Infrastructure
*******************************

* Added support for .elf files to the west flash command for jlink, pyocd and linkserver runners.

* Extracted pickled EDT generation from gen_defines.py into gen_edt.py. This moved the following
  parameters from the cmake variable ``EXTRA_GEN_DEFINES_ARGS`` to ``EXTRA_GEN_EDT_ARGS``:

   * ``--dts``
   * ``--dtc-flags``
   * ``--bindings-dirs``
   * ``--dts-out``
   * ``--edt-pickle-out``
   * ``--vendor-prefixes``
   * ``--edtlib-Werror``

* Switched to using imgtool directly from the build system when signing images instead of calling
  ``west sign``.

* Added support for selecting MCUboot operating mode in sysbuild using ``SB_CONFIG_MCUBOOT_MODE``.

* Added support for RAM-load MCUboot operating mode in build system, including sysbuild support.

Documentation
*************

* Added a new :ref:`interactive board catalog <boards>` enabling users to search boards by criteria
  such as name, architecture, vendor, or SoC.
* Added a new :zephyr:code-sample-category:`interactive code sample catalog <samples>` for quickly
  finding code samples based on name and description.
* Added :rst:dir:`zephyr:board` directive and :rst:role:`zephyr:board` role to mark Sphinx pages as
  board documentation and reference them from other pages. Most existing board documentation pages
  have been updated to use this directive, with full migration planned for the next release.
* Added :rst:dir:`zephyr:code-sample-category` directive to describe and group code samples in the
  documentation.
* Added a link to the source code of the driver matching a binding's compatible string (when one can
  be found in the Zephyr tree) to the :ref:`dt-bindings` documentation.
* Added a button to all code sample README pages allowing to directly browse the sample's source
  code on GitHub.
* Moved Zephyr C API documentation out of main documentation. API references now feature a rich
  tooltip and link to the dedicated Doxygen site.
* Added two new build commands, ``make html-live`` and ``make html-live-fast``, that automatically
  locally host the generated documentation. They also automatically rebuild and rehost the
  documentation when changes to the input ``.rst`` files are detected on the filesystem.

Drivers and Sensors
*******************

* ADC

  * Added proper ADC2 calibration entries in ESP32.
  * Fixed calibration scheme in ESP32-S3.

* Battery

* CAN

  * Added initial support for Renesas RA CANFD (:dtcompatible:`renesas,ra-canfd-global`,
    :dtcompatible:`renesas,ra-canfd`)
  * Added Flexcan support for S32Z27x (:dtcompatible:`nxp,flexcan`, :dtcompatible:`nxp,flexcan-fd`)
  * Improved NXP S32 CANXL error reporting (:dtcompatible:`nxp,s32-canxl`)

* Charger

* Clock control

* Comparator

  * Introduced comparator device driver subsystem selected with :kconfig:option:`CONFIG_COMPARATOR`
  * Introduced comparator shell commands selected with :kconfig:option:`CONFIG_COMPARATOR_SHELL`
  * Added support for Nordic nRF COMP (:dtcompatible:`nordic,nrf-comp`)
  * Added support for Nordic nRF LPCOMP (:dtcompatible:`nordic,nrf-lpcomp`)
  * Added support for NXP Kinetis ACMP (:dtcompatible:`nxp,kinetis-acmp`)

* Counter

* DAC

* Disk

* Display

* EEPROM

  * Added support for using the EEPROM simulator with embedded C standard libraries
    (:dtcompatible:`zephyr,sim-eeprom`).

* Ethernet

  * LiteX: Renamed the ``compatible`` from ``litex,eth0`` to :dtcompatible:`litex,liteeth`.

* Flash

  * Fixed SPI NOR driver issue where wp, hold and reset pins were incorrectly initialized from
    device tee when SFDP at run-time has been enabled (:github:`80383`)
  * Updated all Espressif's SoC driver initialization to allow new chipsets and octal flash support.

  * Added :kconfig:option:`CONFIG_SPI_NOR_ACTIVE_DWELL_MS`, to the SPI NOR driver configuration,
    which allows setting the time during which the driver will wait before triggering Deep Power Down (DPD).
    This option replaces ``CONFIG_SPI_NOR_IDLE_IN_DPD``, aiming at reducing unnecessary power
    state changes and SPI transfers between other operations, specifically when burst type
    access to an SPI NOR device occurs.

  * Added :kconfig:option:`CONFIG_SPI_NOR_INIT_PRIORITY` to allow selecting the SPI NOR driver initialization priority.

  * The flash API has been extended with the :c:func:`flash_copy` utility function which allows performing
    direct data copies between two Flash API devices.

  * Fixed a Flash Simulator issue where offsets were assumed to be absolute instead of relative
    to the device base address (:github:`79082`).

* GNSS

* GPIO

  * tle9104: Add support for the parallel output mode via setting the properties ``parallel-out12`` and
    ``parallel-out34``.

* Hardware info

* I2C

* I2S

  * Added ESP32-S3 and ESP32-C3 driver support.

* I3C

* Input

  * New feature: :dtcompatible:`zephyr,input-double-tap`.

  * New driver: :dtcompatible:`ilitek,ili2132a`.

  * Added power management support to all keyboard matrix drivers, added a
    ``no-disconnect`` property to :dtcompatible:`gpio-keys` so it can be used
    with power management on GPIO drivers that do not support pin
    disconnection.

  * Added a new framework for touchscreen common properties and features
    (screen size, inversion, xy swap).

  * Fixed broken ESP32 input touch sensor driver.

* Interrupt

  * Updated ESP32 family interrupt allocator with proper IRQ flags and priorities.

* LED

  * lp5562: added ``enable-gpios`` property to describe the EN/VCC GPIO of the lp5562.

  * lp5569: added ``charge-pump-mode`` property to configure the charge pump of the lp5569.

  * lp5569: added ``enable-gpios`` property to describe the EN/PWM GPIO of the lp5569.

  * LED code samples have been consolidated under the :zephyr_file:`samples/drivers/led` directory.

* LED Strip

  * Updated ws2812 GPIO driver to support dynamic bus timings

* LoRa

* Mailbox

  * Added driver support for ESP32 and ESP32-S3 SoCs.

* MDIO

* MFD

* Modem

  * Added support for the U-Blox LARA-R6 modem.
  * Added support for setting the modem's UART baudrate during init.

* MIPI-DBI

* MSPI

* Pin control

  * Added support for Microchip MEC5
  * Added SCMI-based driver for NXP i.MX
  * Added support for i.MX93 M33 core
  * Added support for ESP32C2

* PWM

  * rpi_pico: The driver now configures the divide ratio adaptively.

* Regulators

  * Upgraded CP9314 driver to B1 silicon revision
  * Added basic driver for MPS MPM54304

* Reset

* RTC

* RTIO

* SDHC

  * Added ESP32-S3 driver support.

* Sensors

  * The existing driver for the Microchip MCP9808 temperature sensor transformed and renamed
    to support all JEDEC JC 42.4 compatible temperature sensors. It now uses the
    :dtcompatible:`jedec,jc-42.4-temp` compatible string instead to the ``microchip,mcp9808``
    string.

  * WE

    * Added Würth Elektronik HIDS-2525020210002
      :dtcompatible:`we,wsen-hids-2525020210002` humidity sensor driver.

* Serial

  * LiteX: Renamed the ``compatible`` from ``litex,uart0`` to :dtcompatible:`litex,uart`.
  * Nordic: Removed ``CONFIG_UART_n_GPIO_MANAGEMENT`` Kconfig options (where n is an instance
    index) which had no use after pinctrl driver was introduced.
  * NS16550: Added support for Synopsys Designware 8250 UART.
  * Renesas: Added support for SCI UART.
  * Sensry: Added UART support for Ganymed SY1XX.

* SPI

* Steppers

  * Introduced stepper controller device driver subsystem selected with
    :kconfig:option:`CONFIG_STEPPER`
  * Introduced stepper shell commands for controlling and configuring
    stepper motors with :kconfig:option:`CONFIG_STEPPER_SHELL`
  * Added support for ADI TMC5041 (:dtcompatible:`adi,tmc5041`)
  * Added support for gpio-stepper-controller (:dtcompatible:`gpio-stepper-controller`)
  * Added stepper api test-suite
  * Added stepper shell test-suite

* USB

* Video

  * Introduced API to control frame rate
  * Introduced API for partial frames transfer with the video buffer field ``line_offset``
  * Introduced API for :ref:`multi-heap<memory_management_shared_multi_heap>` video buffer allocation with
    :kconfig:option:`CONFIG_VIDEO_BUFFER_USE_SHARED_MULTI_HEAP`
  * Introduced bindings for common video link properties in ``video-interfaces.yaml``
  * Introduced missing :kconfig:option:`CONFIG_VIDEO_LOG_LEVEL`
  * Added support for GalaxyCore GC2145 image sensor (:dtcompatible:`gc,gc2145`)
  * Added support for ESP32-S3 LCD-CAM interface (:dtcompatible:`espressif,esp32-lcd-cam`)
  * Added support for NXP MCUX SMARTDMA interface (:dtcompatible:`nxp,smartdma`)
  * Added support for more OmniVision OV2640 controls (:dtcompatible:`ovti,ov2640`)
  * Added support for more OmniVision OV5640 controls (:dtcompatible:`ovti,ov5640`)

* Watchdog

* Wi-Fi

  * Added ESP32-C2 Wi-Fi support.
  * Added ESP32 driver APSTA support.
  * Updated ESP32 Wi-Fi driver to reflect actual negotiated PHY mode.

Networking
**********

* 802.15.4:

  * Implemented support for beacons without association bit.
  * Implemented support for beacons payload.
  * Fixed a bug where LL address endianness was swapped twice when deciphering a frame.
  * Fixed missing context lock release when checking destination address.
  * Improved error logging in 6LoWPAN fragmentation.
  * Improved error logging in 802.15.4 management commands.

* ARP:

  * Fixed ARP probe verification during IPv4 address conflict detection.

* CoAP:

  * Added new API :c:func:`coap_rst_init` to simplify creating RST replies.
  * Implemented replying with CoAP RST response for unknown queries in CoAP client.
  * Added support for runtime configuration of ACK random factor parameter.
  * Added support for No Response CoAP option.
  * Added a new sample demonstrating downloading a resource with GET request.
  * Fixed handling of received CoAP RST reply in CoAP client.
  * Fixed socket error reporting to the application in CoAP client.
  * Fixed handling of response retransmissions in CoAP client.
  * Fixed a bug where CoAP block numbers were limited to ``uint8_t``.
  * Various fixes in the block transfer support in CoAP client.
  * Improved handling of truncated datagrams in CoAP client.
  * Improved thread safety of CoAP client.
  * Fixed missing ``static`` keyword in some internal functions.
  * Various other minor fixes in CoAP client.

* DHCPv4:

  * Added support for parsing multiple DNS servers received from DHCP server.
  * Added support for DNS Server option in DHCPv4 server.
  * Added support for Router option in DHCPv4 server.
  * Added support for application callback which allows to assign custom addresses
    in DHCPv4 server.
  * Fixed DNS server list allocation in DHCPv4 client.
  * Fixed a bug where system workqueue could be blocked indefinitely by DHCPv4 client.

* DHCPv6:

  * Fixed a bug where system workqueue could be blocked indefinitely by DHCPv6 client.

* DNS/mDNS/LLMNR:

  * Added support for collecting DNS statistics.
  * Added support for more error codes in :c:func:`zsock_gai_strerror`.
  * Fixed handling of DNS responses encoded with capital letters.
  * Fixed DNS dispatcher operation on multiple network interfaces.
  * Fixed error being reported for mDNS queries with query count equal to 0.
  * Various other minor fixes in DNS/mDNS implementations.

* Ethernet:

* gPTP/PTP:

  * Fixed handling of second overflow/underflow.
  * Fixed PTP clock adjusting with offset.

* HTTP:

  * Added support for specifying response headers and response code by the application.
  * Added support for netusb in the HTTP server sample.
  * Added support for accessing HTTP request headers from the application callback.
  * Added support for handling IPv4 connections over IPv6 socket in HTTP server.
  * Added support for creating HTTP server instances without specifying local host.
  * Added overlays to support HTTP over IEEE 802.15.4 for HTTP client and server
    samples.
  * Added support for static filesystem resources in HTTP server.
  * Fixed assertion in HTTP server sample when resource upload was aborted.
  * Refactored dynamic resource callback format for easier handling of short
    requests/replies.
  * Fixed possible busy-looping in case of errors in the HTTP server sample.
  * Fixed possible incorrect HTTP headers matching in HTTP server.
  * Refactored HTTP server sample to better demonstrate server use cases.
  * Fixed processing of multiple HTTP/1 requests over the same connection.
  * Improved HTTP server test coverage.
  * Various other minor fixes in HTTP server.

* IPv4:

  * Improved IGMP test coverage.
  * Fixed IGMPv2 queries processing when IGMPv3 is enabled.
  * Fixed :kconfig:option:`CONFIG_NET_NATIVE_IPV4` dependency for native IPv4 options.
  * Fix net_pkt leak in :c:func:`send_ipv4_fragment`.`

* IPv6:

  * Added a public header for Multicast Listener Discovery APIs.
  * Added new :c:func:`net_ipv6_addr_prefix_mask` API function.
  * Made IPv6 Router Solicitation timeout configurable.
  * Fixed endless IPv6 packet looping with both routing and VLAN support enabled.
  * Fixed unneeded error logging in case of dropped NS packets.
  * Fixed accepting of incoming DAD NS messages.
  * Various fixes improving IPv6 routing.

* LwM2M:

  * Added TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8 to DTLS cipher list.
  * Added LwM2M shell command for listing resources.
  * Added LwM2M shell command to list observations.
  * Added support for accepting SenML-CBOR floats decoded as integers.
  * Added support for X509 hostname verification if using certificates, when
    URI contains valid name.
  * Regenerated generated code files using zcbor 0.9.0 for lwm2m_senml_cbor.
  * Improved thread safety of the LwM2M engine.
  * Fixed block transfer issues for composite operations.
  * Fixed enabler version reporting during bootstrap discovery.
  * Removed unneeded Security object instance from the LwM2M client sample.
  * Fixed buffer size check for U16 resource.
  * Removed deprecated APIs and configs.
  * Optional Location object resources altitude, radius, and speed can now be
    used optionally as per the location object's specification. Users of these
    resources will now need to provide a read buffer.
  * Fixed the retry counter not being reset on successful Registration update.
  * Fixed REGISTRATION_TIMEOUT event not always being emitted on registration
    errors.
  * Fixed c++ support in LwM2M public header.
  * Fixed a bug where DISCONNECTED event was not always emitted when needed.

* Misc:

  * Added support for network packet allocation statistics.
  * Added a new library implementing Prometheus monitoring support.
  * Added USB CDC NCM support for Echo Server sample.
  * Added packet drop statistics for capture interfaces.
  * Added new :c:func:`net_hostname_set_postfix_str` API function to set hostname
    postfix in non-hexadecimal format.
  * Added API version information to public networking headers.
  * Implemented optional periodic SNTP time resynchronization.
  * Improved error reporting when starting/stopping virtual interfaces.
  * Fixed build error of packet capture library when variable sized buffers are used.
  * Fixed build error of packet capture library when either IPv4 or IPv6 is disabled.
  * Fixed CMake complaint about missing sources in net library in certain
    configurations.
  * Fixed compilation issues with networking and SystemView Tracing enabled.
  * Removed redundant DHCPv4 code from telnet sample.
  * Fixed build warnings in Echo Client sample with IPv6 disabled.
  * Removed deprecated net_pkt functions.
  * Extended network tracing support and added documentation page
    (:ref:`network_tracing`).
  * Moved network buffers implementation out of net subsystem into lib directory
    and renamed public header to :zephyr_file:`include/zephyr/net_buf.h`.
  * Deprecated the :c:func:`net_buf_put` and :c:func:`net_buf_get` API functions.
  * Removed ``wpansub`` sample.

* MQTT:

  * Updated information in the mqtt_publisher sample about Mosquitto broker
    configuration.
  * Updated MQTT tests to be self-contained, no longer requiring external broker.
  * Optimized buffer handling in MQTT encoder/decoder.

* Network contexts:

  * Fixed IPv4 destination address setting when using :c:func:`sendmsg` with
    :kconfig:option:`CONFIG_NET_IPV4_MAPPING_TO_IPV6` option enabled.
  * Fixed possible unaligned memory access when in :c:func:`net_context_bind`.
  * Fixed missing NULL pointer check for V6ONLY option read.

* Network Interface:

  * Added new :c:func:`net_if_ipv4_get_gw` API function.
  * Fixed checksum offloading checks for VLAN interfaces.
  * Fixed native IP support being required to  register IP addresses on an
    interface.
  * Fixed missing mutex locks in a few net_if functions.
  * Fixed rejoining of IPv6 multicast groups.
  * Fixed :c:func:`net_if_send_data` operation for offloaded interfaces.
  * Fixed needless IPv6 multicast groups joining if IPv6 is disabled.
  * Fixed compiler warnings when building with ``-Wtype-limits``.

* OpenThread:

  * Added support for :kconfig:option:`CONFIG_IEEE802154_SELECTIVE_TXCHANNEL`
    option in OpenThread radio platform.
  * Added NAT64 send and receive callbacks.
  * Added new Kconfig options:

    * :kconfig:option:`CONFIG_OPENTHREAD_NAT64_CIDR`
    * :kconfig:option:`CONFIG_OPENTHREAD_STORE_FRAME_COUNTER_AHEAD`
    * :kconfig:option:`CONFIG_OPENTHREAD_DEFAULT_RX_SENSITIVITY`
    * :kconfig:option:`CONFIG_OPENTHREAD_CSL_REQUEST_TIME_AHEAD`

  * Fixed deprecated/preferred IPv6 address state transitions.
  * Fixed handling of deprecated IPv6 addresses.
  * Other various minor fixes in Zephyr's OpenThread port.

* Shell:

  * Added support for enabling/disabling individual network shell commands with
    Kconfig.
  * Added new ``net dhcpv4/6 client`` commands for DHCPv4/6 client management.
  * Added new ``net virtual`` commands for virtual interface management.
  * ``net ipv4/6`` commands are now available even if native IP stack is disabled.
  * Added new ``net cm`` commands exposing Connection Manager functionality.
  * Fixed possible assertion if telnet shell backend connection is terminated.
  * Event monitor thread stack size is now configurable with Kconfig.
  * Relocated ``bridge`` command under ``net`` command, i. e. ``net bridge``.
  * Multiple minor improvements in various command outputs.

* Sockets:

  * Added dedicated ``net_socket_service_handler_t`` callback function type for
    socket services.
  * Added TLS 1.3 support for TLS sockets.
  * Fixed socket leak when closing NSOS socket.
  * Moved socket service library out of experimental.
  * Deprecated ``CONFIG_NET_SOCKETS_POLL_MAX``.
  * Moved ``zsock_poll()`` and ``zsock_select`` implementations into ``zvfs``
    library.
  * Removed ``work_q`` parameter from socket service macros as it was no longer
    used.
  * Separated native INET sockets implementation from socket syscalls so that
    it doesn't have to be built when offloaded sockets are used.
  * Fixed possible infinite block inside TLS socket :c:func:`zsock_connect` when
    peer goes down silently.
  * Fixed ``msg_controllen`` not being set correctly in :c:func:`zsock_recvmsg`.
  * Fixed possible busy-looping when polling TLS socket for POLLOUT event.

* TCP:

  * Fixed propagating connection errors to the socket layer.
  * Improved ACK reply logic when peer does not send PSH flag with data.

* Websocket:

  * Added support for Websocket console in the Echo Server sample.
  * Fixed undefined reference to ``MSG_DONTWAIT`` while building websockets
    without POSIX.

* Wi-Fi:

* zperf:

  * Added support for USB CDC NCM in the zperf sample.
  * Fixed DHCPv4 client not being started in the zperf sample in certain
    configurations.

USB
***

Devicetree
**********

Kconfig
*******

Libraries / Subsystems
**********************

* Debug

* Demand Paging

  * Added LRU (Least Recently Used) eviction algorithm.

  * Added on-demand memory mapping support (:kconfig:option:`CONFIG_DEMAND_MAPPING`).

  * Made demand paging SMP compatible.

* Formatted output

* Management

  * MCUmgr

    * Added support for :ref:`mcumgr_smp_group_10`, which allows for listing information on
      supported groups.
    * Fixed formatting of milliseconds in :c:enum:`OS_MGMT_ID_DATETIME_STR` by adding
      leading zeros.
    * Added support for custom os mgmt bootloader info responses using notification hooks, this
      can be enabled with :kconfig:option:`CONFIG_MCUMGR_GRP_OS_BOOTLOADER_INFO_HOOK`, the data
      structure is :c:struct:`os_mgmt_bootloader_info_data`.
    * Added support for img mgmt slot info command, which allows for listing information on
      images and slots on the device.
    * Added support for LoRaWAN MCUmgr transport, which can be enabled with
      :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_LORAWAN`.

  * hawkBit

    * :c:func:`hawkbit_autohandler` now takes one argument. If the argument is set to true, the
      autohandler will reshedule itself after running. If the argument is set to false, the
      autohandler will not reshedule itself. Both variants are sheduled independent of each other.
      The autohandler always runs in the system workqueue.

    * Use the :c:func:`hawkbit_autohandler_wait` function to wait for the autohandler to finish.

    * Running hawkBit from the shell is now executed in the system workqueue.

    * Use the :c:func:`hawkbit_autohandler_cancel` function to cancel the autohandler.

    * Use the :c:func:`hawkbit_autohandler_set_delay` function to delay the next run of the
      autohandler.

    * The hawkBit header file was separated into multiple header files. The main header file is now
      ``<zephyr/mgmt/hawkbit/hawkbit.h>``, the autohandler header file is now
      ``<zephyr/mgmt/hawkbit/autohandler.h>`` and the configuration header file is now
      ``<zephyr/mgmt/hawkbit/config.h>``.

* Logging

* Modem modules

* Power management

  * Added initial ESP32-C6 power management interface to allow light and deep-sleep features.

* Crypto

  * Mbed TLS was updated to version 3.6.2 (from 3.6.0). The release notes can be found at:

    * https://github.com/Mbed-TLS/mbedtls/releases/tag/mbedtls-3.6.1
    * https://github.com/Mbed-TLS/mbedtls/releases/tag/mbedtls-3.6.2

  * The Kconfig symbol :kconfig:option:`CONFIG_MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG_ALLOW_NON_CSPRNG`
    was added to allow ``psa_get_random()`` to make use of non-cryptographically
    secure random sources when :kconfig:option:`CONFIG_MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG`
    is also enabled. This is only meant to be used for test purposes, not in production.
    (:github:`76408`)
  * The Kconfig symbol :kconfig:option:`CONFIG_MBEDTLS_TLS_VERSION_1_3` was added to
    enable TLS 1.3 support from Mbed TLS. When this is enabled the following
    new Kconfig symbols can also be enabled:

    * :kconfig:option:`CONFIG_MBEDTLS_TLS_SESSION_TICKETS` to enable session tickets
      (RFC 5077);
    * :kconfig:option:`CONFIG_MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK_ENABLED`
      for TLS 1.3 PSK key exchange mode;
    * :kconfig:option:`CONFIG_MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_EPHEMERAL_ENABLED`
      for TLS 1.3 ephemeral key exchange mode;
    * :kconfig:option:`CONFIG_MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK_EPHEMERAL_ENABLED`
      for TLS 1.3 PSK ephemeral key exchange mode.

* CMSIS-NN

* FPGA

* Random

* SD

* Settings

  * Settings has been extended to allow prioritizing the commit handlers using
    ``SETTINGS_STATIC_HANDLER_DEFINE_WITH_CPRIO(...)`` for static_handlers and
    ``settings_register_with_cprio(...)`` for dynamic_handlers.

* Shell:

  * Reorganized the ``kernel threads`` and ``kernel stacks`` shell command under the
    L1 ``kernel thread`` shell command as ``kernel thread list`` & ``kernel thread stacks``
  * Added multiple shell command to configure the CPU mask affinity / pinning a thread in
    runtime, do ``kernel thread -h`` for more info.
  * ``kernel reboot`` shell command without any additional arguments will now do a cold reboot
    instead of requiring you to type ``kernel reboot cold``.

* State Machine Framework

* Storage

  * LittleFS: The module has been updated with changes committed upstream
    from version 2.8.1, the last module update, up to and including
    the released version 2.9.3.

  * LittleFS: Fixed an issue where the DTS option for configuring block cycles for LittleFS instances
    was ignored (:github:`79072`).

  * LittleFS: Fixed issue with lookahead buffer size mismatch to actual allocated buffer size
    (:github:`77917`).

  * FAT FS: Added :kconfig:option:`CONFIG_FILE_SYSTEM_LIB_LINK` to allow linking file system
    support libraries without enabling the File System subsystem. This option can be used
    when a user wants to directly use file system libraries, bypassing the File System
    subsystem.

  * FAT FS: Added :kconfig:option:`CONFIG_FS_FATFS_LBA64` to enable support for the 64-bit LBA
    and GPT in FAT file system driver.

  * FAT FS: Added :kconfig:option:`CONFIG_FS_FATFS_MULTI_PARTITION` that enables support for
    devices partitioned with GPT or MBR.

  * FAT FS: Added :kconfig:option:`CONFIG_FS_FATFS_HAS_RTC` that enables RTC usage for time-stamping
    files on FAT file systems.

  * FAT FS: Added :kconfig:option:`CONFIG_FS_FATFS_EXTRA_NATIVE_API` that enables additional FAT
    file system driver functions, which are not exposed via Zephyr File System subsystem,
    for users that intend to directly call them in their code.

  * Stream Flash: Fixed an issue where :c:func:`stream_flash_erase_page` did not properly check
    the requested erase range and possibly allowed erasing any page on a device (:github:`79800`).

  * Shell: Fixed an issue were a failed file system mount attempt using the shell would make it
    impossible to ever succeed in mounting that file system again until the device was reset (:github:`80024`).

  * :ref:`ZMS<zms_api>`: Introduction of a new storage system that is designed to work with all types of
    non-volatile storage technologies. It supports classical on-chip NOR flash as well as
    new technologies like RRAM and MRAM that do not require a separate erase operation at all.

* Task Watchdog

* POSIX API

* LoRa/LoRaWAN

* ZBus

* JWT (JSON Web Token)

  * The following new symbols were added to allow specifying both the signature
    algorithm and crypto library:

    * :kconfig:option:`CONFIG_JWT_SIGN_RSA_PSA` (default) RSA signature using the PSA Crypto API;
    * :kconfig:option:`CONFIG_JWT_SIGN_RSA_LEGACY` RSA signature using Mbed TLS;
    * :kconfig:option:`CONFIG_JWT_SIGN_ECDSA_PSA` ECDSA signature using the PSA Crypto API.

    (:github:`79653`)

HALs
****

* Nordic

* STM32

* ADI

* Espressif

  * Synced HAL to version v5.1.4 to update SoCs low level files, RF libraries and
    overall driver support.

MCUboot
*******

  * Removed broken target config header feature.
  * Removed ``image_index`` from ``boot_encrypt``.
  * Renamed boot_enc_decrypt to boot_decrypt_key.
  * Updated to use ``EXTRA_CONF_FILE`` instead of the deprecated ``OVERLAY_CONFIG`` argument.
  * Updated ``boot_encrypt()`` to instead be ``boot_enc_encrypt()`` and ``boot_enc_decrypt()``.
  * Updated ``boot_enc_valid`` to take slot instead of image index.
  * Updated ``boot_enc_load()`` to take slot number instead of image.
  * Updated logging to debug level in boot_serial.
  * Updated Kconfig to allow disabling NRFX_WDT on nRF devices.
  * Updated CMake ERROR statements into FATAL_ERROR.
  * Added application version that is being booted output prior to booting it.
  * Added sysbuild support to the hello-world sample.
  * Added SIG_PURE TLV to bootutil.
  * Added write block size checking to bootutil.
  * Added check for unexpected flash sector size.
  * Added SHA512 support to MCUboot code and support for calculating SHA512 hash in imgtool.
  * Added fallback to USB DFU option.
  * Added better mode selection checks to bootutil.
  * Added bootuil protected TLV size to image size check.
  * Added functionaliy to remove images with conflicting flags or where features are required
    that are not supported.
  * Added compressed image flags and TLVs to MCUboot, Kconfig options and support for generating
    compressed LZMA2 images with ARM thumb filter to imgtool.
  * Added image header verification before checking image.
  * Added state to ``boot_is_header_valid()`` function.
  * Added ``CONFIG_MCUBOOT_ENC_BUILTIN_KEY`` Kconfig option.
  * Added non-bootable flag to imgtool.
  * Added zephyr prefix to generated header path.
  * Added optional img mgmt slot info feature.
  * Added bootutil support for maximum image size details for additional images.
  * Added support for automatically calculcating max sectors.
  * Added missing ``boot_enc_init()`` function.
  * Added support for keeping image encrypted in scratch area in bootutil.
  * Fixed serial recovery for NXP IMX.RT, LPC55x and MCXNx platforms
  * Fixed issue with public RSA signing in imgtool.
  * Fixed issue with ``boot_serial_enter()`` being defined but not used warning.
  * Fixed issue with ``main()`` in sample returning wrong type warning.
  * Fixed issue with using pointers in bootutil.
  * Fixed wrong usage of slot numbers in boot_serial.
  * Fixed slot info for directXIP/RAM load in bootutil.
  * Fixed bootutil issue with not zeroing AES and SHA-256 contexts with mbedTLS.
  * Fixed boot_serial ``format`` and ``incompatible-pointer-types`` warnings.
  * Fixed booltuil wrong definition of ``find_swap_count``.
  * Fixed bootutil swap move max app size calculation.
  * Fixed imgtool issue where getpub failed for ed25519 key.
  * Fixed issue with sysbuild if something else is named mcuboot.
  * Fixed RAM load chain load address.
  * Fixed issue with properly retrieving image headers after interrupted swap-scratch in bootutil.
  * The MCUboot version in this release is version ``2.1.0+0-dev``.

OSDP
****

Trusted Firmware-M (TF-M)
*************************

* TF-M was updated to version 2.1.1 (from 2.1.0).
  The release notes can be found at: https://trustedfirmware-m.readthedocs.io/en/tf-mv2.1.1/releases/2.1.1.html

LVGL
****

zcbor
*****

* Updated the zcbor library to version 0.9.0.
  Full release notes at https://github.com/NordicSemiconductor/zcbor/blob/0.9.0/RELEASE_NOTES.md
  Migration guide at https://github.com/NordicSemiconductor/zcbor/blob/0.9.0/MIGRATION_GUIDE.md
  Highlights:

    * Many code generation bugfixes

    * You can now decide at run-time whether the decoder should enforce canonical encoding.

    * Allow --file-header to accept a path to a file with header contents

Tests and Samples
*****************

* Together with the deprecation of :ref:`native_posix<native_posix>`, many tests which were
  explicitly run in native_posix now run in :ref:`native_sim<native_sim>` instead.
  native_posix as a platform remains tested though.

Issue Related Items
*******************

Known Issues
============

- :github:`71042` stream_flash: stream_flash_init() size parameter allows to ignore partition layout
- :github:`67407` stream_flash: stream_flash_erase_page allows to accidentally erase stream
