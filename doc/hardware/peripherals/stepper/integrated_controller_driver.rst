.. _stepper-integrated-controller-driver:

Integrated Stepper Motion Control and Driver
############################################

Implements both ``stepper`` and ``stepper_drv`` APIs in a single driver. For instance, :dtcompatible:`adi,tmc50xx`.

Following is an example of a device tree configuration for a stepper driver with integrated motion control:

.. code-block:: dts

   / {
       aliases {
           x_y_stepper_motion_controller = &tmc50xx;
           x_axis_stepper_driver = &motor_x;
           y_axis_stepper_driver =  &motor_y;
       };
   };

   &spi0 {
       /* SPI bus options here, not shown */

       /* Dual controller/driver for up to two 2-phase bipolar stepper motors */
       /* DEVICE_API: stepper api */
       tmc50xx: tmc50xx@0 {
           compatible = "adi,tmc50xx";
           reg = <0>;
           spi-max-frequency = <DT_FREQ_M(8)>; /* Maximum SPI bus frequency */

           #address-cells = <1>;
           #size-cells = <0>;

           poscmp-enable; test-mode; lock-gconf; /* ADI TMC Global configuration flags */
           clock-frequency = <DT_FREQ_M(16)>; /* Internal/External Clock frequency */

           /* DEVICE_API: stepper_drv api */
           motor_x: motor@0 {
               status = "okay";
               reg = <0>;

               /* common stepper controller settings */
               invert-direction;
               micro-step-res = <256>;

               /* ADI TMC stallguard settings specific to TMC50XX */
               activate-stallguard2;
               .....................

               /* ADI TMC ramp generator as well as current settings */
               vstart = <10>;
               ..............
           };

           /* DEVICE_API: stepper_drv api */
           motor_y: motor@1 {
               status = "okay";
               reg = <1>;

               /* common stepper controller settings */
               micro-step-res = <256>;

               /* ADI TMC stallguard settings specific to TMC50XX */
               activate-stallguard2;
               .....................

               /* ADI TMC ramp generator as well as current settings */
               vstart = <10>;
               ..............
           };
       };
   };

All the stepper api functions need a stepper motor index, since a stepper motion controller can control
multiple motors. However, this can be configured via application specific Kconfig to use a specific index,
for instance, :kconfig:option-regex:`CONFIG_STEPPER_AXIS_*`.

Following the aforementioned configurations, the stepper driver subsystem can be used in the application code
as follows:

.. code-block:: c

   /* Configure the index of motor and respective axis via Kconfig */
   stepper_move_to(x_y_stepper_motion_controller, CONFIG_STEPPER_AXIS_X_*, 200);
   stepper_stop(x_y_stepper_motion_controller, CONFIG_STEPPER_AXIS_Y_*);
   stepper_drv_disable(x_axis_stepper_driver);
   stepper_drv_disable(y_axis_stepper_driver);
