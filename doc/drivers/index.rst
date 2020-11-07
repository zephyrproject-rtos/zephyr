.. _device_drivers:

Device Drivers
##############

This page is an index of device drivers. Each device driver implements a Zephyr
:ref:`API <api_overview>`. Device drivers are responsible for creating the
:c:struct:`device` structures at the heart of Zephyr's :ref:`device and driver
model <device_model_api>`.

To enable a device driver -- i.e. compile and link the driver source code into
your Zephyr :ref:`application <application>` -- you will typically need to:

- enable the driver using :ref:`Kconfig <kconfig>`
- configure one or more devices using :ref:`devicetree <dt-guide>`

In order to use the device driver, you will need a pointer to a ``struct
device`` created by the driver source. This is done with the
:c:func:`device_get_binding` function, which requires the name of the device.
For help getting a pointer to a device configured in the devicetree, see
:ref:`dt-get-device`.

.. Note: there's nothing else in this directory because the toctree is pulling
   in files created by gen_driver_rest.py.

.. TODO: figure out how to glob this; "*/index.rst" didn't work with plain :glob:

.. toctree::

   adc/index.rst
   clock_control/index.rst
   counter/index.rst
   entropy/index.rst
   flash/index.rst
   gpio/index.rst
   hwinfo/index.rst
   i2c/index.rst
   ipm/index.rst
   pwm/index.rst
   sensor/index.rst
   serial/index.rst
   spi/index.rst
   usb/index.rst
   watchdog/index.rst
