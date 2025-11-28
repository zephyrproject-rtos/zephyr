.. _api_peripherals:

Peripherals
###########

..
  Please keep the ToC trees sorted based on the titles of the pages included

.. grid:: 1 1 2 3
    :gutter: 2

    .. grid-item-card:: :material-twotone:`show_chart;36px` Analog
        :link: analog_peripherals
        :link-type: ref

        Analog-to-Digital and Digital-to-Analog converters, Comparators, and Amplifiers.

    .. grid-item-card:: :material-twotone:`speaker;36px` Audio
        :link: audio_reference
        :link-type: ref

        Audio interfaces and sound processing.

    .. grid-item-card:: :material-twotone:`settings_ethernet;36px` Communication
        :link: communication_peripherals
        :link-type: ref

        Serial buses (I2C, SPI, UART, CAN) and other communication interfaces.

    .. grid-item-card:: :material-twotone:`monitor;36px` Display
        :link: display_peripherals
        :link-type: ref

        Visual output, video interfaces, and LED control.

    .. grid-item-card:: :material-twotone:`touch_app;36px` GPIO & Input
        :link: gpio_input_peripherals
        :link-type: ref

        General Purpose I/O, tactile feedback, and input devices.

    .. grid-item-card:: :material-twotone:`save;36px` Memory & Storage
        :link: memory_storage_peripherals
        :link-type: ref

        Non-volatile memory, flash storage, and EEPROM.

    .. grid-item-card:: :material-twotone:`battery_charging_full;36px` Power
        :link: power_peripherals
        :link-type: ref

        Power management, charging, regulators, and reset controllers.

    .. grid-item-card:: :material-twotone:`sensors;36px` Sensors
        :link: sensor_peripherals
        :link-type: ref

        Environmental and motion sensors, GNSS, and sensor interfaces.

    .. grid-item-card:: :material-twotone:`precision_manufacturing;36px` Motion & Actuation
        :link: motion_actuation_peripherals
        :link-type: ref

        PWM, stepper motors, haptics, and other actuators.

    .. grid-item-card:: :material-twotone:`bug_report;36px` System
        :link: system_diagnostics_peripherals
        :link-type: ref

        System information, synchronization, DMA, buses, and diagnostics.

    .. grid-item-card:: :material-twotone:`schedule;36px` Timing
        :link: timing_peripherals
        :link-type: ref

        Timers, clocks, watchdogs, and timekeeping primitives.

.. toctree::
   :maxdepth: 2
   :hidden:

   audio/index.rst


.. _analog_peripherals:

Analog
******

.. toctree::
   :maxdepth: 1

   adc.rst
   dac.rst
   comparator.rst
   opamp.rst

.. _communication_peripherals:

Communication
*************

.. toctree::
   :maxdepth: 1

   can/index.rst
   i2c.rst
   i3c.rst
   spi.rst
   pcie.rst
   uart.rst
   espi.rst
   mdio.rst
   mspi.rst
   smbus.rst
   ipm.rst
   mbox.rst
   w1.rst

.. _display_peripherals:

Display
*******

.. toctree::
   :maxdepth: 1

   display/index.rst
   auxdisplay.rst
   video.rst
   led.rst

.. _gpio_input_peripherals:

GPIO & Input
************

.. toctree::
   :maxdepth: 1

   gpio.rst
   ps2.rst
   tgpio.rst

.. _memory_storage_peripherals:

Memory & Storage
****************

.. toctree::
   :maxdepth: 1

   flash.rst
   eeprom/index.rst
   sdhc.rst
   bbram.rst
   retained_mem.rst
   i2c_eeprom_target.rst

.. _power_peripherals:

Power
*****

.. toctree::
   :maxdepth: 1

   bc12.rst
   charger.rst
   fuel_gauge.rst
   regulators.rst
   reset.rst
   tcpc.rst
   usbc_vbus.rst

.. _sensor_peripherals:

Sensors
*******

.. toctree::
   :maxdepth: 1

   sensor/index.rst
   gnss.rst
   psi5.rst
   sent.rst

.. _motion_actuation_peripherals:

Motion & Actuation
******************

.. toctree::
   :maxdepth: 1

   haptics.rst
   pwm.rst
   stepper.rst

.. _system_diagnostics_peripherals:

System & Diagnostics
********************

.. toctree::
   :maxdepth: 1

   hwinfo.rst
   hwspinlock.rst
   entropy.rst
   edac/index.rst
   coredump.rst
   crc.rst
   peci.rst
   dma.rst

.. _timing_peripherals:

Timing
******

.. toctree::
   :maxdepth: 1

   clock_control.rst
   counter.rst
   rtc.rst
   watchdog.rst
