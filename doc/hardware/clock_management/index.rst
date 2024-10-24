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
clock management API, and the internal clock driver API. The clock driver API is
used by clock producers to query and set rates of their parent clocks, as well
as receive reconfiguration notifications when the state of the clock tree
changes. Each clock producer must implement the clock driver API, but clock
consumers should only interact with clocks using the clock management API.

This approach is required because clock consumers should not have knowledge of
how their underlying clock states are applied or defined, as the data is often
hardware specific. Consumers may apply states directly, or request a frequency
range, which can then be satisfied by one of the defined states. For details on
the operation of the clock subsystem, see :ref:`clock_subsystem_operation`.

Accessing Clock Outputs
***********************

In order to interact with a clock output, clock consumers must define a clock
output device. For devices defined in devicetree, using clocks defined within
their ``clock-outputs`` property, :c:macro:`CLOCK_MANAGEMENT_DT_DEFINE_OUTPUT` or
similar may be used. For software applications consuming a clock,
:c:macro:`CLOCK_MANAGEMENT_DEFINE_OUTPUT` should be used.

Clock consumers may then initialize their clock output device using
:c:macro:`CLOCK_MANAGEMENT_DT_GET_OUTPUT` or similar, for consumer devices defined in
devicetree, or :c:macro:`CLOCK_MANAGEMENT_GET_OUTPUT` for software applications
consuming a clock.

Reading Clocks
**************

Once a diver has defined a clock output and initialized it, the clock rate can
be read using :c:func:`clock_management_get_rate`. This will return the frequency of
the clock output in Hz, or a negative error value if the clock could not be
read.

Consumers can also monitor a clock output rate. To do so, the application must
first enable :kconfig:option:`CONFIG_CLOCK_MANAGEMENT_RUNTIME`. The application may
then set a callback of type :c:type:`clock_management_callback_handler_t` using
:c:func:`clock_management_set_callback`. One callback is supported per consumer.

The clock management subsystem will issue a callback directly before applying a
new clock rate, and directly after the new rate is applied. See
:c:struct:`clock_management_event` for more details.

Setting Clock States
********************

Each clock output defines a set of states. Clock consumers can set these states
directly, using :c:func:`clock_management_apply_state`. States are described in
devicetree, and are opaque to the driver/application code consuming the clock.

Each clock consumer described in devicetree can set named clock states for each
clock output. These states are described by the ``clock-state-n`` properties
present on each consumer. The consumer can access states using macros like
:c:macro:`CLOCK_MANAGEMENT_DT_GET_STATE`

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
node, which may either directly configure parent clock nodes to realize a
frequency, or simply define a frequency to request from the parent clock at
runtime (which will only function if
:kconfig:option:`CONFIG_CLOCK_MANAGEMENT_SET_RATE` is enabled).

.. code-block:: devicetree

    &clock_output {
        clock_output_state_default: clock-output-state-default {
            compatible = "clock-state";
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
            /* Will issue runtime frequency request */
            clock-frequency = <DT_FREQ_M(1)>;
        };
    };

Note that the specifier cells for each clock node within a state are device
specific. These specifiers allow configuration of the clock element, such as
setting a divider's division factor or selecting an output for a multiplexer.

Clock consumers will then reference the clock output nodes and their states in
order to define and access clock producers, and apply states. A peripheral clock
consumer's devicetree might look like so:

.. code-block:: devicetree

    periph0: periph@0 {
        compatible = "vnd,mydev";
        /* Clock outputs */
        clock-outputs= <&clock_output>;
        clock-output-names = "default";
        /* Default clock state */
        clock-state-0 = <&clock_output_state_default>;
        /* Sleep state */
        clock-state-1 = <&clock_output_state_sleep>;
        clock-state-names = "default", "sleep";
    };

Requesting Clock Rates
======================

In some applications, the user may not want to directly configure clock nodes
within their devicetree. The clock management subsystem allows applications to
request a clock rate directly as well, by using :c:func:`clock_management_req_rate`.
If any states satisfy the frequency range request, the first state that fits the
provided constraints will be applied. Otherwise if
:kconfig:option:`CONFIG_CLOCK_MANAGEMENT_SET_RATE` is set, the clock management
subsystem will perform runtime calculations to apply a rate within the requested
range. If runtime rate calculation support is disabled, the request will fail if
no defined states satisfy it.

No guarantees are made on how accurate a resulting rate will be versus the
requested value.

Gating Unused Clocks
====================

When :kconfig:option:`CONFIG_CLOCK_MANAGEMENT_RUNTIME` is enabled, it is possible to
gate unused clocks within the system, by calling
:c:func:`clock_management_disable_unused`.

Locking Clock States and Requests
=================================

When :kconfig:option:`CONFIG_CLOCK_MANAGEMENT_RUNTIME` is enabled, requests issued
via :c:func:`clock_management_req_rate` to the same clock by different consumers will
be aggregated to form a "combined" request for that clock. This means that a
request may be denied if it is incompatible with the existing set of aggregated
clock requests. Clock states do not place a request on the clock they configure
by default- if a clock state should "lock" the clock to prevent the frequency
changing, it should be defined with the ``locking-state`` boolean property.
This can be useful for critical system clocks, such as the core clock.

Generally when multiple clocks are expected to be reconfigured at runtime,
:kconfig:option:`CONFIG_CLOCK_MANAGEMENT_RUNTIME` should be enabled to avoid
unexpected rate changes for consumers. Otherwise clock states should be defined
in such a way that each consumer can reconfigure itself without affecting other
clock consumers in the system.


Driver Usage
************

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

Although consumers interact with the clock management subsystem via the
:ref:`clock_management_api`, producers must implement the clock driver API. This
API allows producers to read and set their parent clocks, as well as receive
reconfiguration notifications if their parent changes rate.

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

    +-----------------------------------------------------+----------------------------+
    | Kconfig                                             | API Functions              |
    +-----------------------------------------------------+----------------------------+
    | :kconfig:option:`CONFIG_CLOCK_MANAGEMENT_RUNTIME`   | :c:func:`clock_notify`     |
    +-----------------------------------------------------+----------------------------+
    | :kconfig:option:`CONFIG_CLOCK_MANAGEMENT_SET_RATE`  | :c:func:`clock_set_rate`,  |
    |                                                     | :c:func:`clock_round_rate` |
    +-----------------------------------------------------+----------------------------+

These API functions **must** still be implemented by each clock driver, but they
can should be compiled out when these Kconfig options are not set.

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


Getting Clock Structures
------------------------

A reference to a clock structure can be obtained with :c:macro:`CLOCK_DT_GET`.
Note that as described above, this should only be used to reference the parent
clock(s) of a producer.

Defining Clock Structures
-------------------------

Clock structures may be defined with :c:macro:`CLOCK_DT_INST_DEFINE` or
:c:macro:`CLOCK_DT_DEFINE`. Usage of this macro is very similar to the
:c:macro:`DEVICE_DT_DEFINE`. Clocks are defined as :c:struct:`clk` structures
instead of as :c:struct:`device` structures in order to reduce the flash impact
of the framework.

Root clock structures (a clock without any parents) **must** be defined with
:c:macro:`ROOT_CLOCK_DT_INST_DEFINE` or :c:macro:`ROOT_CLOCK_DT_DEFINE`. This
is needed because the implementation of :c:func:`clock_management_disable_unused`
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

* ``Z_CLOCK_MANAGEMENT_VND_CLOCK_NODE_DATA_DEFINE``: defines any static structures
  needed by this clock node (IE a C structure)

* ``Z_CLOCK_MANAGEMENT_VND_CLOCK_NODE_DATA_GET``: gets a reference to any static
  structure defined by the ``DATA_DEFINE`` macro. This is used to initialize the
  ``void *`` passed to :c:func:`clock_configure`, so for many clock nodes this
  macro can simply expand to an integer value (which may be used for a register
  setting)

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

* ``Z_CLOCK_MANAGEMENT_VND_CLOCK_DIV_DATA_DEFINE``
* ``Z_CLOCK_MANAGEMENT_VND_CLOCK_DIV_DATA_GET``

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

* :c:func:`clock_get_rate`

  * Read parent rate, and calculate rate this node will produce based on node
    specific settings.
  * Multiplexers will instead read the rate of their active parent.
  * Sources will likely return a fixed rate, or 0 if the source is gated. For
    fixed sources, see :dtcompatible:`fixed-clock`.

* :c:func:`clock_configure`

  * Cast the ``void *`` provided in the API call to the data type this clock
    driver uses for configuration.
  * Calculate the new rate that will be set after this configuration is applied.
  * Call :c:func:`clock_children_check_rate` to verify that children can accept
    the new rate. If the return value is less than zero, don't change the clock.
  * Call :c:func:`clock_children_notify_pre_change` to notify children the
    clock is about to change.
  * Reconfigure the clock.
  * Call :c:func:`clock_children_notify_post_change` to notify children the
    clock has just changed.

* :c:func:`clock_notify`

  * Read the node specific settings to determine the rate this node will
    produce, based on the clock management event provided in this call.
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
  * Call :c:func:`clock_children_check_rate` to verify that children can accept
    the new rate. If the return value is less than zero, propagate this error.
  * Multiplexers will typically implement this function by calling
    :c:func:`clock_round_rate` on all parents, and returning the best
    rate found.

* :c:func:`clock_set_rate`

  * Similar implementation to :c:func:`clock_round_rate`, but once all
    settings needed for a given rate have been applied, actually configure it.
  * Call :c:func:`clock_set_rate` on the parent clock to configure the needed
    rate.
  * Call :c:func:`clock_children_notify_pre_change` to notify children the
    clock is about to change.
  * Reconfigure the clock.
  * Call :c:func:`clock_children_notify_post_change` to notify children the
    clock has just changed.

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


    fixed_source: fixed-source {
        compatible = "fixed-clock";
        clock-frequency = <DT_FREQ_M(10)>;
    };

    external_osc: external-osc {
        compatible = "fixed-clock";
        /* User's board can override this
         * based on oscillator they use */
        clock-frequency = <0>;
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
            clocks = <&uart_div 4 &uart_mux 1>;
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

To read a clock rate, the clock consumer would first call
:c:func:`clock_management_get_rate` on its clock output structure. In turn, the clock
management subsystem would call :c:func:`clock_get_rate` on the parent of the
clock output. The implementation of that driver would call
:c:func:`clock_get_rate` on its parent. This chain of calls would continue until
a root clock was reached. At this point, each clock would necessary calculations
on the parent rate before returning the result. These calls would look like so:

.. figure:: images/read-rate.svg

Applying Clock States
=====================

When a consumer applies a clock state, :c:func:`clock_configure` will be called
on each clock node specified by the states ``clocks`` property with the vendor
specific data given by that node's specifier. These calls would look like so:

.. figure:: images/apply-state.svg

Requesting Runtime Rates
========================

When requesting a clock rate, the consumer will either apply a pre-defined state
using :c:func:`clock_configure` if a pre-defined state satisfies the clock
request, or runtime rate resolution will be used (if
:kconfig:option:`CONFIG_CLOCK_MANAGEMENT_SET_RATE` is enabled).

For runtime rate resolution, there are two phases: querying the best clock setup
using :c:func:`clock_round_rate`, and applying it using
:c:func:`clock_set_rate`. During the query phase, clock devices will report the
rate nearest to the requested value they can support. During the application
phase, the clock will actually configure to the requested rate. The call
graph for this process looks like so:

.. figure:: images/rate-request.svg

Clock Callbacks
===============

When reconfiguring, clock producers should notify their children clocks via
:c:func:`clock_notify_children`, which will call :c:func:`clock_notify` on all
children of the clock. The helpers :c:func:`clock_children_check_rate`,
:c:func:`clock_children_notify_pre_change`, and
:c:func:`clock_children_notify_post_change` are available to check that children
can support a given rate, notify them before changing to a new rate, and notify
then once a new rate is applied respectively. For the case of
:c:func:`clock_configure`, the notify chain might look like so:

.. figure:: images/clock-callbacks.svg
