.. _stepper_motor_api:

Stepper Motors
##############

The stepper motor subsystem is divided into two APIs, namely:
    - Stepper Motor Device API
    - Stepper Motor Controller API

The stepper motor device API provides following functions:
    - Calibrating a Motor
    - Freewheeling a motor in +/- direction
    - Stopping the Motor
    - Setting various Motor Positions
    - Getting various Motor Positions

The stepper motor controller API provides following functions:
    - Reset Stepper Motor Controller
    - Write to Stepper Motor Controller
    - Read from Stepper Motor Controller
    - Direct Write Access to Stepper Motor Controller Specific Registers
    - Direct Read Access to Stepper Motor Controller Specific Registers


.. todo::

    Add a architecture diagram for the stepper motor api and how it controls a motor


Now the question might arise as to why should the functionalities be divided
into two different APIs. Brief explanation is like this:

    - Business or proprietary logic could be implemented using stepper motor device api
    - Community benefiting implementation of a stepper motor controller could be upstreamed

Stepper Motor Device
********************

Stepper Motor Device API provides a set of functions in order to drive the stepper motor
device. Stepper Motor Device API could be classified into two drive modes.

    1. Positioning Mode
        - Get Motor Position with :c:func:`stepper_motor_get_position`
        - Set Motor Position with :c:func:`stepper_motor_set_position`
    2. Run Mode a.k.a Freewheeling
        - Set run direction and velocity with :c:func:`stepper_motor_run`

By calling :c:func:`stepper_motor_calibrate`, a pre-registered user-defined calibration function of the type
:c:type:`stepper_motor_calibrate_func_t` can be called in order to retrieve the min and max positions of a
stepper motor device. In order to register the calibration function use :c:func:`stepper_motor_register_calibrate_func`.

By calling :c:func:`stepper_motor_reset`, a user can reset certain register values.

:ref:`stepper_motor_device_api_reference` in turn uses :ref:`stepper_motor_controller_api_reference`
in order to actually set the motor in motion

Stepper Motor Controller
************************

Stepper Motor Controller API provides a set of read/write functions in order to communicate with the
stepper motor controller. Stepper Motor Controller API could be classified into two access modes.

    1. Generalized Access
        If a functionality can be generally described for all kinds of motor controller using :c:enum:`motor_driver_info_channel`

        - Read a register with :c:func:`stepper_motor_controller_read`
        - Write a register with :c:func:`stepper_motor_controller_write`
    2. Specialized Access
        If a functionality is highly controller specific

        - Read a register directly with :c:func:`stepper_motor_controller_read_reg`
        - Write a register directly with :c:func:`stepper_motor_controller_write_reg`


Device Tree
***********

In the example below, **my_motor_controller** is a motor bus node. All generic motor controller properties
should be put in **stepper-motor-controller.yaml**. Just as **my_motor_controller** in **on-bus:spi**, **my_motor**
would be a child node of **my_motor_controller** and **on-bus:motor**. Presently some generic stepper motor device
properties have been included in **stepper-motor-device.yaml**.

.. code-block:: devicetree

    &spi_controller { // bus:spi
      status = "okay";

      my_motor_controller: my_motor_controller@0 { // bus:motor & on-bus:spi
        status = "okay";
        compatible = "my_motor,controller";
        reg = <0>;
        #address-cells = <1>;
        #size-cells = <0>;

        my_motor: my_motor_0@0{ //on-bus: motor
          status = "okay";
          compatible = "tmc5041-stepper-motor-device";
          reg=<0>;
          gear-ratio = "1.234567";
          micro-step-res = <256>;
          steps-per-revolution = <200>;
          stall-guard-setting = <6>;
          controller-spec-reg-settings = <SPEC_REG_ADD_FOR_MOTOR_0(0)  SPEC_REG_VALUE_FOR_MOTOR_0>;
        };

        my_motor: my_motor_1@1{ //on-bus: motor
          status = "okay";
          compatible = "tmc5041-stepper-motor-device";
          reg=<1>;
          gear-ratio = "9.876543";
          micro-step-res = <128>;
          steps-per-revolution = <200>;
          stall-guard-setting = <0>;
          controller-spec-reg-settings = <SPEC_REG_ADD_FOR_MOTOR_1(1)  SPEC_REG_VALUE_FOR_MOTOR_1>;
        };
      };
    };

Stepper-motor-controller.yaml offers a **bus: motor** allowing to have child nodes which are **on-bus:motor**
A Stepper Motor Controller that communicates via spi would have following includes in binding.

.. code-block:: console

    include: [stepper-motor-controller.yaml, spi-device.yaml]

whereas the binding of a stepper-motor-device needs to include at least following snippet.

.. code-block:: console

    include: stepper-motor-device.yaml


.. _stepper_motor_device_api_reference:

Stepper Motor Device API
************************

.. doxygengroup:: stepper_motor_device

.. _stepper_motor_controller_api_reference:

Stepper Motor Controller API
****************************

.. doxygengroup:: stepper_motor_controller
