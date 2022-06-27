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

The following options are enabled by default to provide more information about
the running application and to provide means for debugging and error handling:

:kconfig:option:`CONFIG_BOOT_BANNER`
  This option can be disabled to save a few bytes.

:kconfig:option:`CONFIG_DEBUG`
  This option can be disabled for production builds


MPU/MMU Support
***************

Depending on your application and platform needs, you can disable MPU/MMU
support to gain some memory and improve performance.  Consider the consequences
of this configuration choice though, because you'll lose advanced stack
checking and support.
