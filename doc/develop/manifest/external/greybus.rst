.. _external_module_greybus:

Greybus
#######

Introduction
************

Greybus is a lightweight, message-based protocol framework that provides a standardized way for
hosts to access hardware functions implemented on remote modules. It was originally developed for
modular systems and defines a set of well-specified operation protocols for common peripheral
classes such as GPIO, I²C, SPI, PWM, and firmware updates.

The Greybus module for Zephyr provides an implementation of the Greybus protocol layers that map
Greybus operations to Zephyr subsystems. When enabled, a Zephyr device can expose its hardware
capabilities to a host using the Greybus protocols. The host discovers available functionality
through Greybus manifest data, then issues class-specific requests that the Zephyr module processes
and responds to.

Initially, Greybus was designed for use over `Unipro`_. However, the protocol itself is mostly
independent of underlying transport. Currently, the Greybus module supports TCP socket as transport.
However, any transport such as UART, I2C, etc should work fine. See the contents of
`this directory <Greybus Transport Directory_>`_ for the current supported transport backends. Feel
free to create PRs for new transport backends.

Greybus is licensed under a combination of Apache-2.0 and BSD-3-Clause license.

Usage with Zephyr
*****************

To pull in the Greybus for Zephyr as a Zephyr module, either add it as a West project in the
west.yaml file or pull it in by adding a submanifest (e.g. ``zephyr/submanifests/greybus.yaml``)
file with the following content and run ``west update``:

.. code-block:: yaml

   manifest:
     projects:
       - name: Greybus-Zephyr
         path: modules/lib/greybus
         revision: main
         url: https://github.com/beagleboard/greybus-zephyr

For instructions regarding greybus subsystem usage, refer to the `Greybus module repository`_ and
`Greybus samples`_.

All current development and real-world testing is done using `BeaglePlay`_ and
`BeagleConnect Freedom`_.

Reference
*********

#. `Greybus Specification`_

#. Christopher Friedt, LPC 2020

   - `Slides <LPC 2020 Slides_>`_
   - `Video <LPC 2020 Video_>`_

.. target-notes::

.. _Greybus Specification: https://github.com/projectara/greybus-spec
.. _LPC 2020 Slides: https://linuxplumbersconf.org/event/7/contributions/814/
.. _LPC 2020 Video: https://youtu.be/n4yiCF2wYeo?t=11683
.. _Greybus module repository: https://github.com/beagleboard/greybus-zephyr
.. _Greybus samples: https://github.com/beagleboard/greybus-zephyr/tree/main/samples/basic
.. _BeaglePlay: https://www.beagleboard.org/boards/beagleplay
.. _BeagleConnect Freedom: https://www.beagleboard.org/boards/beagleconnect-freedom
.. _UniPro: https://en.wikipedia.org/wiki/UniPro
.. _Greybus Transport Directory: https://github.com/beagleboard/greybus-zephyr/tree/main/subsys/greybus/transport
