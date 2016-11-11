Developing Bluetooth Applications
#################################

Initialization
**************

The Bluetooth subsystem is initialized using the :c:func:`bt_init()`
function. The caller should ensure that function succeeds by checking
the return code for errors. If a function pointer is passed to
:c:func:`bt_init()` the initialization happens synchronously and the
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

The key APIs employed by the beacon sample are :c:func:`bt_enable()`
that's used to initialize Bluetooth and then :c:func:`bt_le_adv_start()`
that's used to start advertising a specific combination of advertising
and scan response data.

Testing with QEMU
*****************

It's possible to test Bluetooth applications using QEMU. In order to do
so, a Bluetooth controller needs to be exported from the host OS (Linux)
to the emulator.

Using Host System Bluetooth Controller in QEMU
==============================================

The host OS's Bluetooth controller is connected to the second QEMU
serial line using a UNIX socket. This socket employs the QEMU option
:literal:`-serial unix:/tmp/bt-server-bredr`. This option is already
added to QEMU through :makevar:`QEMU_EXTRA_FLAGS` in most Bluetooth
sample Makefiles' and made available through the 'qemu' make target.

On the host side, BlueZ allows to export its Bluetooth controller
through a so-called user channel for QEMU to use:

#. Make sure that the Bluetooth controller is down

#. Use the btproxy tool to open the listening UNIX socket, type:

   .. code-block:: console

      $ sudo tools/btproxy -u
      Listening on /tmp/bt-server-bredr

#. Choose one of the Bluetooth sample applications located in
   :literal:`samples/bluetooth`.

#. To run Bluetooth application in QEMU, type:

   .. code-block:: console

      $ make qemu

Running QEMU now results in a connection with the second serial line to
the :literal:`bt-server-bredr` UNIX socket, letting the application
access the Bluetooth controller.
