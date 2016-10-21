BMI160 Sample Application
##########################

For Arduino 101 users:
=====================

Currently, the GPIO controller driver does not allow for AON GPIO interrupts
to be received on ARC core. Hence, IPM is used to relay the interrupts from x86
to ARC core.

Hence, Arduino 101 users have to build and flash both x86 and arc applications
in order to test the trigger functionality.

By default, the applications are configured to build for Arduino 101.

For other boards:
=================

For boards that have BMI160 connected separately, not in a Curie module (as
it's the case for Arduino 101), the sensor subsystem application is all that's needed,
provided the user configures the driver to use a standalone GPIO.

Also, if the BMI160 is connected to the x86 core, the sensor subsystem (arc)
application makefile has to be changed accordingly.
