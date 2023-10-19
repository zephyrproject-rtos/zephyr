:orphan:

.. _zephyr_3.5:

Zephyr 3.5.0
############

We are pleased to announce the release of Zephyr version 3.5.0.

Major enhancements with this release include:

* Added support for linkable loadable extensions (llext)
* Added native_sim simulator target (successor to native_posix)
* Added new battery charger driver API
* Added new hardware spinlock driver API
* Added new modem subsystem
* Added support for 45+ new boards
* Networking: improvements to CoAP, Connection Manager, DHCP, Ethernet, gPTP, ICMP,
  IPv6 and LwM2M
* Bluetooth: improvements to the Controller, Audio, Mesh, as well as the host stack in
  general
* Improved LVGL graphics library integration
* Integrated support with the CodeChecker static analyzer
* Picolibc is now the default C standard library

An overview of the changes required or recommended when migrating your application from Zephyr
v3.4.0 to Zephyr v3.5.0 can be found in the separate :ref:`migration guide<migration_3.5>`.

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************
The following CVEs are addressed by this release:

More detailed information can be found in:
https://docs.zephyrproject.org/latest/security/vulnerabilities.html

* CVE-2023-3725 `Zephyr project bug tracker GHSA-2g3m-p6c7-8rr3
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-2g3m-p6c7-8rr3>`_

* CVE-2023-4257 `Zephyr project bug tracker GHSA-853q-q69w-gf5j
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-853q-q69w-gf5j>`_

* CVE-2023-4258 `Zephyr project bug tracker GHSA-m34c-cp63-rwh7
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-m34c-cp63-rwh7>`_

* CVE-2023-4259 `Zephyr project bug tracker GHSA-gghm-c696-f4j4
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-gghm-c696-f4j4>`_

* CVE-2023-4260 `Zephyr project bug tracker GHSA-gj27-862r-55wh
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-gj27-862r-55wh>`_

* CVE-2023-4263 `Zephyr project bug tracker GHSA-rf6q-rhhp-pqhf
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-rf6q-rhhp-pqhf>`_

* CVE-2023-4264 `Zephyr project bug tracker GHSA-rgx6-3w4j-gf5j
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-rgx6-3w4j-gf5j>`_

* CVE-2023-4424: Under embargo until 2023-11-01

* CVE-2023-5055: Under embargo until 2023-11-01

* CVE-2023-5139: Under embargo until 2023-10-25

* CVE-2023-5184 `Zephyr project bug tracker GHSA-8x3p-q3r5-xh9g
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-8x3p-q3r5-xh9g>`_

* CVE-2023-5563 `Zephyr project bug tracker GHSA-98mc-rj7w-7rpv
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-98mc-rj7w-7rpv>`_

Kernel
******

* Added support for dynamic thread stack allocation via :c:func:`k_thread_stack_alloc`
* Added support for :c:func:`k_spin_trylock`
* Added :c:func:`k_object_is_valid` to check if a kernel object is valid. This replaces
  code that has been duplicated throughout the tree.

Architectures
*************

* ARC

 * Introduced the scalar port for ARC VPX processors
 * Introduced support for ARCv3 HS (both 32 and 64 bit) SMP platforms with up to 12 CPU cores
 * Reworked GNU helper tools usage for ARC MWDT toolchain. Now helper tools can be used from
   Zephyr SDK (if SDK is installed)
 * Fixed dynamic thread stack allocation
 * Fixed STR assembly macro offset calculation issue which may cause build error for ARCv3 64bit
 * Cleaned-up and made more user friendly handling of the ARC MWDT toolchain path
   (ARCMWDT_TOOLCHAIN_PATH)

* ARM

  * Architectural support for Arm Cortex-M has been separated from Arm
    Cortex-A and Cortex-R. This includes separate source modules to handle
    tasks like IRQ management, exception handling, thread handling and swap.
    For implementation details see :github:`60031`.

* ARM64

* RISC-V

  * Added support for detecting null pointer exception using PMP.
  * Added the :kconfig:option:`CONFIG_RISCV_RESERVED_IRQ_ISR_TABLES_OFFSET`
    option to allow IRQ vector at a specified offset to meet the requirements
    set by the Core-Local Interrupt Controller RISC-V specification.
  * Added the :kconfig:option:`CONFIG_RISCV_SOC_HAS_CUSTOM_SYS_IO` option to
    allow the use of custom system input/output functions.
  * Introduced the :kconfig:option:`CONFIG_RISCV_TRAP_HANDLER_ALIGNMENT` option
    to set the correct alignment of the trap handling code which is dependent on
    the ``MTVEC.BASE`` field size and is platform or application-specific.

* Xtensa

  * Added basic MMU v2 Support.

* x86

  * Added support for Intel Alder Lake boards
  * Added support for Intel Sensor Hub (ISH)

* POSIX

  * Has been reworked to use the native simulator.
  * New boards have been added.
  * For the new boards, embedded C libraries can be used, and conflicts with the host symbols
    and libraries avoided.
  * The :ref:`POSIX OS abstraction<posix_support>` is supported in these new boards.
  * AMP targets are now supported.
  * Added support for LLVM source profiling/coverage.

Bluetooth
*********

* Audio

  Improved memory usage of codec configurations and codec capabilities. Fixed several bugs in BAP
  and the BAP-related services (ASCS, PACS, BASS), as well as missing features such as proper
  notification handling.

  * Added BAP ``bt_bap_stream_get_tx_sync``
  * Added CAP stream send and tx sync
  * Added ``bt_audio_codec_cap_get`` helper functions
  * Added support for long read/write in CAP
  * Fixed ASCS Source ASE link loss state transition
  * Fixed ASCS possible ASE leak
  * Fixed ASCS to drop ISO PDUs if ASE is not in streaming state
  * Fixed BAP ``bt_bap_scan_delegator_find_state`` implementation
  * Fixed BAP issue with PA sync and ID in ``broadcast_sink_create``
  * Fixed TMAS characteristic permissions
  * Fixed ``tbs_client`` missing discovery complete event
  * Fixed audio stack to accept empty CCID list in audio metadata
  * Fixed bad size of metadata_backup in ASCS
  * Fixed possible ASCS ASE stuck in releasing state
  * Refactored ``bt_audio_codec_cap`` to flat arrays
  * Refactored ``bt_audio_codec_cfg`` to flat arrays
  * Removed ``CONFIG_BT_PACS_{SNK,SRC}_CONTEXT``
  * Removed scanning and PA sync from broadcast sink
  * Renamed ``bt_codec`` to ``bt_audio_codec_{cap, conf, data}``
  * Renamed codec qos framing
  * Replaced ``BT_AUDIO_CODEC_LC3_ID`` -> ``BT_HCI_CODING_FORMAT_LC3``
  * Replaced ``BT_AUDIO_CODEC_PARSE_ERR_`` values with errno values.
  * Reworked PACS notify system
  * Updated ASCS ISO QOS based on BAP QOS
  * Updated BAP to filter PA data duplicates by default
  * Updated CSIP to unlock Non-bonded devices immediately.
  * Updated PACS to notify bonded clients on reconnect
  * Updated ``bt_cap_stream_ops_register`` to always register BAP callbacks
  * Updated the ASCS ACL disconnect behavior
  * Updated to split ``bt_audio_codec_meta_get`` to ``cfg`` and ``cap``

* Direction Finding

* Host

  * Added SMP bondable flag overlay per connection
  * Added USE_NRPA advertising option
  * Added ``BT_CONN_PARAM_ANY`` to allow setting any value to connection parameters
  * Added advanced broadcast ISO parameters
  * Added advanced unicast ISO parameters
  * Added new API to manage Bluetooth settings storage
  * Fixed HCI ISO Data packets fragmentation
  * Fixed HCI ISO SDU length sent to controller
  * Fixed OTS ``bt_ots_init`` parameter struct naming
  * Fixed OTS memory leak while procedure is not finished
  * Fixed a connection reference leak
  * Fixed forced pairing request handling
  * Fixed host to invalidate the Resolvable Private Address when starting legacy advertising
  * Fixed issue with ``bt_iso_cig_reconfigure``
  * Fixed possible buffer overflow in ``bt_conn_le_start_encryption``
  * Fixed some SMP issues
  * Fixed to abort pairing if connection disconnected
  * Updated L2CAP accept callbacks
  * Updated LE L2CAP connected callback to be after connection response
  * Updated PAwR implementation to use RPA as responder address if BT_PRIVACY=y

* Mesh

  * Added TF-M support.
  * Added support to use both tinycrypt and PSA based crypto
  * Added full virtual addresses support with the collisions resolution. The
    :kconfig:option:`CONFIG_BT_MESH_LABEL_NO_RECOVER` Kconfig option is introduced to restore the
    addresses for the subscription list and model publication.
  * Added statistic module.
  * Fixed an issue where a node acting as a LPN was triggering Friend Poll messages when sending a
    segmented message over the loopback interface.
  * Fixed an issue where provisioning completes successfully on a node when the identical Public Key
    is used by a  provisioner.
  * Fixed an issue where the :c:func:`settings_load` function called from a cooperative thread other
    than the system workqueue caused the GATT Mesh Proxy Service registration to fail.
  * Fixed an issue where a node could enter IV Update in Progress state if an old SNB with the
    current IV Index and IV Update flag set to 1 was resent.

  * Mesh Protocol v1.1 changes

    * Added storing Private GATT Proxy state persistently.
    * Added support for Firmware Distribution Upload OOB Start message in the Firwmware Distribution
      Server model. The message support can be enabled with the
      :kconfig:option:`CONFIG_BT_MESH_DFD_SRV_OOB_UPLOAD` Kconfig option.
    * Added extended provisioning protocol timeout when OOB methods are used in the provisioning.
    * Added support for Composition Data Pages 2, 129 and 130.
    * Added documentation for Composition Data Pages 0, 1, 2, 128, 129 and 130.
    * Added documentation for the Segmentation and Reassembly in the Transport layer.
    * Added documentation for the SAR Configuration models
    * Fixed an issue where the Opcode Aggregator Server model did not compile without the Opcode
      Aggregator Client model.
    * Fixed an issue where the identity address was used in Private GATT Proxy advertisements
      instead of Non-Resolvable Private Addresses.
    * Fixed the Proxy Privacy parameter support.
    * Fixed an issue where the Composition Data Page 128 was not present on a node that has
      instantiated the Remote Provisioning Server model.
    * Fixed an issue where the Large Composition Data Server model did not support Composition Data
      Pages other then 0.
    * Fixed an issue where the Remote Provisioning Client model instanted on a node together with
      the Remote Provisioning Server model could not reprovision itself.
    * Fixed an issue where the acknowledgment timer in the Segmentation and Reassembly was not
      restarted when the incoming Segment Acknowledgment message did not contain at least one
      segment newly marked as acknowledged.
    * Fixed an issue where the On-Demand Private Proxy Server and Client models had interdependency
      that did not allow to compile them separately.

* Controller

  Improved support for Broadcast and Connected Isochronous channels in the Controller, enabling
  LE audio application development. The Controller is experimental, is missing implementations for
  interleaved packing in Isochronous channels' lower link layer.

  * Added Checks for minimum sizes of Adv PDUs
  * Added Kconfig Option to ignore Tx HCI ISO Data Packet Seq Num
  * Added Kconfig for avoiding ISO SDU fragmentation
  * Added Kconfig to maximize BIG event length and preempt PTO & CTRL subevents
  * Added ``BT_CTLR_EVENT_OVERHEAD_RESERVE_MAX`` Kconfig
  * Added memory barrier to ticker transactions
  * Added missing nRF53x Tx Power Kconfig
  * Added support for Flush Timeout in Connected ISO
  * Fixed BIS payload sliding window overrun check
  * Fixed CIS Central FT calculation
  * Fixed CIS Central error handling
  * Fixed CIS assymmetric PHY usage
  * Fixed CIS encryption when DF support enabled
  * Fixed ISO-AL for quality tests and time stamps
  * Fixed PHY value in HCI LE CIS Established Event
  * Fixed ULL stuck in semaphore under rare conditions
  * Fixed assertion due to late PER CIS active set
  * Fixed compiler instruction re-ordering that caused assertions
  * Fixed connected ISO dynamic tx power
  * Fixed failing advertising conformance tests
  * Fixed handling received Auxiliary PDUs when Coded PHY not supported
  * Fixed leak in scheduled ticker node when rescheduling ticker nodes
  * Fixed missing host feature reset
  * Fixed nRF53 SoC back-to-back PDU chaining
  * Fixed nRF53 SoC back-to-back Tx Rx implementation
  * Fixed regression in Adv PDU overflow calculation
  * Fixed regression in observer that caused assertions and scheduling stall
  * Fixed use of pre-programmed PPI on nRF SoCs
  * Removed HCI ISO data with invalid status in preparation for FT support
  * Updated Extended Advertising Report to not be generated when ``AUX_ADV_IND`` not received
  * Updated to have ``EVENT_OVERHEAD_START_US`` verbose assertion in each state/role LLL
  * Updated to stop following ``aux_ptr`` if ``DATA_LEN_MAX`` is reached during extended scanning

Boards & SoC Support
********************

* Added support for these SoC series:

  * Nuvoton NuMaker M46x series
  * Added support for STM32F072X8 SoC variants
  * Added support for STM32L051X6 SoC variants
  * Added support for STM32L451XX SoC variants
  * Added support for STM32L4Q5XX SoC variants
  * Added support for STM32WBA SoC series

* Removed support for these SoC series:

* Made these changes in other SoC series:

  * i.MX RT SOCs no longer enable CONFIG_DEVICE_CONFIGURATION_DATA by default.
    boards using external SDRAM should set CONFIG_DEVICE_CONFIGURATION_DATA
    and CONFIG_NXP_IMX_EXTERNAL_SDRAM to enabled.
  * i.MX RT SOCs no longer support CONFIG_OCRAM_NOCACHE, as this functionality
    can be achieved using devicetree memory regions
  * Refactored ESP32 SoC folders. So now these are a proper SoC series.
  * RP2040: Changed to reset the I2C device on initializing

* Added support for these ARC boards:

  * Added support for nsim_vpx5 - simulation (nSIM) platform with ARCv2 VPX5 core, close to
    vpx5_integer_full template
  * Added support for nsim_hs5x_smp_12cores - simulation (nSIM) platform with 12 cores SMP 32-bit
    ARCv3 HS
  * Added support for nsim_hs6x_smp_12cores - simulation (nSIM) platform with 12 cores SMP 64-bit
    ARCv3 HS

* Added support for these ARM boards:

  * Nuvoton NuMaker Platform M467
  * ST Nucleo U5A5ZJ Q
  * ST Nucleo WBA52CG

* Added support for these ARM64 boards:

* Added support for these RISC-V boards:

* Added support for these X86 boards:

* Added support for these Xtensa boards:

  * Added ``esp32_devkitc_wroom`` and ``esp32_devkitc_wrover``.

  * Added ``esp32s3_luatos_core``.

  * Added ``m5stack_core2``.

  * Added ``qemu_xtensa_mmu`` utilizing Diamond DC233c SoC to support
    testing Xtensa MMU.

  * Added ``xiao_esp32s3``.

  * Added ``yd_esp32``.

* Added support for these POSIX boards:

  * :ref:`native_sim(_64) <native_sim>`
  * nrf5340bsim_nrf5340_cpu(net|app). A simulated nrf5340 SOC, which uses Babblesim for its radio
    traffic.

* Made these changes for ARC boards:

  * Turned off unsupported stack checking option for hsdk4xd platform
  * Changed vendor prefix for ARC QEMU platforms from "qemu" to "snps"

* Made these changes for ARM boards:

  * ST morpho connector description was added on ST nucleo boards.

  * rpi_pico:

    * The default adapter when debugging with openocd has been changed to cmsis-dap.

* Made these changes for ARM64 boards:

* Made these changes for RISC-V boards:

* Made these changes for X86 boards:

* Made these changes for Xtensa boards:

  * esp32s3_devkitm:

    * Added USB-CDC support.

    * Added CAN support.

* Made these changes for POSIX boards:

  * nrf52_bsim:

    * Has been reworked to use the native simulator as its runner.
    * Multiple HW models improvements and fixes. GPIO & GPIOTE peripherals added.

* Removed support for these ARC boards:

* Removed support for these ARM boards:

* Removed support for these ARM64 boards:

* Removed support for these RISC-V boards:

* Removed support for these X86 boards:

* Removed support for these Xtensa boards:

  * Removed ``esp32``. Use ``esp32_devkitc_*`` instead.

* Made these changes in other boards:

* Added support for these following shields:

  * Adafruit PiCowbell CAN Bus Shield for Pico
  * Arduino UNO click shield
  * G1120B0MIPI MIPI Display
  * MikroElektronika MCP2518FD Click shield (CAN-FD)
  * RK055HDMIPI4M MIPI Display
  * RK055HDMIPI4MA0 MIPI Display
  * Semtech SX1276MB1MAS LoRa Shield

Build system and infrastructure
*******************************

* SCA (Static Code Analysis)

  * Added support for CodeChecker

* Twister now supports ``required_snippets`` in testsuite .yml files, this can
  be used to include a snippet when a test is ran (and exclude any boards from
  running that the snippet cannot be applied to).

* Interrupts

  * Added support for shared interrupts

* Added support for setting MCUboot encryption key in sysbuild which is then
  propagated to the bootloader and target images to automatically create
  encrypted updates.

* Build time priority checking: enable build time priority checking by default.
  This fails the build if the initialization sequence in the final ELF file
  does not match the devicetree hierarchy. It can be turned off by disabling
  the :kconfig:option:`COFNIG_CHECK_INIT_PRIORITIES` option.

* Added a new ``initlevels`` target for printing the final device and
  :c:macro:`SYS_INIT` initialization sequence from the final ELF file.

* Reworked syscall code generations so that not all marshalling functions
  will be included in the final binary. Syscalls associated with disabled
  subsystems no longer have their marshalling functions generated.

* Partially enabled compiler warning about shadow variables for subset of
  in-tree code. Out-of-tree code needs to be patched before we can fully
  enable shadow variable warnings.

Drivers and Sensors
*******************

* ADC

  * Added support for STM32F0 HSI14 clock (dedicated ADC clock)
  * Added support for STM32 ADC source clock and prescaler. On STM32F1 and STM32F3
    series, ADC prescaler can be configured using dedicated RCC Clock Controller
    option.
  * Added support for the ADC sequencer for all STM32 series (except F1)
  * Fixed STM32F4 ADC temperature and Vbat measurement.
  * Added driver for TI ADS1112.
  * Added driver for TI TLA2021.
  * Added driver for Gecko ADC.
  * Added driver for NXP S32 ADC SAR.
  * Added driver for MAX1125x family.
  * Added driver for MAX11102-MAX1117.

* CAN

  * Added support for TI TCAN4x5x CAN-FD controller with integrated transceiver
    (:dtcompatible:`ti,tcan4x5x`).
  * Added support for Microchip MCP251xFD CAN-FD controller (:dtcompatible:`microchip,mcp251xfd`).
  * Added support for CAN statistics to the Bosch M_CAN controller driver backend.
  * Switched the NXP S32 CANXL driver to use clock control for the CAN clock instead of hard-coding
    a CAN clock frequency in the devicetree.

* Clock control

  * Added support for Nuvoton NuMaker M46x

* Counter

  * Added :kconfig:option:`CONFIG_COUNTER_RTC_STM32_SUBSECONDS` to enable subsecond as
    the basic time tick on STM32 RTC based counter driver.

  * Added support for Raspberry Pi Pico Timer

* DAC

  * Added support for Analog Devices AD56xx
  * Added support for NXP lpcxpresso55s36 (LPDAC)

* Disk

  * Ramdisk driver is now configured using devicetree, and supports multiple
    instances

* Display

  * Added support for ST7735S (in ST7735R driver)

* DMA

  * Added support for NXP S32K to the eDMA driver
  * Added support for NXP SMARTDMA
  * Added support for NXP Pixel Pipeline (PXP) for display acceleration
  * Added support for DMA get_status() to the SAM XDMAC driver
  * Fixes for Intel HDA driver for L1 entry/exit, explicit SCS (sample container) settings
  * Fixes for STM32U5 enables error interrupts, fixes block size and data size configuration
  * Better Kconfig options for tuning static memory usage in NXP LPC driver

* EEPROM

  * Added support for Fujitsu MB85RCxx series I2C FRAM (:dtcompatible:`fujitsu,mb85rcxx`).

* Entropy

  * Added a requirement for ``entropy_get_entropy()`` to be thread-safe because
    of random subsystem needs.

* Ethernet

  * Added :kconfig:option:`CONFIG_ETH_NATIVE_POSIX_RX_TIMEOUT` to set rx timeout for native posix.
  * Added support for adin2111.
  * Added support for NXP S32 GMAC.
  * Added support for promiscuous mode in eth_smsc91x.
  * Added support for STM32H5X SOC series.
  * Added support for MDIO Clause 45 APIs.
  * Added support for YD-ESP32 board Ethernet.
  * Fixed stm32 to generate more unique MAC address by using device id as a base for the MAC.
  * Fixed mcux to increase the PTP timestamp accuracy from 20us to 200ns.
  * Fixed Ethernet max header size when using VLAN.
  * Removed the ``mdio`` DT property. Please use :c:macro:`DT_INST_BUS()` in the driver instead.
  * Reworked the device node hierarchy in smsc91x.
  * Renamed the phy-dev property with phy-handle to match the Linux ethernet-controller binding
    and move it up to ethernet.yaml so that it can be used by other drivers.
  * Updated Ethernet PHY to use ``reg`` property in DT bindings.
  * Updated driver DT bindings to use ``ethernet-phy`` devicetree node name consistently.
  * Updated esp32 and sam-gmac DT so that the phy is pointed by a phandle rather than
    a child node, this makes the phy device a child of mdio.

* Flash

  * Introduce npcx flash driver that supports two or more spi nor flashes via a
    single Flash Interface Unit (FIU) module and Direct Read Access (DRA) mode
    for better performance.
  * Added support for Nuvoton NuMaker M46x embedded flash
  * STM32 QSPI driver now supports Jedec SFDP parameter reading.
  * STM32 OSPI driver now supports both Low and High ports of IO manager.

* GPIO

  * Added support for Nuvoton NuMaker M46x

* I2C

  * STM32 V1 driver now supports large transactions (more than 256 bytes chunks)
  * STM32 V2 driver now supports 10-bit addressing.
  * I2C devices can now be used as wakeup source from STOP modes on STM32.
  * Fix long ISR execution in Silicon Labs I2C target callback
  * Fail gracefully on DMA max size for nRF52 devices in the TWIM driver
  * Added support for Intel LPSS DMA usage in the DesignWare driver
  * Added filtering of dumped messages for debugging using DeviceTree
  * Added target mode to Silicon Labs Gecko driver
  * Added Intel SEDI driver
  * Added Infineon XMC4 driver
  * Added Microchip PolarFire SoC driver
  * Added Ambiq driver for Apollo4 SoCs

* I2S

  * Fixed handling of the PCM data format in the NXP MCUX driver.

* I3C

  * ``i3c_cdns``:

    * Fixed build error when :kconfig:option:`CONFIG_I3C_USE_IBI` is disabled.

    * Fixed transfer issue when controller is busy. Now wait for controller to
      idle before proceeding with another transfer.

* IEEE 802.15.4

  * A new mandatory method attr_get() was introduced into ieee802154_radio_api.
    Drivers need to implement at least
    IEEE802154_ATTR_PHY_SUPPORTED_CHANNEL_PAGES and
    IEEE802154_ATTR_PHY_SUPPORTED_CHANNEL_RANGES.
  * The hardware capabilities IEEE802154_HW_2_4_GHZ and IEEE802154_HW_SUB_GHZ
    were removed as they were not aligned with the standard and some already
    existing drivers couldn't properly express their channel page and channel
    range (notably SUN FSK and HRP UWB drivers). The capabilities were replaced
    by the standard conforming new driver attribute
    IEEE802154_ATTR_PHY_SUPPORTED_CHANNEL_PAGES that fits all in-tree drivers.
  * The method get_subg_channel_count() was removed from ieee802154_radio_api.
    This method could not properly express the channel range of existing drivers
    (notably SUN FSK drivers that implement channel pages > 0 and may not have
    zero-based channel ranges or UWB drivers that could not be represented at
    all). The method was replaced by the new driver attribute
    IEEE802154_ATTR_PHY_SUPPORTED_CHANNEL_RANGES that fits all in-tree drivers.

* Interrupt Controller

  * GIC: Architecture version selection is now based on the device tree

* Input

  * New drivers: :dtcompatible:`gpio-qdec`, :dtcompatible:`st,stmpe811`.

  * Drivers converted from Kscan to Input: :dtcompatible:`goodix,gt911`
    :dtcompatible:`xptek,xpt2046` :dtcompatible:`hynitron,cst816s`
    :dtcompatible:`microchip,cap1203`.

  * Added a Kconfig option for dumping all events to the console
    :kconfig:option:`CONFIG_INPUT_EVENT_DUMP` and new shell commands
    :kconfig:option:`CONFIG_INPUT_SHELL`.

  * Merged ``zephyr,gpio-keys`` into :dtcompatible:`gpio-keys` and added
    ``zephyr,code`` codes to all in-tree board ``gpio-keys`` nodes.

  * Renamed the callback definition macro from ``INPUT_LISTENER_CB_DEFINE`` to
    :c:macro:`INPUT_CALLBACK_DEFINE`.

* PCIE

  * Added support in shell to display PCIe capabilities.

  * Added virtual channel support.

  * Added kconfig :kconfig:option:`CONFIG_PCIE_INIT_PRIORITY` to specify
    initialization priority for host controller.

  * Added support to get IRQ from ACPI PCI Routing Table (PRT).

* ACPI

  * Adopted the ACPICA library as a new module to further enhance ACPI support.

* Pin control

  * Added support for Nuvoton NuMaker M46x

* PWM

  * Added 4 channels capture on STM32 PWM driver.
  * Added driver for Intel Blinky PWM.
  * Added driver for MAX31790.
  * Added driver for Infineon XMC4XXX CCU4.
  * Added driver for Infineon XMC4XXX CCU8.
  * Added MCUX CTimer based PWM driver.
  * Added PWM driver based on TI CC13xx/CC26xx GPT timer.
  * Reworked the pwm_nrf5_sw driver so that it can be used also on nRF53 and
    nRF91 Series. Consequently, the driver was renamed to pwm_nrf_sw.
  * Added driver for Nuvoton NuMaker family.
  * Added PWM driver based on NXP S32 EMIOS peripheral.

* Regulators

  * Added support for GPIO-controlled voltage regulator

  * Added support for AXP192 PMIC

  * Added support for NXP VREF regulator

  * Fixed regulators can now specify their operating voltage

  * PFM mode is now support for nPM1300

  * Added new API to configure "ship" mode

  * Regulator shell allows to configure DVS modes

* Reset

  * Added support for Nuvoton NuMaker M46x

* Retained memory

  * Added support for allowing mutex support to be forcibly disabled with
    :kconfig:option:`CONFIG_RETAINED_MEM_MUTEX_FORCE_DISABLE`.

  * Fixed issue with user mode support not working.

* RTC

  * Added support for STM32 RTC API driver. This driver is not compatible with
    the use of RTC based implementation of COUNTER API.

* SDHC

  * Added driver for EMMC Host controller present on Alder lake platforms
  * Added driver for Atmel HSMCI controller present on SAM4E MCU series

* Sensor

  * Reworked the :dtcompatible:`ti,bq274xx` to add ``BQ27427`` support, fixed
    units for capacity and power channels.
  * Added ADC current sense amplifier and voltage sensor drivers.
  * Added ADI LTC2990 voltage, current, and temperature sensor driver.
  * Added AMS TSL2540 ambient light sensor driver.
  * Added Bosch BMI08x accelerometer/gyroscope driver.
  * Added DFRobot A01NYUB distance sensor driver.
  * Added Fintek F75303 temperature sensor driver.
  * Added Isentek IST8310 magnetometer driver.
  * Added Microchip TCN75A temperature sensor driver.
  * Added NXP TEMPMON driver.
  * Added Seeed HM330X dust sensor driver.
  * Added TI TMAG5170 3D Hall sensor driver.
  * Added power management support to BMM150, LM75, and Microchip tachometer
    drivers.
  * Added trigger support to the BMM150 magnetometer driver.
  * Added tap trigger support to the LIS2DH accelerometer driver.
  * Updated ST sensor drivers to use STMEMSC HAL i/f v2.3
  * Updated the decoder APIs to vertically decode raw sensor data.
  * Various fixes and enhancements in the NTC thermistor and INA23x drivers.

* Serial

  * Added support for Nuvoton NuMaker M46x

  * NS16550: Reworked how device initialization macros.

    * ``CONFIG_UART_NS16550_ACCESS_IOPORT`` and ``CONFIG_UART_NS16550_SIMULT_ACCESS``
      are removed. For UART using IO port access, add ``io-mapped`` property to
      device tree node.

  * Added async support for ESP32S3.

  * Added support for serial TTY under ``native_posix``.

  * Added support for UART on Efinix Sapphire SoCs.

  * Added Intel SEDI UART driver.

  * Added support for UART on BCM2711.

  * ``uart_stm32``:

    * Added RS485 support.

    * Added wide data support.

  * ``uart_pl011``: added support for Ambiq SoCs.

  * ``serial_test``: added support for interrupt and async APIs.

  * ``uart_emul``: added support for interrupt API.

  * ``uart_rpi_pico``: fixed handling Modbus DE-RE signal

* SPI

  * Remove npcx spi driver implemented by Flash Interface Unit (FIU) module.
  * Added support for Raspberry Pi Pico PIO based SPI.

* Timer

  * The TI CC13xx/26xx system clock timer compatible was changed from
    :dtcompatible:`ti,cc13xx-cc26xx-rtc` to :dtcompatible:`ti,cc13xx-cc26xx-rtc-timer`
    and the corresponding Kconfig option from :kconfig:option:`CC13X2_CC26X2_RTC_TIMER`
    to :kconfig:option:`CC13XX_CC26XX_RTC_TIMER` for improved consistency and
    extensibility. No action is required unless the internal timer was modified.

* USB

  * Added UDC driver for STM32 based MCU, relying on HAL/PCD. This driver is compatible
    with UDC API (experimental).
  * Added support for STM32H5 series on USB driver.

* WiFi

  * Increased esp32 default network (TCP workq, RX and mgmt event) stack sizes to 2048 bytes.
  * Reduced the RAM usage for esp32s2_saola in Wi-Fi samples.
  * Fixed undefined declarations in winc1500.
  * Fixed SPI buffer length in eswifi.
  * Fixed esp32 data sending and channel selection in AP mode.
  * Fixed esp_at driver init and network interface dormant state setting.

Networking
**********

* CoAP:

  * Optimized CoAP client library to use only a single thread internally.
  * Converted CoAP client library to use ``zsock_*`` API internally.
  * Fixed a bug in CoAP client library, which resulted in an incorrect
    retransmission timeout calculation.
  * Use 64 bit timer values for calculating transmission timeouts. This fixes potential problems for
    devices that stay on for more than 49 days when the 32 bit uptime counter might roll over and
    cause CoAP packets to not timeout at all on this event.
  * API documentation improvements.
  * Added new API functions:

    * :c:func:`coap_has_descriptive_block_option`
    * :c:func:`coap_remove_descriptive_block_option`
    * :c:func:`coap_packet_remove_option`
    * :c:func:`coap_packet_set_path`

* Connection Manager:

  * Added support for auto-connect and auto-down behaviors (controlled by
    :c:enum:`CONN_MGR_IF_NO_AUTO_CONNECT` and :c:enum:`CONN_MGR_IF_NO_AUTO_DOWN`
    flags).
  * Split Connection Manager APIs into separate header files.
  * Extended Connection Manager documentation to cover new functionalities.

* DHCP:

  * Added support for DHCPv4 unicast replies processing.
  * Added support for DHCPv6 protocol.

* Ethernet:

  * Fixed ARP queueing so that the queued network packet is sent immediately
    instead of queued 2nd time in the core network stack.

* gPTP:

  * Added support for detecting gPTP packets that use the default multicast destination address.
  * Fixed Announce and Follow Up message handling.

* ICMP:

  * Fixed ICMPv6 error message type check.
  * Reworked ICMP callback registration and handling, which allows to register
    multiple handlers for the same ICMP message.
  * Introduced an API to send ICMP Echo Request (ping).
  * Added possibility to register offloaded ICMP ping handlers.
  * Added support for setting packet priority for ping.

* IPv6:

  * Made sure that ongoing DAD procedure is cancelled when IPv6 address is removed.
  * Fixed a bug, where Solicited-Node multicast address could be removed while
    still in use.

* LwM2M:

  * Added support for tickless mode. This removes the 500 ms timeout from the socket loop
    so the engine does not constantly wake up the CPU. This can be enabled by
    :kconfig:option:`CONFIG_LWM2M_TICKLESS`.
  * Added new :c:macro:`LWM2M_RD_CLIENT_EVENT_DEREGISTER` event.
  * Block-wise sending now supports LwM2M read and composite-read operations as well.
    When :kconfig:option:`CONFIG_LWM2M_COAP_BLOCK_TRANSFER` is enabled, any content that is larger
    than :kconfig:option:`CONFIG_LWM2M_COAP_MAX_MSG_SIZE` is split into a block-wise transfer.
  * Block-wise transfers don't require tokens to match anymore as this was not in line
    with CoAP specification (CoAP doesn't require tokens re-use).
  * Various fixes to bootstrap. Now client ensures that Bootstrap-Finish command is sent,
    before closing the DTLS pipe. Also allows Bootstrap server to close the DTLS pipe.
    Added timeout when waiting for bootstrap commands.
  * Added support for X509 certificates.
  * Various fixes to string handling. Allow setting string to zero length.
    Ensure string termination when using string operations on opaque resources.
  * Added support for Connection Monitoring object version 1.3.
  * Added protection for Security object to prevent read/writes by the server.
  * Fixed a possible notification stall in case of observation token change.
  * Added new shell command, ``lwm2m create``, which allows to create LwM2M object instances.
  * Added LwM2M interoperability test-suite against Leshan server.
  * API documentation improvements.
  * Several other minor fixes and improvements.

* Misc:

  * Time and timestamps in the network subsystem, PTP and IEEE 802.15.4
    were more precisely specified and all in-tree call sites updated accordingly.
    Fields for timed TX and TX/RX timestamps have been consolidated. See
    :c:type:`net_time_t`, :c:struct:`net_ptp_time`, :c:struct:`ieee802154_config`,
    :c:struct:`ieee802154_radio_api` and :c:struct:`net_pkt` for extensive
    documentation. As this is largely an internal API, existing applications will
    most probably continue to work unchanged.
  * Added support for additional net_pkt filter hooks:

    * :kconfig:option:`CONFIG_NET_PKT_FILTER_IPV4_HOOK`
    * :kconfig:option:`CONFIG_NET_PKT_FILTER_IPV6_HOOK`
    * :kconfig:option:`CONFIG_NET_PKT_FILTER_LOCAL_IN_HOOK`

  * Reworked several networking components to use timepoint API.
  * Added API functions facilitate going through all IPv4/IPv6 registered on an
    interface (:c:func:`net_if_ipv4_addr_foreach`, :c:func:`net_if_ipv6_addr_foreach`).
  * ``NET_EVENT_IPV6_PREFIX_ADD`` and ``NET_EVENT_IPV6_PREFIX_DEL`` events now provide
    more detailed information about the prefix (:c:struct:`net_event_ipv6_prefix`).
  * General cleanup of the shadowed variables across the networking subsystem.
  * Added ``qemu_cortex_a53`` networking support.
  * Introduced new modem subsystem.
  * Added new :zephyr:code-sample:`cellular-modem` sample.
  * Added support for network interface names (instead of reusing underlying device name).
  * Removed support for Google Cloud IoT sample due to service retirement.
  * Fixed a bug where packets passed in promiscuous mode could have been modified
    by L2 in certain cases.
  * Added support for setting syslog server (used for networking log backend)
    IP address at runtime.
  * Removed no longer used ``queued`` and ``sent`` net_pkt flags.
  * Added support for binding zperf TCP/UDP server to a specific IP address.

* MQTT-SN:

  * Improved thread safety of internal buffers allocation.
  * API documentation improvements.

* OpenThread:

  * Reworked :c:func:`otPlatEntropyGet` to use :c:func:`sys_csrand_get` internally.
  * Introduced ``ieee802154_radio_openthread.h`` radio driver extension interface
    specific for OpenThread. Added new transmit mode, specific to OpenThread,
    :c:enum:`IEEE802154_OPENTHREAD_TX_MODE_TXTIME_MULTIPLE_CCA`.

* PPP:

  * Fixed PPP L2 usage of the network interface carrier state.
  * Made PPP L2 thread priority configurable (:kconfig:option:`CONFIG_NET_L2_PPP_THREAD_PRIO`).
  * Moved PPP L2 out of experimental stage.
  * Prevent PPP connection reestablish when carrier is down.

* Sockets:

  * Added support for statically allocated socketpairs (in case no heap is available).
  * Made send timeout configurable (:kconfig:option:`CONFIG_NET_SOCKET_MAX_SEND_WAIT`).
  * Added support for ``FIONREAD`` and ``FIONBIO`` :c:func:`ioctl` commands.
  * Fixed input filtering for connected datagram sockets.
  * Fixed :c:func:`getsockname` operation on unconnected sockets.
  * Added new secure socket options for DTLS Connection ID support:

    * :c:macro:`TLS_DTLS_CID`
    * :c:macro:`TLS_DTLS_CID_VALUE`
    * :c:macro:`TLS_DTLS_PEER_CID_VALUE`
    * :c:macro:`TLS_DTLS_CID_STATUS`

  * Added support for :c:macro:`SO_REUSEADDR` and :c:macro:`SO_REUSEPORT` socket options.

* TCP:

  * Fixed potential stall in data retransmission, when data was only partially acknowledged.
  * Made TCP work queue priority configurable (:kconfig:option:`CONFIG_NET_TCP_WORKER_PRIO`).
  * Added support for TCP new Reno collision avoidance algorithm.
  * Fixed source address selection on bound sockets.
  * Fixed possible memory leak in case listening socket was closed during active handshake.
  * Fixed RST packet handling during handshake.
  * Refactored the code responsible for connection teardown to fix found bugs and
    simplify future maintenance.

* TFTP:

  * Added new :zephyr:code-sample:`tftp-client` sample.
  * API documentation improvements.

* WebSocket

  * WebSocket library no longer closes underlying TCP socket automatically on disconnect.
    This aligns with the connect behavior, where the WebSocket library expects an already
    connected TCP socket.

* Wi-Fi:

  * Added Passive scan support.
  * The Wi-Fi scan API updated with Wi-Fi scan parameter to allow scan mode selection.
  * Updated TWT handling.
  * Added support for generic network manager API.
  * Added support for Wi-Fi mode setting and selection.
  * Added user input validation for SSID and PSK in Wi-Fi shell.
  * Added scan extension for specifying channels, limiting scan results, filtering SSIDs,
    setting active and passive channel dwell times and frequency bands.

USB
***

* USB device HID
  * Kconfig option USB_HID_PROTOCOL_CODE, deprecated in v2.6, is finally removed.

Devicetree
**********

API
===

New general-purpose macros:

- :c:macro:`DT_REG_ADDR_U64`
- :c:macro:`DT_REG_ADDR_BY_NAME_U64`
- :c:macro:`DT_INST_REG_ADDR_BY_NAME_U64`
- :c:macro:`DT_INST_REG_ADDR_U64`
- :c:macro:`DT_FOREACH_STATUS_OKAY_NODE_VARGS`
- :c:macro:`DT_FOREACH_NODE_VARGS`
- :c:macro:`DT_HAS_COMPAT_ON_BUS_STATUS_OKAY`

New special-purpose macros introduced for dependency ordinals:

- :c:macro:`DT_DEP_ORD_STR_SORTABLE`

New general purpose macros introduced for fixed flash partitions:

- :c:macro:`DT_MEM_FROM_FIXED_PARTITION`
- :c:macro:`DT_FIXED_PARTITION_ADDR`

Bindings
========

* Generic or vendor-independent:

  * New bindings:

    * :dtcompatible:`current-sense-amplifier`
    * :dtcompatible:`current-sense-shunt`
    * :dtcompatible:`gpio-qdec`
    * :dtcompatible:`regulator-gpio`
    * :dtcompatible:`usb-audio-feature-volume`

  * Modified bindings:

    * CAN (Controller Area Network) controller bindings:

          * property ``phase-seg1-data`` deprecation status changed from False to True
          * property ``phase-seg1`` deprecation status changed from False to True
          * property ``phase-seg2-data`` deprecation status changed from False to True
          * property ``phase-seg2`` deprecation status changed from False to True
          * property ``prop-seg-data`` deprecation status changed from False to True
          * property ``prop-seg`` deprecation status changed from False to True
          * property ``sjw-data`` default value changed from None to 1
          * property ``sjw-data`` deprecation status changed from False to True
          * property ``sjw`` default value changed from None to 1
          * property ``sjw`` deprecation status changed from False to True

    * Ethernet controller bindings: new ``phy-handle`` property (in some
      bindings, this was renamed from ``phy-dev``), matching the Linux
      ethernet-controller binding.

    * The ``riscv,isa`` property used by RISC-V CPU bindings no longer has an
      ``enum`` value.

    * :dtcompatible:`neorv32-cpu`:

          * new property: ``mmu-type``
          * new property: ``riscv,isa``

    * :dtcompatible:`regulator-fixed`:

          * new property: ``regulator-min-microvolt``
          * new property: ``regulator-max-microvolt``
          * property ``enable-gpios`` is no longer required

    * :dtcompatible:`ethernet-phy`:

          * removed property: ``address``
          * removed property: ``mdio``
          * property ``reg`` is now required

    * :dtcompatible:`usb-audio-hs` and :dtcompatible:`usb-audio-hp`:

          * new property: ``volume-max``
          * new property: ``volume-min``
          * new property: ``volume-res``
          * new property: ``status``
          * new property: ``compatible``
          * new property: ``reg``
          * new property: ``reg-names``
          * new property: ``interrupts``
          * new property: ``interrupts-extended``
          * new property: ``interrupt-names``
          * new property: ``interrupt-parent``
          * new property: ``label``
          * new property: ``clocks``
          * new property: ``clock-names``
          * new property: ``#address-cells``
          * new property: ``#size-cells``
          * new property: ``dmas``
          * new property: ``dma-names``
          * new property: ``io-channels``
          * new property: ``io-channel-names``
          * new property: ``mboxes``
          * new property: ``mbox-names``
          * new property: ``wakeup-source``
          * new property: ``power-domain``
          * new property: ``zephyr,pm-device-runtime-auto``

    * :dtcompatible:`ntc-thermistor-generic`:

          * removed property: ``r25-ohm``

    * :dtcompatible:`ns16550`:

          * new property: ``resets``
          * new property: ``reset-names``

    * :dtcompatible:`fixed-clock`:

          * removed property: ``clocks``

    * All CPU bindings got a new ``enable-method`` property. `pull request
      60210 <https://github.com/zephyrproject-rtos/zephyr/pull/60210>`_ for
      details.

* Analog Devices, Inc. (adi):

  * New bindings:

    * :dtcompatible:`adi,ad5628`
    * :dtcompatible:`adi,ad5648`
    * :dtcompatible:`adi,ad5668`
    * :dtcompatible:`adi,ad5672`
    * :dtcompatible:`adi,ad5674`
    * :dtcompatible:`adi,ad5676`
    * :dtcompatible:`adi,ad5679`
    * :dtcompatible:`adi,ad5684`
    * :dtcompatible:`adi,ad5686`
    * :dtcompatible:`adi,ad5687`
    * :dtcompatible:`adi,ad5689`
    * :dtcompatible:`adi,adin1110`
    * :dtcompatible:`adi,adltc2990`

  * Modified bindings:

    * :dtcompatible:`adi,adin2111-mdio` (on adin2111 bus):

          * removed property: ``protocol``

* Altera Corp. (altr):

  * New bindings:

    * :dtcompatible:`altr,pio-1.0`

* Ambiq Micro, Inc. (ambiq):

  * New bindings:

    * :dtcompatible:`ambiq,am1805`
    * :dtcompatible:`ambiq,apollo4-pinctrl`
    * :dtcompatible:`ambiq,counter`
    * :dtcompatible:`ambiq,i2c`
    * :dtcompatible:`ambiq,mspi`
    * :dtcompatible:`ambiq,pwrctrl`
    * :dtcompatible:`ambiq,spi`
    * :dtcompatible:`ambiq,stimer`
    * :dtcompatible:`ambiq,uart`
    * :dtcompatible:`ambiq,watchdog`

* AMS AG (ams):

  * New bindings:

    * :dtcompatible:`ams,tsl2540`

* Andes Technology Corporation (andestech):

  * New bindings:

    * :dtcompatible:`andestech,atcwdt200`
    * :dtcompatible:`andestech,plic-sw`
    * :dtcompatible:`andestech,qspi-nor`

* ARM Ltd. (arm):

  * New bindings:

    * :dtcompatible:`arm,cortex-a76`
    * :dtcompatible:`arm,gic-v1`
    * :dtcompatible:`arm,gic-v2`
    * :dtcompatible:`arm,gic-v3`
    * :dtcompatible:`arm,psci-1.1`

* ASPEED Technology Inc. (aspeed):

  * Modified bindings:

    * :dtcompatible:`aspeed,ast10x0-reset`:

          * specifier cells for space "reset" are now named: ['id'] (old value: None)
          * specifier cells for space "clock" are now named: None (old value: ['reset_id'])

* Atmel Corporation (atmel):

  * New bindings:

    * :dtcompatible:`atmel,sam-hsmci`

  * Modified bindings:

    * :dtcompatible:`atmel,sam-mdio`:

          * removed property: ``protocol``
          * property ``#address-cells`` const value changed from None to 1
          * property ``#size-cells`` const value changed from None to 0
          * property ``#address-cells`` is now required
          * property ``#size-cells`` is now required

* Bosch Sensortec GmbH (bosch):

  * New bindings:

    * :dtcompatible:`bosch,bmi08x-accel`
    * :dtcompatible:`bosch,bmi08x-accel`
    * :dtcompatible:`bosch,bmi08x-gyro`
    * :dtcompatible:`bosch,bmi08x-gyro`

  * Modified bindings:

    * :dtcompatible:`bosch,bmm150`:

          * new property: ``drdy-gpios``

    * :dtcompatible:`bosch,bmi270`:

          * new property: ``irq-gpios``

* Broadcom Corporation (brcm):

  * New bindings:

    * :dtcompatible:`brcm,bcm2711-aux-uart`

* Cadence Design Systems Inc. (cdns):

  * New bindings:

    * :dtcompatible:`cdns,tensilica-xtensa-lx3`

* DFRobot (dfrobot):

  * New bindings:

    * :dtcompatible:`dfrobot,a01nyub`

* Efinix Inc (efinix):

  * New bindings:

    * :dtcompatible:`efinix,sapphire-gpio`
    * :dtcompatible:`efinix,sapphire-timer0`
    * :dtcompatible:`efinix,sapphire-uart0`

* EPCOS AG (epcos):

  * Modified bindings:

    * :dtcompatible:`epcos,b57861s0103a039`:

          * removed property: ``r25-ohm``

* Espressif Systems (espressif):

  * Modified bindings:

    * :dtcompatible:`espressif,esp-at` (on uart bus):

          * new property: ``external-reset``

    * :dtcompatible:`espressif,esp32-mdio`:

          * removed property: ``protocol``
          * property ``#address-cells`` const value changed from None to 1
          * property ``#size-cells`` const value changed from None to 0
          * property ``#address-cells`` is now required
          * property ``#size-cells`` is now required

    * :dtcompatible:`espressif,riscv`:

          * new property: ``mmu-type``
          * new property: ``riscv,isa``

    * :dtcompatible:`espressif,esp32-spi`:

          * new property: ``line-idle-low``

* Feature Integration Technology Inc. (fintek):

  * New bindings:

    * :dtcompatible:`fintek,f75303`

* FocalTech Systems Co.,Ltd (focaltech):

  * Modified bindings:

    * :dtcompatible:`focaltech,ft5336` (on i2c bus):

          * new property: ``reset-gpios``

* Fujitsu Ltd. (fujitsu):

  * New bindings:

    * :dtcompatible:`fujitsu,mb85rcxx`

* Shenzhen Huiding Technology Co., Ltd. (goodix):

  * Modified bindings:

    * :dtcompatible:`goodix,gt911` (on i2c bus):

          * bus list changed from ['kscan'] to []
          * new property: ``alt-addr``

* Himax Technologies, Inc. (himax):

  * New bindings:

    * :dtcompatible:`himax,hx8394`

* Infineon Technologies (infineon):

  * New bindings:

    * :dtcompatible:`infineon,cat1-counter`
    * :dtcompatible:`infineon,cat1-spi`
    * :dtcompatible:`infineon,xmc4xxx-ccu4-pwm`
    * :dtcompatible:`infineon,xmc4xxx-ccu8-pwm`
    * :dtcompatible:`infineon,xmc4xxx-i2c`

* Intel Corporation (intel):

  * New bindings:

    * :dtcompatible:`intel,agilex5-clock`
    * :dtcompatible:`intel,alder-lake`
    * :dtcompatible:`intel,apollo-lake`
    * :dtcompatible:`intel,blinky-pwm`
    * :dtcompatible:`intel,elkhart-lake`
    * :dtcompatible:`intel,emmc-host`
    * :dtcompatible:`intel,ish`
    * :dtcompatible:`intel,loapic`
    * :dtcompatible:`intel,sedi-gpio`
    * :dtcompatible:`intel,sedi-i2c`
    * :dtcompatible:`intel,sedi-ipm`
    * :dtcompatible:`intel,sedi-uart`
    * :dtcompatible:`intel,socfpga-agilex-sip-smc`
    * :dtcompatible:`intel,socfpga-reset`
    * :dtcompatible:`intel,timeaware-gpio`

  * Removed bindings:

    * ``intel,agilex-socfpga-sip-smc``
    * ``intel,apollo_lake``
    * ``intel,elkhart_lake``
    * ``intel,gna``

  * Modified bindings:

    * :dtcompatible:`intel,niosv`:

          * new property: ``mmu-type``
          * new property: ``riscv,isa``

    * :dtcompatible:`intel,adsp-imr`:

          * new property: ``zephyr,memory-attr``
          * property ``zephyr,memory-region-mpu`` enum value changed from ['RAM', 'RAM_NOCACHE', 'FLASH', 'PPB', 'IO', 'EXTMEM'] to None
          * property ``zephyr,memory-region-mpu`` deprecation status changed from False to True

    * :dtcompatible:`intel,lpss`:

          * new property: ``dma-parent``

    * :dtcompatible:`intel,adsp-shim-clkctl`:

          * new property: ``adsp-clkctl-clk-ipll``

* Isentek Inc. (isentek):

  * New bindings:

    * :dtcompatible:`isentek,ist8310`

* Integrated Silicon Solutions Inc. (issi):

  * New bindings:

    * :dtcompatible:`issi,is31fl3216a`
    * :dtcompatible:`issi,is31fl3733`

* ITE Tech. Inc. (ite):

  * New bindings:

    * :dtcompatible:`ite,it8xxx2-sha`

  * Modified bindings:

    * :dtcompatible:`ite,it8xxx2-pinctrl-func`:

          * new property: ``func3-ext``
          * new property: ``func3-ext-mask``

    * :dtcompatible:`ite,riscv-ite`:

          * new property: ``mmu-type``
          * new property: ``riscv,isa``

    * :dtcompatible:`ite,enhance-i2c`:

          * new property: ``target-enable``
          * new property: ``target-pio-mode``

* Linaro Limited (linaro):

  * New bindings:

    * :dtcompatible:`linaro,ivshmem-ipm`

* Maxim Integrated Products (maxim):

  * New bindings:

    * :dtcompatible:`maxim,max11102`
    * :dtcompatible:`maxim,max11103`
    * :dtcompatible:`maxim,max11105`
    * :dtcompatible:`maxim,max11106`
    * :dtcompatible:`maxim,max11110`
    * :dtcompatible:`maxim,max11111`
    * :dtcompatible:`maxim,max11115`
    * :dtcompatible:`maxim,max11116`
    * :dtcompatible:`maxim,max11117`
    * :dtcompatible:`maxim,max11253`
    * :dtcompatible:`maxim,max11254`
    * :dtcompatible:`maxim,max31790`

* Microchip Technology Inc. (microchip):

  * New bindings:

    * :dtcompatible:`microchip,mcp251xfd`
    * :dtcompatible:`microchip,mpfs-i2c`
    * :dtcompatible:`microchip,tcn75a`

  * Modified bindings:

    * :dtcompatible:`microchip,xec-pwmbbled`:

          * new property: ``enable-low-power-32k``

    * :dtcompatible:`microchip,cap1203` (on i2c bus):

          * bus list changed from ['kscan'] to []
          * new property: ``input-codes``

    * :dtcompatible:`microchip,xec-ps2`:

          * new property: ``wakerx-gpios``

* Motorola, Inc. (motorola):

  * Modified bindings:

    * :dtcompatible:`motorola,mc146818`:

          * new property: ``clock-frequency``

* Murata Manufacturing Co., Ltd. (murata):

  * New bindings:

    * :dtcompatible:`murata,ncp15wb473`

* Nordic Semiconductor (nordic):

  * New bindings:

    * :dtcompatible:`nordic,npm1300-led`
    * :dtcompatible:`nordic,npm1300-wdt`

  * Removed bindings:

    * ``nordic,nrf-cc310``
    * ``nordic,nrf-cc312``

  * Modified bindings:

    * :dtcompatible:`nordic,nrf-ccm`:

          * new property: ``headermask-supported``

    * :dtcompatible:`nordic,nrf-twi`:

          * new property: ``easydma-maxcnt-bits``

    * :dtcompatible:`nordic,nrf-twim` and :dtcompatible:`nordic,nrf-twis`:

          * new property: ``easydma-maxcnt-bits``
          * new property: ``memory-regions``
          * new property: ``memory-region-names``

    * :dtcompatible:`nordic,nrf-spi`, :dtcompatible:`nordic,nrf-spis`, and
      :dtcompatible:`nordic,nrf-spim`:

          * new property: ``wake-gpios``

    * :dtcompatible:`nordic,npm1300-charger`:

          * new property: ``thermistor-cold-millidegrees``
          * new property: ``thermistor-cool-millidegrees``
          * new property: ``thermistor-warm-millidegrees``
          * new property: ``thermistor-hot-millidegrees``
          * new property: ``trickle-microvolt``
          * new property: ``term-current-percent``
          * new property: ``vbatlow-charge-enable``
          * new property: ``disable-recharge``

    * :dtcompatible:`nordic,nrf-uicr`:

          * new property: ``nfct-pins-as-gpios``
          * new property: ``gpio-as-nreset``

    * :dtcompatible:`nordic,npm1300` (on i2c bus):

          * new property: ``host-int-gpios``
          * new property: ``pmic-int-pin``

* Nuclei System Technology (nuclei):

  * Modified bindings:

    * :dtcompatible:`nuclei,bumblebee`:

          * new property: ``mmu-type``
          * new property: ``riscv,isa``

* Nuvoton Technology Corporation (nuvoton):

  * New bindings:

    * :dtcompatible:`nuvoton,nct38xx`
    * :dtcompatible:`nuvoton,nct38xx-gpio`
    * :dtcompatible:`nuvoton,npcx-fiu-nor`
    * :dtcompatible:`nuvoton,npcx-fiu-qspi`
    * :dtcompatible:`nuvoton,numaker-fmc`
    * :dtcompatible:`nuvoton,numaker-gpio`
    * :dtcompatible:`nuvoton,numaker-pcc`
    * :dtcompatible:`nuvoton,numaker-pinctrl`
    * :dtcompatible:`nuvoton,numaker-pwm`
    * :dtcompatible:`nuvoton,numaker-rst`
    * :dtcompatible:`nuvoton,numaker-scc`
    * :dtcompatible:`nuvoton,numaker-spi`
    * :dtcompatible:`nuvoton,numaker-uart`

  * Removed bindings:

    * ``nuvoton,nct38xx-gpio``
    * ``nuvoton,npcx-spi-fiu``

  * Modified bindings:

    * :dtcompatible:`nuvoton,npcx-sha`:

          * new property: ``context-buffer-size``

    * :dtcompatible:`nuvoton,npcx-adc`:

          * new property: ``vref-mv``
          * removed property: ``threshold-reg-offset``

    * :dtcompatible:`nuvoton,adc-cmp`:

          * new property: ``thr-sel``

    * :dtcompatible:`nuvoton,npcx-pcc`:

          * new property: ``pwdwn-ctl-val``
          * property ``clock-frequency`` enum value changed from [100000000, 96000000, 90000000, 80000000, 66000000, 50000000, 48000000, 40000000, 33000000] to [120000000, 100000000, 96000000, 90000000, 80000000, 66000000, 50000000, 48000000]
          * property ``ram-pd-depth`` enum value changed from [12, 15] to [8, 12, 15]

* NXP Semiconductors (nxp):

  * New bindings:

    * :dtcompatible:`nxp,ctimer-pwm`
    * :dtcompatible:`nxp,fs26-wdog`
    * :dtcompatible:`nxp,imx-flexspi-w956a8mbya`
    * :dtcompatible:`nxp,irqsteer-intc`
    * :dtcompatible:`nxp,lpdac`
    * :dtcompatible:`nxp,mbox-imx-mu`
    * :dtcompatible:`nxp,mcux-dcp`
    * :dtcompatible:`nxp,mcux-edma-v3`
    * :dtcompatible:`nxp,pcf8563`
    * :dtcompatible:`nxp,pxp`
    * :dtcompatible:`nxp,s32-adc-sar`
    * :dtcompatible:`nxp,s32-clock`
    * :dtcompatible:`nxp,s32-emios`
    * :dtcompatible:`nxp,s32-emios-pwm`
    * :dtcompatible:`nxp,s32-gmac`
    * :dtcompatible:`nxp,s32-qspi`
    * :dtcompatible:`nxp,s32-qspi-device`
    * :dtcompatible:`nxp,s32-qspi-nor`
    * :dtcompatible:`nxp,s32k3-pinctrl`
    * :dtcompatible:`nxp,smartdma`
    * :dtcompatible:`nxp,tempmon`
    * :dtcompatible:`nxp,vref`

  * Modified bindings:

    * :dtcompatible:`nxp,s32-netc-emdio`:

          * removed property: ``protocol``
          * property ``#address-cells`` const value changed from None to 1
          * property ``#size-cells`` const value changed from None to 0
          * property ``#address-cells`` is now required
          * property ``#size-cells`` is now required

    * :dtcompatible:`nxp,mipi-dsi-2l`:

          * property ``nxp,lcdif`` is no longer required

    * :dtcompatible:`nxp,imx-mipi-dsi`:

          * property ``nxp,lcdif`` is no longer required

    * :dtcompatible:`nxp,pca9633` (on i2c bus):

          * new property: ``disable-allcall``

    * :dtcompatible:`nxp,s32-sys-timer`:

          * removed property: ``clock-frequency``
          * property ``clocks`` is now required

    * :dtcompatible:`nxp,imx-lpspi`:

          * new property: ``data-pin-config``

    * :dtcompatible:`nxp,s32-spi`:

          * property ``clock-frequency`` is no longer required
          * property ``clocks`` is now required

    * :dtcompatible:`nxp,imx-wdog`:

          * pinctrl support

    * :dtcompatible:`nxp,s32-swt`:

          * removed property: ``clock-frequency``
          * property ``clocks`` is now required

    * :dtcompatible:`nxp,lpc-lpadc`:

          * new property: ``nxp,reference-supply``

    * :dtcompatible:`nxp,kinetis-pit`:

          * new property: ``max-load-value``
          * property ``clocks`` is now required

    * :dtcompatible:`nxp,mcux-edma`:

          * new property: ``dmamux-reg-offset``
          * new property: ``channel-gap``
          * new property: ``irq-shared-offset``

    * :dtcompatible:`nxp,imx-elcdif`:

          * new property: ``nxp,pxp``

* ON Semiconductor Corp. (onnn):

  * New bindings:

    * :dtcompatible:`onnn,ncp5623`

* Princeton Technology Corp. (ptc):

  * New bindings:

    * :dtcompatible:`ptc,pt6314`

* Quectel Wireless Solutions Co., Ltd. (quectel):

  * New bindings:

    * :dtcompatible:`quectel,bg95`

* QuickLogic Corp. (quicklogic):

  * New bindings:

    * :dtcompatible:`quicklogic,eos-s3-pinctrl`

  * Modified bindings:

    * :dtcompatible:`quicklogic,usbserialport-s3b`:

      * pinctrl support

* Raspberry Pi Foundation (raspberrypi):

  * New bindings:

    * :dtcompatible:`raspberrypi,pico-header`
    * :dtcompatible:`raspberrypi,pico-i2c`
    * :dtcompatible:`raspberrypi,pico-spi-pio`
    * :dtcompatible:`raspberrypi,pico-timer`

* Raydium Semiconductor Corp. (raydium):

  * New bindings:

    * :dtcompatible:`raydium,rm67162`

* Renesas Electronics Corporation (renesas):

  * New bindings:

    * :dtcompatible:`renesas,smartbond-lp-osc`
    * :dtcompatible:`renesas,smartbond-timer`

  * Modified bindings:

    * :dtcompatible:`renesas,smartbond-flash-controller`:

          * new property: ``read-cs-idle-delay``
          * new property: ``erase-cs-idle-delay``

* Smart Battery System (sbs):

  * New bindings:

    * :dtcompatible:`sbs,default-sbs-gauge`
    * :dtcompatible:`sbs,sbs-charger`

* Seeed Technology Co., Ltd (seeed):

  * New bindings:

    * :dtcompatible:`seeed,hm330x`

* SiFive, Inc. (sifive):

  * Modified bindings:

    * :dtcompatible:`sifive,i2c0`:

          * pinctrl support

* Silicon Laboratories (silabs):

  * New bindings:

    * :dtcompatible:`silabs,gecko-adc`

* Sino Wealth Electronic Ltd (sinowealth):

  * New bindings:

    * :dtcompatible:`sinowealth,sh1106`
    * :dtcompatible:`sinowealth,sh1106`

* Sitronix Technology Corporation (sitronix):

  * Modified bindings:

    * :dtcompatible:`sitronix,st7735r` (on spi bus):

          * property ``reset-gpios`` is no longer required

* Standard Microsystems Corporation (smsc):

  * Modified bindings:

    * :dtcompatible:`smsc,lan91c111-mdio`:

          * removed property: ``protocol``
          * property ``#address-cells`` const value changed from None to 1
          * property ``#size-cells`` const value changed from None to 0
          * property ``#address-cells`` is now required
          * property ``#size-cells`` is now required

    * :dtcompatible:`smsc,lan91c111`:

          * new property: ``local-mac-address``
          * new property: ``zephyr,random-mac-address``
          * property ``reg`` is no longer required

* Synopsys, Inc. (snps):

  * New bindings:

    * :dtcompatible:`snps,dw-timers`

* Solomon Systech Limited (solomon):

  * Modified bindings:

    * :dtcompatible:`solomon,ssd1306fb`

          * new property: ``inversion-on``
          * new property: ``ready-time-ms``

* Sequans Communications (sqn):

  * New bindings:

    * :dtcompatible:`sqn,hwspinlock`

* STMicroelectronics (st):

  * New bindings:

    * :dtcompatible:`st,stm32-bxcan`
    * :dtcompatible:`st,stm32-spi-host-cmd`
    * :dtcompatible:`st,stm32f1-rcc`
    * :dtcompatible:`st,stm32f3-rcc`
    * :dtcompatible:`st,stm32wba-flash-controller`
    * :dtcompatible:`st,stm32wba-hse-clock`
    * :dtcompatible:`st,stm32wba-pll-clock`
    * :dtcompatible:`st,stm32wba-rcc`
    * :dtcompatible:`st,stmpe811`

  * Removed bindings:

    * ``st,stm32-can``

  * Modified bindings:

    * :dtcompatible:`st,stm32-pwm`:

          * new property: ``four-channel-capture-support``

    * :dtcompatible:`st,stm32f4-adc`:

          * new property: ``st,adc-clock-source``
          * new property: ``st,adc-prescaler``
          * new property: ``st,adc-sequencer``
          * removed property: ``temp-channel``
          * removed property: ``vref-channel``
          * removed property: ``vbat-channel``

    * :dtcompatible:`st,stm32-adc`:

          * new property: ``st,adc-clock-source``
          * new property: ``st,adc-prescaler``
          * new property: ``st,adc-sequencer``
          * removed property: ``temp-channel``
          * removed property: ``vref-channel``
          * removed property: ``vbat-channel``

    * :dtcompatible:`st,stm32f1-adc`:

          * new property: ``st,adc-sequencer``
          * removed property: ``temp-channel``
          * removed property: ``vref-channel``
          * removed property: ``vbat-channel``

    * :dtcompatible:`st,stm32-ospi`:

          * new property: ``io-low-port``
          * new property: ``io-high-port``

    * :dtcompatible:`st,stm32c0-hsi-clock`:

          * removed property: ``clocks``

    * :dtcompatible:`st,stm32-hse-clock`:

          * removed property: ``clocks``

    * :dtcompatible:`st,stm32wl-hse-clock`:

          * removed property: ``clocks``

    * :dtcompatible:`st,stm32g0-hsi-clock`:

          * removed property: ``clocks``

    * :dtcompatible:`st,stm32h7-hsi-clock`:

          * removed property: ``clocks``

    * :dtcompatible:`st,stm32-lse-clock`:

          * removed property: ``clocks``

    * :dtcompatible:`st,stm32u5-pll-clock`:

          * new property: ``fracn``

* Telink Semiconductor (telink):

  * Modified bindings:

    * :dtcompatible:`telink,b91-pwm`:

          * pinctrl support

    * :dtcompatible:`telink,b91`:

          * new property: ``mmu-type``
          * new property: ``riscv,isa``

    * :dtcompatible:`telink,b91-i2c`:

          * pinctrl support

    * :dtcompatible:`telink,b91-spi`:

          * pinctrl support

    * :dtcompatible:`telink,b91-uart`:

          * pinctrl support

* Texas Instruments (ti):

  * New bindings:

    * :dtcompatible:`ti,ads1112`
    * :dtcompatible:`ti,bq27z746`
    * :dtcompatible:`ti,cc13xx-cc26xx-rtc-timer`
    * :dtcompatible:`ti,cc13xx-cc26xx-timer`
    * :dtcompatible:`ti,cc13xx-cc26xx-timer-pwm`
    * :dtcompatible:`ti,cc32xx-pinctrl`
    * :dtcompatible:`ti,davinci-gpio`
    * :dtcompatible:`ti,davinci-gpio-nexus`
    * :dtcompatible:`ti,lp5009`
    * :dtcompatible:`ti,lp5012`
    * :dtcompatible:`ti,lp5018`
    * :dtcompatible:`ti,lp5024`
    * :dtcompatible:`ti,lp5030`
    * :dtcompatible:`ti,lp5036`
    * :dtcompatible:`ti,lp5569`
    * :dtcompatible:`ti,tas6422dac`
    * :dtcompatible:`ti,tcan4x5x`
    * :dtcompatible:`ti,tla2021`
    * :dtcompatible:`ti,tmag5170`
    * :dtcompatible:`ti,vim`

  * Removed bindings:

    * ``ti,cc13xx-cc26xx-rtc``
    * ``ti,lp503x``

  * Modified bindings:

    * :dtcompatible:`ti,cc32xx-i2c`:

          * pinctrl support

    * :dtcompatible:`ti,ina230` (on i2c bus):

          * new property: ``alert-config``
          * new property: ``adc-mode``
          * new property: ``vbus-conversion-time-us``
          * new property: ``vshunt-conversion-time-us``
          * new property: ``avg-count``
          * new property: ``rshunt-micro-ohms``
          * removed property: ``rshunt-milliohms``
          * property ``config`` default value changed from None to 0
          * property ``config`` deprecation status changed from False to True
          * property ``config`` is no longer required

    * :dtcompatible:`ti,ina237` (on i2c bus):

          * new property: ``adc-mode``
          * new property: ``vbus-conversion-time-us``
          * new property: ``vshunt-conversion-time-us``
          * new property: ``temp-conversion-time-us``
          * new property: ``avg-count``
          * new property: ``high-precision``
          * new property: ``rshunt-micro-ohms``
          * removed property: ``rshunt-milliohms``
          * property ``adc-config`` default value changed from None to 0
          * property ``config`` default value changed from None to 0
          * property ``adc-config`` deprecation status changed from False to True
          * property ``config`` deprecation status changed from False to True
          * property ``adc-config`` is no longer required
          * property ``config`` is no longer required

    * :dtcompatible:`ti,cc32xx-uart`:

          * pinctrl support

* A stand-in for a real vendor which can be used in examples and tests (vnd):

  * New bindings:

    * :dtcompatible:`vnd,memory-attr`
    * :dtcompatible:`vnd,reg-holder-64`
    * :dtcompatible:`vnd,reserved-compat`

  * Modified bindings:

    * :dtcompatible:`vnd,serial`:

          * property ``reg`` is no longer required

* X-Powers (x-powers):

  * New bindings:

    * :dtcompatible:`x-powers,axp192`
    * :dtcompatible:`x-powers,axp192-gpio`
    * :dtcompatible:`x-powers,axp192-regulator`

* Xen Hypervisor (xen):

  * New bindings:

    * :dtcompatible:`xen,xen`

  * Removed bindings:

    * ``xen,xen-4.15``

* Xilinx (xlnx):

  * New bindings:

    * :dtcompatible:`xlnx,zynqmp-ipi-mailbox`

* Shenzhen Xptek Technology Co., Ltd (xptek):

  * Modified bindings:

    * :dtcompatible:`xptek,xpt2046` (on spi bus):

          * bus list changed from ['kscan'] to []

* Zephyr-specific binding (zephyr):

  * New bindings:

    * :dtcompatible:`zephyr,fake-rtc`
    * :dtcompatible:`zephyr,i2c-dump-allowlist`
    * :dtcompatible:`zephyr,lvgl-button-input`
    * :dtcompatible:`zephyr,lvgl-encoder-input`
    * :dtcompatible:`zephyr,lvgl-pointer-input`
    * :dtcompatible:`zephyr,mdio-gpio`
    * :dtcompatible:`zephyr,native-tty-uart`
    * :dtcompatible:`zephyr,ram-disk`
    * :dtcompatible:`zephyr,sensing`
    * :dtcompatible:`zephyr,sensing-phy-3d-sensor`

  * Removed bindings:

    * ``zephyr,gpio-keys``

  * Modified bindings:

    * :dtcompatible:`zephyr,mmc-disk` (on sd bus):

          * new property: ``bus-width``

    * :dtcompatible:`zephyr,bt-hci-spi` (on spi bus):

          * new property: ``controller-data-delay-us``

    * :dtcompatible:`zephyr,sdhc-spi-slot` (on spi bus):

          * new property: ``pwr-gpios``

    * :dtcompatible:`zephyr,memory-region`:

          * new property: ``zephyr,memory-attr``
          * property ``zephyr,memory-region-mpu`` enum value changed from ['RAM', 'RAM_NOCACHE', 'FLASH', 'PPB', 'IO', 'EXTMEM'] to None
          * property ``zephyr,memory-region-mpu`` deprecation status changed from False to True
          * property ``reg`` is now required

Libraries / Subsystems
**********************

* Management

  * Introduced MCUmgr client support with handlers for img_mgmt and os_mgmt.

  * Added response checking to MCUmgr's :c:enumerator:`MGMT_EVT_OP_CMD_RECV`
    notification callback to allow applications to reject MCUmgr commands.

  * MCUmgr SMP version 2 error translation (to legacy MCUmgr error code) is now
    supported in function handlers by setting ``mg_translate_error`` of
    :c:struct:`mgmt_group` when registering a group. See
    :c:type:`smp_translate_error_fn` for function details.

  * Fixed an issue with MCUmgr img_mgmt group whereby the size of the upload in
    the initial packet was not checked.

  * Fixed an issue with MCUmgr fs_mgmt group whereby some status codes were not
    checked properly, this meant that the error returned might not be the
    correct error, but would only occur in situations where an error was
    already present.

  * Fixed an issue whereby the SMP response function did not check to see if
    the initial zcbor map was created successfully.

  * Fixes an issue with MCUmgr shell_mgmt group whereby the length of a
    received command was not properly checked.

  * Added optional mutex locking support to MCUmgr img_mgmt group, which can
    be enabled with :kconfig:option:`CONFIG_MCUMGR_GRP_IMG_MUTEX`.

  * Added MCUmgr settings management group, which allows for manipulation of
    zephyr settings from a remote device, see :ref:`mcumgr_smp_group_3` for
    details.

  * Added :kconfig:option:`CONFIG_MCUMGR_GRP_IMG_ALLOW_CONFIRM_NON_ACTIVE_IMAGE_SECONDARY`
    and :kconfig:option:`CONFIG_MCUMGR_GRP_IMG_ALLOW_CONFIRM_NON_ACTIVE_IMAGE_ANY`
    that allow to control whether MCUmgr client will be allowed to confirm
    non-active images.

  * Added :kconfig:option:`CONFIG_MCUMGR_GRP_IMG_ALLOW_ERASE_PENDING` that allows
    to erase slots pending for next boot, that are not revert slots.

  * Added ``user_data`` as an optional field to :c:struct:`mgmt_handler` when
    :kconfig:option:`CONFIG_MCUMGR_MGMT_HANDLER_USER_DATA` is enabled.

  * Added optional ``force`` parameter to os mgmt reset command, this can be checked in the
    :c:enum:`MGMT_EVT_OP_OS_MGMT_RESET` notification callback whose data structure is
    :c:struct:`os_mgmt_reset_data`.

  * Added configurable number of SMP encoding levels via
    :kconfig:option:`CONFIG_MCUMGR_SMP_CBOR_MIN_ENCODING_LEVELS`, which automatically increments
    minimum encoding levels for in-tree groups if :kconfig:option:`CONFIG_ZCBOR_CANONICAL` is
    enabled.

  * Added STM32 SPI backend for EC Host command protocol.

  * Fixed settings_mgmt returning unknown error instead of invalid key specified error.

  * Fixed fs_mgmt returning parameter too large error instead of file is empty error when
    attempting to hash/checksum a file which is empty.

* File systems

  * Added support for ext2 file system.
  * Added support of mounting littlefs on the block device from the shell/fs.
  * Added alignment parameter to FS_LITTLEFS_DECLARE_CUSTOM_CONFIG macro, it can speed up read/write
    operation for SDMMC devices in case when we align buffers on CONFIG_SDHC_BUFFER_ALIGNMENT,
    because we can avoid extra copy of data from card bffer to read/prog buffer.

* Random

  * ``CONFIG_XOROSHIRO_RANDOM_GENERATOR``, deprecated a long time ago, is finally removed.

* Retention

  * Added the :ref:`blinfo_api` subsystem.

  * Added support for allowing mutex support to be forcibly disabled with
    :kconfig:option:`CONFIG_RETENTION_MUTEX_FORCE_DISABLE`.

* Binary descriptors

  * Added the :ref:`binary_descriptors` (``bindesc``) subsystem.

* POSIX API

  * Added dynamic thread stack support for :c:func:`pthread_create`
  * Fixed :c:func:`stat` so that it returns file stats instead of filesystem stats
  * Implemented :c:func:`pthread_barrierattr_destroy`, :c:func:`pthread_barrierattr_getpshared`,
    :c:func:`pthread_barrierattr_init`, :c:func:`pthread_barrierattr_setpshared`,
    :c:func:`pthread_condattr_destroy`, :c:func:`pthread_condattr_init`,
    :c:func:`pthread_mutexattr_destroy`, :c:func:`pthread_mutexattr_init`, :c:func:`uname`,
    :c:func:`sigaddset`, :c:func:`sigdelset`, :c:func:`sigemptyset`, :c:func:`sigfillset`,
    :c:func:`sigismember`, :c:func:`strsignal`, :c:func:`pthread_spin_destroy`,
    :c:func:`pthread_spin_init`, :c:func:`pthread_spin_lock`, :c:func:`pthread_spin_trylock`,
    :c:func:`pthread_spin_unlock`, :c:func:`timer_getoverrun`, :c:func:`pthread_condattr_getclock`,
    :c:func:`pthread_condattr_setclock`, :c:func:`clock_nanosleep`
  * Added support for querying the number of bytes available to read via the
    :c:macro:`FIONREAD` request to :c:func:`ioctl`
  * Added :kconfig:option:`CONFIG_FDTABLE` to conditionally compile file descriptor table
  * Added logging to POSIX threads, mutexes, and condition variables
  * Fixed :c:func:`poll` issue with event file descriptors

* LoRa/LoRaWAN

  * Updated ``loramac-node`` from v4.6.0 to v4.7.0

* CAN ISO-TP

  * Added support for CAN FD.

* RTIO

  * Added atomic completion counter fixing a race caught by unit tests
  * Added a :c:macro:`RTIO_SQE_NO_RESPONSE` flag for submissions when no completion notification
    is needed
  * Removed unused Kconfig options for different executors

* ZBus

  * Changed channels' and observers' metadata to comply with the data/config approach. ZBus stores
    immutable config in iterable sections in Flash and the mutable portion of data in the RAM.
  * The relationship between channels and observers is mapped using a new entity called
    observation. The observation enables us to increase the granularity of masking observation.
    Developers can mask individual observations, disable the observer, or use runtime observers.
  * Added API :c:macro:`ZBUS_CHAN_ADD_OBS` macro for adding post-definition static observers of a
    channel. That can replace the runtime observer feature, enabling developers to add static
    observers after the channel definition in different files. It increases the composability of
    the system using ZBus, making post-definition channel observation rely on the stack instead of
    the heap.
  * Added a new type of observer called Message Subscriber. ZBus' VDED will send a copy of the
    message during the publication/notification process.
  * Changed the VDED delivery sequence. Check the ref:`documentation<zbus delivery sequence>`.
  * ZBus runtime observers now rely on the heap instead of a memory pool.
  * Added new iterable section iterators APIs (for channels and observers) can now receive a
    ``user_data`` pointer to keep context between the function calls.
  * Added APIs :c:macro:`ZBUS_LISTENER_DEFINE_WITH_ENABLE` and
    :c:macro:`ZBUS_SUBSCRIBER_DEFINE_WITH_ENABLE` that allows developers to define observers'
    statuses (enabled/disabled) programmatically. With the API, developers can create observers
    initially disabled and enable them in runtime.

* Power management

  * Added :kconfig:option:`CONFIG_PM_NEED_ALL_DEVICES_IDLE`. When this
    option is set the power management will keep the system active
    if there is any device busy.
  * :c:func:`pm_device_runtime_get` can be called from ISR now.
  * Power states can be disabled directly in devicetree doing ``status = "disabled";``
  * Added the helper function, :c:func:`pm_device_driver_init`, for
    initializing devices into a specific power state.

* Modem modules

  * Added the :ref:`modem` subsystem.

HALs
****

* Nordic

  * Updated nrfx to version 3.1.0.

* Nuvoton

  * Added Nuvoton NuMaker M46x

MCUboot
*******

  * Added :kconfig:option:`CONFIG_MCUBOOT_BOOTLOADER_NO_DOWNGRADE`
    that allows to inform application that the on-board MCUboot has been configured
    with downgrade  prevention enabled. This option is automatically selected for
    DirectXIP mode and is available for both swap modes.

  * Added :kconfig:option:`CONFIG_MCUBOOT_BOOTLOADER_MODE_OVERWRITE_ONLY`
    that allows to inform application that the on-board MCUboot will overwrite
    the primary slot with secondary slot contents, without saving the original
    image in primary slot.

  * Fixed issue with serial recovery not showing image details for decrypted images.

  * Fixed issue with serial recovery in single slot mode wrongly iterating over 2 image slots.

  * Fixed an issue with boot_serial repeats not being processed when output was sent, this would
    lead to a divergence of commands whereby later commands being sent would have the previous
    command output sent instead.

  * Fixed an issue with the boot_serial zcbor setup encoder function wrongly including the buffer
    address in the size which caused serial recovery to fail on some platforms.

  * Fixed wrongly building in optimize for debug mode by default, this saves a significant amount
    of flash space.

  * Fixed issue with serial recovery use of MBEDTLS having undefined operations which led to usage
    faults when the secondary slot image was encrypted.

  * Fixed issue with bootutil asserting on maximum alignment in non-swap modes.

  * Added error output when flash device fails to open and asserts are disabled, which will now
    panic the bootloader.

  * Added currently running slot ID and maximum application size to shared data function
    definition.

  * Added P384 and SHA384 support to imgtool.

  * Added optional serial recovery image state and image set state commands.

  * Added ``dumpinfo`` command for signed image parsing in imgtool.

  * Added ``getpubhash`` command to dump the sha256 hash of the public key in imgtool.

  * Added support for ``getpub`` to print the output to a file in imgtool.

  * Added support for dumping the raw versions of the public keys in imgtool.

  * Added support for sharing boot information with application via retention subsystem.

  * Added support for serial recovery to read and handle encrypted seondary slot partitions.

  * Removed ECDSA P224 support.

  * Removed custom image list boot serial extension support.

  * Reworked boot serial extensions so that they can be used by modules or from user repositories
    by switching to iterable sections.

  * Reworked image encryption support for Zephyr, static dummy key files are no longer in the code,
    a pem file must be supplied to extract the private and public keys. The Kconfig menu has
    changed to only show a single option for enabling encryption and selecting the key file.

  * Reworked the ECDSA256 TLV curve agnostic and renamed it to ``ECDSA_SIG``.

  * CDDL auto-generated function code has been replaced with zcbor function calls, this now allows
    the parameters to be supplied in any order.

  * The MCUboot version in this release is version ``2.0.0+0-rc1``.

Nanopb
******

  * Changed project status to maintained.

  * Added a separate nanopb.cmake file to be included by applications.

  * Added helper cmake function ``zephyr_nanopb_sources`` to simplify ``.proto`` file inclusion.

LVGL
****

  * Changed project status to maintained.

  * Library has been updated to release v8.3.7.

  * Added ``zephyr,lvgl-{pointer,button,encoder}-input`` pseudo device bindings.
    :kconfig:option:`CONFIG_LV_Z_KSCAN_POINTER` is still supported but touch controllers
    need a :dtcompatible:`zephyr,kscan-input` child node to emit input events.

  * LVGL shell allows for monkey testing (requires :kconfig:option:`CONFIG_LV_USE_MONKEY`)
    and inspecting memory usage.

Trusted Firmware-A
******************

* Updated to TF-A 2.9.0.

Documentation
*************

* Upgraded Sphinx to 6.2

Tests and Samples
*****************

* Created common sample for file systems (`fs_sample`). It originates from sample for FAT
  (`fat_fs`) and supports both FAT and ext2 file systems.

* Created the zbus confirmed channel sample to demonstrate how to implement a delivery-guaranteed
  channel using subscribers.

* Created the zbus message subscriber sample to demonstrate how to use message subscribers.
