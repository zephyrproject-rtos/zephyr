.. _clock-mgmt-guide:

Clock Management
################

This is a high level to clock management in Zephyr. See :ref:`clock_mgmt_api`
for API reference material.

Introduction
************

SOC clock control peripherals typically expose a mixture of clock generators,
clock outputs, and clock routing blocks to the system. These may include
PLLs, internal oscillators, multiplexers, or dividers/multipliers. The clock
structure of a system is typically very specific to the vendor/SOC itself.
Zephyr provides the clock management subsystem in order to enable clock
consumers and applications to manage clocks in a device-agnostic manner.

Reading Clocks
**************

Drivers must supply a subsystem identifier when reading or subscribing to
a callback for a clock. The clock management defines one subsystem identifier,
:c:macro:`CLOCK_MGMT_SUBSYS_DEFAULT`. Consumers may define additional private
identifiers if they need to query multiple clocks. Each identifier corresponds
to an index within the ``clocks`` property of the node defining the clock
consumer.

Consumers can query a clock output rate via the :c:func:`clock_mgmt_get_rate`
function. This function returns the clock rate as a positive integer,
or a negative error value on failure.

Consumers can also monitor a clock output rate. To do so, they may
register for a callback. Once a callback is initialized with
:c:func:`clock_mgmt_init_callback`, it can be registered with the
:c:func:`clock_mgmt_add_callback` function. Until the callback is removed
with :c:func:`clock_mgmt_remove_callback`, the driver will receive a callback
whenever the clock changes rate, starts, or stops.

Setting Clocks
**************

Clocks are primarily managed via "setpoints". A setpoint corresponds to a
specific group of clock settings, which may have systemwide effects, or simply
configure clocks for a given peripheral. For example, one setpoint may simply
configure the multiplexer and divider for a given peripheral. Another may
reconfigure the system level PLL, and modify the core clock rate.

Setpoints are configured via devicetree, and are usually opaque to the
driver/application code consuming the clock. Instead of having direct
knowledge of a setpoint's contents, clock consumers select a setpoint
based on the power state they are targeting. The following setpoint
power states are defined:

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

Devicetree Representation
*************************

Devicetree is used to define all system specific data for clock management.
The SOC itself will define the system clock tree. Then, the devicetree for
clock consumers may reference the system clock tree nodes to define clock
setpoints or access clock subsystems.

The SOC level clock tree definition will be specific to the SOC, but
may look similar to the following:

.. code-block:: devicetree

    clock_source: clock-source {
        clock-id = "CLK_SOURCE";
        compatible = "clock-source";
        #clock-cells = <1>;

        clock_div: clock-div {
            clock-id = "CLK_DIV";
            compatible = "clock-div";
            #clock-cells = <1>;

            clock_output: clock-output {
                compatible = "clock-output";
                clock-output;
                clock-id = "CLK_OUTPUT";
                #clock-cells = <0>;
            };
        };
    };

The peripheral devicetree will then reference these nodes to query clock rates,
and define setpoints:

.. code-block:: devicetree

    periph0: periph@0 {
        /* Clock subsystems */
        clocks = <&clock_output>;
        clock-names = "default";
        /* Default clock setpoint */
        clock-setpoint-0 = <&clock_source 1000000 &clock_div 3>;
        /* Sleep setpoint */
        clock-setpoint-1 = <&clock_source 200000 &clock_div 1>;
        clock-setpoint-names = "default", "sleep";
    };

Note that the specifier cells for each clock node within a setpoint are
device specific. These specifiers allow configuration of the clock element,
such as setting a divider's division factor or selecting an output for a
multiplexer.

Driver Usage
************

In order to use the clock management subsystem, a driver must call two C
macros. First, the driver must define clock management data. This can be
accomplished with the :c:macro:`CLOCK_MGMT_DEFINE` or
:c:macro:`CLOCK_MGMT_DT_INST_DEFINE` depending on if the driver is instance
based. Then the driver needs to initialize a pointer of type
:c:struct:`clock_mgmt` to pass to the clock management functions. This
can be done with the :c:macro:`CLOCK_MGMT_INIT` or
:c:macro:`CLOCK_MGMT_DT_INST_INIT`. The driver can then utilize the clock
management APIs.

During device init, the device should apply the default clock management
state. This will allow the clock management subsystem to configure any
clocks the driver requires during its initialization phase. The driver
can then query clock rates. A complete example of implementing
clock management within a driver is provided below:

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

    struct mydev_data {
        /* Callback tracking structure */
        struct clock_mgmt_callback *callback;
    };

    ...

    void mydev_clock_cb_handler(struct clock_mgmt_callback *cb,
                                enum clock_mgmt_cb_reason reason)
    {
        ...
    }

    static int mydev_init(const struct device *dev)
    {
        const struct mydev_config *config = dev->config;
        int ret, clock_rate;
        ...
        /* Select "default" state at initialization time */
        ret = clock_mgmt_apply_state(config->clk, CLOCK_MGMT_STATE_DEFAULT);
        if (ret < 0) {
            return ret;
        }
        /* Query clock rate */
        ret = clock_mgmt_get_rate(config->clk, CLOCK_MGMT_SUBSYS_DEFAULT);
        if (ret < 0) {
            return ret;
        }
        /* Register for a callback if clock rate changes (optional) */
        clock_mgmt_init_callback(&data->callback, mydev_clock_cb_handler);
        ret = clock_mgmt_add_callback(config->clk, CLOCK_MGMT_SUBSYS_DEFAULT,
                                      &data->callback);
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
            .clk = CLOCK_MGMT_DT_INST_INIT(i),                                 \
            ...                                                                \
        };                                                                     \
        static struct mydev_data mydev_data##i;                                \
        ...                                                                    \
                                                                               \
        DEVICE_DT_INST_DEFINE(i, mydev_init, NULL, &mydev_data##i,             \
                              &mydev_config##i, ...);

    DT_INST_FOREACH_STATUS_OKAY(MYDEV_DEFINE)

SOC Clock Management Implementation
***********************************

SOC level clock management is implemented via two "templated" functions.
These functions implement support for querying clock subsystem rates, and
apply clock setpoints. The build system will parse the "templated" functions,
and define per subsystem and per setpoint function implementations based on
them. Functions should be implemented using the DT macros defined in
:ref:`clock_mgmt_dt_api`, to maximize the optimization the complier can
apply to the generated functions.

Ideally, a setpoint definition should expand to something like the following

.. code-block:: c

   ...
   if (0) {
       /* Apply configuration for clock not in this setpoint */
   }
   if (1) {
       /* Apply configuration for clock present in this setpoint */
   }
   if (0) {
       /* Apply configuration for clock not in this setpoint */
   }
   ...

Each clock element within a devicetree will have a ``clock-id`` property.
These properties are critical to the SOC implementation, and allow the
handler functions to easily access each clock within the devicetree by
the ID.

Templated functions are defined using the macros ``Z_CLOCK_MGMT_SUBSYS_TEMPL``
and ``Z_CLOCK_MGMT_SETPOINT_TEMPL`` This approach is used as an alternative to
defining C macros that expand to function definitions to simply debugging
implementations, because the errors generated by functions defined within
a macro are often very difficult to debug.

For a SOC clock tree like the following:

.. code-block:: devicetree

    clock_source: clock-source {
        clock-id = "CLK_SOURCE";
        compatible = "clock-source";
        #clock-cells = <1>;

        clock_div: clock-div {
            clock-id = "CLK_DIV";
            compatible = "clock-div";
            #clock-cells = <1>;

            clock_output: clock-output {
                compatible = "clock-output";
                clock-output;
                clock-id = "CLK_OUTPUT";
                #clock-cells = <0>;
            };
        };
    };

The clock management templates might look like so:

.. code-block:: c

    #define CLK_OUTPUT_ID 0x0

    /*
     * This template declares a clock subsystem rate handler. The parameters
     * passed to this template are as follows:
     * @param node: node ID for device with clocks property
     * @param prop: clocks property of node
     * @param idx: index of the clock subsystem within the clocks property
     */
    Z_CLOCK_MGMT_SUBSYS_TEMPL(node, prop, idx)
    {
        /* Should expand to "CLK_OUTPUT_ID" */
        if (CONCAT(DT_STRING_TOKEN(DT_PHANDLE_BY_IDX(node, prop, idx),
                                   clock_id), _ID) == CLK_OUTPUT_ID) {
            return 1000;
        }
        /* Otherwise, querying rate not supported */
        return -ENOTSUP;
    }

    /*
     * This template declares a clock management setpoint. The parameters
     * passed to this template are as follows:
     * @param node: node ID for device with clock-control-state-<n> property
     * @param state: identifier for current state
     */
    Z_CLOCK_MGMT_SETPOINT_TEMPL(node, state)
    {
        if (DT_CLOCK_STATE_HAS_ID(node, state, CLOCK_SOURCE)) {
               set_my_clock_freq(DT_CLOCK_STATE_ID_READ_CELL_OR(node, CLOCK_SOURCE,
                                 freq, state, 0));
               /* Fire callback for all clock subsystems */
               CLOCK_MGMT_FIRE_ALL_CALLBACKS(CLOCK_MGMT_RATE_CHANGED);
        }
        if (DT_CLOCK_STATE_HAS_ID(node, state, CLOCK_DIV)) {
               set_my_clock_div(DT_CLOCK_STATE_ID_READ_CELL_OR(node, CLOCK_DIV,
                                divider, state, 0));
               /* Fire callback for clock output */
               CLOCK_MGMT_FIRE_CALLBACK(CLOCK_OUTPUT, CLOCK_MGMT_RATE_CHANGED);
        }
    }

Generated clock management code will be placed in the build directory, in
the file ``clock_mgmt_soc_generated.c``


.. _clock_mgmt_api:

Clock Management API
********************

.. doxygengroup:: clock_mgmt_interface

.. _clock_mgmt_dt_api:

Devicetree Clock Management Helpers
===================================

.. doxygengroup:: devicetree-clock-mgmt
