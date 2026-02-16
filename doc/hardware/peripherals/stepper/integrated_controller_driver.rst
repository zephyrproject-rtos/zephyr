.. _stepper-integrated-controller-driver:

Integrated Stepper Motion Control and Driver
############################################

A device composed of both motion controller and stepper driver blocks in a single IC is modeled as a
multi-functional device in devicetree, and has two software drivers implementing :c:group:`stepper_ctrl_interface`
and :c:group:`stepper_hw_driver_interface` APIs respectively. An example of such device is :dtcompatible:`adi,tmc50xx`.

.. code-block:: dts

   / {
       aliases {
           x_axis_stepper_ctrl = &tmc50xx_0_motion_controller;
           y_axis_stepper_ctrl =  &tmc50xx_1_motion_controller;
           x_axis_stepper_driver = &tmc50xx_0_stepper_driver;
           y_axis_stepper_driver = &tmc50xx_1_stepper_driver;
       };
   };

   &spi0 {
       /* SPI bus options here, not shown */

       /* Dual controller/driver for up to two 2-phase bipolar stepper motors */
       tmc50xx: tmc50xx@0 {
           compatible = "adi,tmc50xx";
           reg = <0>;
           spi-max-frequency = <DT_FREQ_M(8)>; /* Maximum SPI bus frequency */

           poscmp-enable; test-mode; lock-gconf; /* ADI TMC Global configuration flags */
           clock-frequency = <DT_FREQ_M(16)>; /* Internal/External Clock frequency */

           tmc50xx_0_stepper_driver: stepper-driver-0 {
               idx = <0>;
                compatible = "adi,tmc50xx-stepper-driver";
                micro-step-res = <256>;
                /* ADI TMC stallguard settings specific to TMC50XX */
                stallguard2-threshold=<30>;
            };

           tmc50xx_0_motion_controller: motion-controller-0 {
               idx = <0>;
               compatible = "adi,tmc50xx-stepper-ctrl";
               ...
               vmax = <900000>;
               amax = <50000>;
               ...
               activate-stallguard2;
               ...
            };

           tmc50xx_1_stepper_driver: stepper-driver-1 {
               idx = <1>;
                compatible = "adi,tmc50xx-stepper-driver";
                micro-step-res = <256>;
                /* ADI TMC stallguard settings specific to TMC50XX */
                stallguard2-threshold=<30>;
            };

           tmc50xx_1_motion_controller: motion-controller-1 {
               idx = <1>;
               compatible = "adi,tmc50xx-stepper-ctrl";
               ...
               vstart = <1000>;
               ...
               stallguard-threshold-velocity=<200000>;
            };
        };
   };

Following the aforementioned configurations, the stepper driver subsystem can be used in the application code
as follows:

.. code-block:: c

   static const struct device *x_stepper_ctrl = DEVICE_DT_GET(DT_ALIAS(x_axis_stepper_ctrl));
   static const struct device *x_stepper_driver = DEVICE_DT_GET(DT_ALIAS(x_axis_stepper_driver));
   static const struct device *y_stepper_ctrl = DEVICE_DT_GET(DT_ALIAS(y_axis_stepper_ctrl));
   static const struct device *y_stepper_driver = DEVICE_DT_GET(DT_ALIAS(y_axis_stepper_driver));
   ...
   stepper_ctrl_move_to(x_stepper_ctrl, 200);
   stepper_ctrl_stop(x_stepper_ctrl);
   stepper_disable(x_stepper_driver);
   stepper_disable(y_stepper_driver);
