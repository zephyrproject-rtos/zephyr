.. _external_module_libiio:

libiio
######

Introduction
************

`libiio`_ is an open-source library, developed primarily by Analog Devices, for
interfacing with Linux Industrial Input/Output (IIO) devices. This includes,
but is not limited to, ADCs, DACs, accelerometers, gyroscopes, IMUs, pressure
and temperature sensors, and RF transceivers. libiio can be used natively on
the target itself, or to communicate remotely with a target over USB, Ethernet,
or serial from a host Linux, Windows, or macOS machine.

The libiio repository includes a Zephyr module that turns a Zephyr application
into such a remotely-accessible target. IIO devices and channels integrate with
Zephyr's device model, with built-in drivers adapting existing Zephyr sensor
and ADC/DAC drivers, and a Zephyr port of **iiod**, the IIO daemon, exposes
those devices to a remote host over network sockets, a UART console, USB
CDC-ACM, or a native USB vendor class. The existing libiio Python bindings,
command-line tools (e.g. ``iio_info``), and the `Scopy`_ desktop application
then work against a Zephyr target unmodified, from a host Linux, Windows, or
macOS machine. See the `Zephyr port documentation`_ for the full picture.

The core library is released under the GNU Lesser General Public License (LGPL)
version 2.1, and its examples/test applications are released under the GNU
General Public License (GPL) version 2.0. The Zephyr integration described
above is licensed under the MIT license, as are the core files it statically
links against.

Usage with Zephyr
*****************

To pull in libiio as a Zephyr module, either add it as a West project in the
``west.yml`` file or pull it in by adding a submanifest (e.g.
``zephyr/submanifests/libiio.yaml``) file with the following content and run
``west update``:

.. code-block:: yaml

   manifest:
     remotes:
       - name: analogdevicesinc
         url-base: https://github.com/analogdevicesinc

     projects:
       - name: libiio
         remote: analogdevicesinc
         revision: main
         path: modules/lib/libiio

Reference
*********

.. target-notes::

.. _libiio:
   https://github.com/analogdevicesinc/libiio

.. _Zephyr port documentation:
   https://analogdevicesinc.github.io/libiio/main/zephyr/

.. _Scopy:
   https://analogdevicesinc.github.io/scopy
