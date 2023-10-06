:orphan:

.. _zephyr_3.5:

Zephyr 3.5.0 (Working Draft)
############################

We are pleased to announce the release of Zephyr version 3.5.0.

Major enhancements with this release include:

* Added native_sim (successor to native_posix)

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************
The following CVEs are addressed by this release:

More detailed information can be found in:
https://docs.zephyrproject.org/latest/security/vulnerabilities.html

* CVE-2023-4257: Under embargo until 2023-10-12

* CVE-2023-4258 `Zephyr project bug tracker GHSA-m34c-cp63-rwh7
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-m34c-cp63-rwh7>`_

* CVE-2023-4260 `Zephyr project bug tracker GHSA-gj27-862r-55wh
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-gj27-862r-55wh>`_

* CVE-2023-4264 `Zephyr project bug tracker GHSA-rgx6-3w4j-gf5j
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-rgx6-3w4j-gf5j>`_

* CVE-2023-4424: Under embargo until 2023-11-01

* CVE-2023-5055: Under embargo until 2023-11-01

* CVE-2023-5139: Under embargo until 2023-10-25

* CVE-2023-5184 `Zephyr project bug tracker GHSA-8x3p-q3r5-xh9g
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-8x3p-q3r5-xh9g>`_


Kernel
******

Architectures
*************

* ARM

  * Architectural support for Arm Cortex-M has been separated from Arm
    Cortex-A and Cortex-R. This includes separate source modules to handle
    tasks like IRQ management, exception handling, thread handling and swap.
    For implementation details see :github:`60031`.

* ARM

* ARM64

* RISC-V

* Xtensa

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

* Direction Finding

* Host

* Mesh

* Controller

* HCI Driver

Boards & SoC Support
********************

* Added support for these SoC series:

  * Nuvoton NuMaker M46x series

* Removed support for these SoC series:

* Made these changes in other SoC series:

  * i.MX RT SOCs no longer enable CONFIG_DEVICE_CONFIGURATION_DATA by default.
    boards using external SDRAM should set CONFIG_DEVICE_CONFIGURATION_DATA
    and CONFIG_NXP_IMX_EXTERNAL_SDRAM to enabled.
  * i.MX RT SOCs no longer support CONFIG_OCRAM_NOCACHE, as this functionality
    can be achieved using devicetree memory regions

* Added support for these ARC boards:

* Added support for these ARM boards:

  * Nuvoton NuMaker Platform M467

* Added support for these ARM64 boards:

* Added support for these RISC-V boards:

* Added support for these X86 boards:

* Added support for these Xtensa boards:

* Added support for these POSIX boards:

  * :ref:`native_sim(_64) <native_sim>`
  * nrf5340bsim_nrf5340_cpu(net|app). A simulated nrf5340 SOC, which uses Babblesim for its radio
    traffic.

* Made these changes for ARC boards:

* Made these changes for ARM boards:

* Made these changes for ARM64 boards:

* Made these changes for RISC-V boards:

* Made these changes for X86 boards:

* Made these changes for Xtensa boards:

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

* Made these changes in other boards:

* Added support for these following shields:

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

Drivers and Sensors
*******************

* ADC

* Battery-backed RAM

* CAN

* Clock control

  * Added support for Nuvoton NuMaker M46x

* Counter

* Crypto

* DAC

* DFU

* Disk

* Display

  * Added support for ST7735S (in ST7735R driver)

* DMA

* EEPROM

* Entropy

* ESPI

* Ethernet

  * Added :kconfig:option:`CONFIG_ETH_NATIVE_POSIX_RX_TIMEOUT` to set rx timeout for native posix.

* Flash

  * Introduce npcx flash driver that supports two or more spi nor flashes via a
    single Flash Interface Unit (FIU) module and Direct Read Access (DRA) mode
    for better performance.
  * Added support for Nuvoton NuMaker M46x embedded flash

* FPGA

* Fuel Gauge

* GPIO

  * Added support for Nuvoton NuMaker M46x

* hwinfo

* I2C

* I2S

* I3C

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

* IPM

* KSCAN

* LED

* MBOX

* MEMC

* PCIE

* PECI

* Pin control

  * Added support for Nuvoton NuMaker M46x

* PWM

* Power domain

* Regulators

* Reset

  * Added support for Nuvoton NuMaker M46x

* Retained memory

  * Added support for allowing mutex support to be forcibly disabled with
    :kconfig:option:`CONFIG_RETAINED_MEM_MUTEX_FORCE_DISABLE`.

  * Fixed issue with user mode support not working.

* SDHC

* Sensor

  * Reworked the :dtcompatible:`ti,bq274xx` to add ``BQ27427`` support, fixed
    units for capacity and power channels.

* Serial

  * Added support for Nuvoton NuMaker M46x

  * NS16550: Reworked how device initialization macros.

    * CONFIG_UART_NS16550_ACCESS_IOPORT and CONFIG_UART_NS16550_SIMULT_ACCESS
      are removed. For UART using IO port access, add "io-mapped" property to
      device tree node.

* SPI

  * Remove npcx spi driver implemented by Flash Interface Unit (FIU) module.

* Timer

  * The TI CC13xx/26xx system clock timer compatible was changed from
    :dtcompatible:`ti,cc13xx-cc26xx-rtc` to :dtcompatible:`ti,cc13xx-cc26xx-rtc-timer`
    and the corresponding Kconfig option from :kconfig:option:`CC13X2_CC26X2_RTC_TIMER`
    to :kconfig:option:`CC13XX_CC26XX_RTC_TIMER` for improved consistency and
    extensibility. No action is required unless the internal timer was modified.

* USB

* W1

* Watchdog

* WiFi

Networking
**********

* Time and timestamps in the network subsystem, PTP and IEEE 802.15.4
  were more precisely specified and all in-tree call sites updated accordingly.
  Fields for timed TX and TX/RX timestamps have been consolidated. See
  :c:type:`net_time_t`, :c:struct:`net_ptp_time`, :c:struct:`ieee802154_config`,
  :c:struct:`ieee802154_radio_api` and :c:struct:`net_pkt` for extensive
  documentation. As this is largely an internal API, existing applications will
  most probably continue to work unchanged.

* CoAP:

  * Use 64 bit timer values for calculating transmission timeouts. This fixes potential problems for
    devices that stay on for more than 49 days when the 32 bit uptime counter might roll over and
    cause CoAP packets to not timeout at all on this event.

* LwM2M:

  * Added support for tickless mode. This removes the 500 ms timeout from the socket loop
    so the engine does not constantly wake up the CPU. This can be enabled by
    :kconfig:option:`CONFIG_LWM2M_TICKLESS`.
  * Added new :c:macro:`LWM2M_RD_CLIENT_EVENT_DEREGISTER` event.

* Wi-Fi
  * Added Passive scan support.
  * The Wi-Fi scan API updated with Wi-Fi scan parameter to allow scan mode selection.

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

* File systems

  * Added support for ext2 file system.
  * Added support of mounting littlefs on the block device from the shell/fs.
  * Added alignment parameter to FS_LITTLEFS_DECLARE_CUSTOM_CONFIG macro, it can speed up read/write
    operation for SDMMC devices in case when we align buffers on CONFIG_SDHC_BUFFER_ALIGNMENT,
    because we can avoid extra copy of data from card bffer to read/prog buffer.

* Retention

  * Added the :ref:`blinfo_api` subsystem.

  * Added support for allowing mutex support to be forcibly disabled with
    :kconfig:option:`CONFIG_RETENTION_MUTEX_FORCE_DISABLE`.

* Binary descriptors

  * Added the :ref:`binary_descriptors` (``bindesc``) subsystem.

HALs
****

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

Storage
*******

Trusted Firmware-M
******************

Trusted Firmware-A
******************

* Updated to TF-A 2.9.0.

zcbor
*****

Documentation
*************

Tests and Samples
*****************

* Created common sample for file systems (`fs_sample`). It originates from sample for FAT
  (`fat_fs`) and supports both FAT and ext2 file systems.

Known Issues
************
