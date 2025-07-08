.. _stepper-integrated-controller-driver:

Integrated Stepper Motion Control and Driver
############################################

Devices which comprise of both motion controller and a stepper driver in a single IC. These devices
have to be modelled as multi-functional-device in device tree, implementing both ``stepper`` and
``stepper_drv`` APIs. An example of such a device is :dtcompatible:`adi,tmc50xx`. ``stepper`` API is
implemented by :dtcompatible:`adi,tmc50xx-motion-controller` and ``stepper_drv`` API is implemented by
:dtcompatible:`adi,tmc50xx-stepper-driver`.

.. code-block:: dts

   / {
       aliases {
           x_axis_stepper_motor = &tmc50xx_0_motion_controller;
           y_axis_stepper_motor =  &tmc50xx_1_motion_controller;
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

           /* DEVICE_API: stepper_drv api */
           tmc50xx_0_stepper_driver: tmc50xx_0_stepper_driver {
               idx = <0>;
                compatible = "adi,tmc50xx-stepper-driver";
                micro-step-res = <256>;
                /* ADI TMC stallguard settings specific to TMC50XX */
                stallguard2-threshold=<30>;
            };

           /* DEVICE_API: stepper api */
           tmc50xx_0_motion_controller: tmc50xx_0_motion_controller {
               idx = <0>;
               compatible = "adi,tmc50xx-motion-controller";
               ...
               vmax = <900000>;
               amax = <50000>;
               ...
               activate-stallguard2;
               ...
            };

           /* DEVICE_API: stepper_drv api */
           tmc50xx_1_stepper_driver: tmc50xx_1_stepper_driver {
               idx = <1>;
                compatible = "adi,tmc50xx-stepper-driver";
                micro-step-res = <256>;
                /* ADI TMC stallguard settings specific to TMC50XX */
                stallguard2-threshold=<30>;
            };

           /* DEVICE_API: stepper api */
           tmc50xx_1_motion_controller: tmc50xx_1_motion_controller {
               idx = <1>;
               compatible = "adi,tmc50xx-motion-controller";
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

   static const struct device *x_stepper = DEVICE_DT_GET(DT_ALIAS(x_axis_stepper_motor));
   static const struct device *x_stepper_drv = DEVICE_DT_GET(DT_ALIAS(x_axis_stepper_driver));
   static const struct device *y_stepper = DEVICE_DT_GET(DT_ALIAS(y_axis_stepper_motor));
   static const struct device *y_stepper_drv = DEVICE_DT_GET(DT_ALIAS(y_axis_stepper_driver));
   ...
   stepper_move_to(x_stepper, 200);
   stepper_stop(x_stepper);
   stepper_drv_disable(x_stepper_drv);
   stepper_drv_disable(y_stepper_drv);
