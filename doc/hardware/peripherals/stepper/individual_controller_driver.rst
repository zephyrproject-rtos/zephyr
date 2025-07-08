.. _stepper-individual-controller-driver:

Individual Stepper Motion Controller and Driver
###############################################

A motion control driver implements ``stepper`` API, for instance, :dtcompatible:`zephyr,stepper-motion-control`
and a hardware driver implements ``stepper_drv`` API, for instance, :dtcompatible:`ti,drv84xx` or
:dtcompatible:`zephyr,h-bridge-stepper`.

Following is an example of a device tree configuration for a stepper driver with a dedicated stepper motion
controller:

.. code-block:: dts

   / {
       aliases {
           x_axis_stepper_motion_controller = &stepper_motion_control;
           x_axis_stepper_driver = &drv8424;
       };
   };

   /* DEVICE_API: stepper_drv api */
   drv8424: drv8424 {
       status = "okay";
       compatible = "ti,drv84xx";

       dir-gpios = <&gpio1 0 GPIO_ACTIVE_LOW>;
       step-gpios = <&gpio2 1 GPIO_ACTIVE_LOW>;
       sleep-gpios = <&gpio3 0 GPIO_ACTIVE_LOW>;
       en-gpios = <&gpio4 1 GPIO_ACTIVE_LOW>;
       m0-gpios = <&gpio5 0 GPIO_ACTIVE_LOW>;
       m1-gpios = <&gpio6 1 GPIO_ACTIVE_LOW>;
   };

   /* DEVICE_API: stepper api */
   stepper_motion_control: stepper_motion_control {
       compatible = "zephyr,stepper-motion-control";
       status = "okay";
       counter = <&counter0>;
       stepper = <&drv8424>;
   };

All the stepper api functions need a stepper motor index, since a stepper motion controller can control
multiple motors. However, this can be configured via application specific Kconfig to use a specific index,
for instance, :kconfig:option-regex:`CONFIG_STEPPER_AXIS_*`.

Following the aforementioned configurations, the stepper driver subsystem can be used in the application code
as follows:

.. code-block:: c

   /* Configure the index of motor and respective axis via Kconfig */
   stepper_move_to(x_axis_stepper_motion_controller, CONFIG_STEPPER_AXIS_X_*, 200);
   stepper_stop(x_axis_stepper_motion_controller, CONFIG_STEPPER_AXIS_X_*);
   stepper_drv_disable(x_axis_stepper_driver);
