Stepper API Tests
#################

Tests the general functionality and behaviour of stepper drivers to ensure conformity to the stepper
api specification. All stepper drivers should pass this test suite. It should be noted, that this
test suite is focused solely on the api and thus does not test actual pin outputs.

Hardware Setup
==============

The pin assignment for the boards can be found in the corresponding overlay files.

Further setup characteristics are as following:

ADI TMC2209
___________

Tests on the ``Nucleo F767ZI`` and ``Mimxrt1060-EVKB`` boards are executed without actual stepper controllers, similar
to the tests in the simulation environments.

Allegro A4979
_____________

Tests on the ``Nucleo F767ZI`` and ``Mimxrt1060-EVKB`` boards are executed without actual stepper controllers, similar
to the tests in the simulation environments.

TI DRV8424
__________

Tests on the ``Nucleo F767ZI`` and ``Mimxrt1060-EVKB`` boards use the `Mikroe Stepper 19 Click
<https://www.mikroe.com/stepper-19-click>`_ shield (which is currently not available as a dedicated shield in Zephyr).
