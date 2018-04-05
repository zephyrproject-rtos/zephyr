Developing Bluetooth Applications
#################################

Initialization
**************

The Bluetooth subsystem is initialized using the :c:func:`bt_enable()`
function. The caller should ensure that function succeeds by checking
the return code for errors. If a function pointer is passed to
:c:func:`bt_enable()`, the initialization happens asynchronously, and the
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

.. _bluetooth_bluez:

Using BlueZ with Zephyr
***********************

The Linux Bluetooth Protocol Stack, BlueZ, comes with a very useful set of
tools that can be used to debug and interact with Zephyr's BLE Host and
Controller. In order to benefit from these tools you will need to make sure
that you are running a recent version of the Linux Kernel and BlueZ:

* Linux Kernel 4.10+
* BlueZ 4.45+

Additionally, some of the BlueZ tools might not be bundled by default by your
Linux distribution. If you need to build BlueZ from scratch to update to a
recent version or to obtain all of its tools you can follow the steps below:

.. code-block:: console

   git clone git://git.kernel.org/pub/scm/bluetooth/bluez.git
   cd bluez
   ./bootstrap-configure --disable-android --disable-midi
   make

You can then find :file:`btattach`, :file:`btmgt` and :file:`btproxy` in the
:file:`tools/` folder and :file:`btmon` in the :file:`monitor/` folder.

You'll need to enable BlueZ's experimental features so you can access its
most recent BLE functionality. Do this by editing the file
:file:`/lib/systemd/system/bluetooth.service`
and making sure to include the :literal:`-E` option in the daemon's execution
start line:

.. code-block:: console

   ExecStart=/usr/libexec/bluetooth/bluetoothd -E

Finally you can reload and restart the daemon:

.. code-block:: console

   sudo systemctl daemon-reload
   sudo systemctl restart bluetooth

.. _bluetooth_qemu:

Testing with QEMU
*****************

It's possible to test Bluetooth applications using QEMU. In order to do
so, a Bluetooth controller needs to be exported from the host OS (Linux)
to the emulator. For this purpose you will need some tools described in the
:ref:`bluetooth_bluez` section.

Using Host System Bluetooth Controller in QEMU
==============================================

The host OS's Bluetooth controller is connected to the second QEMU
serial line using a UNIX socket. This socket gets used with the help of
the QEMU option :literal:`-serial unix:/tmp/bt-server-bredr`. This
option gets passed to QEMU through :makevar:`QEMU_EXTRA_FLAGS`
automatically whenever an application has enabled Bluetooth support.

On the host side, BlueZ allows to export its Bluetooth controller
through a so-called user channel for QEMU to use:

#. Make sure that the Bluetooth controller is down

#. Use the btproxy tool to open the listening UNIX socket, type:

   .. code-block:: console

      sudo tools/btproxy -u -i 0
      Listening on /tmp/bt-server-bredr

   You might need to replace :literal:`-i 0` with the index of the Controller
   you wish to proxy.

#. Choose one of the Bluetooth sample applications located in
   :literal:`samples/bluetooth`.

#. To run a Bluetooth application in QEMU, type:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/<sample>
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

Running QEMU now results in a connection with the second serial line to
the :literal:`bt-server-bredr` UNIX socket, letting the application
access the Bluetooth controller.

.. _bluetooth_ctlr_bluez:

Testing Zephyr-based Controllers with BlueZ
*******************************************

If you want to test a Zephyr-powered BLE Controller using BlueZ's Bluetooth
Host, you will need a few tools described in the :ref:`bluetooth_bluez` section.
Once you have installed the tools you can then use them to interact with your
Zephyr-based controller:

   .. code-block:: console

      sudo btmgmt --index 0
      [hci0]# auto-power
      [hci0]# find -l

You might need to replace :literal:`--index 0` with the index of the Controller
you wish to manage.
Additional information about :file:`btmgmt` can be found in its manual pages.

