.. _stepper-individual-controller-driver:

Individual Stepper Motion Controller and Driver
###############################################

A motion control driver implements :c:group:`stepper_interface` API, for instance,
:dtcompatible:`zephyr,gpio-step-dir-stepper` and a hardware driver implements :c:group:`stepper_amp_interface`
API, for instance, :dtcompatible:`adi,tmc2209`.

Following is an example of a device tree configuration for a stepper driver with a dedicated stepper motion
controller:

.. code-block:: dts

    / {
        aliases {
            stepper_amp = &tmc2209
            stepper = &step_dir_motion_control;
        };

        /* DEVICE_API: stepper_amp api */
        tmc2209: tmc2209 {
            compatible = "adi,tmc2209";
            enable-gpios = <&gpioa 6 GPIO_ACTIVE_HIGH>;
            m0-gpios = <&gpiob 0 GPIO_ACTIVE_HIGH>;
            m1-gpios = <&gpioa 7 GPIO_ACTIVE_HIGH>;
        };

        /* DEVICE_API: stepper api */
        step_dir_motion_control: step_dir_motion_control {
            compatible = "zephyr,gpio-step-dir-stepper";
            step-gpios = <&gpioa 9 GPIO_ACTIVE_HIGH>;
            dir-gpios = <&gpioc 7 GPIO_ACTIVE_HIGH>;
            invert-direction;
            stepper-amp = <&tmc2209>;
        };
    };

Following the aforementioned configurations, the stepper driver subsystem can be used in the application code
as follows:

.. code-block:: c

   static const struct device *stepper = DEVICE_DT_GET(DT_ALIAS(stepper));
   static const struct device *stepper_amp = DEVICE_DT_GET(DT_ALIAS(stepper_amp));
   ...
   stepper_move_to(stepper, 200);
   stepper_stop(stepper);
   stepper_amp_disable(stepper_amp);
