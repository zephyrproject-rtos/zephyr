:orphan:

.. _zephyr_2.2:

Zephyr 2.2.0 (Working Draft)
############################

We are pleased to announce the release of Zephyr kernel version 2.2.0.

Major enhancements with this release include:

* CANopen protocol support through 3rd party CANopenNode stack

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************

No security vulnerabilities received.

API Changes
***********

Deprecated in this release
==========================

* Settings

  * SETTINGS_USE_BASE64, encoding values in base64 is marked for removal.

Stable API changes in this release
==================================

* GPIO

  * The gpio_pin_write() and gpio_pin_read() functions have been
    deprecated, and should be replaced with gpio_pin_set_raw() and
    gpio_pin_get_raw(), or if the active level flags were present and
    supported by gpio_pin_set() and gpio_pin_get().  The functions have
    been re-implemented with these substitutes.
  * The gpio_pin_enable_callback() function has been deprecated, and
    should be replaced by gpio_pin_interrupt_configure(), passing the
    appropriate interrupt configuration.
  * The gpio_pin_disable_callback() function has been deprecated, and
    should be replaced by gpio_pin_interrupt_configure() passed
    GPIO_INT_DISABLE.
  * Many GPIO configuration flags have been deprecated and replaced by
    more carefully selected flags.  These include:
    * GPIO_DIR_OUT becomes GPIO_OUTPUT
    * GPIO_DIR_IN becomes GPIO_INPUT
    * GPIO_DS_DISCONNECT_LOW becomes GPIO_OPEN_SOURCE
    * GPIO_DS_DISCONNECT_HIGH becomes GPIO_OPEN_DRAIN
    * GPIO_PUD_NORMAL becomes 0
    * GPIO_PUD_PULL_UP becomes GPIO_PULL_UP
    * GPIO_PUD_PULL_DOWN becomes GPIO_PULL_DOWN
    * GPIO_INT becomes GPIO_INT_ENABLE
    * GPIO_INT_LEVEL becomes not GPIO_INT_EDGE
    * GPIO_INT_ACTIVE_LOW becomes GPIO_ACTIVE_LOW or GPIO_INT_LOW_0
    * GPIO_INT_ACTIVE_HIGH becomes GPIO_ACTIVE_HIGH or GPIO_INT_HIGH_1
    * GPIO_INT_DOUBLE_EDGE becomes GPIO_INT_EDGE_BOTH
    * GPIO_POL_NORMAL becomes GPIO_ACTIVE_HIGH
    * GPIO_POL_INV becomes GPIO_ACTIVE_LOW

* PWM

  * The pwm_pin_set_cycles(), pwm_pin_set_usec(), and
    pwm_pin_set_nsec() functions now take a flags parameter. The newly
    introduced flags are PWM_POLARITY_NORMAL and PWM_POLARITY_INVERTED
    for specifying the polarity of the PWM signal. The flags parameter
    can be set to 0 if no flags are required (the default is
    PWM_POLARITY_NORMAL).
  * Similarly, the pwm_pin_set_t PWM driver API function function now
    takes a flags parameter. The PWM controller driver must check the
    value of the flags parameter and return -ENOTSUP if any
    unsupported flag is set.

* USB

  * The usb_enable() function, which was previously invoked automatically
    by the USB stack, now needs to be explicitly called by the application
    in order to enable the USB subsystem.
  * The usb_enable() function now takes a parameter, usb_dc_status_callback
    which can be set by the application to a callback to receive status events
    from the USB stack. The parameter can also be set to NULL if no callback is required.

* nRF flash driver

  * The nRF Flash driver has changed its default write block size to 32-bit
    aligned. Previous emulation of 8-bit write block size can be selected using
    the CONFIG_SOC_FLASH_NRF_EMULATE_ONE_BYTE_WRITE_ACCESS Kconfig option.
    Usage of 8-bit write block size emulation is only recommended for
    compatibility with older storage contents.

* Clock control

  * The callback prototype (clock_control_cb_t) has now additional argument
    (clock_control_subsys_t) indicating which clock subsystem got started.

Removed APIs in this release
============================

* Shell

  * SHELL_CREATE_STATIC_SUBCMD_SET (deprecated), replaced by
    SHELL_STATIC_SUBCMD_SET_CREATE
  * SHELL_CREATE_DYNAMIC_CMD (deprecated), replaced by SHELL_DYNAMIC_CMD_CREATE

* Newtron Flash File System (NFFS) was removed. NFFS was removed since it has
    serious issues, not fixed since a long time. Where it was possible
    NFFS usage was replaced by LittleFS usage as the better substitute.

Kernel
******

* <TBD>

Architectures
*************

* ARC:

  * <TBD>

* ARM:

  * Removed support for CC2650

* POSIX:

  * <TBD>

* RISC-V:

  * <TBD>

* x86:

  * <TBD>

Boards & SoC Support
********************

* Added support for these SoC series:

.. rst-class:: rst-columns

   * <TBD>

* Added support for these ARM boards:

  .. rst-class:: rst-columns

     * <TBD>

* Added support for these following shields:

  .. rst-class:: rst-columns

     * <TBD>

Drivers and Sensors
*******************

* ADC

  * <TBD>

* Bluetooth

  * <TBD>

* CAN

  * <TBD>

* Clock Control

  * <TBD>

* Console

  * <TBD>

* Counter

  * The counter_read() API function is deprecated in favor of
    counter_get_value(). The new API function adds a return value for
    indicating whether the counter was read successfully.

* Display

  * <TBD>

* DMA

  * <TBD>

* EEPROM

  * <TBD>

* Entropy

  * <TBD>

* Ethernet

  * <TBD>

* Flash

  * <TBD>

* GPIO

  * <TBD>

* Hardware Info

  * <TBD>

* I2C

  * <TBD>

* I2S

  * <TBD>

* IEEE 802.15.4

  * <TBD>

* Interrupt Controller

  * <TBD>

* IPM

  * <TBD>

* Keyboard Scan

  * <TBD>

* LED

  * <TBD>

* Modem

  * <TBD>

* Pinmux

  * <TBD>

* PS/2

  * <TBD>

* PWM

  * <TBD>

* Sensor

  * <TBD>

* Serial

  * <TBD>

* SPI

  * <TBD>

* Timer

  * <TBD>

* USB

  * <TBD>

* Video

  * <TBD>

* Watchdog

  * <TBD>

* WiFi

  * <TBD>

Networking
**********

* <TBD>

Bluetooth
*********

* Host:

  * <TBD>

* BLE split software Controller:

  * <TBD>

* BLE legacy software Controller:

  * <TBD>

Build and Infrastructure
************************

* The minimum Python version supported by Zephyr's build system and tools is
  now 3.6.
* Renamed :file:`generated_dts_board.h` and :file:`generated_dts_board.conf` to
  :file:`devicetree.h` and :file:`devicetree.conf`, along with various related
  identifiers. Including :file:`generated_dts_board.h` now generates a warning
  saying to include :file:`devicetree.h` instead.
* <Other items TBD>

Libraries / Subsystems
***********************

* Random

  * <TBD>

HALs
****

* HALs are now moved out of the main tree as external modules and reside in
  their own standalone repositories.

Documentation
*************

* <TBD>

Tests and Samples
*****************

* <TBD>

Issue Related Items
*******************

These GitHub issues were addressed since the previous 2.1.0 tagged
release:

.. comment  List derived from GitHub Issue query: ...
   * :github:`issuenumber` - issue title
