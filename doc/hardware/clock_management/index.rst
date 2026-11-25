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
produces a frequency as an output. Clock states in devicetree configure
producers directly, but the clock consumer should never access producers via the
clock driver API, as this is the responsibility of the clock management
subsystem.

This approach is required because clock consumers should not have knowledge of
how their underlying clock states are applied or defined, as the data is often
hardware specific. Consumers may request states, or request a frequency, which
can then be satisfied by dynamic clock configuration. For more information on
the operation of the clock subsystem, see :ref:`clock_subsystem_operation`.

Clock Management Usage
**********************

In order to interact with the clock tree, clock consumers must define and
initialize a clock management data structure. Consumers define data structures
used by the clock management subsystem using
:c:macro:`CLOCK_MANAGEMENT_DT_DEFINE`, and get a reference to the data structure
using :c:macro:`CLOCK_MANAGEMENT_DT_GET`. This reference is passed as the first
argument to all clock management API functions.

Clock consumers may then reference their clock outputs using
:c:macro:`CLOCK_MANAGEMENT_DT_GET_OUTPUT`, which will return a
:c:type:`clock_output_t` identifier for the clock output.

Clock output identifiers can then be used with the clock management API to
interface with the clock tree, as described below.

.. note::

   When :kconfig:option:`CONFIG_LOG` is enabled, the clock management subsystem
   serializes access to the clock tree with a mutex rather than a spinlock, as
   logging may sleep. In this configuration, the clock management API must not
   be called from interrupt context.

Reading Clocks
==============

Once a driver has defined a clock output and initialized it, the clock rate can
be read using :c:func:`clock_management_get_rate`. This will return the
frequency of the clock output in Hz, or a negative error value if the clock
could not be read.

Consumers can also monitor a clock output rate. To do so, the application must
first enable :kconfig:option:`CONFIG_CLOCK_MANAGEMENT_RUNTIME`. The application may
then set a callback of type :c:type:`clock_management_callback_handler_t` using
:c:func:`clock_management_set_callback`. One callback is supported per clock
output.

The clock management subsystem will issue a callback while evaluating a new rate,
directly before applying a new clock rate, and directly after the new rate is
applied. See :c:struct:`clock_management_event` for more details.

Requesting Clock States
=======================

Clock states are statically defined configurations for a portion of the clock
tree. States are grouped into clock domains, where each clock domain defines a
set of states that configure a specific subset of the clock producers in the
system. Clock consumers can set a range of acceptable clock states using
:c:func:`clock_management_request_state`. The clock management subsystem will
then select the best state that satisfies the request within each affected clock
domain, and apply it. If no states satisfy the request, ``-ENOENT``
will be returned. States are described in devicetree, and are opaque to the
driver/application code consuming the clock.

Each clock consumer described in devicetree defines its clock requests using
the ``clock-request-n`` properties, where ``n`` is the index of the request, and
names each request with the ``clock-request-names`` property. Each name is
converted to a C identifier of the form ``CLOCK_MANAGEMENT_STATE_{NAME}``, where
``{NAME}`` is the capitalized request name. Consumers pass this identifier to
:c:func:`clock_management_request_state` to apply the request. Each consumer has
at most one active request at a time: applying a new request replaces the
consumer's previous request.

Standard requests
-----------------

The names assigned to clock requests, and the number of them, are up to the
device driver. To keep drivers consistent, a naming convention is established
for the most common use cases:

.. table:: Standardized request names
    :align: center

    +-------------+------------------------------------------+---------------------------+
    | Request     | Identifier                               | Purpose                   |
    +-------------+------------------------------------------+---------------------------+
    | ``default`` | :c:macro:`CLOCK_MANAGEMENT_STATE_DEFAULT`| Clock configuration when  |
    |             |                                          | the device is operational |
    +-------------+------------------------------------------+---------------------------+
    | ``sleep``   | :c:macro:`CLOCK_MANAGEMENT_STATE_SLEEP`  | Clock configuration when  |
    |             |                                          | the device is in a low    |
    |             |                                          | power or sleep mode       |
    +-------------+------------------------------------------+---------------------------+

Custom requests
---------------

Some drivers need requests beyond the standard ones. The driver must have in
scope a definition for each custom identifier, named
``CLOCK_MANAGEMENT_STATE_{NAME}``, *before* :c:macro:`CLOCK_MANAGEMENT_DT_DEFINE`
is expanded. For example, a request named ``bypass`` in ``clock-request-names``
requires:

.. code-block:: c

    #define CLOCK_MANAGEMENT_STATE_BYPASS (CLOCK_MANAGEMENT_STATE_PRIV_START)

.. note::

   Custom identifiers must start from :c:macro:`CLOCK_MANAGEMENT_STATE_PRIV_START`,
   and each identifier used by a consumer must have a distinct value. The
   framework looks requests up by identifier, so two names that share a value
   would resolve to whichever is listed first in devicetree.

If custom requests need to be applied from outside the driver, the identifiers
should be placed in a publicly accessible header.

Skipping requests
-----------------

Some requests are only used when a particular feature is enabled. A typical
case is the ``sleep`` request, which is only used if the device supports power
management. A request is left out of the build if a definition named
``CLOCK_MANAGEMENT_SKIP_{NAME}`` expanding to ``1`` is in scope when
:c:macro:`CLOCK_MANAGEMENT_DT_DEFINE` is expanded. For the ``sleep`` request,
the framework provides this definition unless
:kconfig:option:`CONFIG_CLOCK_MANAGEMENT_KEEP_SLEEP_STATE` is enabled; that
option defaults to enabled when :kconfig:option:`CONFIG_PM` or
:kconfig:option:`CONFIG_PM_DEVICE` is set. Requesting a skipped request returns
``-ENOENT``.

Note that clock states referenced only by a skipped request are still compiled
into their clock domain.

``clock-request-n`` properties describe the allowed states for *any* clock
domain within the system. Each entry in a request is a clock domain phandle
followed by a bitmask of the states of that domain which are acceptable for the
request. Bit ``n`` of the mask selects the ``n``-th ``clock-state`` child of the
domain, counted in the order the children are declared in devicetree. The
``CLK_STATE_n`` macros from
:zephyr_file:`include/zephyr/dt-bindings/clock-management/clock-management.h`
can be combined to build the mask. A request may list several clock domains, and
therefore affect multiple clock outputs. Clock domains are independent of the
clock outputs listed in a consumer's ``clock-outputs`` property: a consumer may
request states from a domain that configures a producer it does not directly
consume, such as a shared parent divider.

Once a set of allowed states is set for a clock domain, the clock management
framework will select the lowest "ranked" clock state that satisfies the
intersection of all allowed state requests for that clock domain.

Clock state rank is a parameter set by the user. If two clock states share
a rank, the first state defined in devicetree will be preferred.

Requesting Clock Rates
======================

In some applications, the user may want to request a specific clock rate. To
enable support for this, the user should set
:kconfig:option:`CONFIG_CLOCK_MANAGEMENT_SET_RATE`, and use the API
:c:func:`clock_management_req_rate`. The clock management subsystem will perform
runtime calculations to apply the closest clock rate possible based on the
request.

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
    };

    clock_div: clock-div@50000000 {
        compatible = "vnd,clock-div";
        input = <&clock_source>;
        #clock-cells = <1>;
        reg = <0x50000000>;

        clock_output: clock-output {
            compatible = "clock-output";
            #clock-cells = <0>;
        };
    };

Clock states are then defined within clock domain nodes, which use the
:dtcompatible:`clock-domain` compatible. Each state directly configures producer
clock nodes to realize a frequency. Clock states include a ``rank`` property,
which is used to determine which state is "best" when multiple states satisfy a
request. Lower ranks are preferred. Clock domains must set
``#clock-state-cells = <1>``, as consumers reference them with a phandle
followed by a state mask.

.. code-block:: devicetree

    / {
        clock-domains {
            clock_div_domain: clock-div-domain {
                compatible = "clock-domain";
                #clock-state-cells = <1>;

                /* CLK_STATE_0: first state declared in this domain */
                clock_div_default: clock-div-default {
                    compatible = "clock-state";
                    clocks = <&clock_div 1>;
                    rank = <1>;
                };
                /* CLK_STATE_1: second state declared in this domain */
                clock_div_sleep: clock-div-sleep {
                    compatible = "clock-state";
                    clocks = <&clock_div 5>;
                    /* Lowest rank: preferred when multiple states satisfy a request */
                    rank = <0>;
                };
            };
        };
    };

Note that the specifier cells for each clock producer within a state are device
specific. These specifiers allow configuration of the clock producer, such as
setting a divider's division factor or selecting an output for a multiplexer.

Each clock producer may only be configured by the states of a single clock
domain. This is checked at build time, and the build will fail if states in two
different clock domains configure the same clock producer. If a producer needs
to be configured on behalf of several consumers (for example a parent divider
shared by multiple peripherals), place it in its own clock domain, and have each
consumer include a state from that domain in its requests.

Clock consumers will then reference the clock output nodes to query and request
clock rates, and reference clock domains, along with a mask of allowed states,
to apply them. A peripheral clock consumer's devicetree might look like so:

.. code-block:: devicetree

    #include <zephyr/dt-bindings/clock-management/clock-management.h>

    periph0: periph@0 {
        compatible = "vnd,mydev";
        reg = <0>;
        /* Clock outputs */
        clock-outputs = <&clock_output>;
        clock-output-names = "default";
        /* Default clock state (first state declared in clock_div_domain) */
        clock-request-0 = <&clock_div_domain CLK_STATE_0>;
        /* This request allows either the default or sleep state to be applied */
        clock-request-1 = <&clock_div_domain (CLK_STATE_0 | CLK_STATE_1)>;
        clock-request-names = "default", "sleep";
    };

Only clock states selected by at least one ``clock-request-n`` property are
included in the build.

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
   consumers are not using the producer.

Gating Unused Clocks
====================

When :kconfig:option:`CONFIG_CLOCK_MANAGEMENT_RUNTIME` is enabled, it is
possible to gate unused clocks within the system, by calling
:c:func:`clock_management_disable_unused`. All clocks that do not have a
reference count set via :c:func:`clock_management_on` will be gated.

Locking Clock States and Requests
=================================

When :kconfig:option:`CONFIG_CLOCK_MANAGEMENT_RUNTIME` is enabled, consumers
may "lock" one of their clock outputs by calling :c:func:`clock_management_lock`.
This will cause any reconfiguration that would affect the clock output (including
reconfiguration of its parent producers) to be rejected with ``-EBUSY`` until
the lock is released with :c:func:`clock_management_unlock`. This can be useful
for critical clock consumers, such as the core clock, which should not be
modified by other consumers.

Clock requests can also lock clocks, by adding ``clock-request-n-locking`` to
the devicetree for the consumer making the request. When request ``n`` is
applied, the following will be locked on behalf of the consumer making the
request (provided :kconfig:option:`CONFIG_CLOCK_MANAGEMENT_RUNTIME` is enabled):

* Each clock domain referenced by the request. Requests from other consumers
  that reference a locked clock domain will fail with ``-EPERM``.
* Each clock producer configured by the state selected within those clock
  domains. Any attempt to reconfigure a locked producer, or to reconfigure one
  of its parents, will be rejected with ``-EPERM``.

The consumer that made a locking request may still issue new requests. Locks
are released when the consumer switches to a new request, and are reapplied if
the new request is also a locking request.

When :kconfig:option:`CONFIG_CLOCK_MANAGEMENT_RUNTIME` is enabled, clock state
requests will be additive. This means that if multiple consumers set a list of
allowed states for a clock domain, the clock domain will be set to the best
state that satisfies all requests.

Generally when multiple clocks are expected to be reconfigured at runtime,
:kconfig:option:`CONFIG_CLOCK_MANAGEMENT_RUNTIME` should be enabled to avoid
unexpected rate changes for consumers. Otherwise clock states should be defined
in such a way that each consumer can reconfigure itself without affecting other
clock consumers in the system.


Driver Usage
============

In order to use the clock management subsystem, a driver must define and
initialize a :c:struct:`clock_management_data` structure, which can be defined
using :c:macro:`CLOCK_MANAGEMENT_DT_DEFINE` and initialized using
:c:macro:`CLOCK_MANAGEMENT_DT_GET`. This structure is then passed to the clock
management API functions to access the clock tree.

Clock outputs can be accessed with the macro
:c:macro:`CLOCK_MANAGEMENT_DT_GET_OUTPUT`, which will return a
:c:type:`clock_output_t` identifier for the clock output. This identifier can
then be used with the clock management subsystem. Note that this macro also has
versions that allow the driver to access an output by name or index, if
multiple clocks are present within the ``clock-outputs`` property for the
device.

In order to configure a clock, the driver may either request a supported clock
rate via :c:func:`clock_management_req_rate`, or request a set of states via
:c:func:`clock_management_request_state`. For most applications,
:c:func:`clock_management_request_state` is recommended, as this allows the
application to customize the clock properties that are set using devicetree.
:c:func:`clock_management_req_rate` should only be used in cases where the
driver knows the frequency it should use.

Requests are identified by the ``CLOCK_MANAGEMENT_STATE_{NAME}`` identifiers
derived from the consumer's ``clock-request-names`` property, as described in
:ref:`clock-management-guide`. Drivers pass these identifiers directly to
:c:func:`clock_management_request_state`; no per-request field in the device
configuration is needed.

For example, consider a peripheral with a high-speed and a low-power clock
output. The ``periph_hs_clock_domain`` clock domain configures the high-speed
clock, and the ``periph_lp_clock_domain`` clock domain configures the low-power
clock. Each domain declares a default state first, followed by a sleep state.
The peripheral devicetree could be defined like so:

.. code-block:: devicetree

    periph0: periph@0 {
        compatible = "vnd,mydev";
        reg = <0>;
        /* Clock outputs */
        clock-outputs = <&periph_hs_clock &periph_lp_clock>;
        clock-output-names = "high-speed", "low-power";
        /* Moves HS clock domain to default state. LP clock domain can be in either state */
        clock-request-0 = <&periph_hs_clock_domain CLK_STATE_0
                           &periph_lp_clock_domain (CLK_STATE_0 | CLK_STATE_1)>;
        /* Moves both HS and LP clock domains to sleep state */
        clock-request-1 = <&periph_hs_clock_domain CLK_STATE_1
                           &periph_lp_clock_domain CLK_STATE_1>;
        clock-request-names = "default", "sleep";
    };

The following C code could be used to apply the ``default`` request at init,
and switch between the ``default`` and ``sleep`` requests as the device is
resumed and suspended:

.. code-block:: c

    /* A driver for the "vnd,mydev" compatible device */
    #define DT_DRV_COMPAT vnd_mydev

    ...
    #include <zephyr/drivers/clock_management.h>
    #include <zephyr/pm/device.h>
    ...

    struct mydev_config {
        ...
        /* Clock management data for this device */
        const struct clock_management_data *clk_mgmt;
        /* Identifier for high-speed clock output */
        clock_output_t hs_clk;
        /* Identifier for low-power clock output */
        clock_output_t lp_clk;
        ...
    };

    ...

    static int hs_clock_cb(const struct clock_management_event *ev, const void *user_data)
    {
        const struct device *dev = user_data;

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

    static int mydev_pm_action(const struct device *dev, enum pm_device_action action)
    {
        const struct mydev_config *config = dev->config;

        switch (action) {
        case PM_DEVICE_ACTION_SUSPEND:
            /* Replace the default request with the sleep request */
            return clock_management_request_state(config->clk_mgmt,
                                                  CLOCK_MANAGEMENT_STATE_SLEEP);
        case PM_DEVICE_ACTION_RESUME:
            /* Restore the default request */
            return clock_management_request_state(config->clk_mgmt,
                                                  CLOCK_MANAGEMENT_STATE_DEFAULT);
        default:
            return -ENOTSUP;
        }
    }

    static int mydev_init(const struct device *dev)
    {
        const struct mydev_config *config = dev->config;
        int ret;
        clock_freq_t hs_clk_rate, lp_clk_rate;
        ...
        /* Apply the default request */
        ret = clock_management_request_state(config->clk_mgmt,
                                             CLOCK_MANAGEMENT_STATE_DEFAULT);
        if (ret < 0) {
            return ret;
        }
        hs_clk_rate = clock_management_get_rate(config->clk_mgmt, config->hs_clk);
        lp_clk_rate = clock_management_get_rate(config->clk_mgmt, config->lp_clk);
        /* Register for a callback if high-speed clock changes rate */
        ret = clock_management_set_callback(config->clk_mgmt, config->hs_clk,
                                            hs_clock_cb, dev);
        if ((ret < 0) && (ret != -ENOTSUP)) {
            /* -ENOTSUP is returned if CONFIG_CLOCK_MANAGEMENT_RUNTIME is disabled */
            return ret;
        }
        ...
        return 0;
    }

    #define MYDEV_DEFINE(i)                                                    \
        /* Define clock management data structures */                          \
        CLOCK_MANAGEMENT_DT_INST_DEFINE(i);                                    \
        ...                                                                    \
        static const struct mydev_config mydev_config_##i = {                  \
            ...                                                                \
            .clk_mgmt = CLOCK_MANAGEMENT_DT_INST_GET(i),                       \
            /* Get identifiers for clock outputs */                            \
            .hs_clk = CLOCK_MANAGEMENT_DT_INST_GET_OUTPUT_BY_NAME(i, high_speed),\
            .lp_clk = CLOCK_MANAGEMENT_DT_INST_GET_OUTPUT_BY_NAME(i, low_power),\
            ...                                                                \
        };                                                                     \
        static struct mydev_data mydev_data_##i;                               \
        ...                                                                    \
        PM_DEVICE_DT_INST_DEFINE(i, mydev_pm_action);                          \
                                                                               \
        DEVICE_DT_INST_DEFINE(i, mydev_init, PM_DEVICE_DT_INST_GET(i),         \
                              &mydev_data_##i, &mydev_config_##i, ...);

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
settings. Each clock producer must implement the clock driver API.

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
mostly orthogonal. The following API classes are available:

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
    | :kconfig:option:`CONFIG_CLOCK_MANAGEMENT_SET_RATE`  | :c:func:`clock_best_rate`             |
    |                                                     | :c:func:`clock_root_best_rate`        |
    |                                                     | :c:func:`clock_set_parent`            |
    +-----------------------------------------------------+---------------------------------------+

All API functions associated with
:kconfig:option:`CONFIG_CLOCK_MANAGEMENT_RUNTIME`
**must** be implemented by each clock driver, but they should be compiled out
when runtime features are disabled. Clock drivers should implement API functions
associated with :kconfig:option:`CONFIG_CLOCK_MANAGEMENT_SET_RATE` whenever
possible, but it is not required.

Clock drivers **must** hold a reference to their parent clock device, if
one exists, and **must not** reference their child clock devices directly.

These constraints are required because the clock subsystem determines which clock
devices can be discarded from the build at link time based on which clock devices
are referenced. If a parent clock is not referenced, that clock and any of its
parents would be discarded. However if a child clock is directly referenced,
that child clock would be linked in regardless of if a consumer was actually
using it.

Clock consumers hold references to the clock output nodes they are using, which
then reference their parent clock producers, which in turn reference their
parents. Similarly, a consumer's ``clock-request-n`` properties reference clock domains,
whose selected clock states reference the clock producers they configure. These reference chains
allow the clock management subsystem to only link in the clocks that the
application actually needs.

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

    #define VND_CLOCK_DEFINE(inst)                                                \
      const struct vnd_clock_driver_data clock_data_##inst = {                    \
		STANDARD_CLK_SUBSYS_DATA_INIT(CLOCK_DT_GET(DT_INST_PHANDLE(inst, input))) \
      };                                                                          \
      CLOCK_DT_INST_DEFINE(inst,                                                  \
                           &clock_data_##inst,                                    \
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
    };

    clock_div: clock-div@50000000 {
        compatible = "vnd,clock-div";
        #clock-cells = <1>;
        input = <&clock_source>;
        reg = <0x50000000>;

        clock_output: clock-output {
            compatible = "clock-output";
            #clock-cells = <0>;
        };
    };

    ....

    clock-domains {
        clock_div_domain: clock-div-domain {
            compatible = "clock-domain";

            clock_div_default: clock-div-default {
                compatible = "clock-state";
                clocks = <&clock_div 1>;
                rank = <0>;
            };
        };
    };

    ....

    periph0: periph@0 {
        /* Clock outputs */
        clock-outputs = <&clock_output>;
        clock-output-names = "default";
        /* Default clock state */
        clock-request-0 = <&clock_div_domain CLK_STATE_0>;
        clock-request-names = "default";
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
and are the nodes which clock consumers will reference to read or request
clock rates. The clock management framework will handle defining clock
structures for each of these nodes.

Clock Domain Nodes
------------------

Clock states are grouped within clock domain nodes, which must have the
compatible :dtcompatible:`clock-domain` and set ``#clock-state-cells = <1>``.
Each child of a clock domain with the compatible :dtcompatible:`clock-state`
defines one state for that domain. Consumers select states with a bitmask in
their ``clock-request-n`` properties, where bit ``n`` refers to the ``n``-th
``clock-state`` child in declaration order. Clock domains are not part of the
clock tree, so they may be defined anywhere in the
devicetree. They are typically defined by the SOC or board devicetree under a
common ``clock-domains`` container node. The clock management framework will
handle defining clock structures for each of these nodes.

Clock domains should be split so that each domain configures a set of clock
producers which can be reconfigured independently of other domains. Each clock
producer may only be configured by the states of a single clock domain. This is
enforced at build time by ``scripts/dts/check_clock_ownership.py``, which
reports an error if states in two clock domains configure the same producer.
Producers that are shared between multiple consumers should be placed in their
own domain, so that each consumer can request a state from that domain.

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
    | :dtcompatible:`fixed-factor-clock`  | Fixed clock divider or multiplier          |
    +-------------------------------------+--------------------------------------------+
    | :dtcompatible:`clock-gate`          | Clock gate to inhibit or pass a clock      |
    +-------------------------------------+--------------------------------------------+
    | :dtcompatible:`clock-mux`           | Multiplexer to select between sources      |
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
    clock framework will interpret as a rate of zero.

* :c:func:`clock_recalc_rate` (standard clocks only)

  * Read device specific registers to recalculate the clock frequency versus
    the provided parent frequency
  * Drivers can return ``-ENOTCONN`` if their hardware is not setup, which the
    clock framework will interpret as a rate of zero.
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


* :c:func:`clock_root_best_rate` (root clocks only)

  * Return (and optionally apply) the closest frequency the root clock can
    produce for the given request. When ``commit`` is true, the rate is applied
    to hardware.

* :c:func:`clock_best_rate` (standard clocks only)

  * Return (and optionally apply) the closest frequency the clock can produce
    for the given requested frequency if the clock is provided with the given
    parent rate. When ``commit`` is true, the rate is applied to hardware.

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
   always pass through clock helper APIs. This ensures that clock consumers are
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
        #clock-cells = <0>;

        fixed_output: fixed-output {
            compatible = "clock-output";
            #clock-cells = <0>;
        };
    };

    external_osc: external-osc {
        compatible = "fixed-clock";
        /* User's board can override this
         * based on oscillator they use */
        clock-frequency = <0>;
        #clock-cells = <0>;
    };

    uart_mux: uart-mux@40001000 {
        compatible = "vnd,clock-mux";
        reg = <0x40001000>;
        #clock-cells = <1>;
        input-sources = <&fixed_source &external_osc>;
    };

    uart_div: uart-div@40001200 {
        compatible = "vnd,clock-div";
        #clock-cells = <1>;
        input = <&uart_mux>;
        reg = <0x40001200>;

        uart_output: clock-output {
            compatible = "clock-output";
            #clock-cells = <0>;
        };
    };

    uart_dev: uart-dev {
        compatible = "vnd,uart-dev";
        clock-outputs = <&uart_output>;
        clock-output-names = "default";
    };

    uart_dev2: uart-dev2 {
        compatible = "vnd,uart-dev";
        clock-outputs = <&uart_output>;
        clock-output-names = "default";
    };

At the board level, a frequency will be defined for the external clock.
Furthermore, a clock domain containing states for the UART portion of the clock
tree will be defined, and its default state will be requested by the first UART
device:

.. code-block:: devicetree

    / {
        clock-domains {
            uart_domain: uart-domain {
                compatible = "clock-domain";
                #clock-state-cells = <1>;

                uart_default: uart-default {
                    compatible = "clock-state";
                    /* Select external source, divide by 4 */
                    clocks = <&uart_mux 1 &uart_div 4>;
                    rank = <0>;
                };
            };
        };
    };

    &external_osc {
        clock-frequency = <DT_FREQ_M(16)>;
    };

    &uart_dev {
        clock-request-0 = <&uart_domain CLK_STATE_0>;
        clock-request-names = "default";
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

.. mermaid::

    graph TD
        %% Reading Clock Rates
        uart_output([uart_output])
        recalc_rate[clock_recalc_rate]
        get_rate[clock_management_get_rate]
        clk_rate_0[clock_management_clk_rate]
        clk_rate_1[clock_management_clk_rate]
        clk_rate_2[clock_management_clk_rate]
        read_parent[Read parent from clock struct]
        get_parent[clock_get_parent]
        get_root_rate[clock_get_rate]
        uart_div([uart_div])
        uart_mux([uart_mux])
        fixed_source([fixed_source])

        uart_output --> get_rate
        get_rate --> uart_div

        uart_div --> clk_rate_0 --> read_parent --> uart_mux
        uart_mux --> clk_rate_1 --> get_parent --> fixed_source
        fixed_source --> clk_rate_2 --> get_root_rate

        get_root_rate --->|return rate| clk_rate_2
        clk_rate_2 --> recalc_rate -->|return rate| clk_rate_1
        clk_rate_1 -->|return rate| clk_rate_0
        clk_rate_0 -->|return rate| get_rate

        classDef consumer fill:#ffd700,stroke:#333,color:#000
        classDef producer fill:#00ced1,stroke:#333,color:#000
        classDef subsystem fill:#00bfff,stroke:#333,color:#000

        class uart_output consumer
        class uart_div,uart_mux,fixed_source,uart_div2 producer
        class get_rate,clk_rate_0,clk_rate_1,clk_rate_2,read_parent,get_parent,get_root_rate,recalc_rate subsystem

Requesting Clock States
=======================

When a consumer issues a request for clock states, the clock management
subsystem will first verify that none of the clock domains referenced by the
request are locked by another consumer. Then, for each clock domain referenced
by the request, it will examine the bitmask of allowed states for the clock
domain, and take the intersection of all requests for that clock domain. From
the set of states that satisfy all requests, the clock management subsystem
will select the state with the lowest rank, and apply that state to the clock
domain. The rank of a state is set by the user in devicetree.

In this example, when ``uart_dev`` requests its ``default`` state, the
``uart-default`` state will be applied to ``uart_domain``, which configures
``uart_mux`` and ``uart_div``. Since ``uart_dev2`` shares ``uart_output``, it
will receive rate change notifications when this state is applied.

When a clock state is applied, the following will happen for each
clock node specified by the state's ``clocks`` property:

* :c:func:`clock_configure_recalc` (or the multiplexer/root clock specific
  variant) will be called on the target clock to determine the rate the clock
  will produce.
* ``clock_notify_children`` will be called to validate that the target clock
  is not locked, and that all children can accept the new rate.
* If either of these checks fail, the state application will fail and an error
  will be returned to the consumer.
* Otherwise, :c:func:`clock_configure` will be called on the clock node with the
  vendor specific data given by that node's specifier
* ``clock_notify_children`` will be called again to notify children of the
  rate change.

This call chain looks like so:

.. mermaid::

   graph LR
       %% Applying a Clock State
       uart_driver([uart driver])
       request_state[clock_management_request_state]
       apply_state[clock_apply_state]

       uart_driver --> request_state --> apply_state

       subgraph mux_config[uart_mux configuration]
           tree_configure_mux[clock_tree_configure]
           mux_configure_recalc[clock_mux_configure_recalc]
           notify_pre_mux[clock_notify_children]
           configure_mux[clock_configure]
           notify_post_mux[clock_notify_children]
       end

       subgraph div_config[uart_div configuration]
           tree_configure_div[clock_tree_configure]
           configure_recalc[clock_configure_recalc]
           notify_pre_div[clock_notify_children]
           configure_div[clock_configure]
           notify_post_div[clock_notify_children]
       end

       uart_mux([uart_mux])
       uart_div([uart_div])

       apply_state --> tree_configure_mux
       tree_configure_mux --> mux_configure_recalc --> uart_mux
       tree_configure_mux --> notify_pre_mux
       tree_configure_mux --> configure_mux --> uart_mux
       tree_configure_mux --> notify_post_mux

       apply_state --> tree_configure_div
       tree_configure_div --> configure_recalc --> uart_div
       tree_configure_div --> notify_pre_div
       tree_configure_div --> configure_div --> uart_div
       tree_configure_div --> notify_post_div

       classDef consumer fill:#ffd700,stroke:#333,color:#000
       classDef producer fill:#00ced1,stroke:#333,color:#000
       classDef subsystem fill:#00bfff,stroke:#333,color:#000

       class uart_driver consumer
       class uart_mux,uart_div producer
       class request_state,apply_state,tree_configure_mux,mux_configure_recalc,notify_pre_mux,configure_mux,notify_post_mux,tree_configure_div,configure_recalc,notify_pre_div,configure_div,notify_post_div subsystem

Requesting Runtime Rates
========================

For runtime rate resolution, there are two phases: querying the best clock setup
and applying it.

During the query phase, the clock subsystem will walk up the clock tree until it
reaches a root clock. Once a root clock is reached, the rate it offers via
:c:func:`clock_root_best_rate` (with ``commit=false``) will be offered as the
parent rate to its child clock when calling :c:func:`clock_best_rate` (with
``commit=false``). Multiplexers have this support implemented generically, via a
function that selects the best rate offered by all of the multiplexer parents.
Proposed parent rates are validated with clock children via
:c:func:`clock_recalc_rate`, or multiplexers via
:c:func:`clock_mux_validate_parent`.

In the application phase, the clock subsystem will once again walk up the clock
tree, but now clock settings will be applied using
:c:func:`clock_root_best_rate` (with ``commit=true``),
:c:func:`clock_best_rate` (with ``commit=true``), and
:c:func:`clock_set_parent`.

Clock ranking is performed within the multiplexer query phase. Clocks may either
be ranked based on their ability to satisfy a frequency request (best accuracy
clock returned) or their rank factor (input with lowest calculated rank factor
that fits within frequency constraints selected).

Note that if no clocks fit within the provided constraint set, a "best effort"
clock will be returned, IE the clock that was closest to the maximum frequency
in the constraint set. This is done so that clocks higher in the clock tree will
still be selected optimally, even if dividers or multipliers which source them
are needed to satisfy the clock constraints.

The call chain of a runtime rate request might look like so (note that in
this example, ``external_osc`` produces a better rate match than
``fixed_source``):

.. mermaid::

    graph TD
       %% Runtime Rate Request
       uart_output([uart_output])
       req_rate[clock_management_req_rate]

       uart_output --> req_rate --> query_phase
       req_rate --> apply_phase


       subgraph query_phase[Query Phase]
           round_internal0[clock_management_round_internal]
           read_parent[Read parent from clock struct]
           best_rate[clock_best_rate]
           round_internal1[clock_management_round_internal]
           best_parent[clock_management_best_parent]
           round_internal2[clock_management_round_internal]
           round_internal3[clock_management_round_internal]
           root_best_rate[clock_root_best_rate]
           root_best_rate2[clock_root_best_rate]
       end

       subgraph apply_phase[Apply Phase]
           set_internal0[clock_management_set_internal]
           read_parent_2[Read parent from clock struct]
           best_rate_set[clock_best_rate]
           set_internal1[clock_management_set_internal]
           root_best_rate_set[clock_root_best_rate]
           set_parent[clock_set_parent]
       end

       uart_div([uart_div])
       uart_mux([uart_mux])
       fixed_source([fixed_source])
       external_osc([external_osc])

       round_internal0 --> read_parent --> uart_div
       round_internal0 <--> round_internal1
       round_internal0 --> best_rate --> uart_div
       round_internal1 --> best_parent --> uart_mux
       best_parent <--> round_internal2
       best_parent <--> round_internal3
       round_internal2 --> root_best_rate --> fixed_source
       round_internal3 --> root_best_rate2 --> external_osc

       set_internal0 --> read_parent_2 --> uart_div
       set_internal0 <--> set_internal1
       set_internal0 --> best_rate_set --> uart_div
       set_internal1 --> best_parent
       set_internal1 --> root_best_rate_set --> external_osc
       set_internal1 --> set_parent --> uart_mux

       classDef consumer fill:#ffd700,stroke:#333,color:#000
       classDef producer fill:#00ced1,stroke:#333,color:#000
       classDef subsystem fill:#00bfff,stroke:#333,color:#000

       class uart_output consumer
       class uart_div,uart_mux,fixed_source,external_osc producer
       class req_rate,round_internal0,read_parent,best_rate,round_internal1,best_parent,round_internal2,round_internal3,root_best_rate,root_best_rate2,set_internal0,read_parent_2,best_rate_set,set_internal1,root_best_rate_set,set_parent subsystem

Clock Notifications
===================

Clock notifications are a critical part of the clock management subsystem. They
allow clocks to validate and notify their consumers of rate changes. There are
three types of notifications: query, pre-change, and post-change. Query
notifications are used to validate that a clock can accept a proposed rate.
These notifications are sent before a clock is reconfigured, and consumer
callbacks may reject the proposed rate by returning ``-ENOTSUP`` (rate not
supported) or ``-EBUSY`` (rate change not permitted at this time). Pre-change
notifications are sent to consumers before a clock is reconfigured, and allow
consumers to prepare for the rate change. Post-change notifications are sent
after a clock is reconfigured.

Notifications also enforce clock locks. If a notification reaches a clock
producer that was locked by a locking clock request, the notification fails
with ``-EPERM``. If it reaches a clock output that was locked with
:c:func:`clock_management_lock`, the notification fails with ``-EBUSY``.
Either failure will cause the reconfiguration to be rejected.

A call chain for clock notifications on ``fixed_source`` might look like so.
Note that the event type in use only changes how the consumer nodes at the leaf
of the tree respond.


.. mermaid::

   graph TD
       %% Clock Notification Chain
       notify_children[clock_notify_children]
       check_parent[Check if parent is connected]
       get_parent[clock_get_parent]
       validate_parent[clock_mux_validate_parent]
       notify_children1[clock_notify_children]
       recalc_rate[clock_recalc_rate]
       notify_children2[clock_notify_children]

       uart_mux([uart_mux])
       uart_div([uart_div])
       uart_output([uart_output])
       fixed_output([fixed_output])

       notify_children --> check_parent --> get_parent --> uart_mux
       notify_children --> validate_parent --> uart_mux
       notify_children --> fixed_output
       notify_children --> notify_children1
       notify_children1 --> recalc_rate --> uart_div
       notify_children1 --> notify_children2 --> uart_output

       classDef consumer fill:#ffd700,stroke:#333,color:#000
       classDef producer fill:#00ced1,stroke:#333,color:#000
       classDef subsystem fill:#00bfff,stroke:#333,color:#000

       class uart_output,fixed_output consumer
       class uart_mux,uart_div producer
       class notify_children,check_parent,get_parent,validate_parent,notify_children1,recalc_rate,notify_children2 subsystem
