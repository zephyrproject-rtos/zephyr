:orphan:

..
  See
  https://docs.zephyrproject.org/latest/releases/index.html#migration-guides
  for details of what is supposed to go into this document.

.. _migration_4.5:

Migration guide to Zephyr v4.5.0 (Working Draft)
################################################

This document describes the changes required when migrating your application from Zephyr v4.4.0 to
Zephyr v4.5.0.

Any other changes (not directly related to migrating applications) can be found in
the :ref:`release notes<zephyr_4.5>`.

.. contents::
    :local:
    :depth: 2

Common
******

Build System
************

* :kconfig:option:`CONFIG_LEGACY_GENERATED_INCLUDE_PATH` has been deprecated, and disabled by
  default, includes must now be prefixed with ``zephyr/`` for zephyr files.

Kernel
******

* ``_k_neg_eagain`` has been renamed to ``_errno_neg_egain`` as ``errno`` has been migrated out of
  kernel into ``lib/libc/common``.

* When :kconfig:option:`CONFIG_SCHED_CPU_MASK_PIN_ONLY` is enabled, calling
  :c:func:`k_thread_cpu_mask_clear`, :c:func:`k_thread_cpu_mask_enable_all`,
  or :c:func:`k_thread_cpu_mask_disable` now triggers an assertion instead of
  silently producing an invalid state.  Applications using these functions
  in PIN_ONLY mode must be updated to use :c:func:`k_thread_cpu_pin` instead.

* :kconfig:option:`CONFIG_SCHED_CPU_MASK` is no longer restricted to
  :kconfig:option:`CONFIG_SCHED_SIMPLE`.  Projects that previously selected
  ``SCHED_SCALABLE`` or ``SCHED_MULTIQ`` and worked around the limitation by
  keeping ``SCHED_SIMPLE`` for affinity purposes can now use their preferred
  backend directly.

Boards
******

* The Kconfig options :kconfig:option:`CONFIG_SRAM_SIZE` and
  :kconfig:option:`CONFIG_SRAM_BASE_ADDRESS` have been deprecated, boards should instead use the
  devicetree ``zephyr.sram`` chosen node to specify the RAM node which will be used (whose values
  populated the Kconfig values). If either option is manually adjusted, it will cause
  :kconfig:option:`CONFIG_SRAM_DEPRECATED_KCONFIG_SET` to be set which indicates this deprecation.

* The internal Nordic SoC platform Kconfig symbols ``NRF_PLATFORM_HALTIUM``
  and ``NRF_PLATFORM_LUMOS`` are no longer used by in-tree code, which now
  relies on explicit :kconfig:option:`CONFIG_SOC_SERIES_NRF54H`,
  :kconfig:option:`CONFIG_SOC_SERIES_NRF92`,
  :kconfig:option:`CONFIG_SOC_SERIES_NRF54L` and
  :kconfig:option:`CONFIG_SOC_SERIES_NRF71` checks. Both symbols are kept
  as deprecated stubs that default to ``y`` when the corresponding SoC
  series is selected and :kconfig:option:`CONFIG_NRF_PLATFORM_DEPRECATED_SYMBOLS` is enabled,
  so existing ``CONFIG_NRF_PLATFORM_*=y`` lines and ``depends on NRF_PLATFORM_*`` clauses keep
  building with a Kconfig deprecation warning. Out-of-tree Kconfig, CMake and code using these
  symbols should be updated:

  * The ``CONFIG_NRF_PLATFORM_HALTIUM`` with
    :kconfig:option:`CONFIG_SOC_SERIES_NRF54H` or
    :kconfig:option:`CONFIG_SOC_SERIES_NRF92`.
  * The ``CONFIG_NRF_PLATFORM_LUMOS`` with
    :kconfig:option:`CONFIG_SOC_SERIES_NRF54L` or
    :kconfig:option:`CONFIG_SOC_SERIES_NRF71`.

* The Nordic sysbuild Kconfig option ``SB_CONFIG_NRF_HALTIUM_GENERATE_UICR``
  has been renamed to :kconfig:option:`SB_CONFIG_NRF_GENERATE_UICR`.
  Update sysbuild configurations to use the new name.

* The Nordic SoC headers :file:`<haltium_power.h>` and :file:`<haltium_pm_s2ram.h>`
  have been renamed to :file:`<soc_power.h>` and :file:`<soc_pm_s2ram.h>` respectively.
  Forwarder headers under the old names remain available and emit a ``#warning``
  pointing to the new include paths. Out-of-tree code that includes the old paths
  should be updated:

  * ``#include <haltium_power.h>`` with ``#include <soc_power.h>``.
  * ``#include <haltium_pm_s2ram.h>`` with ``#include <soc_pm_s2ram.h>``.

* The system clock on STM32H7RS-based boards (stm32h7s78_dk and nucleo_h7s3l8)
  has been increased to 600 MHz. This is achieved by increasing the PLL1 frequency
  to 300 MHz, which also affects the bus and kernel clocks, resulting in slightly
  higher frequencies.

* :kconfig:option:`CONFIG_GPIO` is no longer enabled by default on most STM32 boards.
  (boards with GPIO hogs keep it enabled as GPIO is needed for hogs to work).
  Applications that relied on ``CONFIG_GPIO=y`` being the default will need to enable
  the option explicitly. (:github:`109468`)

* Boards that use UF2 images that migrate to :dtcompatible:`zephyr,mapped-partition` should enable
  HEX output in their defconfig (:kconfig:option:`CONFIG_BUILD_OUTPUT_HEX`), as the UF2 image
  generation can no longer rely on :kconfig:option:`CONFIG_FLASH_LOAD_OFFSET` to determine the code
  address from a BIN output. HEX to UF2 is now the default (instead of BIN). (:github:`107944`)

* Ezurio bl54l15u_dvk has been removed. The bl54l15_dvk remains available and supports
  both the bl54l15 and bl54l15u variants of the module, with the same features.
  Boards using the bl54l15u_dvk should migrate to bl54l15_dvk/nrf54l15/cpuapp or
  bl54l15_dvk/nrf54l15/cpuflpr as appropriate.

* The default MCUboot signature type for the boards stm32h573i_dk and b_u585i_iot02a has
  been changed from RSA-3072 to EC-P256. This affects builds that have MCUboot enabled in
  TF-M (:kconfig:option:`CONFIG_TFM_BL2`). If you wish to keep using RSA-3072, you need
  to set :kconfig:option:`CONFIG_TFM_MCUBOOT_SIGNATURE_TYPE` to ``"RSA-3072"``.
  Otherwise, make sure to have your own signing keys of the signature type in use.

* All Kconfigs under modules/hal_silabs/gecko were renamed from ``SOC_GECKO_*``
  to ``SILABS_GECKO_*``. Adapt your board accordingly.

Device Drivers and Devicetree
*****************************

.. Only place contents common to all device drivers here. Contents specific to one driver subsystem
   goes into its own subsection, below.

* The :c:macro:`DEVICE_API` macro is now mandatory for declaring device driver API instances of any
  upstream driver class, including in out-of-tree drivers. :c:macro:`DEVICE_API_GET` now asserts
  that the API belongs to the requested class, which requires the instance to live in the class's
  iterable section. Out-of-tree driver classes that embed an upstream API as their first member
  must also declare the relationship with :c:macro:`DEVICE_API_EXTENDS`, so that
  :c:macro:`DEVICE_API_GET` for the parent class succeeds on devices implementing the child API.
  See :ref:`device_driver_api` for details.

.. Group contents in this section by subsystem, e.g.:
..
.. ADC
.. ===
..
.. ...

.. zephyr-keep-sorted-start re(^\w) ignorecase

ADC
===

* The ``girqs`` and ``pcrs`` properties (array type) of :dtcompatible:`microchip,xec-adc` have been
  replaced by encoded ``girqs`` (using ``MCHP_XEC_ECIA_GIRQ_ENC`` macros) and ``pcr-scr`` (int type)
  for encoded PCR register index and bit position (:github:`105658`).

* The :kconfig:option:`CONFIG_LPADC_DO_OFFSET_CALIBRATION` option is now only meaningful when
  :kconfig:option:`CONFIG_ADC_MCUX_LPADC` is enabled, and its ``default y`` is now scoped to that
  condition. In-tree boards no longer enable it explicitly in their defconfigs since
  the default already covers them.

Audio Codec
===========

* The audio codec driver backend API now uses :c:struct:`audio_codec_driver_api` instead of
  ``struct audio_codec_api``.

  Out-of-tree audio codec drivers must rename their backend API struct definitions and switch
  their API instances to ``DEVICE_API(audio_codec, ...)``. See :github:`110631` for examples of how
  in-tree drivers have been updated. Application code using the ``audio_codec_...`` APIs is not
  impacted.

Clock Control
=============

* The :dtcompatible:`nxp,imxrt11xx-arm-pll` binding now uses ``loop-div`` and
  ``post-div`` for ARM PLL configuration. The legacy ``clock-mult`` and
  ``clock-div`` properties remain supported but are deprecated. Existing
  RT11xx overlays should be updated using the mapping
  ``loop-div = clock-mult * 2`` and ``post-div = clock-div``.

Controller Area Network (CAN)
=============================

* The header files for the NXP SJA1000 (``can_sja1000.h``) and Bosch M_CAN (``can_mcan.h``) CAN
  controller driver backends where converted to library-specific includes. Out-of-tree drivers based
  on these backends will need to update their include directives accordingly.

* The Bosch M_CAN driver now solely uses RX FIFO0 for processing received CAN frames, ensuring these
  are processed in the order received on the bus. Out-of-tree users may want to update any
  ``bosch,mram-cfg`` devicetree property overrides to allocate all FIFO elements to RX FIFO0.

Devicetree
==========

* ``int`` and ``array`` typed devicetree properties whose DTS source uses a negative literal
  (e.g. ``<(-1)>``) now expand to a negative value instead of the two's-complement unsigned
  value used previously. Code that relied on the old unsigned representation, for example
  unsigned comparisons or ``BUILD_ASSERT(DT_PROP(node, foo) > 0, ...)`` checks, must be updated
  to use signed types or signed-aware checks (:github:`107271`).

Digital Microphone
==================

* The DMIC driver backend API now uses :c:struct:`dmic_driver_api` instead of ``struct _dmic_ops``.

  Out-of-tree DMIC drivers must rename their backend API struct definitions and switch their API
  instances to ``DEVICE_API(dmic, ...)``. See :github:`107695` for examples of how in-tree drivers
  have been updated. Application code using :c:func:`dmic_configure`, :c:func:`dmic_trigger`, and
  :c:func:`dmic_read` is not impacted.

Display
=======

* The Kconfig options ``CONFIG_SDL_DISPLAY_DEFAULT_PIXEL_FORMAT_*`` for SDL display pixel-format
  selection have been removed in favour of setting the pixel-format property directly in devicetree
  on the SDL pseudo-device node using the PANEL_PIXEL_FORMAT_* macros from
  :zephyr_file:`include/zephyr/dt-bindings/display/panel.h`. (:github:`104099`)

* The LVGL ``CONFIG_LV_Z_COLOR_24_BGR_TO_RGB`` Kconfig option has been removed. LVGL's RGB888 color
  format stores bytes in memory as blue, green, red, which matches the in-memory layout of
  :c:enumerator:`PIXEL_FORMAT_RGB_888`, so no channel swap is performed for displays reporting that
  format. Displays whose framebuffer instead expects a red, green, blue byte order must now report
  :c:enumerator:`PIXEL_FORMAT_BGR_888`, for which the LVGL glue performs the red/blue channel swap
  automatically.

* The Kconfig options ``CONFIG_ST730X_POWERMODE_LOW`` for ST7305 and ST7306 displays has been
  removed in favour of toggling the low-power-mode property on the device node.

DMA
===

* :dtcompatible:`silabs,siwx91x-dma` has been renamed :dtcompatible:`silabs,udma`. The Kconfig
  options have also been renamed to align with this new name (``DMA_SILABS_SIWX91X`` in
  ``DMA_SILABS_SIWX91X_UDMA`` and ``DMA_SILABS_SIWX91X_SG_BUFFER_COUNT`` in
  ``DMA_SILABS_SIWX91X_UDMA_DESCR_COUNT``)

* To align with the other drivers, ``GPDMA_SILABS_SIWX91X_DESCRIPTOR_COUNT`` has been renamed in
  ``DMA_SILABS_SIWX91X_GPDMA_DESCR_COUNT``.

ESPI
====

* ECUSTOM_HOST_SUBS_INTERRUPT_EN has been deprecated in favor of new API that allows fine-grained
  enable/disable control of individual eSPI hardware interrupts.
  This replaces the current all-or-nothing approach, which is tightly coupled to
  CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE and single eSPI ACPI HW block instance in all eSPI drivers.
  This will be completely removed in the next Zephyr release to give time for transition.

* The Microchip XEC eSPI v2 driver (:dtcompatible:`microchip,xec-espi-v2`) has been ported from
  MEC172x to also support MEC174x, MEC175x, and MEC165xB. This required several devicetree
  changes that affect out-of-tree boards using this binding (:github:`109519`):

  * The ``pcrs`` property has been replaced by ``pcr-scr``, which is now a single integer
    encoded with the ``MCHP_XEC_SCR_ENCODE(reg, bit)`` helper macro instead of a
    ``<reg bit>`` cell pair. Update existing overlays from ``pcrs = <2 19>;`` to
    ``pcr-scr = <MCHP_XEC_SCR_ENCODE(2, 19)>;``.

  * On MEC174x/5x/165xB only, ``girqs`` cells are now a single integer per entry produced
    by ``MCHP_XEC_ECIA_GIRQ_ENC(reg, bit)`` rather than a ``<reg bit>`` pair. MEC172x
    continues to use the existing ``MCHP_XEC_ECIA(...)`` form.

  * Two new optional properties are available on the eSPI controller node for hosts whose
    address space exceeds 32 bits: ``host-memmap-addr-high`` (host address bits [47:32] for
    memory-mapped logical devices) and ``sram-bar-addr-high`` (host address bits [47:32] for
    both SRAM BARs).

  * The ``interrupt-names`` of the eSPI controller and its host-device children have been
    renamed for consistency. Update overlays accordingly:

    * Controller: ``rst`` → ``erst``; ``vwct_0_6`` / ``vwct_7_10`` →
      ``ht_vw_bank0`` / ``ht_vw_bank1``. Two corresponding interrupts
      (``ht_vw_bank0`` / ``ht_vw_bank1``) must be added to the ``interrupts`` array.
    * KBC child: ``kbc_obe`` / ``kbc_ibf`` → ``obe`` / ``ibf``.
    * ACPI EC children: ``acpi_ibf`` / ``acpi_obe`` → ``ibf`` / ``obe``.

  * In the MEC5 SoC DTSI (MEC174x/5x/165xB), every host-device child of the eSPI
    controller (mailbox, KBC, ACPI EC, ACPI PM1, port92, glue, EMI, BIOS debug port, etc.)
    now declares the ``ldn`` (logical device number) property required by
    :dtcompatible:`microchip,xec-espi-host-dev`. Out-of-tree boards that override or add
    host-device child nodes for these SoCs must set ``ldn`` on each.

Ethernet
========

* ``ETHERNET_CONFIG_TYPE_T1S_PARAM`` and the related ``NET_REQUEST_ETHERNET_SET_T1S_PARAM`` has
  been removed. :c:func:`phy_set_plca_cfg` together with :c:func:`net_eth_get_phy` should be
  used instead to set these parameters (:github:`108136`).

* In the functions implemented by the :c:struct:`ethernet_api` a additional argument was added for
  a pointer to :c:struct:`net_if`. This api is not directly exposed to the application, so only
  out-of-tree drivers need to be updated. (:github:`106086`)

* The ``pinctrl-0`` and ``pinctrl-names`` devicetree properties for the
  :dtcompatible:`nxp,enet-mac` need to be moved from the MAC node to the parent Ethernet controller
  node. (:github:`107352`)

* ``port_generate_random_mac`` of the :c:struct:`dsa_api` got removed. Also
  :c:struct:`dsa_port_config` now uses :c:struct:`net_eth_mac_config` to set the MAC address.
  ``mac_addr`` and ``use_random_mac_addr`` members of :c:struct:`dsa_port_config` were removed.
  Out-of-tree DSA drivers must update their port configuration code to use the new API and
  structures. (:github:`108952`)

* The Kconfig option ``CONFIG_ETH_NATIVE_TAP_PTP_CLOCK`` has been replaced by
  :kconfig:option:`CONFIG_PTP_CLOCK_NATIVE`. A new compatible
  :dtcompatible:`zephyr,native-ptp-clock` has been added for the native_sim PTP clock driver.
  :kconfig:option:`CONFIG_PTP_CLOCK_NATIVE` is enabled by default when the
  :dtcompatible:`zephyr,native-ptp-clock` compatible is present.

* ``port_phylink_change`` of the :c:struct:`dsa_api` is now optional.
  The DSA driver no longer needs to call :c:func:`net_eth_carrier_on` or
  :c:func:`net_eth_carrier_off` on PHY link change, this is now handled by the DSA core.
  The ``void *user_data`` argument of ``port_phylink_change`` has been changed to
  ``const struct device *dev``, so it no longer needs to be cast to obtain the device pointer.
  Out-of-tree DSA drivers must update their ``port_phylink_change`` callback to match the new API and
  can remove any calls to :c:func:`net_eth_carrier_on` or :c:func:`net_eth_carrier_off` from it.
  (:github:`109671`)

* The MAC address of ethernet interfaces is now checked for validity, when bringing the interface up
  with :c:func:`net_if_up`. If the MAC address is invalid, the interface will fail to come up and an
  error will be logged. This check is done before the ``start`` function of the
  :c:struct:`ethernet_api` is called. This also applies to native wifi drivers. (:github:`110435`)

* The Xilinx GEM Ethernet driver (:dtcompatible:`xlnx,gem`) has been switched over to use the
  current MDIO and PHY facilities, splitting up the driver's implementation into separate
  MDIO and Ethernet MAC drivers. The driver's custom PHY management code has been removed.
  The types of Ethernet PHYs supported by the removed custom code, the Marvell Alaska GBit
  PHY family and the TI TLK105/DP83822 100 MBit PHYs are both covered by the standard
  (:dtcompatible:`ethernet-phy`) driver. The QEMU targets which emulate the Xilinx GEM have
  been updated accordingly, as have been the device trees of the Zynq-7000 and ZynqMP /
  UltraScale+ SoC families. (:github:`87313`)

Flash
=====
* :dtcompatible:`jedec,spi-nand` now requires a ``plane-bytes`` property, which indicates the size
  of each plane in the flash device. For devices with a single plane, this should be set to the
  same value as ``size-bytes``.

Fuel Gauge
==========

* Various fuel gauge property enums and union fields have been deprecated in
  favor of new versions with explicit unit suffixes. Applications and drivers
  should migrate to the unit-suffixed names. For example,
  ``FUEL_GAUGE_CURRENT`` (``val.current``) is replaced by
  ``FUEL_GAUGE_CURRENT_UA`` (``val.current_ua``).

* Drivers had inconsistently been reporting full charge/discharge cycles or
  "1/100ths" of a cycle in the ``FUEL_GAUGE_CYCLE_COUNT`` property.
  The property now consistently reports full cycles, and drivers that
  previously reported fractions of a cycle (i.e. ADP5360 and BQ27Z746) have
  been updated to report full cycles instead.
  Applications that relied on the old behavior should be updated.
  (:github:`112276`)

GPIO
====

* The STM32 GPIO driver now returns ``-EINVAL`` when attempting to configure a GPIO pin in disabled
  state with a pull-up/pull-down resistor using :c:func:`gpio_pin_configure`. The driver would
  previously return ``0`` without actually honoring those flags (no PU/PD resistor was enabled).
  Applications encountering this error should remove :c:macro:`GPIO_PULL_UP`/ :c:macro:`GPIO_PULL_DOWN`
  from the ``flags`` they provide to :c:func:`gpio_pin_configure`; this will result in the same
  behavior as before since these flags were effectively ignored. (:github:`104690`)

* On STM32F1 series, GPIO output pins now use 50 MHz max. speed instead of 10 MHz. (:github:`104690`)

Haptics
=======

* The ``cirrus,cs40l5x`` compatible has been replaced by variant-specific compatibles
  :dtcompatible:`cirrus,cs40l50`, :dtcompatible:`cirrus,cs40l51`, :dtcompatible:`cirrus,cs40l52`,
  and :dtcompatible:`cirrus,cs40l53`. Applications using the old compatible must update their
  devicetree nodes accordingly.

Input
=====

* The ``no-disconnect`` property of :dtcompatible:`gpio-keys` has been replaced by enumeration
  property ``zephyr,suspend-action``. The new property currently has three values, two of which
  are direct replacements for the old situations:

    * ``zephyr,suspend-action = "none";`` is the remplacement for the behavior when
      when ``no-disconnect`` was present. Users of the ``no-disconnect`` property
      should replace it with ``zephyr,suspend-action = "none";``.

    * ``zephyr,suspend-action = "disconnect-with-pupd";`` is the replacement for the
      behavior when ``no-disconnect`` was not present. This is selected by default
      for backwards compatibility with the previous behavior.

    * ``zephyr,suspend-action = "full-disconnect";`` is a new value.
      (Refer to :dtcompatible:`the binding <gpio-keys>` for more details)

  Users of the default configuration are advised to reconsider whether it really is appropriate;
  migration to ``zephyr,suspend-action = "full-disconnect";`` is recommended. (:github:`108294`)

* Kconfig options for the ft6146, ft5336, and cst8xx input drivers have been renamed to be
  consistent with the other input drivers. Applications using the following Kconfig options
  must update their configurations accordingly:

  * ``CONFIG_INPUT_FT5336_PERIOD`` → :kconfig:option:`CONFIG_INPUT_FT5336_PERIOD_MS`
  * ``CONFIG_INPUT_CST8XX_PERIOD`` → :kconfig:option:`CONFIG_INPUT_CST8XX_PERIOD_MS`
  * ``CONFIG_INPUT_FT6146_PERIOD`` → :kconfig:option:`CONFIG_INPUT_FT6146_PERIOD_MS`

  * Nunchuk driver wronlgy reported ``INPUT_KEY_Z``, respective ``INPUT_KEY_C`` in button events. This
    has been fixed and ``INPUT_BTN_Z``, respective ``INPUT_BTN_C`` is used now.

Interrupt Controllers
=====================

* All interrupt controller bindings now use ``flags`` as the interrupt cell name
  instead of ``sense``. The following interrupt controller bindings were updated:

  * :dtcompatible:`intel,ioapic`
  * :dtcompatible:`intel,loapic`
  * :dtcompatible:`cdns,xtensa-core-intc`
  * :dtcompatible:`intel,ace-intc`
  * :dtcompatible:`intel,cavs-intc`
  * :dtcompatible:`snps,designware-intc`
  * :dtcompatible:`mediatek,adsp_intc`

  Drivers using these interrupt controllers are updated to use ``flags`` as the cell name.
  However, any out-of-tree drivers that directly access interrupt properties using
  ``DT_INST_IRQ(n, sense)`` or ``DT_IRQ(node, sense)`` should be updated to use ``flags`` instead
  of ``sense``.

* Deprecate ``GIC_NUM_CPU_IF`` from GIC header file :file:`gic.h`. One shall use
  instead.:kconfig:option:`CONFIG_MP_MAX_NUM_CPUS` instead.

NXP
===

* :kconfig:option:`CONFIG_MCUX_LPTMR_TIMER` now defaults to ``y`` when the
  ``/chosen/zephyr,system-timer`` chosen node is enabled and compatible with
  :dtcompatible:`nxp,lptmr`. Boards that do not use LPTMR as the system timer
  must not select an LPTMR node in ``/chosen/zephyr,system-timer``.

* Kinetis KE1xF no longer requires a board overlay to designate the system
  timer when :kconfig:option:`CONFIG_PM` is enabled. The SoC DTSI now sets the
  ``zephyr,system-timer`` chosen property, so boards that added the overlay
  described in the Zephyr 4.4 migration guide can remove it.

PWM
===

* The ``pcrs`` property (array type) of :dtcompatible:`microchip,xec-pwm` has been replaced
  by ``pcr-scr`` (int type) to use encoded PCR register index and bit position macros
  (:github:`104570`).

* STM32 PWM DT bindings macro ``PWM_STM32_COMPLEMENTARY`` that is deprecated since
  Zephyr v3.3.0 is no more defined. One shall use ``STM32_PWM_COMPLEMENTARY`` instead.

RTC
===

* The legacy counter-based DS3231 driver has been removed, completing the deprecation introduced in
  :github:`95221`. Applications using :dtcompatible:`maxim,ds3231`, ``CONFIG_COUNTER_MAXIM_DS3231``,
  or :file:`<zephyr/drivers/rtc/maxim_ds3231.h>` must migrate to the RTC subsystem driver.

  Replace the single legacy I2C node with a :dtcompatible:`maxim,ds3231-mfd` parent and a
  :dtcompatible:`maxim,ds3231-rtc` child. Move ``isw-gpios`` to the RTC child, rename old
  ``32k-gpios`` usage to ``freq-32khz-gpios``, and replace ``maxim_ds3231_*`` helper API usage with
  generic RTC subsystem APIs.

* :dtcompatible:`microcrystal,rv3032` properties ``trickle-resistor-ohms`` and
  ``trickle-charger-mode`` have moved to the parent
  :dtcompatible:`microcrystal,rv3032-mfd` device. The parent MFD device now
  handles configuring the backup supply mode for all child devices.

SD Host Controller
==================

* Renamed the Kconfig option ``CONFIG_SDHC_STM32_POLLING_SUPPORT`` to
  :kconfig:option:`CONFIG_SDHC_STM32_DMA_MODE`. The new symbol enables DMA
  (default ``y``); set it to ``n`` to use polling mode. (:github:`101617`)

* Renamed the Kconfig option ``CONFIG_SDHC_STM32_SDIO`` to
  :kconfig:option:`CONFIG_SDHC_STM32_SDMMC`. (:github:`101617`)

* The devicetree compatible ``st,stm32-sdio`` was renamed. Use
  :dtcompatible:`st,stm32-sdmmc` instead. With this compatible, the legacy
  disk driver and the SDHC driver can target the same node. To migrate to the
  SDHC STM32 SDMMC driver, disable the legacy disk driver:

  .. code-block:: kconfig

     CONFIG_SDMMC_STM32=n

  (:github:`101617`)

* For :dtcompatible:`st,stm32-sdmmc`, the ``sdhi-on-gpios`` property has been
  consolidated into the existing ``pwr-gpios`` property. Replace
  ``sdhi-on-gpios`` with ``pwr-gpios`` in out-of-tree devicetree nodes.

* :dtcompatible:`litex,mmc` now uses the ``dma-coherent`` devicetree property to indicate that the
  controller's DMA accesses are coherent with the CPU.
  :kconfig:option:`CONFIG_SDHC_LITEX_LITESDCARD_NO_COHERENT_DMA` is automatically set based on that
  property and is no longer user-configurable. (:github:`108411`)

Sensor
======

* The ``girqs`` and ``pcrs`` properties (array type) of :dtcompatible:`microchip,xec-tach` have been
  replaced by ``pcr-scr`` (int type) to use encoded PCR register index and bit position macros.
  GIRQ configuration is now handled via the ``microchip,dmec-ecia-girq`` binding include
  (:github:`104808`).
* :dtcompatible:`st,lps22hh` now ignores the ``odr`` property in favor of the one-shot sampling mode
  unless :kconfig:option:`CONFIG_LPS22HH_TRIGGER` is enabled to make use of periodic sampling.

* The devicetree compatible ``tdk,ntcg163jf103ft1`` has been renamed to
  :dtcompatible:`tdk,ntcgxx3jx103x` to reflect that the compensation values are identical for TDK
  NTCG thermistor parts with the same resistance (R25) and beta (B25/85) values, as indicated in the
  part naming scheme (:github:`110123`).

Serial
======

* The return type of :c:func:`uart_irq_update` is now ``void`` instead of ``int``.
  (:github:`105231`)

SPI
===

* ``SPI_SILABS_SIWX91X_GSPI_DMA`` and ``SPI_SILABS_SIWX91X_GSPI_DMA_MAX_BLOCKS`` have been removed.
  They are replaced by ``SPI_SILABS_SIWX91X_GSPI_DMA_DESCR_COUNT`` which allow to enable DMA and
  configure the descriptor count.

* The ``fifo-enable`` property of :dtcompatible:`st,stm32h7-spi` has been removed.
  FIFO is now always used in polling and interrupt mode to enhance performance. A new property
  ``st,fifo-threshold`` can be used to configure the FIFO threshold (default = 1). (:github:`110265`)

Stepper
=======

* The ``activate-stallguard2``, ``stallguard-threshold-velocity`` and ``stallguard-velocity-check-interval-ms``
  properties of :dtcompatible:`adi,tmc50xx-stepper-ctrl` and :dtcompatible:`adi,tmc51xx-stepper-ctrl` have
  been removed. The stallguard configuration is now done at runtime using
  :c:func:`tmc50xx_stepper_ctrl_configure_stallguard`, :c:func:`tmc51xx_stepper_ctrl_configure_stallguard`
  and :c:struct:`tmc_stallguard_settings`. Out-of-tree drivers using these properties
  must be updated to remove them.
  (:github:`110062`)

STM32
=====

* :dtcompatible:`gpio-keys` devices will fail to suspend unless property ``zephyr,suspend-action``
  is present in Devicetree with value ``"none"`` or ``"full-disconnect"``. Refer to the migration
  guide entry related to this binding for more details. (:github:`104690` / :github:`108294`)

* SoC DTSI files now consistently use interrupt priority zero for all peripherals.
  Applications must now explicitly configure interrupt priorities using Devicetree
  if they previously relied on the values found in SoC DTSI files. (:github:`106188`)

* :dtcompatible:`st,stm32-sai` binding has been restructured to reflect the SAI hardware
  topology. The parent node now represents the SAI Block controller, while a new
  ``child-binding`` represents the SAI Sub-Block instances.
  The following properties shall be moved from the parent SAI node to a child sub-block node:
  ``dmas``, ``dma-names`` (now validated against ``enum: [tx, rx]``), ``pinctrl-0``,
  ``pinctrl-names``, ``mclk-enable``, ``mclk-divider``, ``synchronous``, and
  ``fifo-threshold``. (:github:`104423`)

* :dtcompatible:`st,hci-stm32wba` and :dtcompatible:`st,stm32wba-ieee802154` nodes
  (with nodelabels ``bt_hci_wba`` and ``ieee802154`` respectively) are now
  children of a top-level :dtcompatible:`st,stm32wba-radio` node with nodelabel
  ``radio``. The ``interrupts`` property is now set on the ``&radio`` node instead
  of being duplicated on both ``&bt_hci_wba`` and ``&ieee802154`` nodes. Out-of-tree
  boards which modified the ``interrupts`` property on either node must be updated
  to set the property on the top-level ``&radio`` node instead. (:github:`110546`)

* Renamed ST gpio-nexus for camera and display connectors as follow:
  ``st,dsi-lcd-qsh-030`` is renamed into :dtcompatible:`st,dsi-lcd-qsh-030-connector`
  ``st,stm32-dcmi-camera-fpu-330zh`` is renamed into :dtcompatible:`st,dvp-cam-zif-30-connector`

* :dtcompatible:`st,stm32-xspim` is now also used on STM32H5 and STM32H7RS series
  to declare and configure XSPI Manager. Boards making use of XSPI must now enable
  ``&xspim`` node in addition to the desired XSPI controller to use XSPI. (:github:`109903`)

* STM32MP13 SoC DTSI ethernet: rename labels from ``mac:`` and ``mdio:`` to ``mac0:`` and
  ``mdio0:``. The goal is to distinguish the 2 Ethernet controllers available. (:github:`108574`)

Syscon
======

* The syscon API functions :c:func:`syscon_read_reg` and :c:func:`syscon_write_reg` now use
  ``uint32_t`` for the register offset parameter instead of ``uint16_t``. This allows for
  larger register offsets. Code that explicitly declares ``uint16_t`` variables for the
  register parameter or implements the syscon driver API functions may need to be updated.

Timer
=====

* :c:func:`sys_clock_set_timeout`, :c:func:`sys_clock_announce` and
  :c:func:`sys_clock_announce_locked` now take their tick count as an unsigned
  ``uint32_t`` rather than a signed ``int32_t``. Out-of-tree system timer drivers must
  update their :c:func:`sys_clock_set_timeout` definition accordingly, otherwise the build
  fails with a conflicting-types error. The kernel now also caps the requested timeout at
  ``SYS_CLOCK_MAX_WAIT`` and no longer passes ``K_TICKS_FOREVER`` to the driver, so such
  drivers no longer need to clamp the request against the :c:func:`sys_clock_announce`
  range or special-case ``K_TICKS_FOREVER``; only their own hardware cycle-count limits
  still need enforcing (:github:`111022`).

USB
===

* On STM32N6, the ``clocks`` cell which configures the USBPHYC clock mux has been moved
  from :samp:`usbotg_hs{N}` to :samp:`usbphyc{N}` nodes at SoC DTSI level. Boards which
  use an STM32N6 SoC with custom clock mux configuration must now set the ``clocks``
  property on :samp:`usbphyc{N}` instead of :samp:`usbotg_hs{N}`. (:github:`107813`)
* When host issues control transfer with data stage from host to device, the USB control transfer
  callbacks ``control_to_dev`` in :c:struct:`usbd_class_api` and ``to_dev`` in
  :c:struct:`usbd_vreq_node` are now called with NULL ``buf`` before data stage is received.
  This allows the stack to return STALL during data stage. Out-of-tree class and vendor handlers
  need to be updated. (:github:`108840`)

Video
=====

* The :dtcompatible:`ovti,ov7670` and :dtcompatible:`ovti,ov7675` camera drivers now assume a
  24 MHz XCLK input instead of the previous 6 MHz, matching the typical XCLK frequency listed in
  the OV7670 datasheet. Boards driving an OV7670 or OV7675 sensor must update their board-level
  XCLK clock configuration accordingly. For example, ``frdm_mcxn236`` has been switched from
  ``kFRO12M_to_CLKOUT`` (divided by 2 to yield 6 MHz) to ``kFRO_HF_to_CLKOUT`` (divided by 2 to
  yield 24 MHz), and ``frdm_mcxn947`` keeps ``kMAIN_CLK_to_CLKOUT`` but changes the CLKOUT
  divider from 25 to 6 to yield 24 MHz. (:github:`109393`)

WiFi
====

* In the functions implemented by the :c:struct:`net_wifi_mgmt_offload`, internally
  :c:struct:`ethernet_api` and :c:struct:`wifi_mgmt_ops`, a additional argument was added for
  a pointer to :c:struct:`net_if`. This api is not directly exposed to the application, so only
  out-of-tree drivers need to be updated. (:github:`106086`)

* The Espressif Wi-Fi driver Kconfig option ``CONFIG_ESP32_WIFI_STA_AUTO_DHCPV4`` has been
  removed in favor of the generic :kconfig:option:`CONFIG_WIFI_STA_AUTO_DHCPV4`. Applications
  that previously disabled the Espressif-specific option must now disable the generic option
  to retain manual DHCPv4 or static IP behavior after STA connection.

.. zephyr-keep-sorted-stop

Bluetooth
*********

Bluetooth Audio
===============

.. zephyr-keep-sorted-start re(^\* \w)

* BAP

  * :c:member:`bt_bap_stream.codec_cfg` is now ``const``, to better reflect that it is a read-only
    value. Any non-read uses of it will need to be updated with the appropriate operations such as
    :c:func:`bt_bap_stream_config`, :c:func:`bt_bap_stream_reconfig`, :c:func:`bt_bap_stream_enable`
    or :c:func:`bt_bap_stream_metadata`. (:github:`104219`)
  * :c:member:`bt_bap_stream.qos` is now ``const``, to better reflect that it is a read-only
    value. Any non-read uses of it will need to be set with the appropriate operations such as
    :c:func:`bt_bap_unicast_group_create`, :c:func:`bt_bap_unicast_group_reconfig`,
    :c:func:`bt_bap_broadcast_source_create` or :c:func:`bt_bap_broadcast_source_reconfig`.
    (:github:`104887`)
  * Almost all API uses of ``struct bt_bap_qos_cfg *`` are now ``const``, which means that once the
    ``qos`` has been stored in a parameter struct like
    :c:struct:`bt_bap_broadcast_source_param` or
    :c:struct:`bt_bap_unicast_group_stream_param`, then the parameter's pointer cannot be used
    to modify the ``qos``, and the actual definition of the struct should be modified instead.
    (:github:`104219`)
  * :c:member:`bt_bap_unicast_group_info.sink_pd` and :c:member:`bt_bap_unicast_group_info.source_pd`
    now reflect the local values defined for the group, and not the values configured for any remote
    ASEs. (:github:`104887`)
  * :c:func:`bt_bap_unicast_client_discover` and :c:func:`bt_bap_broadcast_assistant_discover` now
    require that the connection has already gone through the pairing process and meets the security
    requirements of BAP before doing any discovery. In most cases this requires a call to
    :c:func:`bt_conn_set_security` for new devices. Bonded devices that reconnect should not require
    anything.
  * Almost all API uses of ``struct bt_audio_codec_cfg *`` are now ``const``, which means that once
    the ``codec_cfg`` has been stored in a parameter struct like
    :c:struct:`bt_bap_stream` or
    :c:struct:`bt_bap_broadcast_source_subgroup_param`, then the parameter's pointer cannot be used
    to modify the ``codec_cfg``, and the actual definition of the struct should be modified instead.
    (:github:`104219`)
  * All BAP roles now require :kconfig:option:`CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE` to be at least
    19 octets, as mandated by the BAP spec.
    If :kconfig:option:`CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE` is set to a lower value when used
    with BAP, then applications need to set it to at least 19 octets. The following Kconfig options
    are affected:

    * :kconfig:option:`CONFIG_BT_ASCS`
    * :kconfig:option:`CONFIG_BT_BAP_UNICAST_SERVER`
    * :kconfig:option:`CONFIG_BT_BAP_UNICAST_CLIENT`
    * :kconfig:option:`CONFIG_BT_BAP_BROADCAST_SOURCE`
    * :kconfig:option:`CONFIG_BT_BAP_BROADCAST_SINK`
    * :kconfig:option:`CONFIG_BT_BAP_SCAN_DELEGATOR`
    * :kconfig:option:`CONFIG_BT_BAP_BROADCAST_ASSISTANT`

    (:github:`107989`)
  * :zephyr:code-sample:`bluetooth_bap_broadcast_assistant`,
    :zephyr:code-sample:`bluetooth_bap_broadcast_sink`,
    :zephyr:code-sample:`bluetooth_bap_broadcast_source`,
    :zephyr:code-sample:`bluetooth_bap_unicast_client` and
    :zephyr:code-sample:`bluetooth_bap_unicast_server` have been moved from
    :zephyr_file:`samples/bluetooth/` to :zephyr_file:`samples/bluetooth/audio`.

* CAP

  * :c:func:`bt_cap_commander_broadcast_reception_start` now waits for the CAP acceptors to sync to
    the broadcast before completing. This means that if the broadcast source is offline,
    including colocated broadcast sources like the ones created by
    :c:func:`bt_cap_handover_unicast_to_broadcast`, shall be active and have the periodic advertising
    enabled with a configured BASE. For :c:func:`bt_cap_handover_unicast_to_broadcast` the newly
    added :c:member:`bt_cap_handover_cb.unicast_to_broadcast_created` can be used to configure the
    BASE. This also means that any current checks implemented by an application to wait for receive
    state updates indicating successful sync can be removed,
    as :c:func:`bt_cap_commander_broadcast_reception_start` now ensures this when
    :c:member:`bt_cap_commander_cb.broadcast_reception_start` is called. This also applies for
    :c:func:`bt_cap_commander_broadcast_reception_stop` in a similar manner. (:github:`101070`)
  * :zephyr:code-sample:`bluetooth_cap_acceptor`,
    :zephyr:code-sample:`bluetooth_cap_handover` and
    :zephyr:code-sample:`bluetooth_cap_initiator` have been moved from
    ``samples/bluetooth/`` to ``samples/bluetooth/audio``.

* CCP

  * :c:member:`bt_tbs_client_cb.technology` has changed the ``value`` parameter from ``uint32_t``
    to ``enum bt_bearer_tech``. Applications using this application should switch the type.
    (:github:`102430`)
  * All ``BT_TBS_TECHNOLOGY_*`` values like ``BT_TBS_TECHNOLOGY_3G`` are renamed to
    ``BT_BEARER_TECH_*`` like ``BT_BEARER_TECH_3G``. Applications can do search-and-replace from
    ``BT_TBS_TECHNOLOGY`` to ``BT_BEARER_TECH``. Additionally the values are now defined in
    :zephyr_file:`include/zephyr/bluetooth/assigned_numbers.h` instead of
    :zephyr_file:`include/zephyr/bluetooth/audio/tbs.h`. (:github:`102430`)
  * ``bt_tbs_register_param.supported_features`` has been renamed to
    :c:member:`bt_tbs_register_param.optional_opcodes`. Applications can do a simple
    search-and-replace for ``supported_features`` to ``optional_opcodes``.
    Additionally the ``BT_TBS_FEATURE_*`` macros have been changed to ``BT_TBS_OPTIONAL_OPCODE_*``.
    Applications can do a simple search-and-replace for ``BT_TBS_FEATURE_`` to
    ``BT_TBS_OPTIONAL_OPCODE_``. (:github:`103350`)
  * :zephyr:code-sample:`bluetooth_ccp_call_control_client` and
    :zephyr:code-sample:`bluetooth_ccp_call_control_server` have been moved from
    ``samples/bluetooth/`` to ``samples/bluetooth/audio``.

* CSIP

  * Optional CSIS characteristics have been made configurable via Kconfig and must be enabled
    explicitly:

    * Coordinated Set Size → :kconfig:option:`CONFIG_BT_CSIP_SET_MEMBER_SIZE_SUPPORT`
    * Set Member Lock → :kconfig:option:`CONFIG_BT_CSIP_SET_MEMBER_LOCK_SUPPORT`
    * Set Member Rank → :kconfig:option:`CONFIG_BT_CSIP_SET_MEMBER_RANK_SUPPORT`

* HAP

  * :zephyr:code-sample:`bluetooth_hap_ha` has been moved from
    ``samples/bluetooth/`` to ``samples/bluetooth/audio``.

* PBP

  * :zephyr:code-sample:`bluetooth_public_broadcast_sink` and
    :zephyr:code-sample:`bluetooth_public_broadcast_source` have been moved from
    ``samples/bluetooth/`` to ``samples/bluetooth/audio``.

* TMAP

  * :zephyr:code-sample:`ble_peripheral_tmap_bmr`,
    :zephyr:code-sample:`ble_peripheral_tmap_bms`,
    :zephyr:code-sample:`ble_peripheral_tmap_central` and
    :zephyr:code-sample:`ble_peripheral_tmap_peripheral` have been moved from
    ``samples/bluetooth/`` to ``samples/bluetooth/audio``.

.. zephyr-keep-sorted-stop

Bluetooth Classic
=================

* The BR/EDR specific callbacks ``role_changed`` and ``br_mode_changed`` in
  :c:struct:`bt_conn_cb` have been moved into a new sub-struct
  :c:struct:`bt_conn_br_cb`, accessible via the ``br`` member. Application code
  using these callbacks must update the designated initializers:

  * ``.role_changed`` → ``.br.role_changed``
  * ``.br_mode_changed`` → ``.br.mode_changed``

  (:github:`108022`)

* Renamed ``BT_DEVICE_VEDNOR_ID`` to :kconfig:option:`BT_DEVICE_VENDOR_ID` to fix a typo.

Bluetooth HCI
=============

* The devicetree compatible ``bflb,bl70x-bt-hci`` has been renamed to
  :dtcompatible:`bflb,bt-hci`, now that a single binding covers all Bouffalo Lab
  on-chip BLE controllers (BL60x/BL70x/BL70XL). Out-of-tree boards and shields
  must update their devicetree nodes accordingly.

* Bluetooth HCI drivers now have to provide a mandatory common struct as the first field of
  their data (:c:struct:`bt_hci_driver_data`) and config (:c:struct:`bt_hci_driver_config`)
  structs.

* The HCI driver :c:member:`bt_hci_driver_api.open` callback no longer has a ``recv`` parameter;
  rather the common HCI driver layer code takes care of managing this as part of the common
  data struct. There is a new :c:func:`bt_hci_recv` API for drivers to pass data the higher
  layer (e.g. the Bluetooth Host stack). For drivers that need access to any error from recv()
  (most don't) there's also a new :c:func:`bt_hci_recv_err` API that leaves the responsibility
  of unrefing the buffer to the caller in case of error situations.

Bluetooth Services
==================

* :kconfig:option:`CONFIG_BT_OTS_MAX_OBJ_CNT` has been changed from ``hex`` to ``int`` for a
  more intuitive type.
  Simply modify any hex values like ``0x30`` to their decimal values like ``48``.

Networking
**********

* Various IP routing related Kconfig options will have now ``IPV6`` prefix added to
  them. This is done so that we can have IPv4 routing symbols that provide same
  functionality as IPv6 ones but can be controlled separately.

* IPv4 and IPv6 unicast route-table support is now exposed through the
  :kconfig:option:`CONFIG_NET_IPV4_ROUTE` and
  :kconfig:option:`CONFIG_NET_IPV6_ROUTE` options.

  These options control the per-family unicast route tables that are used by
  static route management, networking shell route commands, and host-side route
  selection for locally originated traffic such as VPN-bound packets. They do
  not, by themselves, enable packet forwarding between interfaces.

* The Kconfig options :kconfig:option:`CONFIG_NET_IPV4_ROUTING` and
  :kconfig:option:`CONFIG_NET_IPV6_ROUTING` have been renamed to
  :kconfig:option:`CONFIG_NET_IPV4_FORWARDING` and
  :kconfig:option:`CONFIG_NET_IPV6_FORWARDING`.

  The renamed options explicitly describe IP forwarding between interfaces.
  Applications that only need route-table lookups or static routes should
  enable :kconfig:option:`CONFIG_NET_IPV4_ROUTE` or
  :kconfig:option:`CONFIG_NET_IPV6_ROUTE` and leave forwarding disabled.
  Applications acting as routers should enable both the route-table option and
  the corresponding forwarding option.

* Out-of-tree IPv6 configurations should also migrate away from the deprecated
  legacy aliases :kconfig:option:`CONFIG_NET_ROUTE`,
  :kconfig:option:`CONFIG_NET_ROUTING`,
  :kconfig:option:`CONFIG_NET_MAX_ROUTES`, and
  :kconfig:option:`CONFIG_NET_MAX_NEXTHOPS` and use the
  :kconfig:option:`CONFIG_NET_IPV6_*` symbols directly.

* The ``samples/net/wifi/test_certs/rsa2k`` enterprise test certificates have
  been removed. TF-PSA-Crypto cannot decrypt their DES-encrypted PKCS#8 private
  keys. Use ``samples/net/wifi/test_certs/rsa2k_no_des`` instead, or set
  :envvar:`WIFI_TEST_CERTS_DIR` to another AES-encrypted certificate directory.

* ``net_if_config_get`` was removed as it was a duplicate of :c:func:`net_if_get_config`.
  (:github:`110930`)

* The number of ZVFS eventfd's is now determined by a ``ZVFS_EVENTFD_SIZE`` define
  instead of using the :kconfig:option:`CONFIG_ZVFS_EVENTFD_MAX` Kconfig option directly.
  Subsystems can specify their own eventfd count requirements by specifying Kconfig
  options with the prefix ``CONFIG_ZVFS_EVENTFD_ADD_SIZE_``. These are summed together
  and the result is compared against :kconfig:option:`CONFIG_ZVFS_EVENTFD_MAX`; the larger
  of the two values is used. To force :kconfig:option:`CONFIG_ZVFS_EVENTFD_MAX` to be used,
  even when its value is less than the sum of the custom requirements, a new
  :kconfig:option:`CONFIG_ZVFS_EVENTFD_IGNORE_MIN` option has been introduced (which
  defaults to being disabled). As a result, networking subsystems that allocate eventfds
  (e.g. HTTP server, CoAP server, LwM2M, PTP, SSH, the socket service and the WPA
  supplicant) no longer require the application to manually bump
  :kconfig:option:`CONFIG_ZVFS_EVENTFD_MAX` to account for them. (:github:`111201`)

* :kconfig:option:`CONFIG_NET_L2_PTP` has been deprecated and replaced by
  :kconfig:option:`CONFIG_NET_L2_PTP_TIMESTAMPING`. The new option more accurately describes the
  feature it enables. Applications or board configurations that explicitly enable
  :kconfig:option:`CONFIG_NET_L2_PTP` should be updated to use
  :kconfig:option:`CONFIG_NET_L2_PTP_TIMESTAMPING` instead.

* The default WPA supplicant network selection criterion has changed from
  throughput-based to reliability-based (SNR), switching the
  :kconfig:option:`WIFI_NM_WPA_SUPPLICANT_NW_SEL` Kconfig default from
  :kconfig:option:`CONFIG_WIFI_NM_WPA_SUPPLICANT_NW_SEL_THROUGHPUT` to
  :kconfig:option:`CONFIG_WIFI_NM_WPA_SUPPLICANT_NW_SEL_RELIABILITY`.
  Previously, SNR above 25 dBm was considered sufficient and largely excluded
  from AP selection; SNR is now always factored in, improving connection stability
  for embedded Wi-Fi use cases. Users who need the previous behaviour can restore it by enabling
  :kconfig:option:`CONFIG_WIFI_NM_WPA_SUPPLICANT_NW_SEL_THROUGHPUT`.

Ethernet
========

* :kconfig:option:`CONFIG_NET_DEFAULT_IF_ETHERNET` now allows to get the first ethernet interface,
  instead of the first between ethernet and wifi.

* The :kconfig:option:`CONFIG_ETH_QEMU_EXTRA_ARGS` and
  :kconfig:option:`CONFIG_NET_QEMU_USER_EXTRA_ARGS` options can no longer be used to specify the MAC
  address for the QEMU Ethernet device. Instead, :kconfig:option:`CONFIG_NET_QEMU_DEVICE_EXTRA_ARGS`
  can be used. This is because we are no longer using the ``-nic`` option for QEMU, but the
  ``-netdev`` and ``-device`` options. (:github:`107326`)

PTP
===

* The PTP UDP protocol Kconfig symbols have been renamed for consistent capitalization:

  * :kconfig:option:`CONFIG_PTP_UDP_IPv4_PROTOCOL` to
    :kconfig:option:`CONFIG_PTP_UDP_IPV4_PROTOCOL`
  * :kconfig:option:`CONFIG_PTP_UDP_IPv6_PROTOCOL` to
    :kconfig:option:`CONFIG_PTP_UDP_IPV6_PROTOCOL`

gPTP
====

* Converted ``int port`` to ``uint16_t gptp_port`` in
  :c:struct:`ethernet_context` to make it clear that the field used only
  by the gPTP stack to store the gPTP port number.

* Used ``uint16_t`` for ``nb_ports`` in :c:struct:`gptp_default_ds` per
  IEEE 1588 standard.

* Removed ``net_eth_get_ptp_port`` and ``net_eth_set_ptp_port``.
  New :c:func:`gptp_get_port_number` and :c:func:`gptp_set_port_number`
  can be used instead.

Modem
*****

SIMCOM SIM7080
==============

* The Kconfig option :kconfig:option:`CONFIG_MODEM_SIMCOM_SIM7080_LTE_BANDS` has been split
  into :kconfig:option:`CONFIG_MODEM_SIMCOM_SIM7080_LTE_BANDS_M1` and
  :kconfig:option:`CONFIG_MODEM_SIMCOM_SIM7080_LTE_BANDS_NB1` since NB-IoT and CAT-M have
  slightly different usable bands. The type of the newly introduced configuration values
  is a hex bitmap of selected bands. By default bands 8, 20 and 28 are selected.

  Applications configuring the :kconfig:option:`CONFIG_MODEM_SIMCOM_SIM7080_LTE_BANDS`
  must update their configuration.

LoRaWAN
*******

* The native LoRaWAN backend
  (:kconfig:option:`CONFIG_LORA_MODULE_BACKEND_NATIVE`) now requires
  :c:func:`lorawan_start` before the following runtime configuration APIs are
  accepted:

  * :c:func:`lorawan_set_datarate` and :c:func:`lorawan_set_conf_msg_tries`
    return ``-EPERM`` if called before start.
  * :c:func:`lorawan_enable_adr` (which has a ``void`` return type) logs a
    warning and drops the call if invoked before start.

  Configuration values previously latched by these APIs before
  :c:func:`lorawan_start` are no longer retained.  Applications that called
  them during initialization must move the calls to after start. They may still
  run before or after a successful :c:func:`lorawan_join`.

  :c:func:`lorawan_set_channels_mask` is unaffected and remains callable any
  time after :c:func:`lorawan_start`, since the channel mask influences the
  Join-Request channel selection itself.

  These ordering requirements do not apply to the LoRaMac-node backend
  (:kconfig:option:`CONFIG_LORA_MODULE_BACKEND_LORAMAC_NODE`).

Other subsystems
****************

* Demand paging (``subsys/demand_paging``) is moved under Memory Management
  into ``subsys/mem_mgmt/demand_paging``. Custom backing store and eviction algorithm code need
  to be moved there.

* The ring buffer "item" API in ``<zephyr/sys/ring_buffer.h>`` has been deprecated in favor of the new
  fixed-size queue API in ``<zephyr/sys/ringq.h>``.

  Code storing fixed-size items should migrate to :c:struct:`sys_ringq` (see
  :ref:`fixed_size_ringq_api`). Code that only used the item API at the byte level should switch to
  the byte-mode functions :c:func:`ring_buf_put` / :c:func:`ring_buf_get` calls on the same
  :c:struct:`ring_buf`. (:github:`98255`)

* The :c:func:`ZTEST_BENCHMARK_SETUP_TEARDOWN` and :c:func:`ZTEST_BENCHMARK_TIMED_SETUP_TEARDOWN` macros have
  been removed. Their setup/teardown signature has been folded into :c:func:`ZTEST_BENCHMARK` and
  :c:func:`ZTEST_BENCHMARK_TIMED`, which now require explicit ``setup_fn`` and ``teardown_fn``
  arguments at every call site. Pass ``NULL`` when a benchmark genuinely needs neither.

  Update existing call sites as follows:

  .. code-block:: c

     /* Before */
     ZTEST_BENCHMARK(suite, my_bench, 100) { /* ... */ }
     ZTEST_BENCHMARK_TIMED(suite, my_bench, 1000) { /* ... */ }
     ZTEST_BENCHMARK_SETUP_TEARDOWN(suite, my_bench, 100, setup, teardown) { /* ... */ }
     ZTEST_BENCHMARK_TIMED_SETUP_TEARDOWN(suite, my_bench, 1000, setup, teardown) { /* ... */ }

     /* After */
     ZTEST_BENCHMARK(suite, my_bench, 100, NULL, NULL) { /* ... */ }
     ZTEST_BENCHMARK_TIMED(suite, my_bench, 1000, NULL, NULL) { /* ... */ }
     ZTEST_BENCHMARK(suite, my_bench, 100, setup, teardown) { /* ... */ }
     ZTEST_BENCHMARK_TIMED(suite, my_bench, 1000, setup, teardown) { /* ... */ }

Random
======

* ``CONFIG_CTR_DRBG_CSPRNG_GENERATOR`` has been removed.
  Use :kconfig:option:`CONFIG_PSA_CSPRNG_GENERATOR` instead.

* ``CONFIG_CS_CTR_DRBG_PERSONALIZATION`` has been removed. It did not have any effect.

Tools
*****

* The ``openocd`` runner now selects a debug adapter by serial number through the
  canonical ``-i``/``--dev-id`` option, like the other runners. The previous
  ``--serial`` option is deprecated and kept as an alias; it maps onto the same
  mechanism (the value is still passed to the OpenOCD config as
  ``_ZEPHYR_BOARD_SERIAL``). Update any scripts to use ``west flash -i <serial>``.

Modules
*******

* Support for the `CANopenNode <https://github.com/CANopenNode/CANopenNode>`_ protocol stack was
  moved to an :ref:`external module<external_module_canopennode>`.

hal_nxp
=======

* S32K344: The pinmux header file for this SoC was renamed from ``S32K344-172MQFP-pinctrl.h`` to
  ``S32K344_K324_K314_172HDQFP-pinctrl.h``. Out-of-tree boards must update their include directive accordingly::

    #include <nxp/s32/S32K344_K324_K314_172HDQFP-pinctrl.h>

Mbed TLS
========

* :kconfig:option:`CONFIG_MBEDTLS_SSL_EARLY_DATA` is now an explicit opt-in and is no longer
  implicitly enabled by :kconfig:option:`CONFIG_MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK_ENABLED`.
  Out-of-tree applications or board configurations that rely on TLS 1.3 PSK early data (0-RTT)
  must now explicitly enable :kconfig:option:`CONFIG_MBEDTLS_SSL_EARLY_DATA`.

* ``CONFIG_PSA_CRYPTO_CLIENT`` has been removed as it was a duplicate of
  :kconfig:option:`CONFIG_PSA_CRYPTO`. If you were using it, use
  :kconfig:option:`CONFIG_PSA_CRYPTO` instead. (:github:`108960`)

* Interface CMake library ``mbedTLS`` has been renamed to ``mbedtls_iface``. The former is kept
  as an alias to the latter for backward compatibility, but it will be removed in future
  releases.

Snippets
********

* Rename ``xen_dom0`` to ``xen-dom0``.

Architectures
*************

* A new architecture primitive, ``arch_cpu_irqs_are_enabled()``, has been added.
  It returns the current interrupt-enable state of the calling CPU without
  modifying it, complementing ``arch_irq_unlocked()`` which inspects a saved
  key.  Out-of-tree architecture ports must provide an implementation.

* ``CONFIG_XTENSA_MPU_ONLY_SOC_RANGES`` is removed. For SoC or board to override the default
  MPU region table, override :c:var:`xtensa_mpu_ranges` in the SoC or board layer instead.

* ``xtensa_soc_mpu_ranges[]`` and ``xtensa_soc_mpu_ranges_num`` are removed. If SoC or board
  needs its own memory regions at boot, override :c:var:`xtensa_mpu_ranges` instead.

* ``CONFIG_XTENSA_MPU_DEFAULT_MEM_TYPE`` is removed since memory types are now defined via
  ``xtensa_mpu_mem_type_ranges[]``.

* ``CONFIG_XTENSA_BACKTRACE_EXCEPTION_DUMP_HOOK`` is removed, since backtrace is now always
  using :c:macro:`EXCEPTION_DUMP` for output.
