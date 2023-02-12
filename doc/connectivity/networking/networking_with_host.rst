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
   qemu_user_setup.rst
   networking_with_multiple_instances.rst
   qemu_802154_setup.rst
   armfvp_user_networking_setup.rst

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

* QEMU using SLIRP (Qemu User Networking).

  * QEMU User Networking is implemented using "slirp", which provides a full TCP/IP
    stack within QEMU and uses that stack to implement a virtual NAT'd network. As
    this support is built into QEMU, it can be used with any model and requires no
    admin privileges on the host machine, unlike TAP. However, it has several
    limitations including performance which makes it less valuable for practical
    purposes. See :ref:`networking_with_user_qemu` for details.

* Arm FVP (User Mode Networking).

  * User mode networking emulates a built-in IP router and DHCP server, and
    routes TCP and UDP traffic between the guest and host. It uses the user mode
    socket layer of the host to communicate with other hosts. This allows
    the use of a significant number of IP network services without requiring
    administrative privileges, or the installation of a separate driver on
    the host on which the model is running. See :ref:`networking_with_armfvp`
    for details.

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

* Connecting multiple Zephyr instances together.

  * If you have multiple Zephyr instances, either QEMU or native_posix ones,
    and want to create a connection between them, see
    :ref:`networking_with_multiple_instances` for details.

* Simulating IEEE 802.15.4 network between two QEMUs.

  * Here, two Zephyr instances are running and there is IEEE 802.15.4 link layer
    run over an UART between them.
    See :ref:`networking_with_ieee802154_qemu` for details.
