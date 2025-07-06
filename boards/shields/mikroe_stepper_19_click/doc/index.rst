.. _mikroe_stepper_19_click_shield:

MikroElektronika Stepper 19 Click
#################################

Overview
********

The MikroElektronika `Stepper 19 Click`_ shield has a `TI DRV8424`_ stepper driver accessed via
GPIO and a `NXP PCA9538A`_ GPIO expander accessed via I2C. Some DRV8424 pins are accessed
via the GPIO expander.

.. figure:: stepper_19_click.webp
   :align: center
   :alt: MikroElektronika Stepper 19 Click

   MikroElektronika Stepper 19 Click (Credit: MikroElektronika)

Requirements
************

The shield uses a mikroBUS interface. The target board must define
a ``mikrobus_i2c`` and ``mikrobus_header``  node labels
(see :ref:`shields` for more details).

Programming
***********

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/stepper/generic/
   :board: <board>
   :shield: mikroe_stepper_19_click
   :goals: build flash

References
**********

.. target-notes::

.. _Stepper 19 Click:
   https://www.mikroe.com/stepper-19-click

.. _TI DRV8424:
   https://www.ti.com/product/DRV8424

.. _NXP PCA9538A:
   https://www.nxp.com/products/interfaces/ic-spi-i3c-interface-devices/general-purpose-i-o-gpio/low-voltage-8-bit-ic-bus-i-o-port-with-interrupt-and-reset:PCA9538A
