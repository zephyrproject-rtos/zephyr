:orphan:

.. _zephyr_3.6:

Zephyr 3.6.0
############

We are pleased to announce the release of Zephyr version 3.6.0.

Major enhancements with this release include:

* New :ref:`GNSS subsystem <gnss_api>` added, enabling geo-awareness in Zephyr applications.
* New API and drivers introduced for interfacing with :ref:`keyboard matrices <gpio-kbd>`.
* New socket and CoAP service libraries streamlining the implementation of socket and CoAP servers
  respectively, while also optimizing the use of resources.
* Integrated Trusted Firmware-M (TF-M) 2.0, including an update to Mbed TLS 3.5.2.
* Improved LLEXT tooling, simplifying module creation in the Zephyr build system.
* Userspace support extended to Xtensa architecture.
* Build system now supports Link Time Optimization (LTO), reducing the size of the final image.
* Bluetooth Mesh protocol 1.1 now supported by default.
* Major updates to the documentation of the :ref:`native simulator <native_sim>`, clarifying
  supported peripherals and how to use them.
* Over 30 new supported boards, spanning all Zephyr-supported architectures.

An overview of the changes required or recommended when migrating your application from Zephyr
v3.5.0 to Zephyr v3.6.0 can be found in the separate :ref:`migration guide<migration_3.6>`.

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************
The following CVEs are addressed by this release:

More detailed information can be found in:
https://docs.zephyrproject.org/latest/security/vulnerabilities.html

* CVE-2023-5779 `Zephyr project bug tracker GHSA-7cmj-963q-jj47
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-7cmj-963q-jj47>`_

* CVE-2023-6249 `Zephyr project bug tracker GHSA-32f5-3p9h-2rqc
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-32f5-3p9h-2rqc>`_

* CVE-2023-6749 `Zephyr project bug tracker GHSA-757h-rw37-66hw
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-757h-rw37-66hw>`_

* CVE-2023-6881 `Zephyr project bug tracker GHSA-mh67-4h3q-p437
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-mh67-4h3q-p437>`_

* CVE-2023-7060: Under embargo until 2024-03-14

* CVE-2024-1638 `Zephyr project bug tracker GHSA-p6f3-f63q-5mc2
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-p6f3-f63q-5mc2>`_

Architectures
*************

* ARC

  * Enabled hardware prefetcher and shared cluster cache (SCM - Shared Cluster
    Memory) for ARCv3 processors (HS5x & HS6x).
  * Disabled Thread-local Storage support for platforms with two or more register banks.
  * Fixed unstable work of application built with MetaWare toolchain for hardware
    platforms (garbage in .device_states section).

* ARM

  * MPU regions are now always cleared before initialization.
  * Standardized on :c:func:`arch_secondary_cpu_init` to provide consistency
    across all architectures.
  * Renamed :c:func:`z_arm_prep_c` as :c:func:`z_prep_c` to provide
    consistency across all architectures.
  * Renamed the exception header to be consistent across all architectures.
  * GDB stubs added (currently only supports Zynq-7000).
  * Added support for custom interrupt controllers using
    :kconfig:option:`CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER`.
  * MMU and MPU initialization moved to :c:func:`z_prep_c` for Cortex-A and
    Cortex-R to enable initialization by individual cores.
  * Common Cortex-M MPU code moved to ``arch/arm/core/mpu``.

* Xtensa

  * Removed the unused Kconfig option ``CONFIG_XTENSA_NO_IPC``.

  * Added userspace support via MMU.

Bluetooth
*********

* Audio

  * Changed ``bt_bap_scan_delegator_subgroup`` to :c:struct:`bt_bap_bass_subgroup` and
    made it independent of :kconfig:option:`CONFIG_BT_BAP_SCAN_DELEGATOR`.
  * Modified :c:func:`bt_bap_stream_send` to no longer take a timestamp as parameter,
    and added :c:func:`bt_bap_stream_send_ts` that does.
  * Modified :c:func:`bt_cap_stream_send` to no longer take a timestamp as parameter,
    and added :c:func:`bt_cap_stream_send_ts` that does.
  * Assigned number values have been moved from :file:`include/zephyr/bluetooth/audio/lc3.h` to
    :file:`include/zephyr/bluetooth/audio/audio.h` and the ``LC3`` infix have been removed.
  * The CAP initiator APIs have been streamlined and follow the same parameter pattern.
  * Added Kconfig options to make MCC functionality optional to reduce memory usage for simple
    clients.
  * Added CAP Commander change volume and change volume offset.
  * Added proper support for doing decoding in the application instead of in the controller by
    modifying how the ISO data path is configured.
  * Added :c:func:`bt_csip_set_member_unregister` to unregister a CSIS instance.
  * Added helper functions to get and set assigned number values in codec configuration and
    codec capabilities.
  * Added support for the new mono audio location.
  * Added ISO state callbacks for streams so the user knows the state of the CIS.
  * Added :c:func:`bt_pacs_set_available_contexts_for_conn` to set available context per connection.
  * Refactored the :c:struct:`bt_bap_base` to be an abstract struct with new helper functions,
    so that Zephyr supports all BASEs regardless of the size.

* Host

  * Added ``recycled()`` callback to :c:struct:`bt_conn_cb`, which notifies listeners when a
    connection object has been freed, so it can be utilized for different purposes. No guarantees
    are made to what listener will be granted the object, as only the first claim is served.
  * Modified :c:func:`bt_iso_chan_send` to no longer take a timestamp as parameter,
    and added :c:func:`bt_iso_chan_send_ts` that does.

* Mesh

  * Added the delayable messages functionality to apply random delays for
    the transmitted responses on the Access layer.
    The functionality is enabled by the :kconfig:option:`CONFIG_BT_MESH_ACCESS_DELAYABLE_MSG`
    Kconfig option.
  * The Bluetooth Mesh protocol 1.1 is now supported by default.

* Controller

  * Added deinit implementation for ESP32 controller.

* HCI Driver

  * Split ST HCI SPI Bluetooth driver from the Zephyr one to provide more features
    based on ST SPI protocols V1 and V2. As a result, :dtcompatible:`st,hci-spi-v1` and
    :dtcompatible:`st,hci-spi-v2` were introduced.

Boards & SoC Support
********************

* Added support for these SoC series:

  * Added support for Renesas R-Car Gen4 series.
  * Added support for STM32F303xB SoC variants.
  * Added support for STM32H7B0xx SoC variants.
  * Added support for STM32L010xx SoC variants.
  * Added support for STM32L081xx SoC variants.
  * Added support for STM32U5A9xx SoC variants.
  * Added support for NXP S32K1 devices.
  * Added support for NXP IMX8ULP SoC.
  * Added support for NXP MIMXRT595 DSP core.

* Made these changes in other SoC series:

  * Nordic SoCs now imply :kconfig:option:`CONFIG_XIP` instead of selecting it. This allows for
    creating RAM-based applications by disabling it.
  * BLE is now supported on STM32WBA series.
  * xtensa: imx8: Split the generic i.MX8 SoC into i.MX8QXP and i.MX8QM.
  * LPC55xxx: Fixed the system hardware clock cycle rate.

* Added support for these ARM boards:

  * Added support for Adafruit QTPy RP2040 board: ``adafruit_qt_py_rp2040``.
  * Added support for FANKE FK7B0M1-VBT6 board: ``fk7b0m1_vbt6``.
  * Added support for Renesas R-Car Spider board CR52: ``rcar_spider_cr52``.
  * Added support for ST Nucleo F722ZE board: ``nucleo_f722ze``.
  * Added support for ST STM32H750B Discovery Kit: ``stm32h750b_dk``.
  * Added support for ST STM32L4R9I Discovery board: ``stm32l4r9i_disco``.
  * Added support for ST STM32U5A9J-DK discovery kit: ``stm32u5a9j_dk``.
  * Added support for ST Nucleo WBA55CG board: ``nucleo_wba55cg``.
  * Added support for ST STM32WB5MM-DK Discovery board: ``stm32wb5mm_dk``.
  * Added support for Wiznet W5500 Evaluation Pico board: ``w5500_evb_pico``.
  * Added support for ADI boards: ``adi_sdp_k1``, ``adi_eval_adin1110ebz``,
    ``adi_eval_adin2111ebz``.
  * Added support for NXP UCANS32K1SIC board: ``ucans32k1sic``.

* Added support for this RISC-V board:

  * Added Lilygo TTGO T8-C3 board: ``ttgo_t8c3``.

* Added support for these Xtensa boards:

  * Added support for NXP iMX8ULP board: ``nxp_adsp_imx8ulp``.
  * Added Heltec Wireless Stick Lite (V3) board: ``heltec_wireless_stick_lite_v3``.
  * Added KINCONY-KC868-A32 board: ``kincony_kc868_a32``.
  * Added Lolin ESP32-S2 Mini board: ``esp32s2_lolin_mini``.
  * Added Lilygo TTGO LoRa32 board: ``ttgo_lora32``.
  * Added M5Stack AtomS3 board: ``m5stack_atoms3``.
  * Added M5Stack AtomS3-Lite board: ``m5stack_atoms3_lite``.
  * Added M5Stack StampS3 board: ``m5stack_stamps3``.

* Made these changes for ARM boards:

  * Added support for low power on G1120B0MIPI using RT595.
  * Added support for lpspi, lpi2c on NXP board: ``mimx93_evk_a55``.
  * Fixed partition naming on ``lpcxpresso55s69`` to use the standard slot
    naming used by TFM-enabled Zephyr platforms.
  * Enabled support for linkserver debugger on ``frdm_kl25z``, ``mimxrt1015_evk``,
    ``mimxrt1020_evk``, ``mimxrt1050_evk``, ``mimxrt685_evk``, ``frdm_k64f``.
  * Switched MCUBoot FW Update mode on NXP boards from Swap & Scratch to Swap & Move.

* Made these changes for RISC-V boards:

  * Enabled ADC support on ``longan_nano``.

* Made these changes for native/POSIX boards:

  * The :ref:`simulated nrf5340 targets<nrf5340bsim>` now include the IPC and MUTEX peripherals,
    and support OpenAMP to communicate between the cores.
    It is now possible to run the BLE controller or 802.15.4 driver in the net core, and application
    and BT host in the app core.

  * The nrf*_bsim simulated targets now include models of the UART peripheral. It is now possible
    to connect a :ref:`nrf52_bsim<nrf52_bsim>` UART to another, or a UART in loopback, utilizing
    both the new and legacy nRFx UART drivers, in any mode.

  * For the native simulator based targets it is now possible to set via Kconfig command line
    options which will be handled by the executable as if they were provided from the invoking
    shell.

  * For all native boards, the native logger backend will now also be
    used even if the UART is enabled.

  * Several bugfixes and other minor additions to the nRF5x HW models.

  * Multiple documentation updates and fixes for all native boards.

* Added support for these following shields:

  * Added support for M5Stack-Core2 base: ``m5stack_core2_ext``.
  * Added support for MikroElektronika ACCEL 13 Click: ``mikroe_accel13_click``.
  * Added support for Waveshare Pico UPS-B: ``waveshare_pico_ups_b``.
  * Added support for X-NUCLEO-BNRG2A1: BLE expansion board: ``x_nucleo_bnrg2a1``.
  * Added support for X-NUCLEO-IKS4A1: MEMS Inertial and Environmental Multi
    sensor: ``x_nucleo_iks4a1``.

Build system and infrastructure
*******************************

* Added functionality for Link Time Optimization.
  This change includes interrupt script generator rebuilding and adds the
  following Kconfig options:

  - :kconfig:option:`CONFIG_ISR_TABLES_LOCAL_DECLARATION`:
    LTO compatible interrupt tables parser
  - :kconfig:option:`CONFIG_LTO`: Enable Link Time Optimization

  Currently the LTO compatible interrupt tables parser is only supported by ARM architectures and
  GCC compiler/linker.
  See pull request :github:`66392` for details.

* Dropped the ``COMPAT_INCLUDES`` option. It was unused since Zephyr v3.0.

* Fixed an issue whereby board revision ``0`` did not include overlay files for that revision.

* Added ``PRE_IMAGE_CMAKE`` and ``POST_IMAGE_CMAKE`` hooks to sysbuild modules, which allows for
  modules to run code after and before each image's cmake invocation.

* Added :kconfig:option:`CONFIG_ROM_END_OFFSET` option which allows reducing the size of an image.
  This is intended for use with firmware signing scripts which add additional data to the end of
  images outside of the build itself.

* Added MCUboot image size reduction to sysbuild images which include MCUboot. This prevents
  issues with building firmware images that are too large for MCUboot to swap.

* Deprecated :kconfig:option:`CONFIG_BOOTLOADER_SRAM_SIZE`. Users of this should transition to
  having RAM set up properly in their board devicetree files.

* Fixed an issue whereby shields were processed in order of the root they resided in rather than
  the order they were supplied to cmake in.

* Fixed an issue whereby using some shields with sysbuild would cause a cmake Kconfig error.

* Fixed an issue where the macros ``_POSIX_C_SOURCE`` and ``_XOPEN_SOURCE`` would be defined
  globally when building with Picolibc or for the native (``ARCH_POSIX``) targets.
  After this change users may need to define them for their own applications or libraries.

* Added support for sysbuild setting a signing script (``SIGNING_SCRIPT``). See
  :ref:`west-extending-signing` for details.

* Added support for ``FILE_SUFFIX`` in the build system which allows for adding suffixes to
  application Kconfig fragment file names and devicetree overlay file names. See
  :ref:`application-file-suffixes` and :ref:`sysbuild_file_suffixes` for details.

* Deprecated ``CONF_FILE`` ``prj_<build>.conf`` build type.

* Added `-Wdouble-promotion` as a default warning when compiling to warn developers with
  single-precision floats easily being promoted to double-precision.

Drivers and Sensors
*******************

* ADC

  * Power Management for ADC is now supported on STM32 devices.
  * STM32 ADC driver now supports mixing shared and separate IRQs (for instance on STM32G473
    which has 5 ADCs, ADC1 and ADC2 share one IRQ while ADC3, ADC4 and ADC5 each have unique IRQs).
    Enabling all instances in the same application is not possible on such devices as of now.

* Auxiliary Display

  * Added Sparkfun SerLCD driver.

* Audio

  * Added a driver :file:`drivers/audio/dmic_mcux.c` for NXP DMIC peripheral. This peripheral is
    present on the ``iMX RT5xx`` and ``iMX RT6xx`` parts, as well as some LPC SOCs.

* Battery backed up RAM

  * STM32WL devices now support BBRAM.

* CAN

  * Added system call :c:func:`can_get_mode()` for getting the current operation mode of a CAN
    controller.

  * Add system call :c:func:`can_get_transceiver()` for getting the CAN transceiver associated with
    a CAN controller.

  * Added accessor functions for the CAN statistics.

  * Added common bit error counter to the CAN statistics.

  * Added CAN statistics support to the following drivers:

    * :dtcompatible:`microchip,mcp2515`
    * :dtcompatible:`espressif,esp32-twai`
    * :dtcompatible:`kvaser,pcican`

  * Added CAN controller driver for the Nuvoton NuMaker series
    (:dtcompatible:`nuvoton,numaker-canfd`).

  * Added CAN controller driver for the Infineon XMC4xxx family
    (:dtcompatible:`infineon,xmc4xxx-can` and :dtcompatible:`infineon,xmc4xxx-can-node`).

  * Added support for the NXP S32K1xx family to the :dtcompatible:`nxp,flexcan` driver.

  * All Bosch M_CAN-based front-end drivers now use named IRQs, "int0" and "int1".

  * The :dtcompatible:`zephyr,native-linux-can` driver now supports being built with embedded C
    libraries.

  * Added support for setting "raw" timing values from the :ref:`CAN shell <can_shell>`.

* Clock control

  * Renesas R-Car clock control driver now supports Gen4 SoCs.
  * Renamed ``CONFIG_CLOCK_CONTROL_RA`` to :kconfig:option:`CONFIG_CLOCK_CONTROL_RENESAS_RA`.
  * On STM32 devices, :dtcompatible:`st,stm32-hse-clock` now allows setting a ``css-enabled``
    property which enables HSE clock security system (CSS).

* Counter

  * The nRFx counter driver now works with simulated nrf*_bsim targets.
  * Added support for top value configuration and fixed a bug in the native posix driver.
  * Added support for the MRT counter for NXP RT6xx, RT5xx and LPC55xxx.

* Crypto

  * STM32WB devices now support crypto API through AES block.

* Display

  * Introduced frame buffer config to STM32 LTDC driver.

* DMA

  * STM32WBA Devices now support GPDMA.
  * Introduced a new DMA driver :file:`drivers/dma/dma_nxp_edma.c` for NXP's eDMA IP.

* Entropy

  * The "native_posix" entropy driver now accepts a new command line option ``seed-random``.
    When used, the random generator will be seeded from ``/dev/urandom``
  * On STM32devices, RNG block is now suspended when pool is full to save power.

* Ethernet

  * The "native_posix" ethernet driver now supports being built with embedded C libraries.
  * Enabled HW checksum offloading for STM32H7.
  * Added implementation of Open Alliance's TC6 T1S driver.
  * Added xmc4xxx driver.
  * Added NXP enet driver with PTP support.
  * Added KSZ8081 PHY driver.
  * Added proper IPv4 multicast support to NXP mcux driver.
  * Added LAN8651 T1S support.
  * Added DSA support to STM32.
  * Added tja1103 PHY support.
  * Added Nuvoton numaker support.
  * Fixed lan865x driver. Transmission speed improvements, IRQ handling fixes.
  * Fixed s32_gmac driver. Link up/down handling fixes.
  * Fixed phy_mii driver. The invalid phy id was incorrectly checked.
  * Fixed sam_gmac driver. PTP clock adjustment was wrong for negative values.
  * Fixed adin2111 driver. Initialization was done incorrectly when working with adin2110.
  * Fixed ksz8081 driver. Logging changes, RMII clock fixes, GPIO pin fixes.
  * Added a driver :file:`drivers/ethernet/eth_nxp_enet.c` for NXP ENET which is a rework of
    the old driver :file:`drivers/ethernet/eth_mcux.c`. The old driver became
    unmaintainable due to fundamental problems with the lack of PHY abstraction. The new driver
    is still experimental and requires maturation. Eventually the old driver will be deprecated
    and this new driver will be supported instead.

* Flash

  * Redesigned the Atmel SAM controller to fully utilize flash page layout.
  * ``spi_nor`` driver now sleeps between polls in ``spi_nor_wait_until_ready``. If this is not
    desired (For example due to ROM constraints in a bootloader),
    :kconfig:option:`CONFIG_SPI_NOR_SLEEP_WHILE_WAITING_UNTIL_READY` can be disabled.
  * Flash readout protection configuration was added on STM32G4 and STM32L4 series.

  * ``nordic_qspi_nor`` driver now supports user-configurable QSPI timeout with
    :kconfig:option:`CONFIG_NORDIC_QSPI_NOR_TIMEOUT_MS`.

* GNSS

  * Added GNSS device driver API and subsystem for parsing and publishing location,
    datetime, and satellite information, enabled by
    :kconfig:option:`CONFIG_GNSS` and :kconfig:option:`CONFIG_GNSS_SATELLITES`.
    The GNSS subsystem and device drivers are based on the :ref:`modem` subsystem,
    using the ``modem_pipe`` module, modem backends, and ``modem_chat`` module to
    communicate with the modems. For systems which already contain a cellular modem,
    adding a GNSS modem is very efficient due to the reuse of subsystems.

  * Added GNSS-specific, safe, string-to-integer parsing utilities, enabled by
    :kconfig:option:`CONFIG_GNSS_PARSE`.

  * Added NMEA0183 parsing utilities, enabled by
    :kconfig:option:`CONFIG_GNSS_NMEA0183`.

  * Added extensive GNSS data logging, enabled by
    :kconfig:option:`CONFIG_GNSS_DUMP_TO_LOG`.

  * Added generic NMEA0183 over UART based modem device driver, matching the
    devicetree compatible :dtcompatible:`gnss-nmea-generic`.

  * Added fully featured device driver for the Quectel LCX6G series GNSS modems,
    matching the devicetree compatibles :dtcompatible:`quectel,lc26g`,
    :dtcompatible:`quectel,lc76g` and :dtcompatible:`quectel,lc86g`.

* GPIO

  * Renesas R-Car GPIO driver now supports Gen4 SoCs.
  * Renamed ``CONFIG_GPIO_RA`` to :kconfig:option:`CONFIG_GPIO_RENESAS_RA`.
  * Added a new GPIO driver (:file:`drivers/gpio/gpio_mcux_rgpio.c`). This
    driver is used for i.MX93 and i.MX8ULP.

* I2C

  * :c:func:`i2c_get_config` is now supported on the STM32 driver.

* I2S

  * STM32H7 devices now support I2S.

* I3C

  * The Legacy Virtual Register defines have been renamed from ``I3C_DCR_I2C_*``
    to ``I3C_LVR_I2C_*``.

  * Added the ability to specify a start address when searching for a free I3C
    address to be reserved. This requires a new function argument to
    :c:func:`i3c_addr_slots_next_free_find`.

  * Added a field named ``num_xfer`` in :c:struct:`i3c_msg` and
    :c:struct:`i3c_ccc_taget_payload` as an output to indicate the actual
    number of bytes transferred.

  * Cadence I3C driver (:file:`drivers/i3c/i3c_cdns.c`):

    * Added support to handle controller abort where the target does not emit
      end of data for register read but continues sending data.

    * Updated the timeout calculation to be coupled with CPU speed instead of
      a fixed number of retries.

  * NXP MCUX I3C driver (:file:`drivers/i3c/i3c_mcux.c`):

    * Fixed ``mcux_i3c_config_get()`` not returning the configuration to the caller.

    * Improved the FIFO read routine to support higher transfer rates.

    * Removed the infinite wait for MCTRLDONE in auto IBI.

    * Added ``disable-open-drain-high-pp`` property to
      :dtcompatible:`nxp,mcux-i3c`, which allows alternative high time for
      open-drain clock.

* IEEE 802.15.4

  * Removed :kconfig:option:`CONFIG_IEEE802154_SELECTIVE_TXPOWER` Kconfig option.

* Input

  * The ``short-codes`` property of :dtcompatible:`zephyr,input-longpress` is
    now optional. The node can be used by specifying only input and long codes.
  * Added support for keyboard matrix drivers, including a new
    :dtcompatible:`gpio-kbd-matrix` and :dtcompatible:`input-keymap` drivers.
    See :ref:`gpio-kbd` for more details.
  * Added a pair of input codes to HID codes translation functions. See
    :c:func:`input_to_hid_code` and :c:func:`input_to_hid_modifier`.
  * Added power management support to :dtcompatible:`gpio-keys`
    :dtcompatible:`focaltech,ft5336`.
  * Added a :dtcompatible:`zephyr,native-linux-evdev` device node for getting
    input events from a Linux evdev device node.
  * Added support for optical encoders and power management to :dtcompatible:`gpio-qdec`.
  * New driver :dtcompatible:`analog-axis`.
  * Added ESP32 touch sensor driver including a :dtcompatible:`espressif,esp32-touch`.

* MDIO

  * Fixed initialization priorities of NXP s32 NETC drivers.
  * Fixed SAM GMAC transfer timeout errors caused by MDIO clock not being initialized.
  * Fixed ESP32 MDIO driver being enabled when node was not status okay.
  * Added support for C22 and C45 APIs on S32 GMAC.
  * Added MDIO driver for NXP ENET peripheral.
  * Added xmc4xxx MDIO drivers.
  * Fixed build errors caused by mdio.h driver header not including errno.h

* MFD

  * Added support for :dtcompatible:`maxim,max20335`.
  * Added support for :dtcompatible:`adi,ad5592`.
  * Added separate initialization priorities for :dtcompatible:`nordic,npm1300` and
    :dtcompatible:`nordic,npm6001`.

* PCIE

  * Fixed MMIO size calculation by disabling IO/memory decoding beforehand.

  * Modified to use PNP ID for PRT retrieval.

* MEMC

  * Added a new driver for NXP FlexRAM.

* MIPI-DBI

  * Introduced a new :ref:`MIPI DBI driver class <mipi_dbi_api>`.

* Pin control

  * Renesas R-Car pinctrl driver now supports Gen4 SoCs.
  * Renamed ``CONFIG_PINCTRL_RA`` to :kconfig:option:`CONFIG_PINCTRL_RENESAS_RA`.
  * Renesas R-Car pinctrl driver now supports voltage control for R8A77951 and
    R8A77961 SoCs.
  * Added driver for ZynqMP / Mercury XU.
  * Added driver for i.MX8QM/QXP.
  * Added driver for Renesas RZ/T2M.
  * On STM32 devices, pins assigned to JTAG/SW port can now be put to analog state when
    :kconfig:option:`CONFIG_PM` enabled and :kconfig:option:`CONFIG_DEBUG` disabled.

* PWM

  * Fixed ESP32S3 low frequency PWM issue.

* Regulators

  * Added new API functions

    * :c:func:`regulator_set_active_discharge`
    * :c:func:`regulator_get_active_discharge`
    * :c:func:`regulator_list_current_limit`

  * ``startup-delay-us`` and ``off-on-delay-us`` are now supported for all regulators.
  * Added non-multithreading support.
  * Added support for :dtcompatible:`maxim,max20335-regulator`.
  * Added ASYS UVLO configuration for :dtcompatible:`nxp,pca9420`.
  * Added LDO/DCDC support for :dtcompatible:`renesas,smartbond-regulator`.
  * Added LDO soft start configuration for :dtcompatible:`nordic,npm1300-regulator`.
  * Fixed init priority for :dtcompatible:`x-powers,axp192-regulator`.
  * Fixed LDO GPIO control for :dtcompatible:`nordic,npm1300-regulator`.

* Retained memory

  * Retained memory driver backend for registers was added.

  * Retained memory API status was changed from experimental to unstable.

* RTC

  * Added Atmel SAM driver.

* SMBUS:

  * SMBUS is now supported on STM32 devices.

* SDHC

  * Added SDHC driver for Cadence SDHC IP.
  * Added SDHC driver for Infineon CAT1 IP.
  * Added support for SDIO commands to iMX USDHC SDHC driver.

* Sensor

  * Fixed arithmetic overflow in the LTRF216A driver.
  * Fixed negative temperature calculation in MAX31865 driver.
  * Added TI TMAG5273 3D Hall sensor driver.
  * Added Vishay VCNL36825T proximity sensor driver.
  * Added BMA4xx accelerometer sensor emulator.
  * Added white channel support to the VEML7700 ambient light sensor driver.
  * Added ST LIS2DE12 accelerometer sensor driver.
  * Added Bosch BMP581 pressure sensor driver.
  * Added support for triggering multiple sensor devices in the sensor shell.
  * Added Aosong AGS10 TVOC air quality gas sensor driver.
  * Extended MAX31865 temperature sensor driver to support changing three-wire
    mode at runtime.
  * Fixed Bosch BMI160 gyro range calculation and added support for getting
    attributes.
  * Optimized Bosch BMA4xx accelerometer sample calculation, improving
    accuracy.
  * Removed floating point arithmetic from the TI BQ274xx gauge driver.
  * Fixed ST drivers Kconfig dependency to the HAL_ST module.
  * Added Bosch BMA4xx accelerometer sensor driver.
  * Added ST LIS2DU12 accelerometer sensor driver.
  * Extended NTC thermistor driver to support TDK NTCG103JF103FT1.
  * Added NXP S32 quadrature decoder driver.
  * Fixed LSM6DSV16x gyro range table.
  * Fixed missing return value checks in ADLTC2990, TSL2540, MAX17055 drivers.
  * Added ST LPS28DFW pressure sensor driver.
  * Fixed interrupt in BMI323 driver.
  * Added devicetree properties macros to various ST sensor drivers.
  * Added Renesas HS300x temperature/humidity sensor driver.
  * Added Gas Sensing Solutions' ExplorIR-M CO2 sensor driver.
  * Fixed self test delay in ADXL367 accelerometer sensor driver.
  * Added ST LPS22DF pressure sensor driver.
  * Added new streaming APIs and implemented in the ICM42688 driver.
  * Added trigger support to the ADXL367 accelerometer sensor driver.
  * Added PM suspend and resume support to the LSM6DSL accelerometer sensor
    driver.
  * Added AMS TSL2561 light sensor driver.
  * Extended BQ274xx driver to support configuring and confirming the chemistry
    profile.
  * Extended LIS2DH and LSM6DSV16x drivers to support configuring INT1/INT2 in
    devicetree.
  * Added die temperature measurement support to NPM1300 charger driver.
  * Added ADLTC2990 sensor emulator.
  * Extended MPU6050 driver to support MPU6886 variant.
  * Added ADXL367 accelerometer sensor driver.
  * Added LiteOn LTR-F216A illuminance sensor driver.
  * Added Memsic MC3419 accelerometer sensor driver.
  * Added AMD SB temperature sensor driver.
  * Added ESP32S3 internal temperature sensor driver.
  * Added new self-documenting macros for setting ST sensor devicetree
    properties (e.g., LSM6DSV16X_DT_ODR_AT_60Hz).  (:github:`65410`)

* Serial

  * Added drivers to support UART on Renesas RA and RZ/T2M.
  * Added support for higher baud rate for ITE IT8xxx2.
  * Added driver to support Intel Lightweight UART.
  * Added UART asynchronous RX helper.
  * Added support for async API on NS16550 driver.
  * Updated ``uart_esp32`` to use serial port configuration from devicetree.
  * Added an adaptation API to provide interrupt driven API for drivers
    which have only implemented async API.

  * Emulated UART driver (:file:`drivers/serial/uart_emul.c`):

    * Added emulated interrupt based TX.
    * Added emulated error for testing.
    * Modified to use local work queue for data transfer.
    * Modified FIFO size and its handling to be more aligned with real hardware.

  * On STM32 devices, it is now possible to enable FIFO by setting a ``fifo-enable``
    property in targeted serial node, with the following benefits:
    In TX, FIFO allows to work in burst mode, easing scheduling of loaded applications.
    It also allows more reliable communication with UART devices sensitive to variation of inter-frames delays.
    In RX, FIFO reduces overrun occurrences.

* SPI

  * On STM32H7 devices, ``fifo-enable`` property allows using SPI block FIFO. This
    feature is still experimental and requires maturation.
  * On STM32 devices impacted by BSY bit erratum, a workaround was implemented.

* USB

  * On STM2G0 devices, property ``crs-usb-sof`` in ``clk_hsi48`` node enables support
    for Clock Recovery System, allowing a more stable HSI48 clock and hence resilient USB
    connection.
  * On compatible STM32 devices, isochronous endpoints are now functional thanks to the
    use of double buffering.
  * Added new UDC driver for DWC2 controller.
  * Added support for Nuvoton NuMaker series USBD controllers.

* W1

  * Added 1-Wire GPIO master driver. See the :dtcompatible:`zephyr,w1-gpio`
    devicetree binding for more information.

* Wi-Fi

  * Added Infineon airoc driver.
  * Fixed esp32 driver. Decreased minimum heap size, disabled automatic reconnection on leaving.
  * Fixed esp_at driver. Allow building without IPv4 support. Passive Receive mode fixes. Depend on UART runtime configuration.
  * Fixed winc1500 driver. Disconnect result event was not returned when disconnecting.

Networking
**********

* CoAP:

  * Added support for Echo and Request-Tag CoAP options (RFC 9175).
  * Changed :c:func:`coap_remove_observer` API function return type to bool.
  * Introduced CoAP service library, which simplifies implementation of CoAP
    server functionality.
  * Updated CoAP server example to use CoAP service library.
  * Added shell module for CoAP server.
  * Fixed NULL pointer dereference in :c:func:`coap_packet_remove_option`.
  * Added CoAP observer/service network events using the Network Event subsystem.
  * Changed :c:func:`coap_pending_init` API function to take
    :c:struct:`coap_transmission_parameters` instead of retry count.
  * Added new API functions:

    * :c:func:`coap_get_transmission_parameters`
    * :c:func:`coap_set_transmission_parameters`
    * :c:func:`coap_handle_request_len`
    * :c:func:`coap_well_known_core_get_len`
    * :c:func:`coap_uri_path_match`
    * :c:func:`coap_packet_is_request`
    * :c:func:`coap_find_observer`
    * :c:func:`coap_find_observer_by_token`
    * :c:func:`coap_pendings_count`
    * :c:func:`coap_header_set_code`

* Connection Manager:

  * Added a generic Wi-Fi connectivity backend.

* DHCP:

  * Added missing DHCPv6 state structure initialization when initializing
    network interface.
  * DHCP-assigned IPv4 address is now removed when interface goes down.
  * Added DHCPv4 server implementation.
  * Rearranged DHCPv4 file structure. All DHCPv4 related files are now grouped
    within ``subsys/net/lib/dhcpv4``.
  * Moved DHCPv6 files to ``subsys/net/lib/dhcpv6`` to align with DHCPv4.

* DNS:

  * Added support for enabling mDNS listener on all network interfaces.
  * Added VLAN support to the ``mdns_responder`` sample.
  * Fixed TTL/hop limit set on DNS packets.
  * Added :kconfig:option:`CONFIG_DNS_RESOLVER_AUTO_INIT` which allows to disable
    automatic initialization of the default DNS context on boot.

* Ethernet:

  * Manual registration of ARP entries is now supported.
  * Added PHY mode selection to device tree.
  * Added TX-Injection mode support.

* gPTP:

  * The local port identity is now used when forwarding sync messages.
  * Fixed double converted byte order of BMCA info.
  * GM PRIO root system id is now always used for announce messages.
  * Created gPTP handler thread stack size Kconfig option.
  * Inverted the priority of outgoing packets.

* ICMP:

  * Fixed an error being emitted when unhandled ICMP message was received.
  * Fixed a bug where ICMP Echo Reply could be sent without proper source IP
    address set.
  * Fixed a packet leak in ICMP Echo Request handlers in case priority check
    failed.
  * Improved thread safety of the module handling Neighbor Discovery.
  * Added support for IPv6 Neighbor reachability hints, allowing to reduce
    ICMPv6 traffic for active connections.

* IP:

  * Fixed L3/L4 checksum calculation/validation for IP-fragmented packets on
    interfaces that support checksum offload.
  * Fixed net_context not being set on IP fragmented packets, preventing send
    callback from being called.
  * It is now possible to have separate IPv4 TTL value and IPv6 hop limit value for
    unicast and multicast packets. This can be controlled in each socket via
    :c:func:`setsockopt` API.
  * Improved source IP address verification in the IP stack. Addresses received
    to/from loopback address on non-loopback interfaces are dropped.
  * Added new functions to verify if IPv6 address is site local or global.
  * Added support for setting peer IP address in :c:struct:`net_pkt` structure
    for offloaded interfaces. This allows for :c:func:`recvfrom` to return a
    valid address in offloaded case.

* LwM2M:

  * Added :kconfig:option:`CONFIG_LWM2M_UPDATE_PERIOD` which configures the LwM2M
    Update period regardless of the lifetime value.
  * Fixed composite read/write access rights check.
  * Added shell command to delete object and resource instances.
  * Fixed a bug in block-wise transfer where block-wise ACKs were sent with
    wrong response code.
  * Fixed object version reporting for LwM2M version 1.1.
  * Added support for DTLS Connection Identifier in the LwM2M engine.
  * Added support for LwM2M Server Disable executable resource.
  * Implemented fallback mechanism for LwM2M server selection during registration
    phase. The engine will now try to choose a different server if the current one
    becomes unavailable or disabled.
  * Added support for storing LwM2M error list in settings.
  * Fixed pmin observer attribute handling in tickless mode.
  * Added support for notifying the application about ongoing CoAP transmissions
    with ``set_socket_state()`` callback.
  * Deprecated unsigned 64-bit integer value type, as it's not represented in the spec.
    Use signed 64-bit integer instead.
  * Added a callback for LwM2M Gateway object, which allows to handle LwM2M messages
    with prefixed path.
  * Added LwM2M-specific macros for object initialization during boot.
  * Several other minor bugfixes ans improvements.

* Misc:

  * Added support for compile time network event handlers using the macro
    :c:macro:`NET_MGMT_REGISTER_EVENT_HANDLER`.
  * Added the :kconfig:option:`CONFIG_NET_MGMT_EVENT_WORKER` choice to
    allow emitting network events using the system work queue or synchronously.
  * Removed redundant Network Connectivity API documentation page.
  * Improved thread safety of the network connections subsystem.
  * Removed ``eth_native_posix`` sample.
  * Removed redundant ``arb`` and ``fv2015`` fields  from
    ``struct net_pkt_cb_ieee802154``.
  * Introduced a separate mutex for TX at the network interface level to prevent
    concurrent access on TX to drivers that are not re-entrant.
  * Fixed netmask not being registered for loopback address.
  * Added support for binding to a specific network interface at the net_context
    level.
  * Added IGMPv3 support.
  * Added a new network event, ``NET_EVENT_HOSTNAME_CHANGED``, triggered upon
    hostname change.
  * Refactored net_context option getters/setters to reduce code duplication.
  * Fixed a possible packet leak at the ARP level, in case of errors during ARP
    packet creation.
  * Added support for analyzing SNTP time uncertainty.
  * Fixed network interface being brought up even when underlying device is not
    ready.
  * Added start/stop functions for dummy interfaces.
  * Added a detailed :ref:`network configuration <network_configuration_guide>`
    guide to the documentation.
  * Added :kconfig:option:`CONFIG_NET_HOSTNAME_DYNAMIC` option, which allows to
    enable setting hostname at runtime.

* MQTT-SN:

  * Added :c:func:`mqtt_sn_get_topic_name` API function.
  * Fixed handling of incoming Register messages when wildcard subscription is used.

* OpenThread:

  * Implemented the following OpenThread platform APIs:

    * ``otPlatRadioSetRxOnWhenIdle()``
    * ``otPlatResetToBootloader()``
    * ``otPlatCryptoPbkdf2GenerateKey()``

  * Updated OpenThread platform UART driver so that it no longer waits for
    communication with a host to start during boot.
  * Added BLE TCAT implementation in OpenThread platform.
  * Updated Crypto PSA backend for OpenThread with additional algorithms.
  * Fixed ``otPlatAssertFail()`` so that it prints the location of the actual
    assert instead of the function itself.

* PPP:

  * Fixed PPP connection termination when interface goes down.

* Shell:

  * Refactored networking shell module so that instead of large single file, it
    is split into submodules, on a per command basis.
  * Fixed unexpected timeout message when executing loopback ping.
  * Added ``net sockets`` command to print information about open sockets and
    socket services.
  * Join IPv4/IPv6 multicast groups, if needed, when adding IPv4/IPv6 multicast
    addresses via shell.
  * Fixed ``tcp connect`` command operation (TCP context released prematurely).
  * Added support for Echo option in telnet shell backend.
  * Fixed unnecessary connection close in telnet shell backend in case of
    non-fatal EAGAIN or ENOBUFS errors.
  * Fixed double packet dereference in ping reply handler.
  * Fixed possible deadlock when executing ``net arp`` command.
  * Added more detailed Ethernet statistics printout for ``net stats`` command.
  * Added ``net dhcpv4 server`` commands for DHCPv4 server management.
  * Added shell module to manage TLS credentials.

* Sockets:

  * Added support for v4-mapping-to-v6, which allows IPv4 and IPv6 to share the
    same port space.
  * Added support for :c:macro:`IPV6_V6ONLY` socket option.
  * Added support for :c:macro:`SO_ERROR` socket option.
  * Fixed :c:func:`select` not setting ``writefds`` in case of errors.
  * Added support for object core, which allows to track networks sockets and
    their statistics.
  * Added support for :c:func:`recvmsg`.
  * Added support for :c:macro:`IP_PKTINFO` and :c:macro:`IPV6_RECVPKTINFO`
    socket options.
  * Added support for :c:macro:`IP_TTL` socket option.
  * Added support for IPv4 multicast :c:macro:`IP_ADD_MEMBERSHIP` and
    :c:macro:`IP_DROP_MEMBERSHIP` socket options.
  * Added support for IPv6 multicast :c:macro:`IPV6_ADD_MEMBERSHIP` and
    :c:macro:`IPV6_DROP_MEMBERSHIP` socket options.
  * Improved doxygen documentation of BSD socket API.
  * Fixed POLLERR error reporting in TLS sockets.
  * Fixed DTLS handshake processing during :c:func:`poll`.
  * Aligned DTLS socket :c:func:`connect` behavior with regular TLS (handshake
    during connect call).
  * Added Socket Service library, which allows registering multiple socket-based
    network services and processing them within a single thread.
  * Added a new ``echo_service`` sample for Socket Service.
  * Added support for :c:macro:`SO_DOMAIN` socket option.
  * Fixed DTLS connection timeout when monitoring socket with :c:func:`poll`.
  * Fixed NULL link layer address pointer dereference on packet socket, in case
    of packet loopback.
  * Several other minor bugfixes and improvements.

* TCP:

  * TCP stack now replies with RST packet in response to connection attempt on
    a closed port.
  * Fixed remote address passed in :c:func:`accept` call.
  * Fixed reference counting during active handshake to prevent TCP context
    being released prematurely.
  * Fixed compilation with :kconfig:option:`CONFIG_NET_TCP_CONGESTION_AVOIDANCE`
    disabled.
  * Reworked TCP data queueing API to prevent TCP stack from overflowing TX window.
  * Fixed possible race condition between TCP workqueue and other threads when
    releasing TCP context.
  * Fixed possible race condition between input thread and TCP workqueue.
  * Added support for TCP Keep-Alive feature.
  * Fixed a bug where TCP state machine could get stuck in LAST_ACK state
    during passive connection close.
  * Fixed a bug where TCP state machine could get stuck in FIN_WAIT_1 state
    in case peer did not respond.
  * Several other minor bugfixes ans improvements.

* TFTP:

  * Fixed potential buffer overflow when copying TFTP error message.
  * Improved logging in case of errors.

* Wi-Fi:

  * Added Wi-Fi driver version information to Wi-Fi shell.
  * Added AP (Access Point) mode support to Wi-Fi shell.
  * Added Regulatory channel information.
  * Added Wi-Fi bindings to connection manager.
  * Fixed Wi-Fi shell. SSID print fixes. Help text fixes. Channel validation fixes.
  * Fixed TWT functionality. Teardown status was not updated. Powersave fixes.

* zperf:

  * Improved IP address binding. Zperf will now bind to any address by default and
    allow to override this with Kconfig/API provided address.
  * Fixed TCP packet counting when transmitting.
  * Refactored UDP/TCP received to use Socket Service to save memory.
  * Fixed zperf session leak on interrupted downloads.
  * Fixed the calculation ratio between Mbps, Kbps and bps.
  * The zperf sample now supports relocating network code to RAM.

USB
***

* Device support:

  * Introduced new USB Audio 2 implementation that uses devicetree for
    instantiation, hiding descriptor complexity from the application. The initial
    implementation is limited to full speed only and provides the absolute
    minimum set of features required for basic implicit and explicit feedback.
    Interrupt notification is not supported.
  * Added support for SetFeature(TEST_MODE).

Devicetree
**********

Bindings
========

  * Introduced new SPI properties ``spi-cpol``, ``spi-cpha``, and ``spi-hold-cs`` to be used by
    the macro :c:macro:`SPI_CONFIG_DT` in order to set SPI mode in a Devicetree file.

Libraries / Subsystems
**********************

* Management

  * Fixed an issue in MCUmgr image management whereby erasing an already erased slot would return
    an unknown error. It now returns success.

  * Fixed MCUmgr UDP transport structs being statically initialised. This results in about a
    ~5KiB flash saving.

  * Fixed an issue in MCUmgr which would cause a user data buffer overflow if the UDP transport was
    enabled on IPv4 only but IPv6 support was enabled in the kernel.

  * Implemented datetime functionality in MCUmgr OS management group. This makes use of the RTC
    driver API.

  * Fixed an issue in MCUmgr console UART input whereby the FIFO would be read outside of an ISR,
    which is not supported in the next USB stack.

  * Fixed an issue whereby the ``mcuboot erase`` DFU shell command could be used to erase the
    MCUboot or currently running application slot.

  * Fixed an issue whereby messages that were too large to be sent over the UDP transport would
    wrongly return :c:enum:`MGMT_ERR_EINVAL` instead of :c:enum:`MGMT_ERR_EMSGSIZE`.

  * Fixed an issue where confirming an image in Direct XIP mode would always confirm the image in
    the primary slot even when executing from the secondary slot. Now the currently active image is
    always confirmed.

  * Added support for retrieving registered command groups, to support registering and deregistering
    default command groups at runtime, allowing an application to support multiple implementations
    for the same command group.

  * Fixed an issue in MCUmgr FS management whereby the semaphore lock would not be given if an
    error was returned, leading to a possible deadlock.

  * Added support for custom payload MCUmgr handlers. This can be enabled with
    :kconfig:option:`CONFIG_MCUMGR_MGMT_CUSTOM_PAYLOAD`.

  * Fixed an issue in MCUmgr image management whereby an error would be returned if a command was
    sent to erase the slot which was already erased.

  * Added support for image slot size checking to ensure an update can be utilised by MCUboot.
    This can be performed by using sysbuild when building both application and MCUboot by enabling
    :kconfig:option:`CONFIG_MCUMGR_GRP_IMG_TOO_LARGE_SYSBUILD` or by use of bootloader information
    sharing from MCUboot by enabling
    :kconfig:option:`CONFIG_MCUMGR_GRP_IMG_TOO_LARGE_BOOTLOADER_INFO`.

* Logging

  * Added an option to remove string literals from the binary when dictionary-based logging is used.

  * Optimized the most common logging messages (strings with up to 2 numeric arguments). Optimization
    is done for code size (significant gain seen on riscv32) and performance.

  * Extended logging frontend API to optionally implement dedicated functions for optimized messages.
    Optional API is enabled by :kconfig:option:`CONFIG_LOG_FRONTEND_OPT_API`.

  * Added support for runtime message filtering for the logging frontend.

  * Added option to support multiple instances of the UART logging backend.

  * Fixed userspace issue for :c:func:`printk` when :kconfig:option:`CONFIG_LOG_PRINTK` is enabled.

  * Added compile time detection of logging messages that use character pointers for ``%p``.
    It must be avoided when dictionary-based logging is used and strings are stripped from the
    binary. When an erroneous case is detected, the user message is replaced with an error message
    that suggests pointer casting must be added.

  * Removed remaining references to v2 logging. Renamed :c:func:`log2_generic` to :c:func:`log_generic`.

* Modem modules

  * Added ``TRANSMIT_IDLE`` event to the ``modem_pipe`` module which notifies the user of the pipe
    that the backend has transmitted all bytes placed in its buffer using
    :c:func:`modem_pipe_transmit()`.
    The event greatly increases the efficiency of transmitting large quantities of data if used to
    dynamically manage the delay between calls to :c:func:`modem_pipe_transmit()`.

  * Implemented ``TRANSMIT_IDLE`` event in all modem backends.

  * Extended all modem modules to utilize the ``TRANSMIT_IDLE`` event to dynamically manage the delay
    between calls to :c:func:`modem_pipe_transmit()`. This addition reduced the utilization of the
    system workqueue while transmitting large, continuous quantities of data, by 86%, while only
    reducing the throughput by 12%. This optimization additionally allows lower priority threads,
    like the deferred logging thread, to run during the transmission (it was blocked by the
    relentless, continuous calls to :c:func:`modem_pipe_transmit()`).

  * Improved ``modem_pipe`` event dispatching. The ``modem_pipe`` module now invokes the
    ``RECEIVE_READY`` event every time the pipe is attached using :c:func:`modem_pipe_attach()`
    if it has data ready to be read, and always invokes ``TRANSMIT_IDLE`` when the pipe is
    either opened or attached. This ensures event driven users of the modem pipe module can
    rely solely on the events to start read/transmit work. A test suite has been added to
    complement the improvements.

  * Extended ``modem_cmux`` module to support acting both as DTE (user application) and DCE (modem).
    With this addition, two Zephyr applications can communicate with each other through their
    respective ``modem_cmux`` instances.

* Picolibc

  * Updated to version 1.8.6. This removes the :c:macro:`_POSIX_C_SOURCE` definition from the build
    system, so applications will need to add this if they use APIs outside of the Zephyr
    requirements.

  * Added new :c:func:`printf` modes, :kconfig:option:`CONFIG_PICOLIBC_IO_LONG_LONG` and
    :kconfig:option:`CONFIG_PICOLIBC_IO_MINIMAL`. These provide applications with finer grained
    control over the level of support provided by the library to control text space usage. By
    default, the correct level of support is selected based upon other configuration parameters.

  * Added :kconfig:option:`CONFIG_PICOLIBC_ASSERT_VERBOSE`. This option, which is false by default,
    controls whether the :c:func:`assert` function displays verbose information, including the file
    name, line number, function name and failing expression text, when the assertion fails. Leaving
    this disabled saves text space.

  * :kconfig:option:`CONFIG_THREAD_LOCAL_STORAGE` can now be disabled while using Picolibc. This is
    very helpful in diagnosing issues when using Picolibc as those are often caused by enabling TLS
    and not caused by using the library itself.

  * Numerous improvements in the library including code-size reductions in areas like printf and
    ctype and various fixes in the math library.

* Power management

  * Introduced Atmel SAM SUPC functions to allow wakeup sources and poweroff.
  * STM32F4 devices now support stop mode thanks to the use of a RTC based idle timer which
    keeps track of tick evolution while cortex systick is off.

  * :c:func:`pm_device_runtime_put_async()` got a parameter to specify a minimum delay to
    the operation. This is useful to avoid multiple states transitions when a device is used.

  * Devices that don't need to block when suspending or resuming can now be defined as ISR
    safe (``PM_DEVICE_ISR_SAFE``). For those devices, Zephyr is able to reduce RAM consumption
    and runtime device power management can be safely used from interrupts.

  * Optimizations in device runtime power management. :c:func:`pm_device_runtime_get` and
    :c:func:`pm_device_runtime_put` no longer wait for a pending operation to be concluded if it is still
    in the work queue. In this case, the pending work is just canceled and the device state updated.

  * The Kconfig options below were added to customize the initialization priority of different
    power domains.

    * :kconfig:option:`CONFIG_POWER_DOMAIN_GPIO_INIT_PRIORITY`
    * :kconfig:option:`CONFIG_POWER_DOMAIN_GPIO_MONITOR_INIT_PRIORITY`
    * :kconfig:option:`CONFIG_POWER_DOMAIN_INTEL_ADSP_INIT_PRIORITY`

* Crypto

  * Mbed TLS updated to 3.5.2. Full release notes can be found in:
    https://github.com/Mbed-TLS/mbedtls/releases/tag/v3.5.2

* Retention

  * Fixed issue whereby :kconfig:option:`CONFIG_RETENTION_BUFFER_SIZE` values over 256 would cause
    an infinite loop due to use of 8-bit variables.

* SD

  * Added support for SDIO devices.

* Storage

  * File systems: LittleFS module has been updated to version 2.8.1.

  * Following Flash Map API macros, marked in 3.2 as deprecated, have been removed:
    ``FLASH_AREA_ID``, ``FLASH_AREA_OFFSET``, ``FLASH_AREA_SIZE``,
    ``FLASH_AREA_LABEL_EXISTS`` and ``FLASH_AREA_DEVICE``.

* POSIX API

  * Completed support for ``POSIX_THREADS_EXT``, ``XSI_THREADS_EXT``,
    ``POSIX_CLOCK_SELECTION``, and ``POSIX_SEMAPHORES`` Option Groups.

  * Completed support for ``_POSIX_MESSAGE_PASSING`` and
    ``_POSIX_PRIORITY_SCHEDULING`` Options.

  * Fixed Coverity-CID 211585, 334906, 334909, and 340851.

  * Improved structure and accuracy of POSIX documentation.

  * Improved navigation and organization of POSIX Kconfig options.

  * Added support to allocate and free stacks up to 8 MB with pthread_attr_t.

  * Added support for deferred and asynchronous thread cancellation.

  * Added dining philosophers sample application.

  * Added support for named semaphores.

  * Added a top-level ``posix`` command in the Zephyr shell. Zephyr shell utilities for
    the POSIX API can be added as subcommands (e.g. ``posix uname -a``)

  * Added support for async thread cancellation and ``SIGEV_THREAD``, ``CLOCK_REALTIME``.

  * Added compile-time-constant sysconf() implementation.

* LoRa/LoRaWAN

 * Added LoRaWAN remote multicast support with :kconfig:option:`CONFIG_LORAWAN_REMOTE_MULTICAST`
   in preparation for OTA firmware upgrade support.

* ZBus

  * Replaced mutexes with semaphores to lock channels and implement the Highest Locker Protocol (HLP)
    priority boost for the zbus operations. This feature avoids priority inversions and preemptions,
    making the VDED delivery process faster and more consistent. (:github:`63183`)

  * Fixed documentation for :c:func:`zbus_chan_add` and :c:func:`zbus_chan_rm` adding the timeout
    argument. (:github:`65544`)

  * Fixed warning when mixing C and C++ files using zbus. (:github:`65222`)

  * :c:macro:`ZBUS_CHANNEL_DEFINE` macro is now compatible with C++. (:github:`65196`)

  * Fixed parameter order of net buf pool fixed definition. (:github:`65039`)

  * Refactored the benchmark sample, adding message subscribers. (:github:`64524`)

  * Renamed ``CONFIG_ZBUS_MSG_SUBSCRIBER_NET_BUF_DYNAMIC`` and
    ``CONFIG_ZBUS_MSG_SUBSCRIBER_NET_BUF_STATIC`` to
    :kconfig:option:`CONFIG_ZBUS_MSG_SUBSCRIBER_BUF_ALLOC_DYNAMIC` and
    :kconfig:option:`CONFIG_ZBUS_MSG_SUBSCRIBER_BUF_ALLOC_STATIC`. (:github:`65632`)

HALs
****

* STM32

  * Updated STM32F1 to cube version V1.8.5.
  * Updated STM32F7 to cube version V1.17.1.
  * Updated STM32H7 to cube version V1.11.1.
  * Updated STM32L4 to cube version V1.18.0.
  * Updated STM32U5 to cube version V1.4.0.
  * Updated STM32WBA to cube version V1.2.0.
  * Updated STM32WB to cube version V1.18.0.

MCUboot
*******

  * Fixed compatible sector checking in bootutil.

  * Fixed Kconfig issue with saving encrypted TLVs not depending on encryption being enabled.

  * Fixed issue with missing condition check for applications in sysflash include file.

  * Fixed issue with single slot encrypted image listing support in boot_serial.

  * Fixed issue with allowing MBEDTLS Kconfig selection when tinycrypt is used.

  * Fixed missing response if echo command was disabled in boot_serial.

  * Fixed issue with USB configurations not generating usable images.

  * Added debug logging for boot status write in bootutil.

  * Added estimated image overhead size to cache in sysbuild.

  * Added firmware loader operating mode which allows for a dedicated secondary slot image that
    is used to update the primary image.

  * Added error if main thread is not pre-emptible when USB CDC serial recovery is enabled.

  * Added error if USB CDC and console are both enabled and set to the same device.

  * Removed the deprecated ``CONFIG_ZEPHYR_TRY_MASS_ERASE`` Kconfig option.

  * Updated zcbor to version 0.8.1 and re-generated boot_serial files.

  * Moved IO functions out of main to separate file.

  * Made ``align`` parameter of imgtool optional.

  * Added MCUBoot support for ``mimxrt1010_evk``, ``mimxrt1015_evk``,
    ``mimxrt1040_evk``, ``lpcxpresso55s06``, ``lpcxpresso55s16``,
    ``lpcxpresso55s28``, ``lpcxpresso55s36``, ``lpcxpresso55s69_cpu0``.

  * Added :kconfig:option:`CONFIG_MCUBOOT_IMGTOOL_OVERWRITE_ONLY` which passes the --overwrite-only option
    to imgtool to avoid adding the swap status area size when calculating overflow.
    It is used by non-swap update modes.

  * The MCUboot version in this release is version ``2.1.0+0-dev``.

zcbor
*****

zcbor has been updated from 0.7.0 to 0.8.1.
Full release notes can be found at:
https://github.com/zephyrproject-rtos/zcbor/blob/0.8.0/RELEASE_NOTES.md and
https://github.com/zephyrproject-rtos/zcbor/blob/0.8.1/RELEASE_NOTES.md

Highlights:

* Addded support for unordered maps.
* Performance improvements.
* Naming improvements for generated code.
* Bugfixes.

LVGL
****

LVGL has been updated from 8.3.7 to 8.3.11.
Detailed release notes can be found at:
https://github.com/zephyrproject-rtos/lvgl/blob/zephyr/docs/CHANGELOG.md

Additionally, the following changes in Zephyr were done:

  * Added the :dtcompatible:`zephyr,lvgl-keypad-input` compatible for keypad input.

  * Fixed issue with the Zephyr log levels not mapping properly to LVGL log levels.

  * Fixed issue where setting :kconfig:option:`CONFIG_LV_Z_FULL_REFRESH` did not
    set :kconfig:option:`CONFIG_LV_Z_VDB_SIZE` to 100 percent.

Tests and Samples
*****************

* :ref:`native_sim<native_sim>` has replaced :ref:`native_posix<native_posix>` as the default
  test platform.
  :ref:`native_posix<native_posix>` remains supported and used in testing but will be deprecated
  in a future release.

* Bluetooth split stacks tests, where the BT host and controller are run in separate MCUs, are
  now run in CI based on the :ref:`nrf5340_bsim<nrf5340bsim>` targets.
  Several other runtime AMP tests based on these targets have been added to CI, including tests
  of OpenAMP, the mbox and IPC drivers/subsystem, and the logger multidomain functionality.

* Runtime UART tests have been added to CI based on the :ref:`nrf52_bsim<nrf52_bsim>` target.
  These include tests of the nRFx UART driver and networked BT stack tests with the host and
  controller in separate devices communicating over the HCI UART driver.

* Fixed an issue in :zephyr:code-sample:`smp-svr` sample whereby if USB was already initialised,
  application would fail to boot properly.

* Added an LVGL sample :zephyr:code-sample:`lvgl-accelerometer-chart` showcasing displaying of live
  sensor data in a chart widget.

* Added ESP32-S3 IPM support in :zephyr:code-sample:`ipm-esp32`.

* Added ESP32 memory-mapped flash access sample in :zephyr:code-sample:`esp32-flash-memory-mapped`.

* Added ESP32 PWM loopback test case.

* Added support in the mbox sample for NXP boards ``MIMXRT1160-EVK``, ``MIMXRT1170-EVK``,
  ``MIMXRT1170-EVKB``, ``LPCXpresso55S69``.

* Added a sample ``flexram-magic-addr`` for ``mimxrt11xx_cm7`` to show how to use flexram magic
  address functionality when using memc flexram driver.
