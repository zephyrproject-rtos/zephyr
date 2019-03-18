.. _bluetooth-dev:

Developing Bluetooth Applications
#################################

Bluetooth applications are developed using the common infrastructure and
approach that is described in the :ref:`application` section of the
documentation.

Additional information that is only relevant to Bluetooth applications can be
found in this page.

.. _bluetooth-hw-setup:

Hardware setup
**************

This section describes the options you have when building and debugging Bluetooth
applications with Zephyr. Depending on the hardware that is available to you,
the requirements you have and the type of development you prefer you may pick
one or another setup to match your needs.

There are 4 possible hardware setups to use with Zephyr and Bluetooth:

#. Embedded
#. QEMU with an external Controller
#. Native POSIX with an external Controller
#. Simulated nRF52 with BabbleSim

Embedded
========

This setup relies on all software running directly on the embedded platform(s)
that the application is targeting.
All the :ref:`bluetooth-configs` and :ref:`bluetooth-build-types` are supported
but you might need to build Zephyr more than once if you are using a dual-chip
configuration or if you have multiple cores in your SoC each running a different
build type (e.g., one running the Host, the other the Controller).

To start developing using this setup follow the :ref:`Getting Started Guide
<getting_started>`, choose one (or more if you are using a dual-chip solution)
boards that support Bluetooth and then :ref:`run the application
<application_run_board>`).

.. _bluetooth-hci-tracing:

Embedded HCI tracing
--------------------

When running both Host and Controller in actual Integrated Circuits, you will
only see normal log messages on the console by default, without any way of
accessing the HCI traffic between the Host and the Controller.  However, there
is a special Bluetooth logging mode that converts the console to use a binary
protocol that interleaves both normal log messages as well as the HCI traffic.
Set the following Kconfig options to enable this protocol before building your
application:

.. code-block:: console

   CONFIG_BT_DEBUG_MONITOR=y
   CONFIG_UART_CONSOLE=n

Setting :option:`CONFIG_BT_DEBUG_MONITOR` to ``y`` replaces the
:option:`CONFIG_BT_DEBUG_LOG` option, and setting :option:`CONFIG_UART_CONSOLE`
to ``n`` disables the default ``printk``/``printf`` hooks.

To decode the binary protocol that will now be sent to the console UART you need
to use the btmon tool from :ref:`BlueZ <bluetooth_bluez>`:

.. code-block:: console

   $ btmon --tty <console TTY> --tty-speed 115200

QEMU with an external Controller
================================

.. note::
   This is currently only available on GNU/Linux

This setup relies on a "dual-chip" :ref:`configuration <bluetooth-configs>`
which is comprised of the following devices:

#. A :ref:`Host-only <bluetooth-build-types>` application running in the
   :ref:`QEMU <application_run_qemu>` emulator
#. A Controller, which can be one of two types:

   * A commercially available Controller
   * A :ref:`Controller-only <bluetooth-build-types>` build of Zephyr

Refer to :ref:`bluetooth_qemu_posix` for full instructions on how to build and
run an application in this setup.

Native POSIX with an external Controller
========================================

.. note::
   This is currently only available on GNU/Linux

The :ref:`Native POSIX <native_posix>` target builds your Zephyr application
with the Zephyr kernel, and some minimal HW emulation as a native Linux
executable.
This executable is a normal Linux program, which can be debugged and
instrumented like any other.

Just like with QEMU, you also need to use a combination of two devices:

#. A :ref:`Host-only <bluetooth-build-types>` application running in
   native_posix as a Linux application
#. A Controller, which can be one of two types:

   * A commercially available Controller
   * A :ref:`Controller-only <bluetooth-build-types>` build of Zephyr

Refer to :ref:`bluetooth_qemu_posix` for full instructions on how to build and
run an application in this setup.

Simulated nRF52 with BabbleSim
==============================

.. note::
   This is currently only available on GNU/Linux

The :ref:`nrf52_bsim board <nrf52_bsim>`, is a simulated target board
which emulates the necessary peripherals of a nrf52 SOC to be able to develop
and test BLE applications.
This board, uses:

   * `BabbleSim`_ to simulate the nrf52 modem and the radio environment.
   * The POSIX arch to emulate the processor.
   * `Models of the nrf52 HW <https://github.com/BabbleSim/ext_NRF52_hw_models/>`_

Just like with the ``native_posix`` target, the build result is a normal Linux
executable.
You can find more information on how to run simulations with one or several
devices in
:ref:`this board's documentation <nrf52bsim_build_and_run>`

Currently, only :ref:`Combined builds
<bluetooth-build-types>` are possible, as this board does not yet have
any models of a UART, or USB which could be used for an HCI interface towards
another real or simulated device.


Initialization
**************

The Bluetooth subsystem is initialized using the :cpp:func:`bt_enable`
function. The caller should ensure that function succeeds by checking
the return code for errors. If a function pointer is passed to
:cpp:func:`bt_enable`, the initialization happens asynchronously, and the
completion is notified through the given function.

Bluetooth Application Example
*****************************

A simple Bluetooth beacon application is shown below. The application
initializes the Bluetooth Subsystem and enables non-connectable
advertising, effectively acting as a Bluetooth Low Energy broadcaster.

.. literalinclude:: ../../../samples/bluetooth/beacon/src/main.c
   :language: c
   :lines: 19-
   :linenos:

The key APIs employed by the beacon sample are :cpp:func:`bt_enable`
that's used to initialize Bluetooth and then :cpp:func:`bt_le_adv_start`
that's used to start advertising a specific combination of advertising
and scan response data.

.. _BabbleSim: https://babblesim.github.io/
