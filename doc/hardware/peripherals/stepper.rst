.. _stepper_api:

Steppers
########

The stepper driver API provides a set of functions for controlling and configuring stepper drivers.

Configure Stepper Driver
========================

- **Enable** the stepper driver using :c:func:`stepper_enable`.
- Configure **micro-stepping resolution** using :c:func:`stepper_set_micro_step_res`
  and :c:func:`stepper_get_micro_step_res`.
- Configure **actual position a.k.a step count** in microsteps using :c:func:`stepper_set_actual_position`
  and :c:func:`stepper_get_actual_position`.
- Set **max velocity** in micro-steps per second using :c:func:`stepper_set_max_velocity`

Control Stepper
===============

- **Move by** +/- micro-steps a.k.a. **relative movement** using :c:func:`stepper_move`.
- **Move to** a specific position a.k.a. **absolute movement** using :c:func:`stepper_set_target_position`.
- Run incessantly with a **constant velocity** in a certain direction until
  a stop is detected using :c:func:`stepper_enable_constant_velocity_mode`.
- Check if the stepper is **moving** using :c:func:`stepper_is_moving`.

Device Tree
===========

In the context of stepper controllers  device tree provides the initial hardware
configuration for stepper drivers on a per device level. Each device must specify
a device tree binding in Zephyr, and ideally, a set of hardware configuration options
for things such as current settings, ramp parameters and furthermore. These can then
be used in a boards devicetree to configure a stepper driver to its initial state, as
shown in the example of analog devices tmc5041 below.

.. code-block:: dts

   #include <zephyr/dt-bindings/stepper/adi/tmc5041_reg.h>

   &spi0 {
       /* SPI bus options here, not shown */

       tmc5041: tmc5041@0 { /* Dual controller/driver for up to two 2-phase bipolar stepper motors */
           compatible = "adi,tmc5041";
           reg = <0>;
           spi-max-frequency = <DT_FREQ_M(24)>; /* Maximum SPI bus frequency */

           #address-cells = <1>;
           #size-cells = <0>;

           poscmp_enable; test_mode; lock_gconf; /* ADI TMC Global configuration flags */
           clock-frequency = <DT_FREQ_M(16)>; /* Internal/External Clock frequency */

           motor: motor@0 {
               status = "okay";
               reg=<0>;

               /* common stepper controller settings */
               invert-direction;
               micro-step-res = <256>;

               /* ADI TMC stallguard settings specific to TMC5041 */
               activate-stallguard2;
               stallguard-velocity-check-interval-ms=<100>;
               stallguard2-threshold=<9>;
               stallguard-threshold-velocity=<500000>;

               /* ADI TMC ramp generator as well as current settings */
               vstart = <10>;
               a1 = <20>;
               v1 = <30>;
               d1 = <40>;
               vmax = <50>;
               amax = <60>;
               dmax = <70>;
               tzerowait = <80>;
               vhigh = <90>;
               vcoolthrs = <100>;
               ihold = <1>;
               irun = <2>;
               iholddelay = <3>;
           };
    };

Discord
=======

Zephyr has a `stepper discord`_ channel for stepper related discussions, which
is open to all.

.. _stepper-api-reference:

API Reference
*************

A common set of functions which should be implemented by all stepper drivers.

.. doxygengroup:: stepper_interface

Additional functions for Trinamic stepper drivers.

.. doxygengroup:: trinamic_stepper_interface

.. _stepper discord:
   https://discord.com/channels/720317445772017664/1278263869982375946
