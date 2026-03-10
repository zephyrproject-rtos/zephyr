.. _stepper-individual-controller-driver:

Individual Stepper Motion Controller and Driver
###############################################

Following is an example of a device tree configuration for a stepper driver with a dedicated stepper motion
controller:

.. code-block:: dts

    / {
        aliases {
            stepper_driver = &tmc2209
            stepper_ctrl = &step_dir_motion_control;
        };

        tmc2209: tmc2209 {
            compatible = "adi,tmc2209";
            enable-gpios = <&gpioa 6 GPIO_ACTIVE_HIGH>;
            m0-gpios = <&gpiob 0 GPIO_ACTIVE_HIGH>;
            m1-gpios = <&gpioa 7 GPIO_ACTIVE_HIGH>;
        };

        step_dir_motion_control: step-dir-motion-control {
            compatible = "zephyr,gpio-step-dir-stepper-ctrl";
            step-gpios = <&gpioa 9 GPIO_ACTIVE_HIGH>;
            dir-gpios = <&gpioc 7 GPIO_ACTIVE_HIGH>;
            invert-direction;
            stepper-driver = <&tmc2209>;
        };
    };

Following the aforementioned configurations, the stepper driver subsystem can be used in the application code
as follows:

.. code-block:: c

   static const struct device *stepper_driver = DEVICE_DT_GET(DT_ALIAS(stepper_driver));
   static const struct device *stepper_ctrl = DEVICE_DT_GET(DT_ALIAS(stepper_ctrl));
   ...
   stepper_ctrl_move_to(stepper_ctrl, 200);
   stepper_ctrl_stop(stepper_ctrl);
   stepper_disable(stepper_driver);
