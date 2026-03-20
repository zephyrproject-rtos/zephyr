.. _mux_api:

Multiplexer Control (MUX)
#########################

Overview
********

A multiplexer (MUX) selects one signal or path from several candidates and routes
it to a destination. The ZephyrMUX API provides a common interface for SoC
signal-routing blocks and external multiplexers.

MUX consumer data is described by :c:struct:`mux_control` and is typically
generated from devicetree. Two consumer properties are supported:

* ``mux-controls`` identifies a control point. The state is supplied later with
  :c:func:`mux_configure`.
* ``mux-states`` identifies a control point and embeds a default state that can
  be applied with :c:func:`mux_configure_default`.

The API also provides :c:func:`mux_state_get` and :c:func:`mux_lock` when these
operations are implemented by the controller driver.

Devicetree Configuration
************************

MUX controllers are represented by nodes that provide the
``mux-controller`` capability and specify the cell layout of their control and
state specifiers using ``#mux-control-cells`` and ``#mux-state-cells``.

Consumer nodes reference a controller through either ``mux-controls`` or
``mux-states``.

Example devicetree fragment:

.. code-block:: devicetree

   / {
       /* MUX controller with 1 control cell and 2 state cells. */
       test_mux: mux-controller@40000000 {
           compatible = "reg-mux";
           reg = <0x40000000 0x10>;
           #mux-control-cells = <1>;
           #mux-state-cells = <2>;

           /* Each entry maps a register offset to a writable bitmask. */
           mux-reg-masks = <0x00 0x00000003>,
                           <0x00 0x000000f0>,
                           <0x04 0x000000ff>;
       };

       /* Runtime-controlled entries: state value is supplied by software. */
       test_mux_ctrl_node: test-mux-ctrl {
           compatible = "test-reg-mux-consumer";
           mux-controls = <&test_mux 0>, <&test_mux 2>;
           mux-control-names = "ch0", "ch2";
       };

       /* Default-configured entries: state value is encoded in devicetree. */
       test_mux_state_node: test-mux-state {
           compatible = "test-reg-mux-consumer";
           mux-states = <&test_mux 1 10>, <&test_mux 2 7>;
           mux-state-names = "ch1-default", "ch2-default";
       };
   };

Basic Operation
***************

Applications usually obtain a controller device, define one or more
:c:struct:`mux_control` descriptors from devicetree, and then call
:c:func:`mux_configure_default` for ``mux-states`` or :c:func:`mux_configure`
for ``mux-controls``.

Example using node-level helpers:

.. code-block:: c

   #include <zephyr/device.h>
   #include <zephyr/drivers/mux.h>

   #define TEST_MUX_NODE     DT_NODELABEL(test_mux)
   #define TEST_CTRL_NODE    DT_NODELABEL(test_mux_ctrl_node)
   #define TEST_STATE_NODE   DT_NODELABEL(test_mux_state_node)

   static const struct device *const mux_dev = DEVICE_DT_GET(TEST_MUX_NODE);

   MUX_CONTROL_DT_SPEC_DEFINE_ALL(TEST_CTRL_NODE);
   MUX_CONTROL_DT_SPEC_DEFINE_ALL(TEST_STATE_NODE);

   #define CH0_CTRL   MUX_CONTROL_DT_GET_BY_IDX(TEST_CTRL_NODE, 0)
   #define CH1_STATE  MUX_CONTROL_DT_GET_BY_IDX(TEST_STATE_NODE, 0)

   static int configure_muxes(void)
   {
       int ret;

       ret = mux_configure(mux_dev, CH0_CTRL, 3);
       if (ret < 0) {
           return ret;
       }

       return mux_configure_default(mux_dev, CH1_STATE);
   }

Driver Instance Helpers
+++++++++++++++++++++++

Drivers that configure multiple MUX entries for each devicetree instance can
use the instance helper macros instead of open-coding
``DT_INST_FOREACH_PROP_ELEM()``.

Example using :c:macro:`MUX_CONTROL_DT_INST_DEFINE_BY_PROP` for a driver that
expects ``mux-states`` and applies them during initialization:

.. code-block:: c

   struct example_config {
       const struct device *mux_dev;
       struct mux_control **mux_controls;
       size_t num_muxes;
   };

   #define EXAMPLE_MUX_DEFINE(n) \
       MUX_CONTROL_DT_INST_DEFINE_BY_PROP(example, n, mux_states)
   DT_INST_FOREACH_STATUS_OKAY(EXAMPLE_MUX_DEFINE)

   #define EXAMPLE_INIT(n) \
       static const struct example_config example_config_##n = { \
           .mux_dev = MUX_CONTROL_DT_INST_DEV_GET_BY_PROP(n, mux_states), \
           .mux_controls = MUX_CONTROL_DT_INST_ARRAY_GET(example, n), \
           .num_muxes = MUX_CONTROL_DT_INST_COUNT_BY_PROP(n, mux_states), \
       };

   static int example_init(const struct device *dev)
   {
       const struct example_config *config = dev->config;

       for (size_t i = 0; i < config->num_muxes; i++) {
           mux_configure_default(config->mux_dev, config->mux_controls[i]);
       }

       return 0;
   }

Use the ``*_BY_PROP`` variants when a driver must explicitly select
``mux-states`` or ``mux-controls``. Use the default instance helpers when the
driver can accept whichever property is present on the instance.

Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_MUX_CONTROL`
* :kconfig:option:`CONFIG_MUX_CONTROL_INIT_PRIORITY`
* :kconfig:option:`CONFIG_MUX_CONTROL_MAX_CELLS`
* Controller-specific options under ``drivers/mux/Kconfig``

API Reference
*************

.. doxygengroup:: mux_interface
