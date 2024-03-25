.. _bluetooth-dev:

Developing Bluetooth Applications
#################################

Bluetooth applications are developed using the common infrastructure and
approach that is described in the :ref:`application` section of the
documentation.

Additional information that is only relevant to Bluetooth applications can be
found on this page.

.. contents::
    :local:
    :depth: 2

Thread safety
*************

Calling into the Bluetooth API is intended to be thread safe, unless otherwise
noted in the documentation of the API function. The effort to ensure that this
is the case for all API calls is an ongoing one, but the overall goal is
formally stated in this paragraph. Bug reports and Pull Requests that move the
subsystem in the direction of such goal are welcome.

.. _bluetooth-hw-setup:

Hardware setup
**************

This section describes the options you have when building and debugging Bluetooth
applications with Zephyr. Depending on the hardware that is available to you,
the requirements you have and the type of development you prefer you may pick
one or another setup to match your needs.

There are 3 possible setups:

#. :ref:`Embedded <bluetooth-hw-setup-embedded>`
#. :ref:`External controller <bluetooth-hw-setup-external-ll>`

   - :ref:`QEMU host <bluetooth-hw-setup-qemu-host>`
   - :ref:`native_sim host <bluetooth-hw-setup-native-sim-host>`

#. :ref:`Simulated nRF5x with BabbleSim <bluetooth-hw-setup-bsim>`

.. _bluetooth-hw-setup-embedded:

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

There is a way to access the :ref:`HCI <bluetooth-hci>` traffic between the Host
and Controller, even if there is no physical transport. See :ref:`Embedded HCI
tracing <bluetooth-embedded-hci-tracing>` for instructions.

.. _bluetooth-hw-setup-external-ll:

Host on Linux with an external Controller
=========================================

.. note::
   This is currently only available on GNU/Linux

This setup relies on a "dual-chip" :ref:`configuration <bluetooth-configs>`
which is comprised of the following devices:

#. A :ref:`Host-only <bluetooth-build-types>` application running in the
   :ref:`QEMU <application_run_qemu>` emulator or the :ref:`native_sim <native_sim>` native
   port of Zephyr
#. A Controller, which can be one of the following types:

   * A commercially available Controller
   * A :ref:`Controller-only <bluetooth-build-types>` build of Zephyr
   * A :ref:`Virtual controller <bluetooth_virtual_posix>`

.. warning::
   Certain external Controllers are either unable to accept the Host to
   Controller flow control parameters that Zephyr sets by default (Qualcomm), or
   do not transmit any data from the Controller to the Host (Realtek). If you
   see a message similar to::

     <wrn> bt_hci_core: opcode 0x0c33 status 0x12

   when booting your sample of choice (make sure you have enabled
   :kconfig:option:`CONFIG_LOG` in your :file:`prj.conf` before running the
   sample), or if there is no data flowing from the Controller to the Host, then
   you need to disable Host to Controller flow control. To do so, set
   ``CONFIG_BT_HCI_ACL_FLOW_CONTROL=n`` in your :file:`prj.conf`.

.. _bluetooth-hw-setup-qemu-host:

QEMU
----

You can run the Zephyr Host on the :ref:`QEMU emulator<application_run_qemu>`
and have it interact with a physical external Bluetooth Controller.

Refer to :ref:`bluetooth_qemu_native` for full instructions on how to build and
run an application in this setup.

.. _bluetooth-hw-setup-native-sim-host:

native_sim
----------

.. note::
   This is currently only available on GNU/Linux

The :ref:`native_sim <native_sim>` target builds your Zephyr application
with the Zephyr kernel, and some minimal HW emulation as a native Linux
executable.

This executable is a normal Linux program, which can be debugged and
instrumented like any other, and it communicates with a physical or virtual
external Controller. Refer to:

- :ref:`bluetooth_qemu_native` for the physical controller
- :ref:`bluetooth_virtual_posix` for the virtual controller

.. _bluetooth-hw-setup-bsim:

Simulated nRF5x with BabbleSim
==============================

.. note::
   This is currently only available on GNU/Linux

The :ref:`nrf52_bsim <nrf52_bsim>` and :ref:`nrf5340bsim <nrf5340bsim>` boards,
are simulated target boards
which emulate the necessary peripherals of a nRF52/53 SOC to be able to develop
and test BLE applications.
These boards, use:

   * `BabbleSim`_ to simulate the nRF5x modem and the radio environment.
   * The POSIX arch and native simulator to emulate the processor, and run natively on your host.
   * `Models of the nrf5x HW <https://github.com/BabbleSim/ext_NRF_hw_models/>`_

Just like with the :ref:`native_sim <native_sim>` target, the build result is a normal Linux
executable.
You can find more information on how to run simulations with one or several
devices in either of :ref:`these boards's documentation <nrf52bsim_build_and_run>`.

With the :ref:`nrf52_bsim <nrf52_bsim>`, typically you do :ref:`Combined builds
<bluetooth-build-types>`, but it is also possible to build the controller with one of the
:ref:`HCI UART <bluetooth-hci-uart-sample>` samples in one simulated device, and the host with
the H4 driver instead of the integrated controller in another simulated device.

With the :ref:`nrf5340bsim <nrf5340bsim>`, you can build with either, both controller and host
on its network core, or, with the network core running only the controller, the application
core running the host and your application, and the HCI transport over IPC.

Initialization
**************

The Bluetooth subsystem is initialized using the :c:func:`bt_enable`
function. The caller should ensure that function succeeds by checking
the return code for errors. If a function pointer is passed to
:c:func:`bt_enable`, the initialization happens asynchronously, and the
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

The key APIs employed by the beacon sample are :c:func:`bt_enable`
that's used to initialize Bluetooth and then :c:func:`bt_le_adv_start`
that's used to start advertising a specific combination of advertising
and scan response data.

More Examples
*************

More :ref:`sample Bluetooth applications <bluetooth-samples>` are available in
``samples/bluetooth/``.

.. _BabbleSim: https://babblesim.github.io/
