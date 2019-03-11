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
#. Native POSIX with BabbleSim

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

The :ref:`Native POSIX <native_posix>` simulator allows Zephyr builds to run as
a native POSIX applications. Just like with QEMU, you also need to use a
combination of two devices:

#. A :ref:`Host-only <bluetooth-build-types>` application running as a native
   POSIX application
#. A Controller, which can be one of two types:

   * A commercially available Controller
   * A :ref:`Controller-only <bluetooth-build-types>` build of Zephyr

Refer to :ref:`bluetooth_qemu_posix` for full instructions on how to build and
run an application in this setup.

Native POSIX with BabbleSim
===========================

.. note::
   This is currently only available on GNU/Linux

`BabbleSim`_ is a simulator of the physical layer of shared medium networks,
and it can be used to develop BLE applications with Zephyr when combined with
the Native POSIX target.
When developing with BabbleSim, only :ref:`Combined builds
<bluetooth-build-types>` are possible, since the whole Linux application is a
single entity that communicates with the simulator.

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
