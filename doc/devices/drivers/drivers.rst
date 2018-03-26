.. _device_drivers:

Device Drivers and Device Model
###############################

Introduction
************
The Zephyr kernel supports a variety of device drivers. Whether a
driver is available depends on the board and the driver.

The Zephyr device model provides a consistent device model for configuring the
drivers that are part of a system. The device model is responsible
for initializing all the drivers configured into the system.

Each type of driver (UART, SPI, I2C) is supported by a generic type API.

In this model the driver fills in the pointer to the structure containing the
function pointers to its API functions during driver initialization. These
structures are placed into the RAM section in initialization level order.

Standard Drivers
****************

Device drivers which are present on all supported board configurations
are listed below.

* **Interrupt controller**: This device driver is used by the kernel's
  interrupt management subsystem.

* **Timer**: This device driver is used by the kernel's system clock and
  hardware clock subsystem.

* **Serial communication**: This device driver is used by the kernel's
  system console subsystem.

* **Random number generator**: This device driver provides a source of random
  numbers.

  .. important::

    Certain implementations of this device driver do not generate sequences of
    values that are truly random.

Synchronous Calls
*****************

Zephyr provides a set of device drivers for multiple boards. Each driver
should support an interrupt-based implementation, rather than polling, unless
the specific hardware does not provide any interrupt.

High-level calls accessed through device-specific APIs, such as i2c.h
or spi.h, are usually intended as synchronous. Thus, these calls should be
blocking.

Driver APIs
***********

The following APIs for device drivers are provided by :file:`device.h`. The APIs
are intended for use in device drivers only and should not be used in
applications.

:c:func:`DEVICE_INIT()`
   create device object and set it up for boot time initialization.

:c:func:`DEVICE_AND_API_INIT()`
   Create device object and set it up for boot time initialization.
   This also takes a pointer to driver API struct for link time
   pointer assignment.

:c:func:`DEVICE_NAME_GET()`
   Expands to the full name of a global device object.

:c:func:`DEVICE_GET()`
   Obtain a pointer to a device object by name.

:c:func:`DEVICE_DECLARE()`
   Declare a device object.

Driver Data Structures
**********************

The device initialization macros populate some data structures at build time
which are
split into read-only and runtime-mutable parts. At a high level we have:

.. code-block:: C

  struct device {
        struct device_config *config;
        void *driver_api;
        void *driver_data;
  };

  struct device_config {
	char    *name;
	int (*init)(struct device *device);
	const void *config_info;
    [...]
  };

The ``config`` member is for read-only configuration data set at build time. For
example, base memory mapped IO addresses, IRQ line numbers, or other fixed
physical characteristics of the device. This is the ``config_info`` structure
passed to the ``DEVICE_*INIT()`` macros.

The ``driver_data`` struct is kept in RAM, and is used by the driver for
per-instance runtime housekeeping. For example, it may contain reference counts,
semaphores, scratch buffers, etc.

The ``driver_api`` struct maps generic subsystem APIs to the device-specific
implementations in the driver. It is typically read-only and populated at
build time. The next section describes this in more detail.


Subsystems and API Structures
*****************************

Most drivers will be implementing a device-independent subsystem API.
Applications can simply program to that generic API, and application
code is not specific to any particular driver implementation.

A subsystem API definition typically looks like this:

.. code-block:: C

  typedef int (*subsystem_do_this_t)(struct device *device, int foo, int bar);
  typedef void (*subsystem_do_that_t)(struct device *device, void *baz);

  struct subsystem_api {
        subsystem_do_this_t do_this;
        subsystem_do_that_t do_that;
  };

  static inline int subsystem_do_this(struct device *device, int foo, int bar)
  {
        struct subsystem_api *api;

        api = (struct subsystem_api *)device->driver_api;
        return api->do_this(device, foo, bar);
  }

  static inline void subsystem_do_that(struct device *device, void *baz)
  {
        struct subsystem_api *api;

        api = (struct subsystem_api *)device->driver_api;
        api->do_that(device, foo, bar);
  }

A driver implementing a particular subsystem will define the real implementation
of these APIs, and populate an instance of subsystem_api structure:

.. code-block:: C

  static int my_driver_do_this(struct device *device, int foo, int bar)
  {
        ...
  }

  static void my_driver_do_that(struct device *device, void *baz)
  {
        ...
  }

  static struct subsystem_api my_driver_api_funcs = {
        .do_this = my_driver_do_this,
        .do_that = my_driver_do_that
  };

The driver would then pass ``my_driver_api_funcs`` as the ``api`` argument to
``DEVICE_AND_API_INIT()``, or manually assign it to ``device->driver_api``
in the driver init function.

.. note::

        Since pointers to the API functions are referenced in the ``driver_api``
        struct, they will always be included in the binary even if unused;
        ``gc-sections`` linker option will always see at least one reference to
        them. Providing for link-time size optimizations with driver APIs in
        most cases requires that the optional feature be controlled by a
        Kconfig option.

Single Driver, Multiple Instances
*********************************

Some drivers may be instantiated multiple times in a given system. For example
there can be multiple GPIO banks, or multiple UARTS. Each instance of the driver
will have a different ``config_info`` struct and ``driver_data`` struct.

Configuring interrupts for multiple drivers instances is a special case. If each
instance needs to configure a different interrupt line, this can be accomplished
through the use of per-instance configuration functions, since the parameters
to ``IRQ_CONNECT()`` need to be resolvable at build time.

For example, let's say we need to configure two instances of ``my_driver``, each
with a different interrupt line. In ``drivers/subsystem/subsystem_my_driver.h``:

.. code-block:: C

  typedef void (*my_driver_config_irq_t)(struct device *device);

  struct my_driver_config {
        u32_t base_addr;
        my_driver_config_irq_t config_func;
  };

In the implementation of the common init function:

.. code-block:: C

  void my_driver_isr(struct device *device)
  {
        /* Handle interrupt */
        ...
  }

  int my_driver_init(struct device *device)
  {
        const struct my_driver_config *config = device->config->config_info;

        /* Do other initialization stuff */
        ...

        config->config_func(device);

        return 0;
  }

Then when the particular instance is declared:

.. code-block:: C

  #if CONFIG_MY_DRIVER_0

  DEVICE_DECLARE(my_driver_0);

  static void my_driver_config_irq_0(void)
  {
        IRQ_CONNECT(MY_DRIVER_0_IRQ, MY_DRIVER_0_PRI, my_driver_isr,
                    DEVICE_GET(my_driver_0), MY_DRIVER_0_FLAGS);
  }

  const static struct my_driver_config my_driver_config_0 = {
        .base_addr = MY_DRIVER_0_BASE_ADDR,
        .config_func = my_driver_config_irq_0
  }

  static struct my_driver_data_0;

  DEVICE_AND_API_INIT(my_driver_0, MY_DRIVER_0_NAME, my_driver_init,
                      &my_driver_data_0, &my_driver_config_0, SECONDARY,
                      MY_DRIVER_0_PRIORITY, &my_driver_api_funcs);

  #endif /* CONFIG_MY_DRIVER_0 */

Note the use of ``DEVICE_DECLARE()`` to avoid a circular dependency on providing
the IRQ handler argument and the definition of the device itself.

Initialization Levels
*********************

Drivers may depend on other drivers being initialized first, or
require the use of kernel services. The DEVICE_INIT() APIs allow the user to
specify at what time during the boot sequence the init function will be
executed. Any driver will specify one of five initialization levels:

``PRE_KERNEL_1``
        Used for devices that have no dependencies, such as those that rely
        solely on hardware present in the processor/SOC. These devices cannot
        use any kernel services during configuration, since the kernel services are
        not yet available. The interrupt subsystem will be configured however
        so it's OK to set up interrupts. Init functions at this level run on the
        interrupt stack.

``PRE_KERNEL_2``
        Used for devices that rely on the initialization of devices initialized
        as part of the PRIMARY level. These devices cannot use any kernel
        services during configuration, since the kernel services are not yet
        available. Init functions at this level run on the interrupt stack.

``POST_KERNEL``
        Used for devices that require kernel services during configuration.
        Init functions at this level run in context of the kernel main task.

``APPLICATION``
        Used for application components (i.e. non-kernel components) that need
        automatic configuration. These devices can use all services provided by
        the kernel during configuration. Init functions at this level run on
        the kernel main task.

Within each initialization level you may specify a priority level, relative to
other devices in the same initialization level. The priority level is specified
as an integer value in the range 0 to 99; lower values indicate earlier
initialization.  The priority level must be a decimal integer literal without
leading zeroes or sign (e.g. 32), or an equivalent symbolic name (e.g.
``\#define MY_INIT_PRIO 32``); symbolic expressions are *not* permitted (e.g.
``CONFIG_KERNEL_INIT_PRIORITY_DEFAULT + 5``).


System Drivers
**************

In some cases you may just need to run a function at boot. Special ``SYS_*``
macros exist that map to ``DEVICE_*INIT()`` calls.
For ``SYS_INIT()`` there are no config or runtime data structures and there
isn't a way
to later get a device pointer by name. The same policies for initialization
level and priority apply.

For ``SYS_DEVICE_DEFINE()`` you can obtain pointers by name, see
:ref:`power management <power_management>` section.

:c:func:`SYS_INIT()`

:c:func:`SYS_DEVICE_DEFINE()`

Error handling
**************

In general, it's best to use ``__ASSERT()`` macros instead of
propagating return values unless the failure is expected to occur
during the normal course of operation (such as a storage device
full). Bad parameters, programming errors, consistency checks,
pathological/unrecoverable failures, etc., should be handled by
assertions.

When it is appropriate to return error conditions for the caller to
check, 0 should be returned on success and a POSIX errno.h code
returned on failure.  See
https://github.com/zephyrproject-rtos/zephyr/wiki/Naming-Conventions#return-codes
for details about this.

