.. _bluetooth:

Bluetooth Implementation
########################

Initialization
**************

Initialize the Bluetooth subsystem using :c:func:`bt_init()`. Caller shall be
either task or a fiber. Caller must ensure that function succeeds by checking
return code for errors.

Application Program Interfaces
******************************

+------------------------------------------+------------------------------------------------------+
| Call                                     | Description                                          |
+==========================================+======================================================+
| :c:func:`bt_init()`                      | Initializes the Bluetooth subsystem                  |
+------------------------------------------+------------------------------------------------------+
| :c:func:`bt_start_advertising()`         | Set advertisement and scan data and start            |
|                                          | advertising                                          |
+------------------------------------------+------------------------------------------------------+

TODO: Describe all API

Bluetooth Application Example
*****************************

A simple Bluetooth beacon application is shown below. The application
initializes a Bluetooth Subsystem and enables non-connectable advertising.
It acts as a Bluetooth Low Energy broadcaster.

.. literalinclude:: ../../samples/bluetooth/beacon/src/main.c
   :language: c
   :lines: 33-
   :linenos:

Testing with QEMU
*****************

A Bluetooth application might be tested with QEMU. In order to do so,
a Bluetooth controller needs to be connected to the emulator.

Using Host System Bluetooth Controller in QEMU
==============================================

The host system's Bluetooth controller is connected to the second QEMU
serial line through a UNIX socket
(QEMU option :literal:`-serial unix:/tmp/bt-server-bredr`).
This option is already added to QEMU through :makevar:`QEMU_EXTRA_FLAGS`
in the Makefile.

On the Host side, BlueZ allows to "connect" Bluetooth controller through
a so-called user channel.

#. Use the btproxy tool to open the listening UNIX socket, type:

    .. code-block:: console

        $ sudo tools/btproxy -u
        Listening on /tmp/bt-server-bredr

    .. note:: Ensure that the Bluetooth controller is down before using the
              btproxy command.


#. To run application in the QEMU, type:

    .. code-block:: console

        $ make qemu

Running QEMU now results in a connection with the second serial line to
:literal:`bt-server-bredr` UNIX socket.
Now, an application can use the Bluetooth device.
