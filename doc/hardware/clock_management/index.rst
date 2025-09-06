.. _clock-management-guide:

Clock Management
################

This is a high level overview of clock management in Zephyr. See
:ref:`clock_management_api` for API reference material.

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
clock management API, and the internal clock driver API implemented by clock
producers. Clock consumers should only interact with the clock tree using the
clock management API.

Clock producers are described by devicetree nodes, and are considered to be any
element of a clock tree that takes one (or multiple) frequencies as input and
produces a frequency as an output. Clock states in devicetree may configure
producers directly, but the clock consumer should never access producers via the
clock driver API, as this is the responsibility of the clock management
subsystem.

This approach is required because clock consumers should not have knowledge of
how their underlying clock states are applied or defined, as the data is often
hardware specific. Consumers may apply states directly, or request a frequency
range, which can then be satisfied by one of the defined states. For details on
the operation of the clock subsystem, see :ref:`clock_subsystem_operation`.

Clock Management Usage
**********************

In order to interact with the clock tree, clock consumers must define and
initialize a clock output device. For devices defined in devicetree, which
define clocks within their ``clock-outputs`` property,
:c:macro:`CLOCK_MANAGEMENT_DT_DEFINE_OUTPUT` or similar may be used. For
software applications consuming a clock,
:c:macro:`CLOCK_MANAGEMENT_DEFINE_OUTPUT` should be used.

Clock consumers may then initialize their clock output device using
:c:macro:`CLOCK_MANAGEMENT_DT_GET_OUTPUT` or similar for consumer devices
defined in devicetree, or :c:macro:`CLOCK_MANAGEMENT_GET_OUTPUT` for software
applications consuming a clock.

Clock output devices can then be used with the clock management API to
interface with the clock tree, as described below.

Reading Clocks
==============

Once a driver has defined a clock output and initialized it, the clock rate can
be read using :c:func:`clock_management_get_rate`. This will return the
frequency of the clock output in Hz, or a negative error value if the clock
could not be read.

Consumers can also monitor a clock output rate. To do so, the application must
first enable :kconfig:option:`CONFIG_CLOCK_MANAGEMENT_RUNTIME`. The application may
then set a callback of type :c:type:`clock_management_callback_handler_t` using
:c:func:`clock_management_set_callback`. One callback is supported per consumer.

The clock management subsystem will issue a callback directly before applying a
new clock rate, and directly after the new rate is applied. See
:c:struct:`clock_management_event` for more details.

Setting Clock States
====================

Each clock output defines a set of states. Clock consumers can set these states
directly, using :c:func:`clock_management_apply_state`. States are described in
devicetree, and are opaque to the driver/application code consuming the clock.

Each clock consumer described in devicetree can set named clock states for each
clock output. These states are described by the ``clock-state-n`` properties
present on each consumer. The consumer can access states using macros like
:c:macro:`CLOCK_MANAGEMENT_DT_GET_STATE`


Requesting Clock Rates
======================

In some applications, the user may not want to directly configure clock nodes
within their devicetree. The clock management subsystem allows applications to
request a clock rate directly as well, by using
:c:func:`clock_management_req_rate`.  If any states satisfy the frequency range
request, the state offering the frequency closest to the maximum frequency
requested. will be applied.  Otherwise if
:kconfig:option:`CONFIG_CLOCK_MANAGEMENT_SET_RATE` is set, the clock management
subsystem will perform runtime calculations to apply a rate within the requested
range. If runtime rate calculation support is disabled, the request will fail if
no defined states satisfy it.

Clock Ranking
=============

The clock subsystem also supports a user-specified "rank" that can be applied to
the devicetree node for any clock producer. Two properties are provided:

* ``clock-ranking``: A fixed ranking value for this clock. Max value of 255.

* ``clock-rank-factor``: Rank factor, scales with frequency according to the
  following: ``<factor> * <clock-freq> / 255``. Max value of 255.

These properties can be used to guide the framework when selecting a clock
output. The function :c:func:`clock_management_req_ranked` will apply the clock
state with the best ranking that fits the bounds of the frequency request.
Lower ranks are preferred, so a rank of 0 is considered an "ideal" clock
setting.  When runtime rate calculation is used, the rank is calculated by
summing the rank for every clock producer that will be used to produce the
frequency. For fixed states, the ranking is encoded statically.

Ranking is intentionally user specific - the application can define this to
be a hardware property such as power consumption, or may choose to use it
arbitrarily to prevent the clock framework from selecting certain clocks as
inputs.

Devicetree Representation
=========================

Devicetree is used to define all system specific data for clock management. The
SOC (and any external clock producers) will define clock producers within the
devicetree. Then, the devicetree for clock consumers may reference the clock
producer nodes to configure the clock tree or access clock outputs.

The devicetree definition for clock producers will be specific to the system,
but may look similar to the following:

.. code-block:: devicetree

    clock_source: clock-source {
        compatible = "fixed-clock";
        clock-frequency = <DT_FREQ_M(10)>;
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

At the board level, applications will define clock states for each clock output
node, which may either directly configure producer clock nodes to realize a
frequency, or simply define a frequency to request from the parent clock at
runtime (which will only function if
:kconfig:option:`CONFIG_CLOCK_MANAGEMENT_SET_RATE` is enabled).

.. code-block:: devicetree

    &clock_output {
        clock_output_state_default: clock-output-state-default {
            compatible = "clock-state";
            /* Directly configure clock producers */
            clocks = <&clock_div 1>;
            clock-frequency = <DT_FREQ_M(10)>
        };
        clock_output_state_sleep: clock-output-state-sleep {
            compatible = "clock-state";
            clocks = <&clock_div 5>;
            clock-frequency = <DT_FREQ_M(2)>
        };
        clock_output_state_runtime: clock-output-state-runtime {
            compatible = "clock-state";
            /* Will issue runtime frequency request to parent */
            clock-frequency = <DT_FREQ_M(1)>;
        };
    };

Note that the specifier cells for each clock producer within a state are device
specific. These specifiers allow configuration of the clock producer, such as
setting a divider's division factor or selecting an output for a multiplexer.

Clock consumers will then reference the clock output nodes and their states in
order to query and request clock rates, and apply states. A peripheral clock
consumer's devicetree might look like so:

.. code-block:: devicetree

    periph0: periph@0 {
        compatible = "vnd,mydev";
        /* Clock outputs */
        clock-outputs = <&clock_output>;
        clock-output-names = "default";
        /* Default clock state */
        clock-state-0 = <&clock_output_state_default>;
        /* Sleep state */
        clock-state-1 = <&clock_output_state_sleep>;
        clock-state-names = "default", "sleep";
    };

Enabling and Disabling Clocks
=============================

Clocks can be enabled or disabled by using the functions
:c:func:`clock_management_on` and :c:func:`clock_management_off`. These functions
will enable or disable all producers a given clock output depends on. When
:kconfig:option:`CONFIG_CLOCK_MANAGEMENT_RUNTIME` is set, calls to these functions
use reference counting, so producers with multiple consumers will not be disabled
until all consumers have balanced their call to :c:func:`clock_management_on`
with a call to :c:func:`clock_management_off`.

.. note::

   When :kconfig:option:`CONFIG_CLOCK_MANAGEMENT_RUNTIME` is disabled,
   :c:func:`clock_management_off` will gate all parent producers unconditionally.
   This can be a dangerous operation, as no check is made to validate other
   consumers are not using the producer

Gating Unused Clocks
====================

When :kconfig:option:`CONFIG_CLOCK_MANAGEMENT_RUNTIME` is enabled, it is
possible to gate unused clocks within the system, by calling
:c:func:`clock_management_disable_unused`. All clocks that do not have a
reference count set via :c:func:`clock_management_on` will be gated.

Locking Clock States and Requests
=================================

When :kconfig:option:`CONFIG_CLOCK_MANAGEMENT_RUNTIME` is enabled, requests
issued via :c:func:`clock_management_req_rate` or
:c:func:`clock_management_req_ranked` to the same clock by different consumers
will be aggregated to form a "combined" request for that clock. This means that
a request may be denied if it is incompatible with the existing set of
aggregated clock requests. Clock states do not place a request on the clock they
configure by default- if a clock state should "lock" the clock to prevent the
frequency changing, it should be defined with the ``locking-state`` boolean
property.  This can be useful for critical system clocks, such as the core
clock.

Generally when multiple clocks are expected to be reconfigured at runtime,
:kconfig:option:`CONFIG_CLOCK_MANAGEMENT_RUNTIME` should be enabled to avoid
unexpected rate changes for consumers. Otherwise clock states should be defined
in such a way that each consumer can reconfigure itself without affecting other
clock consumers in the system.


Driver Usage
============

In order to use the clock management subsystem, a driver must define and
initialize a :c:struct:`clock_output` for the clock it wishes to interact with.
The clock output structure can be defined with
:c:macro:`CLOCK_MANAGEMENT_DT_DEFINE_OUTPUT`, and then accessed with
:c:macro:`CLOCK_MANAGEMENT_DT_GET_OUTPUT`. Note that both these macros also have
versions that allow the driver to access an output by name or index, if
multiple clocks are present within the ``clock-outputs`` property for the
device.

In order to configure a clock, the driver may either request a supported
clock rate range via :c:func:`clock_management_req_rate`, or apply a clock state
directly via :c:func:`clock_management_apply_state`. For most applications,
:c:func:`clock_management_apply_state` is recommended, as this allows the application
to customize the clock properties that are set using devicetree.
:c:func:`clock_management_req_rate` should only be used in cases where the driver
knows the frequency range it should use, and cannot accept a frequency outside
of that range.

Drivers can define states of type :c:type:`clock_management_state_t` using
:c:macro:`CLOCK_MANAGEMENT_DT_GET_STATE`, or the name/index based versions of this
macro.

For example, if a peripheral devicetree was defined like so:

.. code-block:: devicetree

    periph0: periph@0 {
        compatible = "vnd,mydev";
        /* Clock outputs */
        clock-outputs= <&periph_hs_clock &periph_lp_clock>;
        clock-output-names = "high-speed", "low-power";
        /* Default clock state */
        clock-state-0 = <&hs_clock_default &lp_clock_default>;
        /* Sleep state */
        clock-state-1 = <&hs_clock_sleep &lp_clock_sleep>;
        clock-state-names = "default", "sleep";
    };

The following C code could be used to apply the default state for the
``high-speed`` clocks, and sleep state for the ``low-power`` clock:

.. code-block:: c

    /* A driver for the "vnd,mydev" compatible device */
    #define DT_DRV_COMPAT vnd_mydev

    ...
    #include <zephyr/drivers/clock_management.h>
    ...

    struct mydev_config {
        ...
        /* Reference to high-speed clock */
        const struct clock_output *hs_clk;
        /* Reference to low-power clock */
        const struct clock_output *lp_clk;
        /* high-speed clock default state */
        const clock_management_state_t hs_default_state;
        /* low-power sleep state */
        const clock_management_state_t lp_sleep_state;
        ...
    };

    ...

    int hs_clock_cb(const struct clock_management_event *ev, const void *data)
    {
        const struct device *dev = (const struct device *)data;

        if (ev->new_rate > HS_MAX_CLK_RATE) {
            /* Can't support this new rate */
            return -ENOTSUP;
        }
        if (ev->type == CLOCK_MANAGEMENT_POST_RATE_CHANGE) {
            /* Handle clock rate change ... */
        }
        ...
        return 0;
    }

    static int mydev_init(const struct device *dev)
    {
        const struct mydev_config *config = dev->config;
        int hs_clk_rate, lp_clk_rate;
        ...
        /* Set high-speed clock to default state */
        hs_clock_rate = clock_management_apply_state(config->hs_clk, config->hs_default_state);
        if (hs_clock_rate < 0) {
            return hs_clock_rate;
        }
        /* Register for a callback if high-speed clock changes rate */
        ret = clock_management_set_callback(config->hs_clk, hs_clock_cb, dev);
        if (ret < 0) {
            return ret;
        }
        /* Set low-speed clock to sleep state */
        lp_clock_rate = clock_management_apply_state(config->lp_clk, config->lp_sleep_state);
        if (lp_clock_rate < 0) {
            return lp_clock_rate;
        }
        ...
    }

    #define MYDEV_DEFINE(i)                                                    \
        /* Define clock outputs for high-speed and low-power clocks */         \
        CLOCK_MANAGEMENT_DT_INST_DEFINE_OUTPUT_BY_NAME(i, high_speed);         \
        CLOCK_MANAGEMENT_DT_INST_DEFINE_OUTPUT_BY_NAME(i, low_power);          \
        ...                                                                    \
        static const struct mydev_config mydev_config_##i = {                  \
            ...                                                                \
            /* Initialize clock outputs */                                     \
            .hs_clk = CLOCK_MANAGEMENT_DT_INST_GET_OUTPUT_BY_NAME(i, high_speed),\
            .lp_clk = CLOCK_MANAGEMENT_DT_INST_GET_OUTPUT_BY_NAME(i, low_power),\
            /* Read states for high-speed and low-power */                     \
            .hs_default_state = CLOCK_MANAGEMENT_DT_INST_GET_STATE(i, high_speed,\
                                                             default),         \
            .lp_sleep_state = CLOCK_MANAGEMENT_DT_INST_GET_STATE(i, low_power, \
                                                           sleep),             \
            ...                                                                \
        };                                                                     \
        static struct mydev_data mydev_data##i;                                \
        ...                                                                    \
                                                                               \
        DEVICE_DT_INST_DEFINE(i, mydev_init, NULL, &mydev_data##i,             \
                              &mydev_config##i, ...);

    DT_INST_FOREACH_STATUS_OKAY(MYDEV_DEFINE)

.. _clock_management_api:

Clock Management API
********************

.. doxygengroup:: clock_management_interface

.. _clock_management_dt_api:

Devicetree Clock Management Helpers
===================================

.. doxygengroup:: devicetree-clock-management


.. _clock-producers:

Clock Producers
***************

This is a high level overview of clock producers in Zephyr. See
:ref:`clock_driver_api` for API reference material.

Introduction
============

Consumers interact with the clock management subsystem via the
:ref:`clock_management_api`, which leverages the :ref:`clock_driver_api` to
interface with clock producers, which configure SOC-specific clock tree
settings. Each clock producer must implmenent the clock driver API.

Clock Driver Implementation
===========================

Each devicetree node within a clock tree should be implemented within a clock
driver. Devicetree nodes should describe clock producers, and should be split
into the smallest logical components. For example, a multiplexer, divider, and
PLL would all be considered independent producers. Each producer should implement the
:ref:`clock_driver_api`.

Clock producers are represented by :c:struct:`clk` structures. These structures
store clock specific hardware data (which the driver may place in ROM or RAM,
depending on implementation needs), as well as a reference to the clock's API
and a list of the clock's children. For more details on defining and
accessing these structures, see :ref:`clock_model_api`.

Clock producers are split into three API classes, depending on their
functionality. This implementation was chosen in order to reduce flash
utilization, as the set of APIs needed by different clock producer types is
mostly orthagonal. The following API classes are available:

* Standard clocks, which implement :c:struct:`clock_management_standard_api`.
  Standard clocks take one clock source as an input, scale it, and produce
  a clock output. Examples include multipliers, dividers, or PLLs.

* Root clocks, which implement :c:struct:`clock_management_root_api`. Root
  clocks are those clocks which do not have any parent they source a frequency
  from. Examples include external or internal oscillators, or clocks sourced
  from an SOC pin.

* Multiplexer clocks, which implement :c:struct:`clock_management_mux_api`.
  Multiplexer clocks take multiple clock sources as input, and *do not* scale
  the clock input- they may only select one of the inputs to use as an output.

Note that in order to conserve flash, many of the APIs of the clock driver layer
are only enabled when certain Kconfigs are set. A list of these API functions is
given below:

.. table:: Optional Clock Driver APIs
    :align: center

    +-----------------------------------------------------+---------------------------------------+
    | Kconfig                                             | API Functions                         |
    +-----------------------------------------------------+---------------------------------------+
    | :kconfig:option:`CONFIG_CLOCK_MANAGEMENT_RUNTIME`   | :c:func:`clock_configure_recalc`      |
    |                                                     | :c:func:`clock_mux_configure_recalc`  |
    |                                                     | :c:func:`clock_mux_validate_parent`   |
    |                                                     | :c:func:`clock_root_configure_recalc` |
    +-----------------------------------------------------+---------------------------------------+
    | :kconfig:option:`CONFIG_CLOCK_MANAGEMENT_SET_RATE`  | :c:func:`clock_set_rate`              |
    |                                                     | :c:func:`clock_round_rate`            |
    |                                                     | :c:func:`clock_root_round_rate`       |
    |                                                     | :c:func:`clock_root_set_rate`         |
    |                                                     | :c:func:`clock_set_parent`            |
    +-----------------------------------------------------+---------------------------------------+

All API functions associated with
:kconfig:option:`CONFIG_CLOCK_MANAGEMENT_RUNTIME`
**must** be implemented by each clock driver, but they should be compiled out
when runtime features are disabled. Clock drivers should implement API functions
associated with :kconfig:option:`CONFIG_CLOCK_MANAGEMENT_SET_RATE` whenever
possible, but it is not required.

Clock drivers will **must** hold a reference to their parent clock device, if
one exists. And **must** not reference their child clock devices directly.

These constraints are required because the clock subsystem determines which clock
devices can be discarded from the build at link time based on which clock devices
are referenced. If a parent clock is not referenced, that clock and any of its
parents would be discarded. However if a child clock is directly referenced,
that child clock would be linked in regardless of if a consumer was actually
using it.

Clock consumers hold references to the clock output nodes they are using, which
then reference their parent clock producers, which in turn reference their
parents. These reference chains allow the clock management subsystem to only
link in the clocks that the application actually needs.

Shared Clock Data
-----------------

All multiplexer and standard clocks **must** define shared data as the first
section of their device-specific data structure. This data is stored within the
same pointer to conserve flash resources. Drivers can define the shared data like
so for standard clocks:

.. code-block:: c

    struct vnd_clock_driver_data {
        STANDARD_CLK_SUBSYS_DATA_DEFINE
        /* Vendor specific data */
        ...
    };

Multiplexer clocks use a similar macro:


.. code-block:: c

    struct vnd_mux_driver_data {
        MUX_CLK_SUBSYS_DATA_DEFINE
        /* Vendor specific data */
        ...
    };

The driver should then initialize the shared data within the driver macros
using the macros :c:macro:`STANDARD_CLK_SUBSYS_DATA_INIT` and
:c:macro:`MUX_CLK_SUBSYS_DATA_INIT` respectively.

Defining Clock Structures
-------------------------

Clock structures may be defined with :c:macro:`CLOCK_DT_INST_DEFINE` or
:c:macro:`CLOCK_DT_DEFINE`. Usage of this macro is very similar to the
:c:macro:`DEVICE_DT_DEFINE`. Clocks are defined as :c:struct:`clk` structures
instead of as :c:struct:`device` structures in order to reduce the flash impact
of the framework.

For root clocks, the macros :c:macro:`ROOT_CLOCK_DT_INST_DEFINE` or
:c:macro:`ROOT_CLOCK_DT_DEFINE` should be used. Similarly, multiplexer clocks
should use :c:macro:`MUX_CLOCK_DT_INST_DEFINE` or
:c:macro:`MUX_CLOCK_DT_DEFINE`.

See below for a simple example of defining a standard clock structure:

.. code-block:: c

    #define DT_DRV_COMPAT vnd_clock

    struct vnd_clock_driver_data {
        STANDARD_CLK_SUBSYS_DATA_DEFINE
        uint32_t *reg;
    };

    ...
    /* API implementations */
    ...

    const struct clock_management_standard_api vnd_clock_api = {
        ...
    };

    #define VND_CLOCK_DEFINE(inst)                                           \
      const struct vnd_clock_driver_data clock_data_##inst = {               \
        STANDARD_CLK_SUBSYS_DATA_INIT(CLOCK_DT_GET(DT_INST_PARENT(inst)))    \
      };                                                                     \
      CLOCK_DT_INST_DEFINE(inst,                                             \
                           &clock_data_##inst,                               \
                           &vnd_clock_api);

    DT_INST_FOREACH_STATUS_OKAY(VND_CLOCK_DEFINE)

Clock Node Specifier Data
-------------------------

Clock nodes in devicetree will define a set of specifiers with their DT binding,
which are used to configure the clock directly. When an application references a
clock node, the clock management subsystem expects the following macros to be
defined:

* ``Z_CLOCK_MANAGEMENT_DATA_DEFINE_<compatible>``: defines any static structures
  needed by this clock node (IE a C structure)

* ``Z_CLOCK_MANAGEMENT_DATA_GET_<compatible>``: gets a reference to any static
  structure defined by the ``DATA_DEFINE`` macro. This is used to initialize the
  ``void *`` passed to :c:func:`clock_configure`, so for many clock nodes this
  macro can simply expand to an integer value (which may be used for a register
  setting)

Where ``<compatible>`` is the compatible of the clock node being referenced.

As an example, for the following devicetree:

.. code-block:: devicetree

    clock_source: clock-source {
        compatible = "fixed-clock";
        clock-frequency = <10000000>;
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

    &clock_output {
        clock_output_state_default: clock-output-state-default {
            compatible = "clock-state";
            clocks = <&clock_div 1>;
            clock-frequency = <DT_FREQ_M(10)>
        };
    }

    ....

    periph0: periph@0 {
        /* Clock outputs */
        clock-outputs= <&clock_output>;
        clock-output-names = "default";
        /* Default clock state */
        clock-state-0 = <&clock_output_state_default>;
        clock-state-names = "default";
    };

The clock subsystem would expect the following macros be defined:

* ``Z_CLOCK_MANAGEMENT_DATA_DEFINE_vnd_clock_div``
* ``Z_CLOCK_MANAGEMENT_DATA_GET_vnd_clock_div``

These macros should be defined within a header file. The header file can then
be added to the list of clock management driver headers to include using the
CMake function ``add_clock_management_header`` or ``add_clock_management_header_ifdef``.

Output Clock Nodes
------------------

Clock trees should define output clock nodes as leaf nodes within their
devicetree. These nodes must have the compatible :dtcompatible:`clock-output`,
and are the nodes which clock consumers will reference. The clock management
framework will handle defining clock drivers for each of these nodes.

Common Clock Drivers
--------------------

For some common clock nodes, generic drivers already exist to simplify vendor
implementations. For a list, see the table below:


.. table:: Common Clock Drivers
    :align: center

    +-------------------------------------+--------------------------------------------+
    | DT compatible                       | Use Case                                   |
    +-------------------------------------+--------------------------------------------+
    | :dtcompatible:`fixed-clock`         | Fixed clock sources that cannot be gated   |
    +-------------------------------------+--------------------------------------------+
    | :dtcompatible:`clock-source`        | Gateable clock sources with a fixed rate   |
    +-------------------------------------+--------------------------------------------+

Implementation Guidelines
-------------------------

Implementations of each clock driver API will be vendor specific, but some
general guidance on implementing each API is provided below:

* :c:func:`clock_configure`

  * Cast the ``void *`` provided in the API call to the data type this clock
    driver uses for configuration.
  * Reconfigure the clock by writing to device-specific registers.

* :c:func:`clock_onoff`

  * Power the clock on or off depending on the argument provided from the clock
    framework


* :c:func:`clock_get_rate` (root clocks only)

  * Sources will likely return a fixed rate, or 0 if the source is gated. For
    fixed sources, see :dtcompatible:`fixed-clock`.
  * Generic drivers can generally be used, unless a device-specific method is
    needed to power down the source clock.
  * Drivers can return ``-ENOTCONN`` if their hardware is not setup, which the
    clock framework will intepret as a rate of zero.

* :c:func:`clock_recalc_rate` (standard clocks only)

  * Read device specific registers to recalculate the clock frequency versus
    the provided parent frequency
  * Drivers can return ``-ENOTCONN`` if their hardware is not setup, which the
    clock framework will intepret as a rate of zero.
  * Any other error value indicates that the clock has rejected the parent
    rate, and will cause the clock framework to mark this clock as not usable
    for the current clock request being serviced.


* :c:func:`clock_get_parent` (multiplexer clocks only)

  * Read device specific registers to determine the parent clock index. Clocks
    can return ``-ENOTCONN`` to indicate their hardware is not setup, and that
    they are effectively disconnected.


* :c:func:`clock_configure_recalc` (standard clocks only)

  * Report the frequency that the clock would produce for the provided parent
    rate if the ``void *`` provided as a clock driver configuration was used
    with :c:func:`clock_configure`

* :c:func:`clock_root_configure_recalc` (root clocks only)

  * Report the frequency that the clock would produce if the ``void *``
    provided as a clock driver configuration was used with
    :c:func:`clock_configure`

* :c:func:`clock_mux_configure_recalc` (multiplexer clocks only)

  * Report the parent index that the clock would use if the ``void *``
    provided as a clock driver configuration was used with
    :c:func:`clock_configure`

* :c:func:`clock_mux_validate_parent` (multiplexer clocks only)

  * Return 0 if and only if the provided parent frequency and index are
    acceptable for the multiplexer, otherwise return an error.


* :c:func:`clock_root_round_rate` (root clocks only)

  * Return the closest frequency the root clock can produce for the given request.

* :c:func:`clock_root_set_rate` (root clocks only)

  * Set and return the closest frequency the root clock can produce for the given request.


* :c:func:`clock_round_rate` (standard clocks only)

  * Return the closest frequency the root clock can produce for the given
    requested frequency if the clock is provided with the given parent rate

* :c:func:`clock_set_rate` (standard clocks only)

  * Set and return the closest frequency the root clock can produce for the
    given requested frequency using the provided parent rate

* :c:func:`clock_set_parent` (multiplexer clocks only)

  * Set the multiplexer to use the parent at the provided index in the multiplexer
    parent clock array

Clock Driver Helpers
====================

In some cases, clock drivers need to call into the clock management subsystem
in order to properly support the clock driver API. Examples include the
following:

* Clock which needs to request a specific frequency from its parent in order
  to produce the frequency the framework is requesting
* Clock which needs to directly access the frequency of another clock in the
  system, which may not be its parent
* Clock which must gate to reconfigure, and needs to validate this is safe with
  its consumers

For cases like this the subsystem provides "clock helper" APIs. These APIs
should be used sparingly, but are available for cases where the generic
clock tree management code won't suffice.

.. note::

   Clock drivers should **never** directly call clock driver APIs, they should
   always pass through clock helper APIs. This insures that clock consumers are
   properly notified of rate changes, and that these rate changes are validated
   appropriately.

The clock helper API is documented below:

.. doxygengroup:: clock_driver_helpers

.. _clock_driver_api:

Clock Driver API
================

.. doxygengroup:: clock_driver_interface

.. _clock_model_api:

Clock Model API
===============

.. doxygengroup:: clock_model

.. _clock_subsystem_operation:

Clock Management Subsystem Operation
************************************

The below section is intended to provide an overview of how the clock management
subsystem operates, given a hypothetical clock tree and clock consumers. For the
purpose of this example, consider a clock tree for a UART clock output, which
sources its clock from a divider. This divider's input is a multiplexer, which
can select between a fixed internal clock or external clock input. Two UART
devices use this clock output as their clock source. A topology like this might
be described in devicetree like so:

.. code-block:: devicetree

    fixed_source: fixed-source {
        compatible = "fixed-clock";
        clock-frequency = <DT_FREQ_M(10)>;

        fixed-output: fixed-output {
            compatible = "clock-output";
            #clock-cells = <0>;
        };
    };

    external_osc: external-osc {
        compatible = "fixed-clock";
        /* User's board can override this
         * based on oscillator they use */
        clock-frequency = <0>;
    };

    uart_mux: uart-mux@40001000 {
        compatible = "vnd,clock-mux";
        reg = <0x40001000>
        #clock-cells = <1>;
        input-sources = <&fixed_source &external_osc>;

        uart_div: uart-div@40001200 {
            compatible = "vnd,clock-div";
            #clock-cells = <1>;
            reg = <0x40001200>;

            uart_output: clock-output {
                compatible = "clock-output";
                #clock-cells = <0>;
            };
        };
    };

    uart_dev: uart-dev {
        compatible = "vnd,uart-dev";
        clock-outputs = <&uart_output>;
        clock-output-names = "default";
    };

    uart_dev2: uart-dev {
        compatible = "vnd,uart-dev";
        clock-outputs = <&uart_output>;
        clock-output-names = "default";
    };

At the board level, a frequency will be defined for the external clock.
Furthermore, states for the UART clock output will be defined, and assigned
to the first UART device:

.. code-block:: devicetree

    &uart_output {
        uart_default: uart-default {
            compatible = "clock-state";
            /* Select external source, divide by 4 */
            clocks = <&uart_mux 1 &uart_div 4>;
            clock-frequency = <DT_FREQ_M(4)>;
        };
    };

    &external_osc {
        clock-frequency = <DT_FREQ_M(16)>;
    };

    &uart_dev {
        clock-state-0 = <&uart_default>;
        clock-state-names = "default";
    };


Now, let's consider some examples of how consumers would interact with the
clock management subsystem.

Reading Clock Rates
===================

Reading a clock rate involves walking up the clock tree to find the root clock,
reading the root clock's rate, and then walking back down the tree to calculate
the final rate. This follows the following process:

* Starting from the clock output node, call :c:func:`clock_get_parent` on each
  multiplexer node to find the parent clock until a root clock is found.
* Read the root clock's rate with :c:func:`clock_get_rate`.
* Walk back down the clock tree, calling :c:func:`clock_recalc_rate` on each
  standard clock node to calculate the final rate.
* If :kconfig:option:`CONFIG_CLOCK_MANAGEMENT_RUNTIME` is enabled, cache the
  calculated rates at the output node to improve performance on future reads.

If the user requested a rate for the ``uart_output``, the call tree might
look like so:

.. graphviz::

   digraph G {
     # Set global styles
     fontname="Helvetica,Arial,sans-serif";
     node [fontname="Helvetica,Arial,sans-serif",align="left"];
     edge [fontname="Helvetica,Arial,sans-serif"];

     fontsize=40;
     label="Reading Clock Rates";
     labelloc=t;

     {
         # Nodes to describe components of clock tree (producers)
         node [style=filled, fillcolor=cyan2, shape=ellipse];
         fixed_source;
         uart_mux;
         uart_div;
         uart_div2 [label="uart_div"]
     }
     {
         # Nodes to describe consumers
         node [style=filled, fillcolor=gold, shape=ellipse];
         uart_output;
     }
     {
         # Other nodes are used to describe the calls that act on objects
         node [style="filled,rounded", fillcolor=deepskyblue, shape=rect, height=0.5, margin="0.3,0"];
         clock_management_get_rate;
         "Read parent from clock struct";
         clock_get_parent;
         clock_get_rate;
         clock_management_clk_rate_0 [label="clock_management_clk_rate"];
         clock_management_clk_rate_1 [label="clock_management_clk_rate"];
         clock_management_clk_rate_2 [label="clock_management_clk_rate"];
         clock_recalc_rate_0 [label="clock_recalc_rate"];
     }
     {rank=same; uart_output->clock_management_get_rate->clock_management_clk_rate_0
        [minlen=3, label="walk up tree"];}
     {rank=same; clock_management_clk_rate_0->clock_management_clk_rate_1->clock_management_clk_rate_2 [minlen=3, label="walk up tree"];}
     {rank=same; clock_management_clk_rate_2->clock_management_clk_rate_1->clock_management_clk_rate_0 [minlen=3, label="return rate"];}
     {rank=same; clock_management_clk_rate_0->clock_management_get_rate->uart_output
        [minlen=3, label="return rate"];}

     clock_management_clk_rate_0->"Read parent from clock struct"->uart_div;
     clock_management_clk_rate_1->clock_get_parent->uart_mux;
     clock_management_clk_rate_2->clock_get_rate->fixed_source;
     clock_management_clk_rate_0->clock_recalc_rate_0->uart_div2;

     # Lengend for the graph
     subgraph legend_pad {
         cluster=true;
         margin=20;
         pencolor=white;
         fontsize=20;
         label=""
         subgraph legend {
             cluster=true;
             pencolor=black;
             label="Legend";
             "Clock Consumers" [style=filled, fillcolor=gold, shape=ellipse];
             "Clock Management Subsystem" [style="filled, rounded", fillcolor=deepskyblue, shape=rect,
                                         height=0.5, margin="0.3,0"];
             "Clock Producers" [style=filled, fillcolor=cyan2, shape=ellipse];
             "Clock Consumers"->"Clock Management Subsystem"->"Clock Producers" [color=white];
         }
     }
   }

Applying Clock States
=====================

When a consumer applies a clock state, the following will happen for each
clock node specified by the states ``clocks`` property:

* :c:func:`clock_configure_recalc` (or the multiplexer/root clock specific
  variant) will be called on the target clock to determine the rate the clock
  will produce.
* ``clock_notify_children`` will be called to validate that all children
  can accept the new rate.
* If either of these checks fail, the state application will fail and an error
  will be returned to the consumer.
* Otherwise, :c:func:`clock_configure` will be called on the clock node with the
  vendor specific data given by that node's specifier
* ``clock_notify_children`` will be called again to notify children of the
  rate change.

This call chain looks like so:

.. graphviz::

   digraph G {
    # Set global styles
    fontname="Helvetica,Arial,sans-serif";
    node [fontname="Helvetica,Arial,sans-serif"];
    edge [fontname="Helvetica,Arial,sans-serif"];
    rankdir="LR";

    fontsize=40;
    label="Applying a Clock State";
    labelloc=t;

    {
        # Nodes to describe components of clock tree (producers)
        node [style=filled, fillcolor=cyan2, shape=ellipse];
        uart_mux;
        uart_div;
    }
    {
        # Nodes to describe consumers
        node [style=filled, fillcolor=gold, shape=ellipse];
        "uart driver";
    }
    {
        # Other nodes are used to describe the calls that act on objects
        node [style="filled,rounded", fillcolor=deepskyblue, shape=rect, height=0.5, margin="0.3,0"];
        clock_tree_configure_mux [label="clock_tree_configure"];
        clock_tree_configure_div [label="clock_tree_configure"];
        "uart driver"->"clock_management_apply_state"->"clock_apply_state";
        "clock_apply_state"->clock_tree_configure_mux;
    }
    subgraph mux_apply {
        label="uart_mux configuration";
        # Other nodes are used to describe the calls that act on objects
        node [style="filled,rounded", fillcolor=deepskyblue, shape=rect, height=0.5, margin="0.3,0"];
        fontsize=20;
        cluster=true;
        style=rounded;
        clock_tree_configure_mux->"clock_mux_configure_recalc"->uart_mux;
        clock_tree_notify_pre_mux [label="clock_notify_children"];
        clock_tree_configure_mux->clock_tree_notify_pre_mux;
        clock_configure_mux [label="clock_configure"];
        clock_tree_configure_mux->clock_configure_mux->uart_mux;
        clock_tree_notify_post_mux [label="clock_notify_children"];
        clock_tree_configure_mux->clock_tree_notify_post_mux;
    }


    {
        node [style="filled,rounded", fillcolor=deepskyblue, shape=rect, height=0.5, margin="0.3,0"];
        "clock_apply_state"->clock_tree_configure_div;
    }
    subgraph div_apply {
        label="uart_div configuration";
        # Other nodes are used to describe the calls that act on objects
        node [style="filled,rounded", fillcolor=deepskyblue, shape=rect, height=0.5, margin="0.3,0"];
        fontsize=20;
        cluster=true;
        style=rounded;
        clock_tree_configure_div->"clock_configure_recalc"->uart_div;
        clock_tree_notify_pre_div [label="clock_notify_children"];
        clock_tree_configure_div->clock_tree_notify_pre_div;
        clock_configure_div [label="clock_configure"];
        clock_tree_configure_div->clock_configure_div->uart_div;
        clock_tree_notify_post_div [label="clock_notify_children"];
        clock_tree_configure_div->clock_tree_notify_post_div;
    }

    # Lengend for the graph
    subgraph legend_pad {
        cluster=true;
        margin=20;
        pencolor=white;
        fontsize=20;
        label=""
        subgraph legend {
            cluster=true;
            pencolor=black;
            label="Legend";
            "Clock Consumers" [style=filled, fillcolor=gold, shape=ellipse];
            "Clock Management Subsystem" [style="filled, rounded", fillcolor=deepskyblue, shape=rect,
                                        height=0.5, margin="0.3,0"];
            "Clock Producers" [style=filled, fillcolor=cyan2, shape=ellipse];
            "Clock Consumers"->"Clock Management Subsystem"->"Clock Producers" [color=white];
        }
    }
  }

Requesting Runtime Rates
========================

When requesting a clock rate, the consumer will either apply a pre-defined state
using :c:func:`clock_configure` if a pre-defined state satisfies the clock
request, or runtime rate resolution will be used (if
:kconfig:option:`CONFIG_CLOCK_MANAGEMENT_SET_RATE` is enabled).

For runtime rate resolution, there are two phases: querying the best clock setup
and applying it.

During the query phase, the clock subsystem will walk up the clock tree until it
reaches a root clock. Once a root clock is reached, the rate it offers via
:c:func:`clock_root_round_rate` will be offered as the parent rate to its child
clock when calling :c:func:`clock_round_rate`. Multiplexers have this support
implemented generically, via a function that selects the best rate offered by
all of the multiplexer parents. Proposed parent rates are validated with clock
children via :c:func:`clock_recalc_rate`, or multipexers via
:c:func:`clock_mux_validate_parent`.

In the application phase, the clock subsystem will once again walk up the clock
tree, but now clock settings will be applied using
:c:func:`clock_root_set_rate`, :c:func:`clock_set_rate` and
:c:func:`clock_set_parent`.

Clock ranking is performed within the muliplexer query phase. Clocks may either
be ranked based on their ability to satisfy a frequency request (best accuracy
clock returned) or their rank factor (input with lowest calculated rank factor
that fits within frequency constraints selected).

Note that if no clocks fit within the provided constraint set, a "best effort"
clock will be returned, IE the clock that was closest to the maximum frequency
in the constaint set.  This is done so that clocks higher in the clock tree will
still be selected optimimally, even if dividers or multipliers which source them
are needed to satisfy the clock constraints.

The call chain of a runtime rate request might look like so (note that in
this example, ``external_osc`` produces a better rate match than
``fixed_source``):

.. graphviz::

   digraph G {
     # Set global styles
     fontname="Helvetica,Arial,sans-serif";
     node [fontname="Helvetica,Arial,sans-serif",align="left"];
     edge [fontname="Helvetica,Arial,sans-serif"];

     fontsize=40;
     label="Runtime Rate Request";
     labelloc=t;

     {
         # Nodes to describe components of clock tree (producers)
         node [style=filled, fillcolor=cyan2, shape=ellipse];
         fixed_source;
         external_osc;
         uart_mux;
         uart_div;
     }
     {
         # Nodes to describe consumers
         node [style=filled, fillcolor=gold, shape=ellipse];
         uart_output;
     }
     {
         # Other nodes are used to describe the calls that act on objects
         node [style="filled,rounded", fillcolor=deepskyblue, shape=rect, height=0.5, margin="0.3,0"];
         clock_management_req_rate;
         clock_management_apply_state;
         "Read parent from clock struct";
         read_parent_2 [label="Read parent from clock struct"];
         clock_round_rate;
         clock_management_best_parent;
         clock_management_round_internal1 [label="clock_management_round_internal"];
         clock_management_round_internal2 [label="clock_management_round_internal"];
         clock_management_round_internal3 [label="clock_management_round_internal"];
         clock_root_round_rate;
         clock_root_round_rate2 [label="clock_root_round_rate"];
         clock_management_set_internal1 [label="clock_management_set_internal"];
         clock_set_rate;
         clock_set_parent;
         clock_root_set_rate;
     }
     {
         # Ranked the same so that the splitter node doesn't mess up the alignment of these nodes
         rank=same;
         node [style="filled,rounded", fillcolor=deepskyblue, shape=rect, height=0.5, margin="0.3,0"]
         clock_management_apply_state;
         clock_management_round_internal0 [label="clock_management_round_internal"];
         clock_management_set_internal0 [label="clock_management_set_internal"];
     }
     {
         # Other nodes are used to describe the calls that act on objects
         node [shape=plaintext];
         "Static state fits constraints?";
     }
     uart_output->clock_management_req_rate->"Static state fits constraints?";
     "Static state fits constraints?"->clock_management_apply_state [label="yes"];
     # Hidden node to split the arrow after the "no"
     splitter[shape=point, style="invis"]
     "Static state fits constraints?"->splitter [label="no"];
     splitter->clock_management_round_internal0;
     splitter->clock_management_set_internal0;
     clock_management_round_internal0->"Read parent from clock struct"->uart_div;
     clock_management_round_internal0->clock_management_round_internal1 [dir=both, minlen=2];
     clock_management_round_internal0->clock_round_rate->uart_div;
     clock_management_round_internal1->clock_management_best_parent->uart_mux;
     clock_management_best_parent->clock_management_round_internal2 [dir=both, minlen=2];
     clock_management_best_parent->clock_management_round_internal3 [dir=both, minlen=2];
     clock_management_round_internal2->clock_root_round_rate->fixed_source;
     clock_management_round_internal3->clock_root_round_rate2->external_osc;
     clock_management_set_internal0->read_parent_2->uart_div;
     clock_management_set_internal0->clock_management_set_internal1 [dir=both, minlen=2];
     clock_management_set_internal0->clock_set_rate->uart_div;
     clock_management_set_internal1->clock_management_best_parent [minlen=3];
     clock_management_set_internal1->clock_root_set_rate->external_osc;
     clock_management_set_internal1->clock_set_parent->uart_mux;

     # Lengend for the graph
     subgraph legend_pad {
         cluster=true;
         margin=20;
         pencolor=white;
         fontsize=20;
         label=""
         subgraph legend {
             cluster=true;
             pencolor=black;
             label="Legend";
             "Clock Consumers" [style=filled, fillcolor=gold, shape=ellipse];
             "Clock Management Subsystem" [style="filled, rounded", fillcolor=deepskyblue, shape=rect,
                                         height=0.5, margin="0.3,0"];
             "Clock Producers" [style=filled, fillcolor=cyan2, shape=ellipse];
             "Clock Consumers"->"Clock Management Subsystem"->"Clock Producers" [color=white];
         }
     }
   }

Clock Notifications
===================

Clock notifications are a critical part of the clock management subsystem. They
allow clocks to validate and notify their consumers of rate changes. There are
three types of notifications: query, pre-change, and post-change. Query
notifications are used to validate that a clock can accept a proposed rate.
These notifications are sent before a clock is reconfigured, and are not
passed to clock callbacks. Instead the framework will automatically reject
the rate change if it violates constraints set by consumers. Pre-change
notifications are sent to consumers before a clock is reconfigured, and allow
consumers to prepare for the rate change. Post-change notifications are sent
after a clock is reconfigured.

A call chain for clock notifications on ``fixed_source`` might look like so.
Note that the event type in use only changes how the consumer nodes at the leaf
of the tree respond.


.. graphviz::

   digraph G {
    # Set global styles
    fontname="Helvetica,Arial,sans-serif";
    node [fontname="Helvetica,Arial,sans-serif",align="left"];
    edge [fontname="Helvetica,Arial,sans-serif"];

    fontsize=40;
    label="Clock Notification Chain";
    labelloc=t;

    {
        # Nodes to describe components of clock tree (producers)
        node [style=filled, fillcolor=cyan2, shape=ellipse];
        rank=same
        uart_mux;
        uart_div;
        # Nodes to describe consumers
        node [style=filled, fillcolor=gold, shape=ellipse];
        uart_output;
        fixed_output
    }
    {
        rank=same
        node [style="filled,rounded", fillcolor=deepskyblue, shape=rect, height=0.5, margin="0.3,0"];
        clock_notify_children;
        clock_notify_children1 [label="clock_notify_children"];
        clock_notify_children2 [label="clock_notify_children"];
    }
    {
        rank=same
        # Other nodes are used to describe the calls that act on objects
        node [style="filled,rounded", fillcolor=deepskyblue, shape=rect, height=0.5, margin="0.3,0"];
        clock_get_parent;
        clock_mux_validate_parent;
        clock_recalc_rate;

    }
    {
        rank=same
        node [style="filled,rounded", fillcolor=deepskyblue, shape=rect, height=0.5, margin="0.3,0"];
        clock_get_parent;
        clock_mux_validate_parent;
    }
    {
        node [shape=plaintext];
        "Check if parent is connected";
    }

    clock_notify_children->"Check if parent is connected"->clock_get_parent->uart_mux;
    clock_notify_children->clock_mux_validate_parent->uart_mux;
    clock_notify_children->fixed_output;
    clock_notify_children->clock_notify_children1->clock_recalc_rate->uart_div;
    clock_notify_children1->clock_notify_children2->uart_output [minlen=2];


    # Lengend for the graph
    subgraph legend_pad {
        cluster=true;
        margin=20;
        pencolor=white;
        fontsize=20;
        label=""
        subgraph legend {
            cluster=true;
            pencolor=black;
            label="Legend";
            "Clock Consumers" [style=filled, fillcolor=gold, shape=ellipse];
            "Clock Management Subsystem" [style="filled, rounded", fillcolor=deepskyblue, shape=rect,
                                        height=0.5, margin="0.3,0"];
            "Clock Producers" [style=filled, fillcolor=cyan2, shape=ellipse];
            "Clock Consumers"->"Clock Management Subsystem"->"Clock Producers" [color=white];
        }
    }
  }

Runtime Clock Resolution
========================

The clock management subsystem will automatically calculate the combined
frequency constraint imposed on a clock output by all its consumers when
:kconfig:option:`CONFIG_CLOCK_MANAGEMENT_RUNTIME` is enabled. When a parent
clock is attempting to reconfigure, the clock management subystem wil verify the
new frequency fits within the consumers' constraints automatically, so clock
consumers do not need to handle this case. For the case below, assume that
``uart_output`` has already received a request that sets its frequency
constraints.

.. graphviz::

   digraph G {
    # Set global styles
    fontname="Helvetica,Arial,sans-serif";
    node [fontname="Helvetica,Arial,sans-serif",align="left"];
    edge [fontname="Helvetica,Arial,sans-serif"];

    fontsize=40;
    label="Clock Rate Rejected";
    labelloc=t;

    {
        # Nodes to describe components of clock tree (producers)
        node [style=filled, fillcolor=cyan2, shape=ellipse];
        rank=same
        uart_mux;
        uart_div;
        # Nodes to describe consumers
        node [style=filled, fillcolor=gold, shape=ellipse];
        uart_output;
        fixed_output
    }
    rejectfinal [label="Reject rate", fontcolor="red", shape=plaintext, fillcolor=white];
    {
        rank=same
        node [style="filled,rounded", fillcolor=deepskyblue, shape=rect, height=0.5, margin="0.3,0"];
        "Clock validates rate";
        clock_notify_children;
        clock_notify_children1 [label="clock_notify_children"];
        clock_notify_children2 [label="clock_notify_children"];
    }
    {
        rank=same
        # Other nodes are used to describe the calls that act on objects
        node [style="filled,rounded", fillcolor=deepskyblue, shape=rect, height=0.5, margin="0.3,0"];
        clock_get_parent;
        clock_mux_validate_parent;
        clock_recalc_rate;
    }
    {
        rank=same
        node [style="filled,rounded", fillcolor=deepskyblue, shape=rect, height=0.5, margin="0.3,0"];
        clock_get_parent;
        clock_mux_validate_parent;
    }
    {
        rank=same;
        node [shape=plaintext];
        "Check if parent is connected";
        reject1 [label="Reject rate", fontcolor="red"];
        reject2 [label="Reject rate", fontcolor="red"];
        reject3 [label="Reject rate", fontcolor="red"];
    }

    clock_notify_children->rejectfinal->"Clock validates rate";
    "Clock validates rate"->clock_notify_children;
    clock_notify_children->"Check if parent is connected"->clock_get_parent->uart_mux;
    clock_notify_children->clock_mux_validate_parent->uart_mux;
    clock_notify_children->fixed_output;
    clock_notify_children->clock_notify_children1->clock_recalc_rate->uart_div;
    clock_notify_children1->clock_notify_children2->uart_output [minlen=2];
    uart_output->reject1->clock_notify_children2 [constraint=false];
    clock_notify_children2->reject2->clock_notify_children1;
    clock_notify_children1->reject3->clock_notify_children;



    # Lengend for the graph
    subgraph legend_pad {
        cluster=true;
        margin=20;
        pencolor=white;
        fontsize=20;
        label=""
        subgraph legend {
            cluster=true;
            pencolor=black;
            label="Legend";
            "Clock Consumers" [style=filled, fillcolor=gold, shape=ellipse];
            "Clock Management Subsystem" [style="filled, rounded", fillcolor=deepskyblue, shape=rect,
                                        height=0.5, margin="0.3,0"];
            "Clock Producers" [style=filled, fillcolor=cyan2, shape=ellipse];
            "Clock Consumers"->"Clock Management Subsystem"->"Clock Producers" [color=white];
        }
    }
  }

Note that each clock output starts with no constraints set. A consumer must
make a request to enforce a constraint. A consumer may modify a constraint it
has set by requesting a new constraint, which may be less restrictive than
the original setting.

If two clock consumers share a clock output node, and both make conflicting
requests to the clock output, the first consumer to make a request will be
given priority, and the second will be rejected.
