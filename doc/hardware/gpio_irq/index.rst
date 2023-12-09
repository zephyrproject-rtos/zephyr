.. _gpio_irq_api:

GPIO interrupt requests
#######################

Overview
********

Devices may generate interrupt requests which are routed from device to SOC
through GPIO pins.

The following example shows an I2C attached sensor which can generate an
interrupt on a pin which is connected to the GPIO controller ``gpio0``'s
pin 2 in the devicetree.

.. code-block:: devicetree

   #include <zephyr/dt-bindings/gpio/gpio.h>
   #include <zephyr/dt-bindings/interrupt-controller/irq.h>

   &i2c0 {
           sensor: sensor@21 {
                   compatible = "vnd,sensor";
                   reg = <0x21>;
                   interrupt-parent = <&gpio0>;
                   interrupts = <2 (IRQ_TYPE_EDGE_FALLING | GPIO_PULL_UP)>;
                   status = "okay";
           };
   };

GPIO controllers, like ``gpio0``, may double as interrupt controllers.
Interrupt controllers configure interrupt sources, which could be a level
change on a GPIO pin, and invoke interrupts when the interrupt source is
triggered.

Devicetree layout
*****************

This sections covers a variaty of GPIO IRQ layouts in the devicetree.

Multiple GPIO controllers
=========================

A single device may be connected to multiple GPIO controllers, in which case
the ``interrupts-extended`` property is used.

.. code-block:: devicetree

   #include <zephyr/dt-bindings/gpio/gpio.h>
   #include <zephyr/dt-bindings/interrupt-controller/irq.h>

   &i2c0 {
           sensor: sensor@21 {
                   compatible = "vnd,sensor";
                   reg = <0x21>;
                   interrupts-extended = <&gpio0 2 (IRQ_TYPE_EDGE_FALLING | GPIO_PULL_UP)>,
                                         <&gpio1 4 (IRQ_TYPE_EDGE_FALLING | GPIO_PULL_UP)>;
                   status = "okay";
           };
   };

Mapping GPIO interrupt lines
============================

Devices may contain multiple interrupt lines, of which some may be
optional, which can be routed to the GPIO controller(s).

Using interrupt line indexes
----------------------------

In case there is one or more obligatory interrupt lines, either followed by
a single or zero optional interrupt lines, the index of the interrupt lines
within the ``interrupts`` or ``interrupts-extended`` property can be used.

In the following example, the first interrupt line (index 0) is obligatory,
the second (index 1) is optional. The bindings file will define which
interrupt index is mapped to which interrupt line:

.. code-block:: yaml

   compatible: "vnd,sensor"

   include: sensor-device.yaml

   interrupts:
     description: |
       Interrupt line(s) connected to the INT1 and optional INT2 interrupt lines.
       The first interrupt line is INT1, the second is INT2.

In the following devicetree snippet, INT1 is connected to ``gpio0`` pin 2,
and INT2 is connected to ``gpio1`` pin 4:

.. code-block:: devicetree

   #include <zephyr/dt-bindings/gpio/gpio.h>
   #include <zephyr/dt-bindings/interrupt-controller/irq.h>

   &i2c0 {
           sensor: sensor@21 {
                   compatible = "vnd,sensor";
                   reg = <0x21>;
                   interrupts-extended = <&gpio0 2 (IRQ_TYPE_EDGE_FALLING | GPIO_PULL_UP)>,
                                         <&gpio1 4 (IRQ_TYPE_EDGE_FALLING | GPIO_PULL_UP)>;
                   status = "okay";
           };
   };

Using interrupt line names
--------------------------

The interrupt lines within the ``interrupts`` and ``interrupts-extended``
properties can mapped to a name, which can be used by drivers to identify
the connected interrupt lines.

In the following example, the DRDY line is obligatory, and the interrupt
lines INT1 and INT2 are optional. The bindings file will define which
interrupt name is mapped to which interrupt line:

.. code-block:: yaml

   compatible: "vnd,sensor"

   include: sensor-device.yaml

   interrupts:
     description: |
       Interrupt line(s) connected to the DRDY, and optional INT1 and INT2
       interrupt lines. The interrupt lines are mapped to the names "drdy",
       "int1" and "int2".

In the following devicetree snippet, DRDY is connected to ``gpio0`` pin 2,
and INT2 is connected to ``gpio1`` pin 4:

.. code-block:: devicetree

   #include <zephyr/dt-bindings/gpio/gpio.h>
   #include <zephyr/dt-bindings/interrupt-controller/irq.h>

   &i2c0 {
           sensor: sensor@21 {
                   compatible = "vnd,sensor";
                   reg = <0x21>;
                   interrupts-extended = <&gpio0 2 (IRQ_TYPE_EDGE_FALLING | GPIO_PULL_UP)>,
                                         <&gpio1 4 (IRQ_TYPE_EDGE_FALLING | GPIO_PULL_UP)>;
                   interrupt-names = "drdy", "int2";
                   status = "okay";
           };
   };

GPIO IRQ device driver example
******************************

This example is based on the following devicetree snippet:

.. code-block:: devicetree

   #include <zephyr/dt-bindings/gpio/gpio.h>
   #include <zephyr/dt-bindings/interrupt-controller/irq.h>

   &i2c0 {
           sensor: sensor@21 {
                   compatible = "vnd,sensor";
                   reg = <0x21>;
                   interrupts-extended = <&gpio0 2 (IRQ_TYPE_EDGE_FALLING | GPIO_PULL_UP)>,
                                         <&gpio1 4 (IRQ_TYPE_EDGE_FALLING | GPIO_PULL_UP)>;
                   interrupt-names = "drdy", "int2";
                   status = "okay";
           };
   };

The DRDY line is obligatory, and the interrupt lines INT1 and INT2 are
optional.

The example device driver gets the interrupt specifications from the
devicetree, and configures and enables all connected interrupt lines on init
(for the provided devicetree snippet, only DRDY and INT2 are connected).

.. code-block:: c

   #include <zephyr/kernel.h>
   #include <zephyr/drivers/sensor.h>
   #include <zephyr/drivers/gpio/gpio_irq.h>

   #define DT_DRV_COMPAT vnd_sensor

   struct vnd_sensor_config {
           struct gpio_irq_dt_spec drdy_irq_spec;
           struct gpio_irq_dt_spec int1_irq_spec;
           struct gpio_irq_dt_spec int2_irq_spec;
   };

   struct vnd_sensor_data {
           struct gpio_irq drdy_irq;
           struct gpio_irq int1_irq;
           struct gpio_irq int2_irq;
   };

   static void vnd_sensor_drdy_callback(struct gpio_irq *irq)
   {
           struct vnd_sensor_data *data = CONTAINER_OF(irq, struct vnd_sensor_data, drdy_irq);
   }

   static void vnd_sensor_int1_callback(struct gpio_irq *irq)
   {
           struct vnd_sensor_data *data = CONTAINER_OF(irq, struct vnd_sensor_data, int1_irq);
   }

   static void vnd_sensor_int2_callback(struct gpio_irq *irq)
   {
           struct vnd_sensor_data *data = CONTAINER_OF(irq, struct vnd_sensor_data, int2_irq);
   }

   static int vnd_sensor_init(const struct device *dev)
   {
           const struct vnd_sensor_config *config = dev->config;
           struct vnd_sensor_data *data = dev->data;
           int ret;

           /*
            * The sensor's interrupt lines must be configured before requesting, and thus
            * enabling, the interrupt request(s), to match the interrupt request flags
            * defined in the devicetree.
            *
            * The flags are found in the struct gpio_irq_dt_spec irq_flags member.
            *
            *     if (config->drdy_irq_spec.irq_flags & IRQ_TYPE_EDGE_RISING) {
            *             ...
            *     }
            *
            * If the sensor's interrupt lines can not be configured, simply validate
            * that the interrupt request flags match the configuration of the sensor's
            * interrupt lines.
            *
            *     if (config->drdy_irq_spec.irq_flags != IRQ_TYPE_EDGE_RISING) {
            *             return -EPERM;
            *     }
            */

           /*
            * IRQ is configured and enabled when requested.
            * DRDY is obligatory so no need to check if interrupt line is connected.
            */
           ret = gpio_irq_request_dt(&config->drdy_irq_spec, &data->drdy_irq,
                                     vnd_sensor_drdy_callback);
           if (ret < 0) {
                   return ret;
           }

           /* Remember to check if interrupt line is connected if it's optional */
           if (gpio_irq_dt_spec_exists(&config->int1_irq_spec)) {
                   ret = gpio_irq_request_dt(&config->int1_irq_spec, &data->int1_irq,
                                             vnd_sensor_int1_callback);
                   if (ret < 0) {
                           return ret;
                   }
           }

           if (gpio_irq_dt_spec_exists(&config->int2_irq_spec)) {
                   ret = gpio_irq_request_dt(&config->int2_irq_spec, &data->int2_irq,
                                             vnd_sensor_int2_callback);
                   if (ret < 0) {
                      return ret;
                   }
           }

           return 0;
   }

   #define VND_SENSOR_DEVICE(inst)                                                      \
           static struct vnd_sensor_data vnd_sensor_data##inst;                         \
           static const struct vnd_sensor_config vnd_sensor_config##inst = {            \
                   .drdy_irq_spec = GPIO_IRQ_DT_INST_SPEC_GET_BY_NAME(inst, drdy),      \
                   .int1_irq_spec = GPIO_IRQ_DT_INST_SPEC_GET_OPT_BY_NAME(inst, int1),  \
                   .int2_irq_spec = GPIO_IRQ_DT_INST_SPEC_GET_OPT_BY_NAME(inst, int2),  \
           };                                                                           \
                                                                                        \
           SENSOR_DEVICE_DT_INST_DEFINE(inst, vnd_sensor_init, NULL,                    \
                                        &vnd_sensor_data##inst,                         \
                                        &vnd_sensor_config##inst, POST_KERNEL, 99,      \
                                        NULL);

   DT_INST_FOREACH_STATUS_OKAY(VND_SENSOR_DEVICE)

GPIO IRQ API
************

.. doxygengroup:: gpio_irq
