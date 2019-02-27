.. _networking_with_host:

Networking with the host system
###############################

.. toctree::
   :maxdepth: 1
   :hidden:

   native_posix_setup.rst
   qemu_eth_setup.rst
   qemu_setup.rst
   usbnet_setup.rst

While developing networking software, it is usually necessary to connect and
exchange data with the host system like a Linux desktop computer.
Depending on what board is used for development, the following options are
possible:

* QEMU using SLIP (Serial Line Internet Protocol).

  * Here IP packets are exchanged between Zephyr and the host system via serial
    port. This is the legacy way of transferring data. It is also quite slow so
    use it only when necessary. See :ref:`networking_with_qemu` for details.

* QEMU using built-in Ethernet driver.

  * Here IP packets are exchanged between Zephyr and the host system via QEMU's
    built-in Ethernet driver. Not all QEMU boards support built-in Ethernet so
    in some cases, you might need to use the SLIP method for host connectivity.
    See :ref:`networking_with_eth_qemu` for details.

* native_posix board.

  * The Zephyr instance can be executed as a user space process in the host
    system. This is the most convenient way to debug the Zephyr system as one
    can attach host debugger directly to the running Zephyr instance. This
    requires that there is an adaptation driver in Zephyr for interfacing
    with the host system. An Ethernet driver exists in Zephyr for this purpose.
    See :ref:`networking_with_native_posix` for details.

* USB device networking.

  * Here, the Zephyr instance is run on a real board and the connectivity to
    the host system is done via USB.
    See :ref:`usb_device_networking_setup` for details.
