.. _footprint:

Optimizing for Footprint
########################

Stack Sizes
***********

Stack sizes of various system threads are specified generously to allow for
usage in different scenarios on as many supported platforms as possible. You
should start the optimization process by reviewing all stack sizes and adjusting
them for your application:

:kconfig:option:`CONFIG_ISR_STACK_SIZE`
  Set to 2048 by default

:kconfig:option:`CONFIG_MAIN_STACK_SIZE`
  Set to 1024 by default

:kconfig:option:`CONFIG_IDLE_STACK_SIZE`
  Set to 320 by default

:kconfig:option:`CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE`
  Set to 1024 by default

:kconfig:option:`CONFIG_PRIVILEGED_STACK_SIZE`
  Set to 1024 by default, depends on userspace feature.


Unused Peripherals
******************

Some peripherals are enabled by default. You can disable unused
peripherals in your project configuration, for example::


        CONFIG_GPIO=n
        CONFIG_SPI=n

Various Debug/Informational Options
***********************************

The following options output more information about
the running application and provide means for debugging and error handling:

:kconfig:option:`CONFIG_BOOT_BANNER`
  This option can be disabled to save a few bytes.

:kconfig:option:`CONFIG_DEBUG`
  This option can be enabled for debug builds.

Note that the boot banner is enabled by default.


MPU/MMU Support
***************

Depending on your application and platform needs, you can disable MPU/MMU
support to gain some memory and improve performance.  Consider the consequences
of this configuration choice though, because you'll lose advanced stack
checking and support.

Link-Time Optimization
**********************

Link-Time Optimization (LTO) can significantly reduce your firmware's footprint by
performing cross-module inlining and aggressively stripping unused symbols.

:kconfig:option:`CONFIG_LTO`
  Enable this option to turn on Link-Time Optimization.

When LTO is enabled, the build system will automatically select the safest and most
optimal mode for your target hardware. You generally do not need to manually specify
the mode. There are two underlying modes managed via the
:kconfig:option:`CONFIG_LTO_MODE` Kconfig choice:

Global LTO (:kconfig:option:`CONFIG_LTO_MODE_GLOBAL`)
   Applies LTO across the entire Zephyr build. This maximizes footprint savings. This is
   the default for most robust architectures.

Selective LTO (:kconfig:option:`CONFIG_LTO_MODE_SELECTIVE`)
   Intelligently applies LTO only to the user application and high-level middleware
   (e.g. Bluetooth, Networking). Low-level hardware abstractions (``arch/``, ``soc/``,
   ``modules/hal/``) are strictly exempted from optimization passes to guarantee system
   stability and successful booting. This mode is automatically enforced on fragile
   architectures (such as Espressif SoCs) where Global LTO would break early boot code
   or exception vectors.
