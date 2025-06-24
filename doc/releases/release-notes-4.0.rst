:orphan:

.. _zephyr_4.0:

Zephyr 4.0.0
############

We are pleased to announce the release of Zephyr version 4.0.0.

Major enhancements with this release include:

* **Secure Storage Subsystem**:
  A newly introduced :ref:`secure storage<secure_storage>` subsystem allows the use of the
  PSA Secure Storage API and of persistent keys in the PSA Crypto API on *all* board targets. It
  is now the standard way to provide device-specific protection to data at rest. (:github:`76222`)

* **ZMS (Zephyr Memory Storage) Subsystem**:
  :ref:`ZMS <zms_api>` is a new key-value storage subsystem compatible with all non-volatile storage
  types, including traditional NOR flash and advanced technologies like RRAM and MRAM that support
  write without erasure.

* **Analog Comparators**:
  A new :ref:`comparator<comparator_api>` device driver subsystem for analog comparators has been
  added, complete with shell support. It supports initial configuration through Devicetree and
  runtime configuration through vendor specific APIs. Initially the :dtcompatible:`nordic,nrf-comp`,
  :dtcompatible:`nordic,nrf-lpcomp` and :dtcompatible:`nxp,kinetis-acmp` are supported.

* **Stepper Motors**:
  It is now possible to interact with stepper motors using a standard API thanks to the new
  :ref:`stepper<stepper_api>` device driver subsystem, which also comes with shell support.
  Initially implemented drivers include a simple :dtcompatible:`zephyr,gpio-steppers` and a complex
  sensor-less stall-detection capable with integrated ramp-controller :dtcompatible:`adi,tmc5041`.

* **Haptics**:
  A new :ref:`haptics_api` device driver subsystem allows unified access to haptic controllers,
  enabling users to add haptic feedback to their applications.

* **Multimedia Capabilities**
  Zephyr's audio and video capabilities have been expanded with support for new image sensors, video
  interfaces, audio interfaces, and codecs being supported.

* **Prometheus Library**:
  A `Prometheus`_ metrics library has been added to the networking stack. It provides a way to
  expose metrics to Prometheus clients over HTTP, facilitating the consolidated remote monitoring of
  Zephyr devices alongside other systems typically monitored using Prometheus.

* **Documentation Improvements**:
  Several enhancements were made to the online documentation to improve content discovery and
  navigation. These include a new :ref:`interactive board catalog <boards>` and an interactive
  directory for :zephyr:code-sample-category:`code samples <samples>`.

* **Expanded Board Support**:
  Over 60 :ref:`new boards <boards_added_in_zephyr_4_0>` and
  :ref:`shields <shields_added_in_zephyr_4_0>` are supported in Zephyr 4.0.

.. _`Prometheus`: https://prometheus.io/

An overview of the changes required or recommended when migrating your application from Zephyr
v3.7.0 to Zephyr v4.0.0 can be found in the separate :ref:`migration guide<migration_4.0>`.

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************
The following CVEs are addressed by this release:

More detailed information can be found in:
https://docs.zephyrproject.org/latest/security/vulnerabilities.html

* :cve:`2024-8798`: Under embargo until 2024-11-22
* :cve:`2024-10395`: Under embargo until 2025-01-23
* :cve:`2024-11263` `Zephyr project bug tracker GHSA-jjf3-7x72-pqm9
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-jjf3-7x72-pqm9>`_

API Changes
***********

Removed APIs in this release
============================

* Macro ``K_THREAD_STACK_MEMBER``, deprecated since v3.5.0, has been removed.
  Use :c:macro:`K_KERNEL_STACK_MEMBER` instead.

* ``CBPRINTF_PACKAGE_COPY_*`` macros, deprecated since Zephyr 3.5.0, have been removed.

* ``_ENUM_TOKEN`` and ``_ENUM_UPPER_TOKEN`` macros, deprecated since Zephyr 2.7.0,
  are no longer generated.

* Removed deprecated arch-level CMSIS header files
  ``include/zephyr/arch/arm/cortex_a_r/cmsis.h`` and
  ``include/zephyr/arch/arm/cortex_m/cmsis.h``. ``cmsis_core.h`` needs to be
  included now.

* Removed deprecated ``ceiling_fraction`` macro. :c:macro:`DIV_ROUND_UP` needs
  to be used now.

* Removed deprecated header file
  ``include/zephyr/random/rand32.h``. ``random.h`` needs to be included now.

* Deprecated ``EARLY``, ``APPLICATION`` and ``SMP`` init levels can no longer be
  used for devices.

* Removed deprecated net_pkt functions.

Deprecated in this release
==========================

* Deprecated the :c:func:`net_buf_put` and :c:func:`net_buf_get` API functions in favor of
  :c:func:`k_fifo_put` and :c:func:`k_fifo_get`.

* The :ref:`kscan_api` subsystem has been marked as deprecated.

* Deprecated the TinyCrypt shim driver ``CONFIG_CRYPTO_TINYCRYPT_SHIM``.

* ``native_posix`` has been deprecated in favour of
  :ref:`native_sim<native_sim>`.

* ``include/zephyr/net/buf.h`` is deprecated in favor of
  ``include/zephyr/net_buf.h>``. The old header will be removed in future releases
  and its usage should be avoided.

* Deprecated the :c:func:`net_buf_put` and :c:func:`net_buf_get` API functions.

Architectures
*************

* ARC

* ARM

  * Added support of device memory attributes on Cortex-M (arm_mpu_v8)

* ARM64

  * Added initial support for :c:func:`arch_stack_walk` that supports unwinding via esf only
  * Added sys_arch_reboot() support to ARM64

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

  * Fixed an ltk derive issue in L2CAP
  * Added listener callback for discovery (BR)
  * Corrected BR bonding type (SSP)
  * Added support for non-bondable mode (SSP)
  * Changed SSP so that no MITM if required level is less than L3
  * Added checking the receiving buffer length before pulling data (AVDTP)
  * Added support of security level 4 to SSP
  * Fixed LE LTK cannot be derived
  * Added support for Multi-Command Packet (l2cap)
  * Improved the L2CAP code to Set flags in CFG RSP
  * Improved the L2CAP code to handle all configuration options
  * Improved the SSP code to clear pairing flag if ssp pairing completed area
  * Improved the SMP code to check if remote supports CID 0x0007
  * Added support for SMP CT2 flag
  * Improved the SSP code so the proper callback is called when pairing fails

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
  * Added STM32U0 series with GPIO, Serial, I2C, DAC, ADC, flash, PWM and counter driver support.
  * Added STM32WB0 series with GPIO, Serial, I2C, SPI, ADC, DMA and flash driver support.
  * Added STM32U545xx SoC variant.
  * Added NXP i.MX93's Cortex-M33 core
  * Added NXP MCXW71, MCXC242, MCXA156, MCXN236, MCXC444, RT1180

* Made these changes in other SoC series:

  * NXP S32Z270: Added support for the new silicon cut version 2.0. Note that the previous
    versions (1.0 and 1.1) are no longer supported.
  * NXP s32k3: fixed RAM retention issue
  * NXP s32k1: obtain system clock frequency from Devicetree
    versions (1.0 and 1.1) are no longer supported.
  * Added ESP32 WROVER-E-N16R4 variant.
  * STM32H5: Added support for OpenOCD through STMicroelectronics OpenOCD fork.
  * MAX32: Enabled Segger RTT and SystemView support.
  * Silabs Series 2: Use oscillator, clock and DCDC configuration from device tree during init.
  * Silabs Series 2: Added initialization for SMU (Security Management Unit).
  * Silabs Series 2: Use sleeptimer as the default OS timer instead of systick.
  * NXP i.MX8MP: Enable the IRQ_STEER interrupt controller.
  * NXP RWxxx:

      * added additional support to Wakeup from low power modes
      * RW61x: increased main stack size to avoid stack overflows when running BLE
      * RW612: enabled SCTIMER

  * NXP IMXRT: Fixed flexspi boot issue caused by an erroneous relocation of the Flash Configuration Block
    of Kconfig defaults being sourced
  * NXP RT11xx: enabled FlexIO
  * NXP IMXRT116x: Fixed bus clocking to align with the settings of the MCUXpresso SDK
  * NXP mimxrt685: fixed clocks to enable DMIC
  * NXP MCX N Series: Fixed NXP LPSPI native chip select when using synchronous API with DMA bug
  * Nordic nRF54H: Added support for the FLPR (Fast Lightweight Processor) RISC-V CPU.

.. _boards_added_in_zephyr_4_0:

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
   * :zephyr:board:`Renesas RA2A1 Evaluation Kit <ek_ra2a1>` (``ek_ra2a1``)
   * :zephyr:board:`Renesas RA4E2 Evaluation Kit <ek_ra4e2>` (``ek_ra4e2``)
   * :zephyr:board:`Renesas RA4M2 Evaluation Kit <ek_ra4m2>` (``ek_ra4m2``)
   * :zephyr:board:`Renesas RA4M3 Evaluation Kit <ek_ra4m3>` (``ek_ra4m3``)
   * :zephyr:board:`Renesas RA4W1 Evaluation Kit <ek_ra4w1>` (``ek_ra4w1``)
   * :zephyr:board:`Renesas RA6E2 Evaluation Kit <ek_ra6e2>` (``ek_ra6e2``)
   * :zephyr:board:`Renesas RA6M1 Evaluation Kit <ek_ra6m1>` (``ek_ra6m1``)
   * :zephyr:board:`Renesas RA6M2 Evaluation Kit <ek_ra6m2>` (``ek_ra6m2``)
   * :zephyr:board:`Renesas RA6M3 Evaluation Kit <ek_ra6m3>` (``ek_ra6m3``)
   * :zephyr:board:`Renesas RA6M4 Evaluation Kit <ek_ra6m4>` (``ek_ra6m4``)
   * :zephyr:board:`Renesas RA6M5 Evaluation Kit <ek_ra6m5>` (``ek_ra6m5``)
   * :zephyr:board:`Renesas RA8D1 Evaluation Kit <ek_ra8d1>` (``ek_ra8d1``)
   * :zephyr:board:`Renesas RA6E1 Fast Prototyping Board <fpb_ra6e1>` (``fpb_ra6e1``)
   * :zephyr:board:`Renesas RA6E2 Fast Prototyping Board <fpb_ra6e2>` (``fpb_ra6e2``)
   * :zephyr:board:`Renesas RA8T1 Motor Control Kit <mck_ra8t1>` (``mck_ra8t1``)
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

  * The nrf54l15bsim target now includes models of the AAR, CCM and ECB peripherals, and many
    other improvements.
  * Support for Google Kukui EC board (``google_kukui``) has been dropped.
  * STM32: Deprecated MCO configuration via Kconfig in favour of setting it through Devicetree.
    See ``samples/boards/st/mco`` sample.
  * STM32: STM32CubeProgrammer is now the default runner on all STMicroelectronics STM32 boards.
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
  * Added uart, flexio pwm, flexio spi, watchdog, flash, rtc, i2c, lpspi, edma, gpio, acmp, adc and lptmr support
    to NXP ``frdm_ke17z`` and ``frdm_ke17z512``
  * Enabled support for MCUmgr on NXP boards
  * Enabled MCUboot, FlexCAN, LPI2C, VREF, LPADC and timers (TPM, LPTMR, counter, watchdog) on NXP ``frdm_mcxw71``
  * Enabled I2C, PWM on NXP ``imx95_evk``
  * Enabled FLEXCAN, LPI2C on NXP ``s32z2xxdc2``
  * Enabled DSPI and EDMA3 on NXP ``s32z270dc2``
  * Enabled ENET ethernet on NXP ``imx8mm`` and ``imx8mn``
  * Added support for the NXP ``imx8qm`` and ``imx8qxp`` DSP core to enable the openAMP sample


.. _shields_added_in_zephyr_4_0:

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

* Added a script parameter to Twister to enable HW specific arguments, such as a system specific
  timeout

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
  be found in the Zephyr tree) to the :ref:`Devicetree bindings <devicetree_binding_index>` documentation.
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
  * STM32H7: Added support for higher sampling frequencies thanks to boost mode implementation.
  * Added initial support for Renesas RA8 ADC driver (:dtcompatible:`renesas,ra-adc`)
  * Added driver for Analog Devices MAX32 SoC series (:dtcompatible:`adi,max32-adc`).
  * Added support for NXP S32 SAR_ADC (:dtcompatible:`nxp,s32-adc-sar`)
  * Added support for Ambiq Apollo3 series (:dtcompatible:`ambiq,adc`).

* CAN

  * Added initial support for Renesas RA CANFD (:dtcompatible:`renesas,ra-canfd-global`,
    :dtcompatible:`renesas,ra-canfd`)
  * Added Flexcan support for S32Z27x (:dtcompatible:`nxp,flexcan`, :dtcompatible:`nxp,flexcan-fd`)
  * Improved NXP S32 CANXL error reporting (:dtcompatible:`nxp,s32-canxl`)

* Clock control

  * STM32 MCO (Microcontroller Clock Output) is now available on STM32U5 series.
  * STM32 MCO can and should now be configured with device tree.
  * STM32: :kconfig:option:`CONFIG_CLOCK_CONTROL` is now enabled by default at family level and doesn't need
    to be enabled at board level anymore.
  * STM32H7: PLL FRACN can now be configured (see :dtcompatible:`st,stm32h7-pll-clock`)
  * Added initial support for Renesas RA clock control driver (:dtcompatible:`renesas,ra-cgc-pclk`,
    :dtcompatible:`renesas,ra-cgc-pclk-block`, :dtcompatible:`renesas,ra-cgc-pll`,
    :dtcompatible:`renesas,ra-cgc-external-clock`, :dtcompatible:`renesas,ra-cgc-subclk`,
    :dtcompatible:`renesas,ra-cgc-pll-out`)
  * Silabs: Added support for Series 2+ Clock Management Unit (see :dtcompatible:`silabs,series-clock`)
  * Added initial support for Nordic nRF54H Series clock controllers.

* Codec (Audio)

  * Added a driver for the Wolfson WM8904 audio codec (:dtcompatible:`wolfson,wm8904`)

* Comparator

  * Introduced comparator device driver subsystem selected with :kconfig:option:`CONFIG_COMPARATOR`
  * Introduced comparator shell commands selected with :kconfig:option:`CONFIG_COMPARATOR_SHELL`
  * Added support for Nordic nRF COMP (:dtcompatible:`nordic,nrf-comp`)
  * Added support for Nordic nRF LPCOMP (:dtcompatible:`nordic,nrf-lpcomp`)
  * Added support for NXP Kinetis ACMP (:dtcompatible:`nxp,kinetis-acmp`)

* Counter

  * Added initial support for Renesas RA8 AGT counter driver (:dtcompatible:`renesas,ra-agt`)
  * Added driver for Analog Devices MAX32 SoC series (:dtcompatible:`adi,max32-counter`).
  * Updated the NXP counter_mcux_lptmr driver to support multiple instances of the lptmr
    peripheral.
  * Converted the NXP S32 System Timer Module driver to native Zephyr code
  * Added support for late and short relative alarms area to NXP nxp_sys_timer (:dtcompatible:`nxp,s32-sys-timer`)

* Crypto

  * Added support for STM32L4 AES.

* DAC

  * DAC API now supports specifying channel path as internal. Support has been added in STM32 drivers.

* Disk

  * STM32F7 SDMMC driver now supports usage of DMA.
  * STM32 mem controller driver now supports FMC for STM32H5.
  * SDMMC subsystem driver will now power down the SD card when the disk is
    deinitialized

* Display

  * NXP ELCDIF driver now supports flipping the image along the horizontal
    or vertical axis using the PXP. Use
    :kconfig:option:`CONFIG_MCUX_ELCDIF_PXP_FLIP_DIRECTION` to set the desired
    flip.
  * ST7789V driver now supports BGR565, enabled with
    :kconfig:option:`CONFIG_ST7789V_BGR565`.
  * Added driver for SSD1327 OLED display controller (:dtcompatible:`solomon,ssd1327fb`).
  * Added driver for SSD1322 OLED display controller (:dtcompatible:`solomon,ssd1322`).
  * Added driver for IST3931 monochrome display controller (:dtcompatible:`istech,ist3931`).

* DMA

  * Added driver for Analog Devices MAX32 SoC series (:dtcompatible:`adi,max32-dma`).
  * Added flip feature to the NXP dma_mcux_pxp driver (:dtcompatible:`nxp,pxp`)
  * Added support for eDMAv5 and cyclic mode (:github:`80584`) to the NXP EMDA driver (:dtcompatible:`nxp,edma`)

* EEPROM

  * Added support for using the EEPROM simulator with embedded C standard libraries
    (:dtcompatible:`zephyr,sim-eeprom`).

* Entropy

  * Added initial support for Renesas RA8 Entropy driver (:dtcompatible:`renesas,ra-rsip-e51a-trng`)
  * Added driver for Analog Devices MAX32 SoC series (:dtcompatible:`adi,max32-trng`).

* Ethernet

  * Added a :c:func:`get_phy` function to the ethernet driver api, which returns the phy device
    associated to a network interface.
  * Added 2.5G and 5G link speeds to the ethernet hardware capabilities api.
  * Added check for null api pointer in :c:func:`net_eth_get_hw_capabilities`, fixing netusb crash.
  * Added synopsis dwc_xgmac ethernet driver.
  * Added NXP iMX NETC driver.
  * Adin2111

    * Fixed bug that resulted in double RX buffer read when generic spi protocol is used.
    * Fixed essential thread termination on OA read failure.
    * Skip checks for port 2 on the adin1110 since it doesn't apply, as there is no port 2.
  * ENC28J60

    * Added support for the ``zephyr,random-mac-address`` property.
    * Fixed race condition between interrupt service and L2 init affecting carrier status in init.
  * ENC424j600: Added ability to change mac address at runtime with net management api.
  * ESP32: Added configuration of interrupts from DT.
  * Lan865x

    * Enable all multicast MAC address for IPv6. All multicast mac address can now be
      received and allows for correct handling of the IPv6 neighbor discovery protocol.
    * Fixed transmission stopping when setting mac address or promiscuous mode.
  * LiteX

    * Renamed the ``compatible`` from ``litex,eth0`` to :dtcompatible:`litex,liteeth`.
    * Added support for multiple instances of the liteX ethernet driver.
    * Added support for VLAN to the liteX ethernet driver.
    * Added phy support.
  * Native_posix

    * Implemented getting the interface name from the command line.
    * Now prints error number in error message when creating an interface.
  * NXP ENET_QOS: Fixed check for ``zephyr,random-mac-address`` property.
  * NXP ENET:

    * Fixed fused MAC address initialization code.
    * Fixed code path for handling tx errors with timestamped frames.
    * Fixed network carrier status race condition during init.
  * NXP S32: Added configs to enable VLAN promiscuous and untagged, and enable SI message interrupt.
  * STM32

    * Driver can now be configured to use a preemptive RX thread priority, which could be useful
      in case of high network traffic load (reduces jitter).
    * Added support for DT-defined mdio.
    * Fixed bus error after network disconnection that happened in some cases.
  * TC6: Combine read chunks into continuous net buffer. This fixes IPv6 neighbor discovery protocol
    because 64 bytes was not enough for all headers.
  * PHY driver changes

    * Added Qualcomm AR8031 phy driver.
    * Added DP83825 phy driver.
    * PHY_MII

      * Fixed generic phy_mii driver not using the value of the ``no-reset`` property from Devicetree.
      * Removed excess newlines from log output of phy_mii driver.
    * KSZ8081

      * Fixed reset times during init that were unnecessarily long.
      * Removed unnecessary reset on every link configuration that blocked system workqueue
      * Fixed issue relating to strap-in override bits.


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
  * Extended STM32 OSPI drivers to support QUAL, DUAL and SPI modes. Additionally, added support
    for custom write and SFDP:BFP opcodes.
  * Added possibility to run STM32H7 flash driver from Cortex-M4 core.
  * Implemented readout protection handling (RDP levels) for STM32F7 SoCs.
  * Added initial support for Renesas RA8 Flash controller driver (:dtcompatible:`renesas,ra-flash-hp-controller`)
  * Added driver for Analog Devices MAX32 SoC series (:dtcompatible:`adi,max32-flash-controller`).
  * Added support for W25Q512JV and W25Q512NW-IQ/IN to NXP's MCUX Flexspi driver
  * Renamed the binding :dtcompatible:`nxp,iap-msf1` to :dtcompatible:`nxp,msf1` for accuracy

* GPIO

  * tle9104: Add support for the parallel output mode via setting the properties ``parallel-out12`` and
    ``parallel-out34``.
  * Converted the NXP S32 SIUL2 drivers to native Zephyr code
  * Converted the NXP wake-up drivers to native Zephyr code

* Haptics

  * Introduced a haptics device driver subsystem selected with :kconfig:option:`CONFIG_HAPTICS`
  * Added support for TI DRV2605 haptic driver IC (:dtcompatible:`ti,drv2605`)
  * Added a sample for the DRV2605 haptic driver to trigger ROM events (:zephyr:code-sample:`drv2605`)

* I2C

  * Added initial support for Renesas RA8 I2C driver (:dtcompatible:`renesas,ra-iic`)

* I2S

  * Added ESP32-S3 and ESP32-C3 driver support.

* I3C

  * Added support for SETAASA optimization during initialization. Added a
    ``supports-setaasa`` property to ``i3c-devices.yaml``.
  * Added sending DEFTGTS if any devices that support functioning as a secondary
    controller on the bus.
  * Added retrieving GETMXDS within :c:func:`i3c_device_basic_info_get` if BCR mxds
    bit is set.
  * Added helper functions for sending CCCs for ENTTM, VENDOR, DEFTGTS, SETAASA,
    GETMXDS, SETBUSCON, RSTACT DC, ENTAS0, ENTAS1, ENTAS2, and ENTAS3.
  * Added shell commands for sending CCCs for ENTTM, VENDOR, DEFTGTS, SETAASA,
    GETMXDS, SETBUSCON, RSTACT DC, ENTAS0, ENTAS1, ENTAS2, and ENTAS3.
  * Added shell commands for setting the I3C speed, sending HDR-DDR, raising IBIs,
    enabling IBIs, disabling IBIs, and scanning I2C addresses.
  * :c:func:`i3c_ccc_do_setdasa` has been modified to now require specifying the assigned
    dynamic address rather than having the dynamic address be determined within the function.
  * :c:func:`i3c_determine_default_addr` has been removed
  * ``attach_i3c_device`` now no longer requires the attached address as an argument. It is now
    up to the driver to determine the attached address from the ``i3c_device_desc``.

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

  * gt911:
    * Fixed the INT pin to be always set during probe to allow for proper initialization
    * Fixed OOB buffer write to touch points array
    * Add support for multitouch events

* Interrupt

  * Updated ESP32 family interrupt allocator with proper IRQ flags and priorities.
  * Implemented a function to set pending interrupts for Arm GIC
  * Added a safe configuration option so multiple OS'es can share the same GIC and avoid reconfiguring
    the distributor

* LED

  * lp5562: added ``enable-gpios`` property to describe the EN/VCC GPIO of the lp5562.

  * lp5569: added ``charge-pump-mode`` property to configure the charge pump of the lp5569.

  * lp5569: added ``enable-gpios`` property to describe the EN/PWM GPIO of the lp5569.

  * LED code samples have been consolidated under the :zephyr_file:`samples/drivers/led` directory.

* LED Strip

  * Updated ws2812 GPIO driver to support dynamic bus timings

* Mailbox

  * Added driver support for ESP32 and ESP32-S3 SoCs.

* MDIO

  * Added litex MDIO driver.
  * Added support for mdio shell to stm32 mdio.
  * Added mdio driver for dwc_xgmac synopsis ethernet.
  * Added NXP IMX NETC mdio driver.
  * NXP ENET MDIO: Fixed inconsistent behavior by keeping the mdio interrupt enabled all the time.

* MEMC

  * Add driver for APS6404L PSRAM using NXP FLEXSPI

* MFD

* Modem

  * Added support for the U-Blox LARA-R6 modem.
  * Added support for setting the modem's UART baudrate during init.

* MIPI-DBI

  * Added bitbang MIPI-DBI driver, supporting 8080 and 6800 mode
    (:dtcompatible:`zephyr,mipi-dbi-bitbang`).
  * Added support for STM32 FMC memory controller (:dtcompatible:`st,stm32-fmc-mipi-dbi`).
  * Added support for 8080 mode to NXP LCDIC controller (:dtcompatible:`nxp,lcdic`).
  * Fixed the calculation of the reset delay for NXP's LCD controller (:dtcompatible:`nxp,lcdic`)

* MIPI-CSI

  * Improve NXP CSI and MIPI_CSI2Rx drivers to support varibale frame rates

* Pin control

  * Added support for Microchip MEC5
  * Added SCMI-based driver for NXP i.MX
  * Added support for i.MX93 M33 core
  * Added support for ESP32C2
  * STM32: :kconfig:option:`CONFIG_PINCTRL` is now selected by drivers requiring it and
    shouldn't be enabled at board level anymore.

* PWM

  * rpi_pico: The driver now configures the divide ratio adaptively.
  * Added initial support for Renesas RA8 PWM driver (:dtcompatible:`renesas,ra8-pwm`)
  * Added driver for Analog Devices MAX32 SoC series (:dtcompatible:`adi,max32-pwm`).
  * Fixed a build issue of the NXP TPM driver for variants without the capability to combine channels

* Regulators

  * Upgraded CP9314 driver to B1 silicon revision
  * Added basic driver for MPS MPM54304

* RTC

  * STM32: HSE can now be used as domain clock.
  * Added the NXP IRTC Driver.

* RTIO

* SAI

  * Improved NXP's SAI driver to use a default clock if none is provided in the DT
  * Fixed a bug in the NXP SAI driver that caused a crash on a FIFO under- and overrun
  * Fixed a bug that reset the NXP ESAI during initialization (unnecessary)
  * Added support for PM operations in NXP's SAI driver

* SDHC

  * Added ESP32-S3 driver support.
  * SPI SDHC driver now handles SPI devices with runtime PM support correctly
  * Improved NXP's imx SDHC driver to assume card is present if no detection method is provided

* Sensors

  * General

    * The existing driver for the Microchip MCP9808 temperature sensor transformed and renamed to
      support all JEDEC JC 42.4 compatible temperature sensors. It now uses the
      :dtcompatible:`jedec,jc-42.4-temp` compatible string instead to the ``microchip,mcp9808``
      string.
    * Added support for VDD based ADC reference to the NTC thermistor driver.
    * Added Avago APDS9253 (:dtcompatible:`avago,apds9253`) and APDS9306
      (:dtcompatible:`avago,apds9306`) ambient light sensor drivers.
    * Added gain and resolution attributes (:c:enum:`SENSOR_ATTR_GAIN` and
      :c:enum:`SENSOR_ATTR_RESOLUTION`).

  * ADI

    * Add RTIO streaming support to ADXL345, ADXL362, and ADXL372 accelerometer drivers.

  * Bosch

    * Merged BMP390 into BMP388.
    * Added support for power domains to BMM150 and BME680 drivers.
    * Added BMP180 pressure sensor driver (:dtcompatible:`bosch,bmp180`).

  * Memsic

    * Added MMC56X3 magnetometer and temperature sensor driver (:dtcompatible:`memsic,mmc56x3`).

  * NXP

    * Added P3T1755 digital temperature sensor driver (:dtcompatible:`nxp,p3t1755`).
    * Added FXLS8974 accelerometer driver (:dtcompatible:`nxp,fxls8974`).

  * ST

    * Aligned drivers to stmemsc HAL i/f v2.6.
    * Added LSM9DS1 accelerometer/gyroscope/magnetometer sensor driver (:dtcompatible:`st,lsm9ds1`).

  * TDK

    * Added I2C bus support to ICM42670.

  * TI

    * Added support for INA236 to the existing INA230 driver.
    * Added support for TMAG3001 to the existing TMAG5273 driver.
    * Added TMP1075 temperature sensor driver (:dtcompatible:`ti,tmp1075`).

  * Vishay

    * Added trigger capability to VCNL36825T driver.

  * WE

    * Added Würth Elektronik HIDS-2525020210002
      :dtcompatible:`we,wsen-hids-2525020210002` humidity sensor driver.
    * Added general samples for triggers

* Serial

  * LiteX: Renamed the ``compatible`` from ``litex,uart0`` to :dtcompatible:`litex,uart`.
  * Nordic: Removed ``CONFIG_UART_n_GPIO_MANAGEMENT`` Kconfig options (where n is an instance
    index) which had no use after pinctrl driver was introduced.
  * NS16550: Added support for Synopsys Designware 8250 UART.
  * Renesas: Added support for SCI UART.
  * Sensry: Added UART support for Ganymed SY1XX.

* SPI

  * Added initial support for Renesas RA8 SPI driver (:dtcompatible:`renesas,ra8-spi-b`)
  * Added RTIO support to the Analog Devices MAX32 driver.
  * Silabs: Added support for EUSART (:dtcompatible:`silabs,gecko-spi-eusart`)

* Steppers

  * Introduced stepper controller device driver subsystem selected with
    :kconfig:option:`CONFIG_STEPPER`
  * Introduced stepper shell commands for controlling and configuring
    stepper motors with :kconfig:option:`CONFIG_STEPPER_SHELL`
  * Added support for ADI TMC5041 (:dtcompatible:`adi,tmc5041`)
  * Added support for gpio-stepper-controller (:dtcompatible:`zephyr,gpio-steppers`)
  * Added stepper api test-suite
  * Added stepper shell test-suite

* Timer

  * Silabs: Added support for Sleeptimer (:dtcompatible:`silabs,gecko-stimer`)

* USB

  * Added support for USB HS on STM32U59x/STM32U5Ax SoC variants.
  * Enhanced DWC2 UDC driver
  * Added UDC drivers for Smartbond, NuMaker USBD and RP2040 device controllers
  * Enabled SoF in NXP USB drivers (UDC)
  * Enabled cache maintenance in the NXP EHCI USB driver

* Video

  * Introduced API to control frame rate
  * Introduced API for partial frames transfer with the video buffer field ``line_offset``
  * Introduced API for :ref:`multi-heap<memory_management_shared_multi_heap>` video buffer allocation with
    :kconfig:option:`CONFIG_VIDEO_BUFFER_USE_SHARED_MULTI_HEAP`
  * Introduced bindings for common video link properties in ``video-interfaces.yaml``. Migration to the
    new bindings is tracked in :github:`80514`
  * Introduced missing :kconfig:option:`CONFIG_VIDEO_LOG_LEVEL`
  * Added a sample for capturing video and displaying it with LVGL
    (:zephyr:code-sample:`video-capture-to-lvgl`)
  * Added an automatic test to check colorbar pattern correctness
  * Added support for GalaxyCore GC2145 image sensor (:dtcompatible:`galaxycore,gc2145`)
  * Added support for ESP32-S3 LCD-CAM interface (:dtcompatible:`espressif,esp32-lcd-cam`)
  * Added support for NXP MCUX SMARTDMA interface (:dtcompatible:`nxp,smartdma`)
  * Added support for more OmniVision OV2640 controls (:dtcompatible:`ovti,ov2640`)
  * Added support for more OmniVision OV5640 controls (:dtcompatible:`ovti,ov5640`)
  * STM32: Implemented :c:func:`video_get_ctrl` and :c:func:`video_set_ctrl` APIs.
  * Removed an init order circular dependency for the camera pipeline on NXP RT10xx platforms
    (:github:`80304`)
  * Added an NXP's smartdma based video driver (:dtcompatible:`nxp,video-smartdma`)
  * Added frame interval APIs to support variable frame rates (video_sw_generator.c)
  * Added image controls to the OV5640 driver

* W1

  * Added 1-Wire master driver for Analog Devices MAX32 SoC series (:dtcompatible:`adi,max32-w1`)

* Watchdog

  * Added driver for Analog Devices MAX32 SoC series (:dtcompatible:`adi,max32-watchdog`).
  * Converted NXP S32 Software Watchdog Timer driver to native Zephyr code

* Wi-Fi

  * Add Wi-Fi Easy Connect (DPP) support.
  * Add support for Wi-Fi credentials library.
  * Add enterprise support for station.
  * Add Wi-Fi snippet support for networking samples.
  * Add build testing for various Wi-Fi config combinations.
  * Add regulatory domain support to Wi-Fi shell.
  * Add WPS support to Wi-Fi shell.
  * Add 802.11r connect command usage in Wi-Fi shell.
  * Add current PHY rate to hostap status message.
  * Allow user to reset Wi-Fi statistics in Wi-Fi shell.
  * Display RTS threshold in Wi-Fi shell.
  * Fix SSID array length size in scanning results.
  * Fix the "wifi ap config" command using the STA interface instead of SAP interface.
  * Fix memory leak in hostap when doing a disconnect.
  * Fix setting of frequency band both in AP and STA mode in Wi-Fi shell.
  * Fix correct channel scan range in Wi-Fi 6GHz.
  * Fix scan results printing in Wi-Fi shell.
  * Increase main and shell stack sizes for Wi-Fi shell sample.
  * Increase the maximum count of connected STA to 8 in Wi-Fi shell.
  * Relocate AP and STA Wi-Fi sample to samples/net/wifi directory.
  * Run Wi-Fi tests together with network tests.
  * Updated ESP32 Wi-Fi driver to reflect actual negotiated PHY mode.
  * Add ESP32-C2 Wi-Fi support.
  * Add ESP32 driver APSTA support.
  * Add NXP RW612 driver support.
  * Added nRF70 Wi-Fi driver.

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
  * Fixed tx_pkts slab leak in send_ipv4_fragment

* IPv6:

  * Added a public header for Multicast Listener Discovery APIs.
  * Added new :c:func:`net_ipv6_addr_prefix_mask` API function.
  * Made IPv6 Router Solicitation timeout configurable.
  * Fixed endless IPv6 packet looping with both routing and VLAN support enabled.
  * Fixed unneeded error logging in case of dropped NS packets.
  * Fixed accepting of incoming DAD NS messages.
  * Various fixes improving IPv6 routing.
  * Added onlink and forwarding check to IPv6-prepare

* LwM2M:

  * Location object: optional resources altitude, radius, and speed can now be
    used optionally as per the location object's specification. Users of these
    resources will now need to provide a read buffer.
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
  * Extended network tracing support and added documentation page
    (:ref:`network_tracing`).
  * Moved network buffers implementation out of net subsystem into lib directory
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

  * Add a 80211R fast BSS transition argument usage to the wifi shell's connect command.
  * Fixed the shell's ap config command using the sta interface area
  * Added AP configuration cmd support to the NXP Wifi drivers
  * Fixed the dormant state in the NXP WiFi driver to be set to off once a connection to an AP is achieved

* zperf:

  * Added support for USB CDC NCM in the zperf sample.
  * Fixed DHCPv4 client not being started in the zperf sample in certain
    configurations.

USB
***

* New USB device stack:

  * Added USB CDC Network Control Model implementation
  * Enhanced USB Audio class 2 implementation
  * Made USB device stack high-bandwidth aware
  * Enhanced CDC ACM and HID class implementations

Devicetree
**********

* Added support for string-array and array type properties to be enums.
  Many new macros added for this, for example :c:macro:`DT_ENUM_IDX_BY_IDX`.
* Added :c:macro:`DT_ANY_COMPAT_HAS_PROP_STATUS_OKAY`.
* Added :c:macro:`DT_NODE_HAS_STATUS_OKAY`.
* Added :c:macro:`DT_INST_NUM_IRQS`.
* Added macros :c:macro:`DT_NODE_FULL_NAME_UNQUOTED`, :c:macro:`DT_NODE_FULL_NAME_TOKEN`,
  and :c:macro:`DT_NODE_FULL_NAME_UPPER_TOKEN`.
* ``DT_*_REG_ADDR`` now returns an explicit unsigned value with C's ``U`` suffix.
* Fixed escaping of double quotes, backslashes, and new line characters from DTS
  so that they can be used in string properties.
* Renamed ``power-domain`` base property to ``power-domains``,
  and introduced ``power-domain-names`` property. ``#power-domain-cells`` is now required as well.
* Moved the NXP Remote Domain Controller property to its own schema file

Kconfig
*******

Libraries / Subsystems
**********************

* Debug

    * Added west runner for probe-rs, a Rust-based embedded toolkit.

* Demand Paging

  * Added LRU (Least Recently Used) eviction algorithm.

  * Added on-demand memory mapping support (:kconfig:option:`CONFIG_DEMAND_MAPPING`).

  * Made demand paging SMP compatible.

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
      autohandler will not reshedule itself. Both variants are scheduled independent of each other.
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

* SD

  * No significant changes in this release

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

* Storage

  * LittleFS: The module has been updated with changes committed upstream
    from version 2.8.1, the last module update, up to and including
    the released version 2.9.3.
  * Fixed static analysis error caused by mismatched variable assignment in NVS

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

* Tracing

  * Added support for a "user event" trace, with the purpose to allow driver or
    application developers to quickly add tracing for events for debug purposes

* POSIX API

  * Added support for the following Option Groups:

    * :ref:`POSIX_DEVICE_IO <posix_option_group_device_io>`
    * :ref:`POSIX_SIGNALS <posix_option_group_signals>`

  * Added support for the following Options:

    * :ref:`_POSIX_SYNCHRONIZED_IO <posix_option_synchronized_io>`
    * :ref:`_POSIX_THREAD_PRIO_PROTECT <posix_option_thread_prio_protect>`

  * :ref:`POSIX_FILE_SYSTEM <posix_option_group_file_system>` improvements:

    * Support for :c:macro:`O_TRUNC` flag in :c:func:`open()`.
    * Support for :c:func:`rmdir` and :c:func:`remove`.

  * :ref:`_POSIX_THREAD_SAFE_FUNCTIONS <posix_option_thread_safe_functions>` improvements:

    * Support for :c:func:`asctime_r`, :c:func:`ctime_r`, and :c:func:`localtime_r`.

  * :ref:`POSIX_THREADS_BASE <posix_option_group_threads_base>` improvements:

    * Use the :ref:`user mode semaphore API <semaphores_v2>` instead of the
      :ref:`spinlock API <smp_arch>` for pool synchronization.

* LoRa/LoRaWAN

* ZBus

* JWT (JSON Web Token)

  * The following new symbols were added to allow specifying both the signature
    algorithm and crypto library:

    * :kconfig:option:`CONFIG_JWT_SIGN_RSA_PSA` (default) RSA signature using the PSA Crypto API;
    * :kconfig:option:`CONFIG_JWT_SIGN_RSA_LEGACY` RSA signature using Mbed TLS;
    * :kconfig:option:`CONFIG_JWT_SIGN_ECDSA_PSA` ECDSA signature using the PSA Crypto API.

    (:github:`79653`)

* Firmware

  * Introduced basic support for ARM's System Control and Management Interface, which includes:

    * Subset of clock management protocol commands
    * Subset of pin control protocol commands
    * Shared memory and mailbox-based transport

HALs
****

* Nordic

  * Updated nrfx to version 3.7.0.
  * Added OS agnostic parts of the nRF70 Wi-Fi driver.

* STM32

  * Updated STM32C0 to cube version V1.2.0.
  * Updated STM32F1 to cube version V1.8.6.
  * Updated STM32F2 to cube version V1.9.5.
  * Updated STM32F4 to cube version V1.28.1.
  * Updated STM32G4 to cube version V1.6.0.
  * Updated STM32H5 to cube version V1.3.0.
  * Updated STM32H7 to cube version V1.11.2.
  * Updated STM32H7RS to cube version V1.1.0.
  * Added STM32U0 Cube package (1.1.0)
  * Updated STM32U5 to cube version V1.6.0.
  * Updated STM32WB to cube version V1.20.0.
  * Added STM32WB0 Cube package (1.0.0)
  * Updated STM32WBA to cube version V1.4.1.

* ADI

* Espressif

  * Synced HAL to version v5.1.4 to update SoCs low level files, RF libraries and
    overall driver support.
* NXP

    * Updated the MCUX HAL to the SDK version 2.16.000
    * Updated the NXP S32ZE HAL drivers to version 2.0.0

* Silabs

  * Updated Series 2 to Simplicity SDK 2024.6, while Series 0/1 continue to use Gecko SDK 4.4.

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
  * Added bootutil protected TLV size to image size check.
  * Added functionality to remove images with conflicting flags or where features are required
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
  * Added support for automatically calculating max sectors.
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
  * Fixed bootutil wrong definition of ``find_swap_count``.
  * Fixed bootutil swap move max app size calculation.
  * Fixed imgtool issue where getpub failed for ed25519 key.
  * Fixed issue with sysbuild if something else is named mcuboot.
  * Fixed RAM load chain load address.
  * Fixed issue with properly retrieving image headers after interrupted swap-scratch in bootutil.
  * The MCUboot version in this release is version ``2.1.0+0-dev``.
  * Add the following nxp boards as test targets area: ``frdm_ke17z``, ``frdm_ke17z512``,
    ``rddrone_fmuk66``, ``twr_ke18f``, ``frdm_mcxn947/mcxn947/cpu0``

OSDP
****

Trusted Firmware-M (TF-M)
*************************

* TF-M was updated to version 2.1.1 (from 2.1.0).
  The release notes can be found at: https://trustedfirmware-m.readthedocs.io/en/tf-mv2.1.1/releases/2.1.1.html

Nanopb
******

* Updated the nanopb module to version 0.4.9.
  Full release notes at https://github.com/nanopb/nanopb/blob/0.4.9/CHANGELOG.txt

LVGL
****

* Added definition of ``LV_ATTRIBUTE_MEM_ALIGN`` so library internal data structures can be aligned
  to a specific boundary.
* Provided alignment definition to accommodate the alignment requirement of some GPU's

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

* Together with the deprecation of ``native_posix``, many tests which were
  explicitly run in native_posix now run in :ref:`native_sim<native_sim>` instead.
  native_posix as a platform remains tested though.
* Extended the tests of counter_basic_api with a testcase for counters without alarms
* Added support for testing SDMMC devices to the fatfs API test
* Extended net/vlan to add IPv6 prefix config to each vlan-iface
* Enhanced the camera fixture test by adding a color bar to enable automation
* Added a number crunching (maths such as FFT, echo cancellation) sample using an optimized
  library for the NXP ADSP board
* Tailored the SPI_LOOPBACK test to the limitations of NXP Kinetis MCU's
* Enabled the video sample to run video capture (samples/drivers/video)

* Added :zephyr:code-sample:`smf_calculator` sample demonstrating the usage of the State Machine framework
  in combination with LVGL to create a simple calculator application.
* Consolidated display sample where possible to use a single testcase for all shields

Issue Related Items
*******************

Known Issues
============

- :github:`71042` stream_flash: stream_flash_init() size parameter allows to ignore partition layout
- :github:`67407` stream_flash: stream_flash_erase_page allows to accidentally erase stream
- :github:`80875` stepper_api: incorrect c-prototype stepper.h and absence of NULL check stepper_shell.c
