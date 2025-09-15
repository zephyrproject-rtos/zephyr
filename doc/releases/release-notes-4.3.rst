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

.. _zephyr_4.3:

Zephyr 4.3.0 (Working Draft)
############################

We are pleased to announce the release of Zephyr version 4.3.0.

Major enhancements with this release include:

An overview of the changes required or recommended when migrating your application from Zephyr
v4.2.0 to Zephyr v4.3.0 can be found in the separate :ref:`migration guide<migration_4.3>`.

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************
The following CVEs are addressed by this release:

More detailed information can be found in:
https://docs.zephyrproject.org/latest/security/vulnerabilities.html

API Changes
***********

* RTIO

  * :c:func:`rtio_is_spi`
  * :c:func:`rtio_is_cspi`
  * :c:func:`rtio_is_i3c`
  * :c:func:`rtio_read_regs_async`

Removed APIs and options
========================

* The TinyCrypt library was removed as the upstream version is no longer maintained.
  PSA Crypto API is now the recommended cryptographic library for Zephyr.

Deprecated APIs and options
===========================

New APIs and options
====================

..
  Link to new APIs here, in a group if you think it's necessary, no need to get
  fancy just list the link, that should contain the documentation. If you feel
  like you need to add more details, add them in the API documentation code
  instead.

.. zephyr-keep-sorted-start re(^\* \w)

* Architectures

  * :kconfig:option:`CONFIG_SRAM_SW_ISR_TABLE`

* Bluetooth

  * Audio

    * :c:struct:`bt_audio_codec_cfg` now contains a target_latency and a target_phy option

* Logging:

  * Added rate-limited logging macros to prevent log flooding when messages are generated frequently.

    * :c:macro:`LOG_ERR_RATELIMIT` - Rate-limited error logging macro (convenience)
    * :c:macro:`LOG_WRN_RATELIMIT` - Rate-limited warning logging macro (convenience)
    * :c:macro:`LOG_INF_RATELIMIT` - Rate-limited info logging macro (convenience)
    * :c:macro:`LOG_DBG_RATELIMIT` - Rate-limited debug logging macro (convenience)
    * :c:macro:`LOG_HEXDUMP_ERR_RATELIMIT` - Rate-limited error hexdump macro (convenience)
    * :c:macro:`LOG_HEXDUMP_WRN_RATELIMIT` - Rate-limited warning hexdump macro (convenience)
    * :c:macro:`LOG_HEXDUMP_INF_RATELIMIT` - Rate-limited info hexdump macro (convenience)
    * :c:macro:`LOG_HEXDUMP_DBG_RATELIMIT` - Rate-limited debug hexdump macro (convenience)
    * :c:macro:`LOG_ERR_RATELIMIT_RATE` - Rate-limited error logging macro (explicit rate)
    * :c:macro:`LOG_WRN_RATELIMIT_RATE` - Rate-limited warning logging macro (explicit rate)
    * :c:macro:`LOG_INF_RATELIMIT_RATE` - Rate-limited info logging macro (explicit rate)
    * :c:macro:`LOG_DBG_RATELIMIT_RATE` - Rate-limited debug logging macro (explicit rate)
    * :c:macro:`LOG_HEXDUMP_ERR_RATELIMIT_RATE` - Rate-limited error hexdump macro (explicit rate)
    * :c:macro:`LOG_HEXDUMP_WRN_RATELIMIT_RATE` - Rate-limited warning hexdump macro (explicit rate)
    * :c:macro:`LOG_HEXDUMP_INF_RATELIMIT_RATE` - Rate-limited info hexdump macro (explicit rate)
    * :c:macro:`LOG_HEXDUMP_DBG_RATELIMIT_RATE` - Rate-limited debug hexdump macro (explicit rate)

* Power management

   * :c:func:`pm_device_driver_deinit`

* Settings

   * :kconfig:option:`CONFIG_SETTINGS_TFM_ITS`

.. zephyr-keep-sorted-stop

New Boards
**********

..
  You may update this list as you contribute a new board during the release cycle, in order to make
  it visible to people who might be looking at the working draft of the release notes. However, note
  that this list will be recomputed at the time of the release, so you don't *have* to update it.
  In any case, just link the board, further details go in the board description.

New Drivers
***********

..
  Same as above for boards, this will also be recomputed at the time of the release.
  Just link the driver, further details go in the binding description

* Input

   * :dtcompatible:`chipsemi,chsc5x`

* Interrupt controller

   * STM32 EXTI interrupt/event controller (:dtcompatible:`st,stm32-exti`) has a dedicated driver and API now, separate from STM32 GPIO Interrupt Control driver.

* RTC

   * STM32 RTC driver has been updated to use the new STM32 EXTI interrupt controller API


New Samples
***********

..
  Same as above for boards and drivers, this will also be recomputed at the time of the release.
 Just link the sample, further details go in the sample documentation itself.

Libraries / Subsystems
**********************

* Logging:

  * Added hybrid rate-limited logging macros to prevent log flooding when messages are generated frequently.
    The system provides both convenience macros (using default rate from :kconfig:option:`CONFIG_LOG_RATELIMIT_INTERVAL_MS`)
    and explicit rate macros (with custom rate parameter). This follows Linux's ``printk_ratelimited`` pattern
    while providing more flexibility. The rate limiting is per-macro-call-site, meaning that each unique call
    to a rate-limited macro has its own independent rate limit. Rate-limited logging can be globally enabled/disabled
    via :kconfig:option:`CONFIG_LOG_RATELIMIT`. When rate limiting is disabled, the behavior can be controlled
    via :kconfig:option:`CONFIG_LOG_RATELIMIT_FALLBACK` to either log all messages or drop them completely.
    For more details, see :ref:`logging_ratelimited`.

Other notable changes
*********************

..
  Any more descriptive subsystem or driver changes. Do you really want to write
  a paragraph or is it enough to link to the api/driver/Kconfig/board page above?

* Nordic Semiconductor nRF54L09 PDK (``nrf54l09pdk``), which only targeted an emulator, has been removed
  from the tree. It will be replaced with a proper board definition as soon as it's available.

* Removed support for Nordic Semiconductor nRF54L20 PDK (``nrf54l20pdk``) since it is
  replaced with :zephyr:board:`nrf54lm20dk` (``nrf54lm20dk``).
