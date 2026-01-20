:orphan:

..
  See
  https://docs.zephyrproject.org/latest/releases/index.html#migration-guides
  for details of what is supposed to go into this document.

.. _migration_4.4:

Migration guide to Zephyr v4.4.0 (Working Draft)
################################################

This document describes the changes required when migrating your application from Zephyr v4.3.0 to
Zephyr v4.4.0.

Any other changes (not directly related to migrating applications) can be found in
the :ref:`release notes<zephyr_4.4>`.

.. contents::
    :local:
    :depth: 2

Build System
************

* Zephyr now officially defaults to C17 (ISO/IEC 9899:2018) as its minimum required
  C standard version.  If your toolchain does not support this standard you will
  need to use one of the existing and now deprecated options:
  :kconfig:option:`CONFIG_STD_C99` or :kconfig:option:`CONFIG_STD_C11`.
* The ``full_name`` property of ``board``/``boards`` entries corresponding to new boards in
  board.yml files is now required.

Kernel
******

Boards
******

* m5stack_fire: Removed unused pinctrl entries for UART2, and updated the UART1
  pin mapping from GPIO32/GPIO33 to GPIO16/GPIO17 to match the documented Grove
  PORT.C wiring.

* Compile definitions 'XIP_EXTERNAL_FLASH', 'USE_HYPERRAM' and 'XIP_BOOT_HEADER_XMCD_ENABLE'
  are only used in :zephyr_file:`boards/nxp/mimxrt1180_evk/xip/evkmimxrt1180_flexspi_nor_config.c`
  and :zephyr_file:`boards/nxp/mimxrt1170_evk/xmcd/xmcd.c`, we have changed them to local scope
  in the respective board CMakeLists.txt files. Applications that depended on these definitions
  being globally available may need to be updated. (:github:`101322`)

* Renesas ``ek_ra8t2/r7ka8t2lfecac/cm85`` is renamed to ``ek_ra8t2/r7ka8t2lflcac/cm85``.

* NXP has changed the scope of some in-tree compile flags to limit their visibility to only where
  they are needed. Out-of-tree applications or boards that depended on these flags being globally
  available may need to add them to their own CMakeLists.txt files to ensure they continue to build
  correctly. (:github:`100252`)
  The affected flags are listed below:

  * For the RT10xx and RT11xx families, the compile flag ``BOARD_FLASH_SIZE``, originally defined in
    ``boards/nxp/mimxrt10xx_evk/CMakeLists.txt`` and ``boards/nxp/mimxrt11xx_evk/CMakeLists.txt``, is
    used only by the HAL header ``fsl_flexspi_nor_boot.h``, which is included by
    :zephyr_file:`soc/nxp/imxrt/imxrt10xx/soc.c` and :zephyr_file:`soc/nxp/imxrt/imxrt11xx/soc.c`.
    To avoid potential collisions with other global flags, the macro is now defined at the SoC layer
    using ``zephyr_library_compile_definitions()`` in :zephyr_file:`soc/nxp/imxrt/imxrt10xx/CMakeLists.txt`
    and :zephyr_file:`soc/nxp/imxrt/imxrt11xx/CMakeLists.txt`. This change has been applied to all
    RTxxxx boards.

  * For the RTxxx family, the compile flag ``BOARD_FLASH_SIZE``, originally defined in
    ``boards/nxp/mimxrtxxx_evk/CMakeLists.txt``, is not used in the Zephyr tree and has
    therefore been removed from all RTxxx board CMakeLists.txt files.

  * For the RTxxx family, the compile flag ``BOOT_HEADER_ENABLE``, previously defined in
    ``boards/nxp/mimxrtxxx_evk/CMakeLists.txt`` and used in ``boards/nxp/rtxxx/<boot_header>.c``,
    has been replaced by a Kconfig option. Consequently, the line
    ``zephyr_compile_definitions(BOOT_HEADER_ENABLE=1)`` has been removed from the RTxxx board
    CMakeLists.txt files.

  * Removed compile flag ``BOOT_HEADER_ENABLE`` definition from :zephyr_file:`boards/nxp/rd_rw612_bga/CMakeLists.txt`,
    as it is not used in the Zephyr tree.

  * Originally, the compile flags ``XIP_BOOT_HEADER_ENABLE`` and ``XIP_BOOT_HEADER_DCD_ENABLE`` were
    used in ``boards/nxp/rt1xxx/<boot_header>.c``. These flags have been converted to Kconfig options
    across NXP RTxxxx evaluation boards, allowing boot-header configuration via the Kconfig build system
    instead of compile-time defines. Consequently, we removed ``zephyr_compile_definitions(XIP_BOOT_HEADER_ENABLE=1)``
    and ``zephyr_compile_definitions(XIP_BOOT_HEADER_DCD_ENABLE=1)`` from the RTxxxx board-level CMakeLists.txt files.
    Because these macros are also required by ``hal_nxp/rt10xx/fsl_flexspi_nor_boot.h`` and
    ``hal_nxp/rt11xx/fsl_flexspi_nor_boot.h``, they were added to the corresponding SoC-layer CMakeLists.txt files
    using ``zephyr_library_compile_definitions()`` to limit their scope.

* The following Nordic SoC Kconfigs have been deprecated and replaced, and Kconfig/CMake/code
  needs to be updated if they reference the deprecated Kconfigs:

  * :kconfig:option:`CONFIG_SOC_SERIES_NRF51X` with :kconfig:option:`CONFIG_SOC_SERIES_NRF51`
  * :kconfig:option:`CONFIG_SOC_SERIES_NRF52X` with :kconfig:option:`CONFIG_SOC_SERIES_NRF52`
  * :kconfig:option:`CONFIG_SOC_SERIES_NRF53X` with :kconfig:option:`CONFIG_SOC_SERIES_NRF53`
  * :kconfig:option:`CONFIG_SOC_SERIES_NRF54HX` with :kconfig:option:`CONFIG_SOC_SERIES_NRF54H`
  * :kconfig:option:`CONFIG_SOC_SERIES_NRF54LX` with :kconfig:option:`CONFIG_SOC_SERIES_NRF54L`
  * :kconfig:option:`CONFIG_SOC_SERIES_NRF91X` with :kconfig:option:`CONFIG_SOC_SERIES_NRF91`
  * :kconfig:option:`CONFIG_SOC_SERIES_NRF92X` with :kconfig:option:`CONFIG_SOC_SERIES_NRF92`

* ITE ``it515xx_evb`` is renamed to ``it51xxx_evb``.

Device Drivers and Devicetree
*****************************

.. zephyr-keep-sorted-start re(^\w) ignorecase

ADC
===

* The :dtcompatible:`renesas,ra-adc` compatible has been replaced by
  :dtcompatible:`renesas,ra-adc12`. Applications using the old compatible
  must update their devicetree nodes.

* The :dtcompatible:`renesas,ra-adc16` compatible was added. This must be
  used when working with the EK-RA2A1 board, which provides a 16-bit ADC
  resolution.

* Renamed the :kconfig:option:`CONFIG_ADC_MCUX_SAR_ADC` to :kconfig:option:`CONFIG_ADC_NXP_SAR_ADC`.
* Renamed the driver file from ``adc_mcux_sar_adc.c`` to :zephyr_file:`drivers/adc/adc_nxp_sar_adc.c`.
* Applications using the SAR ADC driver need to update the nodes in the devicetree to include
  ``zephyr,input-positive`` to specify the hardware channel. For SoCs that currently support SAR ADC,
  the reference voltage should use ``ADC_REF_VDD_1`` instead of ``ADC_REF_INTERNAL``. This driver
  update also corrects this issue, so users also need to update the value of this property in the
  devicetree accordingly. (:github:`100978`)

* :dtcompatible:`st,stm32-adc` no longer has the ``resolutions`` property. It is replaced by the
  ``st,adc-resolutions`` property. For STM32H7 devices in revision Y, it is no longer needed to
  replace the 14 and 12-bit resolution values. This change may have an impact on power consumption
  if 14 or 12-bit resolutions are used. Previously, power-optimized values were used, now the
  standard values (not power-optimized but better accuracy) are used. No impact on other series.

Controller Area Network (CAN)
=============================

* Removed ``CONFIG_CAN_MAX_FILTER``, ``CONFIG_CAN_MAX_STD_ID_FILTER``,
  ``CONFIG_CAN_MAX_EXT_ID_FILTER`` (:github:`100596`). These are replaced by the following
  driver-specific Kconfig symbols, some of which have had their default value increased to meet
  typical software needs:

  * :kconfig:option:`CONFIG_CAN_LOOPBACK_MAX_FILTERS` for :dtcompatible:`zephyr,can-loopback`
  * :kconfig:option:`CONFIG_CAN_MAX32_MAX_FILTERS` for :dtcompatible:`adi,max32-can`
  * :kconfig:option:`CONFIG_CAN_MCP2515_MAX_FILTERS` for :dtcompatible:`microchip,mcp2515`
  * :kconfig:option:`CONFIG_CAN_MCP251XFD_MAX_FILTERS` for :dtcompatible:`microchip,mcp251xfd`
  * :kconfig:option:`CONFIG_CAN_MCUX_FLEXCAN_MAX_FILTERS` for :dtcompatible:`nxp,flexcan`
  * :kconfig:option:`CONFIG_CAN_NATIVE_LINUX_MAX_FILTERS` for
    :dtcompatible:`zephyr,native-linux-can`
  * :kconfig:option:`CONFIG_CAN_RCAR_MAX_FILTERS` for :dtcompatible:`renesas,rcar-can`
  * :kconfig:option:`CONFIG_CAN_SJA1000_MAX_FILTERS` for :dtcompatible:`kvaser,pcican` and
    :dtcompatible:`espressif,esp32-twai`
  * :kconfig:option:`CONFIG_CAN_STM32_BXCAN_MAX_EXT_ID_FILTERS` for :dtcompatible:`st,stm32-bxcan`
  * :kconfig:option:`CONFIG_CAN_STM32_BXCAN_MAX_STD_ID_FILTERS` for :dtcompatible:`st,stm32-bxcan`
  * :kconfig:option:`CONFIG_CAN_STM32_FDCAN_MAX_EXT_ID_FILTERS` for :dtcompatible:`st,stm32-fdcan`
  * :kconfig:option:`CONFIG_CAN_STM32_FDCAN_MAX_STD_ID_FILTERS` for :dtcompatible:`st,stm32-fdcan`
  * :kconfig:option:`CONFIG_CAN_XMC4XXX_MAX_FILTERS` for :dtcompatible:`infineon,xmc4xxx-can-node`

* Replaced Kconfig option ``CONFIG_CAN_MAX_MB`` for :dtcompatible:`nxp,flexcan` and
  :dtcompatible:`nxp,flexcan-fd` with per-instance ``number-of-mb`` and
  ``number-of-mb-fd`` devicetree properties (:github:`99483`).

* The :dtcompatible:`nxp,flexcan` ``clk-source`` devicetree property, if present, now automatically
  selects between the named input clocks ``clksrc0`` and ``clksrc1`` for use as the CAN protocol
  engine clock.

Counter
=======

* The NXP LPTMR driver (:dtcompatible:`nxp,lptmr`) has been updated to fix incorrect
  prescaler and glitch filter configuration:

  * The ``prescale-glitch-filter`` property valid range changed from ``[0-16]`` to ``[0-15]``.
    The value ``16`` was invalid for pulse counter mode and has been removed. Device trees using
    value ``16`` must be updated to use values in the range ``[0-15]``.

  * A new boolean property ``prescale-glitch-filter-bypass`` has been introduced to explicitly
    control prescaler/glitch filter bypass. Previously, setting ``prescale-glitch-filter = <0>``
    implicitly enabled bypass mode, which was ambiguous.

    In v4.4 and later, bypass is controlled only by the presence of
    ``prescale-glitch-filter-bypass``. If the property is absent, the prescaler/glitch filter is
    active and ``prescale-glitch-filter`` is applied.

  * The prescaler/glitch filter behavior has been clarified:

    * In Time Counter mode: prescaler divides the clock by ``2^(prescale-glitch-filter + 1)``
    * In Pulse Counter mode: glitch filter recognizes change after ``2^prescale-glitch-filter``
      rising edges (value 0 is not supported for glitch filtering)

  * All in-tree device tree nodes have been updated to use ``prescale-glitch-filter-bypass;``
    instead of ``prescale-glitch-filter = <0>;``. Out-of-tree boards should be updated
    accordingly.

  * If both ``prescale-glitch-filter-bypass`` and ``prescale-glitch-filter`` are set,
    bypass mode takes precedence and the ``prescale-glitch-filter`` value is ignored.

  Example migration:

  .. code-block:: devicetree

     /* Old (deprecated) */
     lptmr0: counter@40040000 {
         compatible = "nxp,lptmr";
         /* Implicitly bypassed */
         prescale-glitch-filter = <0>;
     };

     /* New (correct) */
     lptmr0: counter@40040000 {
         compatible = "nxp,lptmr";
         /* Explicitly bypassed */
         prescale-glitch-filter-bypass;
     };

  .. rubric:: Examples of using ``prescale-glitch-filter``

  .. note::

     ``prescale-glitch-filter-bypass`` is a boolean. If present, bypass is enabled. If absent,
     bypass is disabled and ``prescale-glitch-filter`` is applied.

     In Pulse Counter mode, ``prescale-glitch-filter = <0>`` is not a supported glitch filter
     configuration. To request no filtering, use ``prescale-glitch-filter-bypass;``.

  * Time Counter mode: divide the counter clock

    In Time Counter mode the prescaler divides by ``2^(N + 1)``.

    .. code-block:: devicetree

       /* Divide by 2^(0+1) = 2 */
       lptmr0: counter@40040000 {
           compatible = "nxp,lptmr";
           /* Time Counter mode */
           timer-mode-sel = <0>;
           clk-source = <1>;
           clock-frequency = <32768>;
           /* /2 */
           prescale-glitch-filter = <0>;
           resolution = <16>;
       };

       /* Divide by 2^(3+1) = 16 */
       lptmr1: counter@40041000 {
           compatible = "nxp,lptmr";
           /* Time Counter mode */
           timer-mode-sel = <0>;
           clk-source = <1>;
           clock-frequency = <32768>;
           /* /16 */
           prescale-glitch-filter = <3>;
           resolution = <16>;
       };

  * Time Counter mode: explicit bypass (no division)

    .. code-block:: devicetree

       lptmr0: counter@40040000 {
           compatible = "nxp,lptmr";
           /* Time Counter mode */
           timer-mode-sel = <0>;
           clk-source = <1>;
           clock-frequency = <32768>;
           /* no prescaler */
           prescale-glitch-filter-bypass;
           resolution = <16>;
       };

  * Pulse Counter mode: glitch filtering

    In Pulse Counter mode the glitch filter recognizes a change after ``2^N`` rising edges.
    Value ``0`` is not supported for glitch filtering; use bypass if you want no filtering.

    .. code-block:: devicetree

       /* Recognize change after 2^2 = 4 rising edges */
       lptmr0: counter@40040000 {
           compatible = "nxp,lptmr";
           /* Pulse Counter mode */
           timer-mode-sel = <1>;
           clk-source = <1>;
           input-pin = <0>;
           prescale-glitch-filter = <2>;
           resolution = <16>;
       };

       /* No filtering (explicit bypass) */
       lptmr1: counter@40041000 {
           compatible = "nxp,lptmr";
           /* Pulse Counter mode */
           timer-mode-sel = <1>;
           clk-source = <1>;
           input-pin = <0>;
           prescale-glitch-filter-bypass;
           resolution = <16>;
       };

* The NXP i.MX GPT counter driver (:dtcompatible:`nxp,imx-gpt`) now
  defaults to ``run-mode = "restart"`` instead of the previous hardcoded free-run behavior.

  * **Previous behavior** (Zephyr ≤ 4.3): GPT counter always ran in free-run mode
    (``enableFreeRun = true``). The counter continued counting without reset on compare events.

  * **New behavior** (Zephyr ≥ 4.4): GPT counter defaults to restart mode unless explicitly
    configured. A new ``run-mode`` devicetree property controls the behavior:

    * ``"restart"`` (default): Counter resets to 0 when reaching Compare Channel 1 value
    * ``"free-run"``: Counter continues counting without reset (previous behavior)

  **Migration Required**: Out-of-tree boards and applications using GPT counters must add
  ``run-mode = "free-run";`` to their devicetree nodes to preserve the previous behavior.

  .. code-block:: devicetree

     /* Out-of-tree boards: add this to preserve previous behavior */
     gpt2: gpt@400f0000 {
         compatible = "nxp,imx-gpt";
         /* Explicitly restore Zephyr ≤4.3 behavior */
         run-mode = "free-run";
         /* ... other properties ... */
     };

  .. warning::

     The driver uses Compare Channel 1 for Zephyr counter alarm functionality. When using
     ``run-mode = "restart"``, setting alarms will cause the counter to reset at the alarm
     compare point. If your application relies on alarms and continuous counting, you must
     use ``run-mode = "free-run"``.

  .. note::

     This change standardizes NXP counter driver run mode configuration.
     GPT now uses explicit devicetree properties rather than hardcoded values, allowing
     per-instance customization.

Display
=======

* For ILI9XXX controllers, the usage of ``ILI9XXX_PIXEL_FORMAT_x`` in devicetrees for panel color
  format selection has been updated to ``PANEL_PIXEL_FORMAT_x``. Out-of-tree boards and shields
  should be updated accordingly. (:github:`99267`).

* For ILI9341 controller, display mirroring configuration has been updated to conform with
  the described behavior of the sample ``samples/drivers/display``. (:github:`99267`).

* The ``PIXEL_FORMAT_BGR_565`` pixel format has been renamed to
  :c:macro:`PIXEL_FORMAT_RGB_565X` to correctly reflect that it is a
  byte-swapped version of RGB_565, not a channel-swapped format.
  Applications using ``PIXEL_FORMAT_BGR_565`` must update to use
  :c:macro:`PIXEL_FORMAT_RGB_565X`. (:github:`99276`)

* The devicetree macro ``PANEL_PIXEL_FORMAT_BGR_565`` has been renamed to
  :c:macro:`PANEL_PIXEL_FORMAT_RGB_565X`. (:github:`99276`)

* The Kconfig options ``SDL_DISPLAY_DEFAULT_PIXEL_FORMAT_BGR_565`` and
  ``ST7789V_BGR565`` have been renamed to
  :kconfig:option:`SDL_DISPLAY_DEFAULT_PIXEL_FORMAT_RGB_565X` and :kconfig:option:`ST7789V_RGB565X`
  respectively. (:github:`99276`)

* ``CONFIG_SSD1327`` symbol has been renamed to :kconfig:option:`CONFIG_SSD1327_5` to include ``SSD1325`` as well.

* ``solomon,ssd1327fb`` devicetree compatible has been renamed :dtcompatible:`solomon,ssd1327`
  to harmonize with other display controllers and eliminate the zephyr-irrelevant ``fb`` suffix.

* ``solomon,ssd1306fb`` and ``solomon,ssd1309fb`` devicetree compatibles has been renamed
  :dtcompatible:`solomon,ssd1306` and :dtcompatible:`solomon,ssd1309` respectively,
  to harmonize with other display controllers and eliminate the zephyr-irrelevant ``fb`` suffix.

DMA
===

* Removed the :kconfig:option:`CONFIG_DMA_MCUX_EDMA_V5` (:github:`100341`). This macro previously distinguished between
  nxp,version(5) and nxp,version(4). It now supports unified maintenance for both versions.
  Users can modify ``DMA_MCUX_EDMA_V5`` to ``DMA_MCUX_EDMA_V4``.

EEPROM
======

* Added :c:func:`eeprom_target_read_data()` and :c:func:`eeprom_target_write_data()` which takes an
  offset and length and deprecated :c:func:`eeprom_target_program()` for the I2C EEPROM target driver.

ESP32-S3
========

* The former ``espressif,esp32-lcd-cam`` binding has been restructured. The
  LCD_CAM peripheral is now represented by a common ``lcd_cam`` node, with its
  functional blocks split into two separate child nodes:

    * :dtcompatible:`espressif,esp32-lcd-cam-dvp` compatible node for the DVP
      (camera) input module, labeled as ``lcd_cam_dvp``.
    * :dtcompatible:`espressif,esp32-lcd-cam-mipi-dbi` compatible node for the
      LCD output module, labeled as ``lcd_cam_disp``.

  The original :dtcompatible:`espressif,esp32-lcd-cam` compatible node keeps the
  common pinctrl, clock, and interrupt properties, while camera-specific
  properties have moved into the new ``lcd_cam_dvp`` child node.

  Camera-related properties must be moved from ``lcd_cam`` node to the new
  ``lcd_cam_dvp`` child node, and  ``zephyr,camera`` chosen property should
  point to ``lcd_cam_dvp`` instead.

Ethernet
========

* Driver MAC address configuration support using :c:struct:`net_eth_mac_config` has been introduced
  for the following drivers:

  * :dtcompatible:`atmel,sam-gmac` and :dtcompatible:`atmel,sam0-gmac` (:github:`96598`)

    * Removed ``CONFIG_ETH_SAM_GMAC_MAC_I2C_EEPROM``
    * Removed ``CONFIG_ETH_SAM_GMAC_MAC_I2C_INT_ADDRESS``
    * Removed ``CONFIG_ETH_SAM_GMAC_MAC_I2C_INT_ADDRESS_SIZE``
    * Removed ``mac-eeprom`` property

  * :dtcompatible:`litex,liteeth` (:github:`100620`)
  * :dtcompatible:`microchip,lan865x` (:github:`100318`)
  * :dtcompatible:`microchip,lan9250` (:github:`99127`)
  * :dtcompatible:`sensry,sy1xx-mac` (:github:`100619`)
  * :dtcompatible:`virtio,net` (:github:`100106`)
  * :dtcompatible:`vnd,ethernet` (:github:`96598`)
  * :dtcompatible:`wiznet,w5500` (:github:`100919`)

* The ``fixed-link`` property has been removed from :dtcompatible:`ethernet-phy`. Use
  the new :dtcompatible:`ethernet-phy-fixed-link` compatible instead, if that functionality
  is needed. There you need to specify the fixed link parameters using the ``default-speeds``
  property (:github:`100454`).

* The ``reset-gpios`` property of :dtcompatible:`microchip,ksz8081` has been
  reworked to be used as active low, you may have to set the pin as
  ``GPIO_ACTIVE_LOW`` in devicetree (:github:`100751`).

GPIO
====

* The LiteX GPIO driver :dtcompatible:`litex,gpio` has been reworked to support changing direction.
  The driver now uses the reg-names property to detect supported modes of the GPIO controller.
  The Devicetree property ``port-is-output`` has been removed.
  The reg-names are now taken directly from LiteX. (:github:`99329`)

* The ``irqs`` property of :dtcompatible:`renesas,rz-gpio` has been reworked
  to map a pin to an interrupt phandle explicitly instead of an interrupt index (:github:`101256`).

  .. code-block:: devicetree

     /* Old (Zephyr ≤ 4.3) */
     &gpio16 {
         /* Map port16 pin3 to tint7 */
         irqs = <3 7>;
     };

     /* New (Zephyr ≥ 4.4) */
     &tint7 {
         status = "okay";
     };

     &gpio16 {
         /* Map port16 pin3 to tint7 */
         irqs = <&tint7 3>;
     };

Infineon
========

* Infineon driver file names have been renamed to remove ``cat1`` from their names to support
  reusability across multiple device categories. The following drivers have been renamed
  (:github:`99174`):

  * ``adc_ifx_cat1.c`` → ``adc_ifx.c``
  * ``clock_control_ifx_cat1.c`` → ``clock_control_ifx.c``
  * ``counter_ifx_cat1.c`` → ``counter_ifx.c``
  * ``dma_ifx_cat1.c`` → ``dma_ifx.c``
  * ``dma_ifx_cat1_pdl.c`` → ``dma_ifx_pdl.c``
  * ``flash_ifx_cat1.c`` → ``flash_ifx.c``
  * ``flash_ifx_cat1_qspi.c`` → ``flash_ifx_qspi.c``
  * ``flash_ifx_cat1_qspi_mtb_hal.c`` → ``flash_ifx_qspi_mtb_hal.c``
  * ``gpio_ifx_cat1.c`` → ``gpio_ifx.c``
  * ``i2c_ifx_cat1.c`` → ``i2c_ifx.c``
  * ``i2c_ifx_cat1_pdl.c`` → ``i2c_ifx_pdl.c``
  * ``mbox_ifx_cat1.c`` → ``mbox_ifx.c``
  * ``pinctrl_ifx_cat1.c`` → ``pinctrl_ifx.c``
  * ``rtc_ifx_cat1.c`` → ``rtc_ifx.c``
  * ``ifx_cat1_sdio.c`` → ``ifx_sdio.c``
  * ``sdio_ifx_cat1_pdl.c`` → ``sdio_ifx_pdl.c``
  * ``serial_ifx_cat1_uart.c`` → ``serial_ifx_uart.c``
  * ``spi_ifx_cat1.c`` → ``spi_ifx.c``
  * ``spi_ifx_cat1_pdl.c`` → ``spi_ifx_pdl.c``
  * ``uart_ifx_cat1.c`` → ``uart_ifx.c``
  * ``uart_ifx_cat1_pdl.c`` → ``uart_ifx_pdl.c``
  * ``wdt_ifx_cat1.c`` → ``wdt_ifx.c``

  Corresponding Kconfig symbols and binding files have also been updated:

  * ``CONFIG_*_INFINEON_CAT1`` → ``CONFIG_*_INFINEON``
  * ``compatible: "infineon,cat1-adc"`` → ``compatible: "infineon,adc"``

MDIO
====

* The ``mdio_bus_enable()`` and ``mdio_bus_disable()`` functions have been removed.
  MDIO bus enabling/disabling is now handled internally by the MDIO drivers.
  (:github:`99690`).

QSPI
====

* :dtcompatible:`st,stm32-qspi` compatible nodes configured with ``dual-flash`` property
  now need to also include the ``ssht-enable`` property to reenable sample shifting.
  Sample shifting is configurable now and disabled by default.
  (:github:`98999`).

Radio
=====

* The following devicetree bindings have been renamed for consistency with the ``radio-`` prefix:

  * :dtcompatible:`generic-fem-two-ctrl-pins` is now :dtcompatible:`radio-fem-two-ctrl-pins`
  * :dtcompatible:`gpio-radio-coex` is now :dtcompatible:`radio-gpio-coex`

* A new :dtcompatible:`radio.yaml` base binding has been introduced for generic radio hardware
  capabilities. The ``tx-high-power-supported`` property has been renamed to
  ``radio-tx-high-power-supported`` for consistency.

* Device trees and overlays using the old compatible strings must be updated to use the new names.

Shell
=====

* The :c:func:`shell_set_bypass` now requires a user data pointer to be passed. And accordingly the
  :c:type:`shell_bypass_cb_t` now has a user data argument. (:github:`100311`)

Stepper
=======

* For :dtcompatible:`adi,tmc2209`, the property ``msx-gpios`` is now replaced by ``m0-gpios`` and
  ``m1-gpios`` for consistency with other step/dir stepper drivers.

* Since :github:`91979`, All stepper-drv driver APIs have been refactored out of the stepper API.
  The following APIs have been moved from :c:group:`stepper_interface` to :c:group:`stepper_drv_interface`:

  * :c:func:`stepper_enable` is replaced by :c:func:`stepper_drv_enable`.
  * :c:func:`stepper_disable` is replaced by :c:func:`stepper_drv_disable`.
  * :c:func:`stepper_set_micro_step_res` is replaced by :c:func:`stepper_drv_set_micro_step_res`.
  * :c:func:`stepper_get_micro_step_res` is replaced by :c:func:`stepper_drv_get_micro_step_res`.

* :c:enum:`stepper_micro_step_resolution` is replaced by :c:enum:`stepper_drv_micro_step_resolution`.
* ``STEPPER_DRV_EVENT_STALL_DETECTED`` and ``STEPPER_DRV_EVENT_FAULT_DETECTED`` events have been
  refactored to :c:enum:`stepper_drv_event`.

* :dtcompatible:`zephyr,gpio-step-dir-stepper` implements :c:group:`stepper_interface` for
  controlling stepper motors via GPIO step and direction signals. Refer to
  :ref:`stepper-individual-controller-driver` for more details.

  * ``step-gpios``, ``dir-gpios``, ``invert-direction`` and ``counter`` properties are removed
    from :dtcompatible:`adi,tmc2209`, :dtcompatible:`ti,drv84xx` and :dtcompatible:`allegro,a4979`,
    these are now are implemented by :dtcompatible:`zephyr,gpio-step-dir-stepper`.
  * :c:func:`stepper_move_by`, :c:func:`stepper_move_to`, :c:func:`stepper_run`,
    :c:func:`stepper_stop`, :c:func:`stepper_is_moving`, :c:func:`stepper_set_microstep_interval`
    and :c:func:`stepper_set_event_callback` APIs are removed from :dtcompatible:`adi,tmc2209`,
    :dtcompatible:`ti,drv84xx` and :dtcompatible:`allegro,a4979`.
  * :dtcompatible:`adi,tmc2209`, :dtcompatible:`ti,drv84xx` and :dtcompatible:`allegro,a4979`
    implement :c:group:`stepper_drv_interface`.

* :c:func:`stepper_enable`, :c:func:`stepper_disable`, :c:func:`stepper_set_micro_step_res` and
  :c:func:`stepper_get_micro_step_res` APIs are removed from :dtcompatible:`zephyr,h-bridge-stepper`.
* ``en-gpios`` property is removed from :dtcompatible:`zephyr,h-bridge-stepper`.
* ``micro-step-res`` property is replaced by ``lut-step-gap`` property in
  :dtcompatible:`zephyr,h-bridge-stepper`.

* :dtcompatible:`adi,tmc50xx` and :dtcompatible:`adi,tmc51xx` devices are now modeled as MFDs.
* :dtcompatible:`adi,tmc50xx-stepper` and :dtcompatible:`adi,tmc51xx-stepper` drivers implement
  :c:group:`stepper_interface`.
* :dtcompatible:`adi,tmc50xx-stepper-drv` and :dtcompatible:`adi,tmc51xx-stepper-drv` drivers implement
  :c:group:`stepper_drv_interface`.

STM32
=====

* STM32 power supply configuration is now performed using Devicetree properties.
  New bindings :dtcompatible:`st,stm32h7-pwr`, :dtcompatible:`st,stm32h7rs-pwr`
  and :dtcompatible:`st,stm32-dualreg-pwr` have been introduced, and all Kconfig
  symbols related to power supply configuration have been removed:

  * ``CONFIG_POWER_SUPPLY_LDO``

  * ``CONFIG_POWER_SUPPLY_DIRECT_SMPS``,

  * ``CONFIG_POWER_SUPPLY_SMPS_1V8_SUPPLIES_LDO``

  * ``CONFIG_POWER_SUPPLY_SMPS_2V5_SUPPLIES_LDO``,

  * ``CONFIG_POWER_SUPPLY_SMPS_1V8_SUPPLIES_EXT_AND_LDO``

  * ``CONFIG_POWER_SUPPLY_SMPS_2V5_SUPPLIES_EXT_AND_LDO``

  * ``CONFIG_POWER_SUPPLY_SMPS_1V8_SUPPLIES_EXT``

  * ``CONFIG_POWER_SUPPLY_SMPS_2V5_SUPPLIES_EXT``

  * ``CONFIG_POWER_SUPPLY_EXTERNAL_SOURCE``

* The ST-specific chosen property ``/chosen/zephyr,ccm`` is replaced by ``/chosen/zephyr,dtcm``.
  Attribute macros ``__ccm_data_section``, ``__ccm_bss_section`` and ``__ccm_noinit_section`` are
  deprecated, but retained for backwards compatibility; **they will be removed in Zephyr 4.5**.
  The generic ``__dtcm_{data,bss,noinit}_section`` macros should be used instead. (:github:`100590`)

* STM32 platforms now use the default MCUboot operating mode ``swap using offset``
  (:kconfig:option:`SB_CONFIG_MCUBOOT_MODE_SWAP_USING_OFFSET`). To support this bootloader mode,
  some changes to the board devicetrees are required. Several boards already support this mode
  (see :github:`100385`).
  The previous ``swap using move`` mode can still be selected in sysbuild by enabling
  :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_SWAP_USING_MOVE`.

* For STM32F2x/F4x/F7x, the different PLL bindings (:dtcompatible:`st,stm32f2-pll-clock`,
  :dtcompatible:`st,stm32f4-pll-clock`, :dtcompatible:`st,stm32f4-plli2s-clock`,
  :dtcompatible:`st,stm32f411-plli2s-clock`, :dtcompatible:`st,stm32f7-pll-clock` and
  :dtcompatible:`st,stm32fx-pllsai-clock` ) has been merged into a single one
  :dtcompatible:`st,stm32fx-pll-clock`. This merge brings some changes, notably ``div-divq`` and
  ``div-divr`` properties have been renamed respectively to ``post-div-q`` and ``post-div-r``.
  Besides, when applicable to the SoC, these properties need to be defined if the corresponding
  ``div-q`` or ``div-r`` properties are used.

USB
===

  * :dtcompatible:`maxim,max3421e_spi` has been renamed to :dtcompatible:`maxim,max3421e-spi`.

Video
=====

* CONFIG_VIDEO_OV7670 is now gone and replaced by CONFIG_VIDEO_OV767X.  This allows supporting both the OV7670 and 0V7675.
* :kconfig:option:`CONFIG_VIDEO_BUFFER_POOL_SZ_MAX` is replaced by
  :kconfig:option:`CONFIG_VIDEO_BUFFER_POOL_HEAP_SIZE` which represent the
  size in byte allocated for the whole video buffer pool.

* The :dtcompatible:`ovti,ov2640` reset pin handling has been corrected, resulting in an inverted
  active level compared to before, to match the active level expected by the sensor.

.. zephyr-keep-sorted-stop

Bluetooth
*********

Bluetooth Host
==============

* :kconfig:option:`CONFIG_BT_SIGNING` has been deprecated.
* :c:macro:`BT_GATT_CHRC_AUTH` has been deprecated.
* :c:member:`bt_conn_le_info.interval` has been deprecated. Use
  :c:member:`bt_conn_le_info.interval_us` instead. Note that the units have changed: ``interval``
  was in units of 1.25 milliseconds, while ``interval_us`` is in microseconds.
* Legacy Bluetooth LE pairing using the passkey entry method no longer grants authenticated (MITM)
  protection as of the Bluetooth Core Specification v6.2. Stored bonds that were generated using
  this method will be downgraded to unauthenticated when loaded from persistent storage, resulting
  in a lower security level.

Bluetooth Audio
===============

* :c:func:`bt_bap_broadcast_assistant_discover` will now no longer perform reads of the remote BASS
  receive states at the end of the procedure. Users will have to manually call
  :c:func:`bt_bap_broadcast_assistant_read_recv_state` to read the existing receive states, if any,
  prior to performing any operations. (:github:`91587`)
* :kconfig:option:`CONFIG_BT_AUDIO` now depends on :kconfig:option:`CONFIG_UTF8`.
  Applications that enable :kconfig:option:`CONFIG_BT_AUDIO` must also have
  :kconfig:option:`CONFIG_UTF8` enabled. (:github:`102350`)

Bluetooth Mesh
==============

* :kconfig:option:`CONFIG_BT_MESH_MODEL_VND_MSG_CID_FORCE` has been deprecated. Enabling it no
  longer has any effect on message handling performance.

Networking
**********

* Networking APIs found in

  * :zephyr_file:`include/zephyr/net/net_ip.h`
  * :zephyr_file:`include/zephyr/net/socket.h`

  and relevant code in ``subsys/net`` etc. is namespaced. This means that either
  ``net_``, ``NET_`` or ``ZSOCK_`` prefix is added to the network API name. This is done in order
  to avoid circular dependency with POSIX or libc that might define the same symbol.
  A compatibility header file :zephyr_file:`include/zephyr/net/net_compat.h`
  is created that provides the old symbols allowing the user to continue use the old symbols.
  External network applications can continue to use POSIX defined network symbols and
  include relevant POSIX header files like ``sys/socket.h`` to get the POSIX symbols as Zephyr
  networking header files will no longer include those. If the application or Zephyr internal
  code cannot use POSIX APIs, then the relevant network API prefix needs to be added to the
  code calling a network API.

* The enum for HTTP server transaction status has been renamed from ``http_data_status``
  to ``http_transaction_status`` to better reflect its purpose. The enum values have also been
  renamed as follows:

  - ``HTTP_SERVER_DATA_ABORTED`` → ``HTTP_SERVER_TRANSACTION_ABORTED``
  - ``HTTP_SERVER_DATA_MORE`` → ``HTTP_SERVER_REQUEST_DATA_MORE``
  - ``HTTP_SERVER_DATA_FINAL`` → ``HTTP_SERVER_REQUEST_DATA_FINAL``

  The handler callback type for dynamic resources has been updated accordingly to use the new enum
  and its renamed values. Applications using dynamic HTTP resources must update their handler
  callbacks to use the new enum and handle the renamed values.

* The HTTP server now reports for dynamic resources the ``HTTP_SERVER_TRANSACTION_COMPLETE``
  status when the response has been sent completely to the client. Applications should now also
  handle this status in the handler callback to properly reset resource state after successful
  response transmission.

* The protocol version passed to :c:func:`zsock_socket` when creating a secure socket is now
  enforced as the minimum TLS version to use for the TLS session.

Modem
*****

Modem HL78XX
============

* The Kconfig options related to HL78XX startup timing have been renamed in
  :kconfig:option:`CONFIG_MODEM_HL78XX_DEV_*` as follows:

  - ``MODEM_HL78XX_DEV_POWER_PULSE_DURATION`` → ``MODEM_HL78XX_DEV_POWER_PULSE_DURATION_MS``
  - ``MODEM_HL78XX_DEV_RESET_PULSE_DURATION`` → ``MODEM_HL78XX_DEV_RESET_PULSE_DURATION_MS``
  - ``MODEM_HL78XX_DEV_STARTUP_TIME`` → ``MODEM_HL78XX_DEV_STARTUP_TIME_MS``
  - ``MODEM_HL78XX_DEV_SHUTDOWN_TIME`` → ``MODEM_HL78XX_DEV_SHUTDOWN_TIME_MS``

* The default startup timing was changed from 1000 ms to 120 ms to improve
  initialization reliability across all supported boards.

  Applications depending on the previous defaults must update their configuration.

LoRaWAN
*******

* The LoRaWAN region Kconfig symbols have been renamed from ``LORAMAC_REGION_*`` to
  ``LORAWAN_REGION_*`` to make them backend-agnostic. Applications using any of the following
  symbols must update their configuration files:

  * ``CONFIG_LORAMAC_REGION_AS923`` → :kconfig:option:`CONFIG_LORAWAN_REGION_AS923`
  * ``CONFIG_LORAMAC_REGION_AU915`` → :kconfig:option:`CONFIG_LORAWAN_REGION_AU915`
  * ``CONFIG_LORAMAC_REGION_CN470`` → :kconfig:option:`CONFIG_LORAWAN_REGION_CN470`
  * ``CONFIG_LORAMAC_REGION_CN779`` → :kconfig:option:`CONFIG_LORAWAN_REGION_CN779`
  * ``CONFIG_LORAMAC_REGION_EU433`` → :kconfig:option:`CONFIG_LORAWAN_REGION_EU433`
  * ``CONFIG_LORAMAC_REGION_EU868`` → :kconfig:option:`CONFIG_LORAWAN_REGION_EU868`
  * ``CONFIG_LORAMAC_REGION_KR920`` → :kconfig:option:`CONFIG_LORAWAN_REGION_KR920`
  * ``CONFIG_LORAMAC_REGION_IN865`` → :kconfig:option:`CONFIG_LORAWAN_REGION_IN865`
  * ``CONFIG_LORAMAC_REGION_US915`` → :kconfig:option:`CONFIG_LORAWAN_REGION_US915`
  * ``CONFIG_LORAMAC_REGION_RU864`` → :kconfig:option:`CONFIG_LORAWAN_REGION_RU864`

Other subsystems
****************
* The DAP subsystem initialization and configuration has changed. Please take a look at
  :zephyr:code-sample:`cmsis-dap` sample on how to initialize Zephyr DAP Link with USB backend.

* Cache

  * Use :kconfig:option:`CONFIG_CACHE_HAS_MIRRORED_MEMORY_REGIONS` instead of
    :kconfig:option:`CONFIG_CACHE_DOUBLEMAP` as the former is more descriptive of the feature.

Flash
=====

* Previously deprecated ``CONFIG_FLASH_AREA_CHECK_INTEGRITY_MBEDTLS`` is now
  removed.

* ``CONFIG_FLASH_AREA_CHECK_INTEGRITY_PSA`` is also removed since there is
  now no alternative for the crypto library backend.

JWT
===

* Previously deprecated ``CONFIG_JWT_SIGN_RSA_LEGACY`` is removed. This removal happens before
  the usual deprecation period of 2 releases because it has been agreed (see :github:`97660`)
  that Mbed TLS is an external module, so normal deprecation rules do not apply in this case.

Libsbc
======

* Libsbc (sbc.c and sbc.h) is moved under the Bluetooth subsystem. The sbc.h is in
  include/zephyr/bluetooth now.

Management
==========

* MCUmgr

  * If using :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_UART` then
    :kconfig:option:`CONFIG_UART_MCUMGR` must now also be selected, this has changed to be
    ``depends on`` rather than ``select``.

Tracing
========

* CTF: Changed uint8_t id to uint16_t id in the CTF metadata event header. This
  doubles the space used for event IDs but allows 65,535 events instead of 255.

  With this change, existing CTF traces with 8-bit IDs won't be compatible.

Serial
========

* pl011 UART driver: Remove Read Status Register (RSR) error handling
  from :c:func:`pl011_poll_in`. RSR handling is already implemented in
  :c:func:`pl011_err_check`, which is the appropriate place to detect,
  and report receive error conditions. (:github:`101715`)

Settings
========

* ``CONFIG_SETTINGS_TFM_ITS`` has been renamed to :kconfig:option:`CONFIG_SETTINGS_TFM_PSA`.

Modules
*******

Trusted Firmware-M
==================

* The ``SECURE_UART1`` TF-M define is now controlled by Zephyr's
  :kconfig:option:`CONFIG_TFM_SECURE_UART`. This option will override any platform values previously
  specified in the TF-M repository.

Architectures
*************

* Renamed ``CONFIG_ARCH_HAS_COHERENCE`` to :kconfig:option:`CONFIG_CACHE_CAN_SAY_MEM_COHERENCE` as
  the feature is cache related so move it under cache.

  * Use :c:func:`sys_cache_is_mem_coherent` instead of :c:func:`arch_mem_coherent`.

* :kconfig:option:`CONFIG_RISCV` now requires, that the :dtcompatible:`riscv` is present in the
  devicetree.

* The ``riscv,isa-base`` and  ``riscv,isa-extensions`` devicetree properties of
  :dtcompatible:`riscv` are now used to set the Base Integer Instruction Set and the RISC-V
  extensions. They are no longer set by the SoC. The devicetree property ``riscv,isa`` has been
  deprecated in favor of the two new properties. (:github:`97540`)

  * ``CONFIG_SOC_CV64A6_IMAFDC`` and ``CONFIG_SOC_CV64A6_IMAC`` are now combined into
    :kconfig:option:`CONFIG_SOC_CV64A6`, as the RISC-V extensions are now set by the devicetree.

  * The following options of :kconfig:option:`CONFIG_SOC_SERIES_AE350` had been removed, as they
    now can be set via the devicetree:

    * ``CONFIG_RV32I_CPU``
    * ``CONFIG_RV32E_CPU``
    * ``CONFIG_RV64I_CPU``
    * ``CONFIG_NO_FPU``
    * ``CONFIG_SINGLE_PRECISION_FPU``
    * ``CONFIG_DOUBLE_PRECISION_FPU``
