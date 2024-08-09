.. _clock-mgmt-guide:

Clock Management
################

This is a high level overview of clock management in Zephyr. See
:ref:`clock_mgmt_api` for API reference material.

Introduction
************

Clock topology within a system typically consists of a mixture of clock
generators, clock dividers, clock gates and clock selection blocks. These may
include PLLs, internal oscillators, multiplexers, or dividers/multipliers. The
clock structure of a system is typically very specific to the vendor/SOC itself.
Zephyr provides the clock management subsystem in order to enable clock
consumers and applications to manage clocks in a device-agnostic manner.

Clock Management versus Clock Drivers
=====================================

.. note::

   This section describes the clock management subsystem API and usage. For
   documentation on how to implement clock producer drivers, see
   :ref:`clock-producers`.

The clock management subsystem is split into two portions: the consumer facing
clock management API, and the internal clock driver API. The clock driver API is
used by clock producers to query and set rates of their parent clocks, as well
as receive reconfiguration notifications when the state of the clock tree
changes. Each clock producer must implement the clock driver API, but clock
consumers should only interact with clocks using the clock management API.

This approach is required because clock states may configure clocks by directly
configuring each producer with hardware specific specifiers, or by requesting a
rate from a given clock output. Since not all applications will enable support
for requesting a clock rate, consumers should not have knowledge of the
underlying contents of the clock state (as this data is often hardware
specific).

Reading Clocks
**************

Drivers must supply a output identifier when reading or subscribing to a
callback for a clock. The clock management defines one output identifier,
:c:macro:`CLOCK_MGMT_OUTPUT_DEFAULT`. Consumers may define additional private
identifiers (starting from :c:macro:`CLOCK_MGMT_OUTPUT_PRIV_START`) if they need
to query multiple clocks. Each identifier corresponds to an index within the
``clock-outputs`` property of the node defining the clock consumer.

Consumers can query a clock output rate via the :c:func:`clock_mgmt_get_rate`
function. This function returns the clock rate as a positive integer,
or a negative error value on failure.

Consumers can also monitor a clock output rate. To do so, the application must
first enable :kconfig:option:`CONFIG_CLOCK_MGMT_NOTIFY`. The application may
then set a callback of type :c:type:`clock_mgmt_callback_handler_t` using
:c:func:`clock_mgmt_set_callback`. One callback is supported per consumer.

The clock management subsystem will issue callbacks just prior to setting a new
clock rate. When a consumer receives a callback, they can check the
``output_idx`` parameter identify which clock will change. If the consumer
returns a negative error code from their callback handler, the clock management
subsystem will not apply the new rate (indicated by the ``rate`` parameter).

Setting Clocks
**************

Clocks are managed via "clock states". A clock state corresponds to a specific
group of clock settings, which may have systemwide effects, or simply configure
clocks for a given peripheral. For example, one state may configure the
multiplexer and divider for a given peripheral. Another may reconfigure the
system level PLL, and modify the core clock rate.

States are described in devicetree, and are opaque to the driver/application
code consuming the clock. Instead of having direct knowledge of a state's
contents, clock consumers select a state based on the power state they are
targeting. The following standard power states are defined:

.. table:: Standardized state names
    :align: center

    +-------------+-------------------------------------+-------------------------+
    | State       | Identifier                          | Purpose                 |
    +-------------+-------------------------------------+-------------------------+
    | ``default`` | :c:macro:`CLOCK_MGMT_STATE_DEFAULT` | Clock state when        |
    |             |                                     | the device is in        |
    |             |                                     | operational state       |
    +-------------+-------------------------------------+-------------------------+
    | ``sleep``   | :c:macro:`CLOCK_MGMT_STATE_SLEEP`   | Clock state when        |
    |             |                                     | the device is in low    |
    |             |                                     | power or sleep modes    |
    +-------------+-------------------------------------+-------------------------+

Consumers can define addition states, starting from
:c:macro:`CLOCK_MGMT_STATE_PRIV_START`. Each state index corresponds to a
``clock-state-n`` property, where ``n`` is the state index.

Devicetree Representation
*************************

Devicetree is used to define all system specific data for clock management. The
SOC (and any external clock producers) will define clock producers within the
devicetree. Then, the devicetree for clock consumers may reference the clock
producer nodes to configure the clock tree or access clock outputs.

The devicetree definition for clock producers will be specific to the system,
but may look similar to the following:

.. code-block:: devicetree

    clock_source: clock-source {
        compatible = "fixed-clock-source";
	frequency = <10000000>;
        #clock-cells = <0>;

        clock_div: clock-div@50000000 {
            compatible = "vnd,clock-div";
            #clock-cells = <1>;
	    reg = <0x5000000>;

            clock_output: clock-output {
                compatible = "clock-output";
                #clock-cells = <1>;
            };
        };
    };

Clock consumers will then reference the producer nodes in order to define clock
states and access clock outputs. A peripheral clock consumer's devicetree might
look like so:

.. code-block:: devicetree

    periph0: periph@0 {
        /* Clock outputs */
        clock-outputs= <&clock_output>;
        clock-names = "default";
        /* Default clock state */
        clock-state-0 = <&clock_div 1>;
        /* Sleep state */
        clock-state-1 = <&clock_div 3>;
        clock-state-names = "default", "sleep";
    };

Note that the specifier cells for each clock node within a state are
device specific. These specifiers allow configuration of the clock element,
such as setting a divider's division factor or selecting an output for a
multiplexer.

Requesting Clock Rates
======================

In some applications, the user may not want to directly configure clock nodes
within their devicetree. The clock management subsystem allows applications to
request a clock rate directly when :kconfig:option:`CONFIG_CLOCK_MGMT_SET_RATE`.
is set. This is accomplished by setting a specifier for a clock producer node
with the :dtcompatible:`clock-output` compatible to a specific frequency. Note
that clock state configuration using these clock producer nodes will fail if
:kconfig:option:`CONFIG_CLOCK_MGMT_SET_RATE` is not set. An example of setting
rate directly for a peripheral is given below:

.. code-block:: devicetree

    periph0: periph@0 {
        /* Clock outputs */
        clock-outputs= <&clock_output>;
        clock-names = "default";
        /* Request 10MHz clock for this peripheral */
        clock-state-0 = <&clock_output 10000000>;
        clock-state-names = "default";
    };

No guarantees are made on how accurate a resulting rate will be versus the
requested value.

Note that when a consumer requests a given clock rate, it takes a lock on
all clock producers used to provide that rate. This lock prevents other
consumers from reconfiguring those nodes, although this consumer can
reconfigure them. This way, the first consumers to request rates during system
initialization receive priority access to the most accurate clock sources.
Other consumers will then reuse the frequency from the locked producers at
their current rate, or use a different less accurate producer if that producer
can provide a clock rate closer to what is requested.

Driver Usage
************

In order to use the clock management subsystem, a driver must call two C macros.
First, the driver must define clock management data. This can be accomplished
with the :c:macro:`CLOCK_MGMT_DEFINE` or :c:macro:`CLOCK_MGMT_DT_INST_DEFINE`
depending on if the driver is instance based. Then the driver needs to
initialize a pointer of type :c:struct:`clock_mgmt` to pass to the clock
management functions. This can be done with the
:c:macro:`CLOCK_MGMT_DT_DEV_CONFIG_GET` or
:c:macro:`CLOCK_MGMT_DT_INST_DEV_CONFIG_GET`. The driver can then utilize the
clock management APIs.

During device init, the driver should apply the default clock management state.
This will allow the clock management subsystem to configure any clocks the
driver requires during its initialization phase. The driver can then query clock
rates. A complete example of implementing clock management within a driver is
provided below:

.. code-block:: c

    /* A driver for the "mydev" compatible device */
    #define DT_DRV_COMPAT mydev

    ...
    #include <zephyr/drivers/clock_mgmt.h>
    ...

    struct mydev_config {
        ...
        /* Reference to clock management configuration */
        const struct clock_mgmt *clk;
        ...
    };

    ...

    int mydev_clock_cb_handler(uint8_t output_idx, uint32_t new_rate,
                               const void *data)
    {
	      const struct device *dev = (const struct device *)data;

	      if (new_rate > MYDEV_MAX_CLK_RATE) {
	          /* Can't support this new rate */
	          return -ENOTSUP;
	      }
	      /* Handle clock rate change ... */
	      ...
	      return 0;
    }

    static int mydev_init(const struct device *dev)
    {
        const struct mydev_config *config = dev->config;
        int ret, clock_rate;
        ...
        /* Select default state at initialization time */
        ret = clock_mgmt_apply_state(config->clk, CLOCK_MGMT_STATE_DEFAULT);
        if (ret < 0) {
            return ret;
        }
        /* Query clock rate */
        clock_rate = clock_mgmt_get_rate(config->clk, CLOCK_MGMT_OUTPUT_DEFAULT);
        if (clock_rate < 0) {
            return clock_rate;
        }
        /* Register for a callback if clock rate changes (optional) */
        ret = clock_mgmt_set_callback(config->clk, mydev_clock_cb_handler, dev);
        if (ret < 0) {
            return ret;
        }
        ...
    }

    #define MYDEV_DEFINE(i)                                                    \
        /* Define all clock management configuration for instance "i" */       \
        CLOCK_MGMT_DT_INST_DEFINE(i);                                          \
        ...                                                                    \
        static const struct mydev_config mydev_config_##i = {                  \
            ...                                                                \
            /* Keep a ref. to the clock management configuration */            \
            /* for instance "i" */                                             \
            .clk = CLOCK_MGMT_DT_INST_DEV_CONFIG_GET(i),                       \
            ...                                                                \
        };                                                                     \
        static struct mydev_data mydev_data##i;                                \
        ...                                                                    \
                                                                               \
        DEVICE_DT_INST_DEFINE(i, mydev_init, NULL, &mydev_data##i,             \
                              &mydev_config##i, ...);

    DT_INST_FOREACH_STATUS_OKAY(MYDEV_DEFINE)

Gating Unused Clocks
====================

When :kconfig:option:`CONFIG_CLOCK_MGMT_NOTIFY` is enabled, it is possible to
gate unused clocks within the system, by calling
:c:func:`clock_mgmt_disable_unused`. Note that due to the implementation of the
clock management subsystem, this will trigger clock callback notifications for
clock consumers with the current clock rate. These notifications can safely be
ignored.

.. _clock_mgmt_api:

Clock Management API
********************

.. doxygengroup:: clock_mgmt_interface

.. _clock_mgmt_dt_api:

Devicetree Clock Management Helpers
===================================

.. doxygengroup:: devicetree-clock-mgmt


.. _clock-producers:

Clock Producers
***************

This is a high level overview of clock producers in Zephyr. See
:ref:`clock_driver_api` for API reference material.

Introduction
============

Although consumers interact with the clock management subsystem via the
:ref:`clock_mgmt_api`, producers must implement the clock driver API. This
API allows producers to receive reconfiguration notifications from their
parent clocks sources, as well as read or set their clock rates.

Clock Driver Implementation
===========================

Each node within a clock tree should be implemented within a clock driver. Clock
nodes are typically defined as elements in the clock tree. For example, a
multiplexer, divider, and PLL would all be considered independent nodes. Each
node should implement the :ref:`clock_driver_api`.

Clock nodes are represented by :c:struct:`clk` structures. These structures
store clock specific hardware data (which the driver may place in ROM or RAM,
depending on implementation needs), as well as a reference to the clock's API
and a list of the clock's children. For more details on defining and
accessing these structures, see :ref:`clock_model_api`.

Note that in order to conserve flash, many of the APIs of the clock driver layer
are only enabled when certain Kconfigs are set. A list of these API functions is
given below:

.. table:: Optional Clock Driver APIs
    :align: center

    +-----------------------------------------------+----------------------------+
    | Kconfig                                       | API Functions              |
    +-----------------------------------------------+----------------------------+
    | :kconfig:option:`CONFIG_CLOCK_MGMT_NOTIFY`    | :c:func:`clock_notify`     |
    +-----------------------------------------------+----------------------------+
    | :kconfig:option:`CONFIG_CLOCK_MGMT_SET_RATE`  | :c:func:`clock_set_rate`,  |
    |                                               | :c:func:`clock_round_rate` |
    +-----------------------------------------------+----------------------------+

These API functions **must** still be implemented by each clock driver, but they
can should be compiled out when these Kconfig options are not set.

Clock drivers will **must** hold a reference to their parent clock device, if
one exists. This is required because the clock subsystem determines which clock
devices can be discarded from the build at link time based on which clock devices
are referenced. Clock consumers will hold references to clock driver objects
they wish to configure or read rates from, which is how the linker determines
which clocks are being used by the application and must be linked into the
final build.

Getting Clock Structures
------------------------

A reference to a clock structure can be obtained with :c:macro:`CLOCK_DT_GET`.
note that clocks **must never** directly reference their children nodes. Doing
so would result in all children clock structures being linked into the build if
the parent was included, which will increase flash usage. By only referencing
parent clocks directly, clocks that have no consumer are discarded from the
build in the link phase.

Defining Clock Structures
-------------------------

Clock structures may be defined with :c:macro:`CLOCK_DT_INST_DEFINE` or
:c:macro:`CLOCK_DT_DEFINE`. Usage of this macro is very similar to the
:c:macro:`DEVICE_DT_DEFINE`. Clocks are defined as :c:struct:`clk` structures
instead of as :c:struct:`device` structures in order to reduce the flash impact
of the framework.

Root clock structures (any clock without any parents) **must** be defined with
:c:macro:`ROOT_CLOCK_DT_INST_DEFINE` or :c:macro:`ROOT_CLOCK_DT_DEFINE`. This
is needed because the implementation of :c:func:`clock_mgmt_disable_unused`
will call :c:func:`clock_notify` on root clocks only, so if a root clock
is not notified then it and its children will not be able to determine if
they can power off safely.

See below for a simple example of defining a (non root) clock structure:

.. code-block:: c

    #define DT_DRV_COMPAT clock_output

    ...
    /* API implementations */
    ...

    const struct clock_driver_api clock_output_api = {
        ...
    };

    #define CLOCK_OUTPUT_DEFINE(inst)                                        \
      CLOCK_DT_INST_DEFINE(inst,                                             \
                       /* Clock data is simply a pointer to the parent */    \
                       CLOCK_DT_GET(DT_INST_PARENT(inst)),                   \
                                    &clock_output_api);

    DT_INST_FOREACH_STATUS_OKAY(CLOCK_OUTPUT_DEFINE)

Clock Node Specifier Data
-------------------------

Clock nodes in devicetree will define a set of specifiers with their DT binding,
which are used to configure the clock directly. When an application references a
clock node with the compatible ``vnd,clock-node``, the clock management
subsystem expects the following macros be defined:

* ``Z_CLOCK_MGMT_VND_CLOCK_NODE_DATA_DEFINE``: defines any static structures
  needed by this clock node (IE a C structure)

* ``Z_CLOCK_MGMT_VND_CLOCK_NODE_DATA_GET``: gets a reference to any static
  structure defined by the ``DATA_DEFINE`` macro. This is used to initialize the
  ``void *`` passed to :c:func:`clock_configure`, so for many clock nodes this
  macro can simply expand to an integer value (which may be used for a register
  setting)

As an example, for the following devicetree:

.. code-block:: devicetree

    clock_source: clock-source {
        compatible = "fixed-clock-source";
	frequency = <10000000>;
        #clock-cells = <0>;

        clock_div: clock-div@50000000 {
            compatible = "vnd,clock-div";
            #clock-cells = <1>;
	    reg = <0x5000000>;

            clock_output: clock-output {
                compatible = "clock-output";
                #clock-cells = <0>;
            };
        };
    };

    ....

    periph0: periph@0 {
        /* Clock outputs */
        clock-outputs= <&clock_output>;
        clock-names = "default";
        /* Default clock state */
        clock-state-0 = <&clock_div 1>;
        clock-state-names = "default";
    };

The clock subsystem would expect the following macros be defined:

* ``Z_CLOCK_MGMT_VND_CLOCK_DIV_DATA_DEFINE``
* ``Z_CLOCK_MGMT_VND_CLOCK_DIV_DATA_GET``

These macros should be defined within a header file in the
``drivers/clock_mgmt`` folder. This header can be included from the root
producer header :zephyr_file:`drivers/clock_mgmt/clock_mgmt_drivers.h` when the
Kconfig for the clock producer driver is enabled.

Output Clock Nodes
------------------

Clock trees should define output clock nodes as leaf nodes within their
devicetree. These nodes must have the compatible :dtcompatible:`clock-output`.
They accept one specifier, a frequency to request.

The implementation of the :c:func:`clock_configure` function for clock output
nodes simply calls :c:func:`clock_set_rate` on the parent clock, using the
requested rate provided as a devicetree specifier.

Clock Locking
-------------

When clock rates are requested with :c:func:`clock_set_rate`, there must be
some form of priority. Otherwise, all requests would likely end up at the most
flexible clock source (likely a PLL), and requesting rates for peripherals would
likely reconfigure the clocks of other consumers that had already initialized.

To resolve this issue, clocks are locked when :c:func:`clock_set_rate` is called
on them. Clocks calling :c:func:`clock_set_rate` or :c:func:`clock_round_rate`
should provide a pointer to their :c:struct:`clk` structure as the ``owner`` or
``consumer`` parameter. This way, the current clock will take a lock on its
parent, which allows it and only it to reconfigure the parent until the lock is
released.

Clocks may unlock a parent by calling :c:func:`clock_unlock`. This is typically
only useful for multiplexers, which may unlock a parent when they are no longer
using its output. Clock drivers do not need to call :c:func:`clock_lock`
directly, it will be handled by :c:func:`clock_set_rate`.

Implementation Guidelines
-------------------------

Implementations of each clock driver API will be vendor specific, but some
general guidance on implementing each API is provided below:

* :c:func:`clock_get_rate`

  * Read parent rate, and calculate rate this node will produce based on node
    specific settings.
  * Multiplexers will instead read the rate of their active parent.
  * Sources will likely return a fixed rate, or 0 if the source is gated. For
    fixed sources, see :dtcompatible:`fixed-clock-source`.

* :c:func:`clock_configure`

  * Cast the ``void *`` provided in the API call to the data type this clock
    driver uses for configuration.
  * Calculate the new rate that will be set after this configuration is applied.
  * Call :c:func:`clock_notify_children` to notify children of the new rate. If
    this call returns an error, return this value and don't reconfigure the
    clock.
  * Otherwise if :c:func:`clock_notify_children` does not return an error, apply
    the new configuration value.

* :c:func:`clock_notify`

  * Read the node specific settings to determine the rate this node will
    produce, based on the ``parent_rate`` provided in this call.
  * Return an error if this rate cannot be supported by the node.
  * Forward the notification of clock reconfiguration to children by calling
    :c:func:`clock_notify_children` with the new rate.
  * Multiplexers may also return ``-ENOTCONN`` to indicate they are not
    using the output of the clock specified by ``parent``.
  * If the return code of :c:func:`clock_notify_children` is
    :c:macro:`CLK_NO_CHILDREN`, the clock may safely power off its output.

* :c:func:`clock_round_rate`

  * Determine what rate should be requested from the parent in order
    to produce the requested rate.
  * Call :c:func:`clock_round_rate` on the parent clock to determine if
    the parent can produce the needed rate.
  * Calculate the best possible rate this node can produce based on the
    parent's best rate.
  * Multiplexers will typically implement this function by calling
    :c:func:`clock_round_rate` on all parents, and returning the best
    rate found.

* :c:func:`clock_set_rate`

  * Similar implementation to :c:func:`clock_round_rate`, but once all
    settings needed for a given rate have been applied, actually configure it.
  * Call :c:func:`clock_notify_children` to notify children of the new rate. If
    this call returns an error, return this value and don't reconfigure the
    clock.
  * Call :c:func:`clock_set_rate` on the parent clock to configure the needed
    rate. Then configure this clock node.
  * Multiplexers should call :c:func:`clock_unlock` on their old parent clock if
    they have switched sources, to allow the parent to be reconfigured.

.. _clock_driver_api:

Clock Driver API
================

.. doxygengroup:: clock_driver_interface

.. _clock_model_api:

Clock Model API
===============

.. doxygengroup:: clock_model
