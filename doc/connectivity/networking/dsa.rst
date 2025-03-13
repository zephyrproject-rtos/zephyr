.. _dsa:

Distributed Switch Architecture (DSA)
#####################################

.. contents::
    :local:
    :depth: 2

Distributed Switch Architecture (DSA) is not something new. It has been a mature
subsystem in Linux for many years. This document just skips the background,
terms and any knowledge related description, as user may find all of these in
`Linux DSA documentation`_.


Architecture
************

Same with Linux, from network device perspective the DSA architecture is as below.

.. code-block:: console

                Unaware application
              opens and binds socket
                       |  ^
                       |  |
           +-----------v--|--------------------+
           |+------+ +------+ +------+ +------+|
           || swp0 | | swp1 | | swp2 | | swp3 ||
           |+------+-+------+-+------+-+------+|
           |          DSA switch driver        |
           +-----------------------------------+
                         |        ^
            Tag added by |        | Tag consumed by
           switch driver |        | switch driver
                         v        |
           +-----------------------------------+
           | Unmodified host interface driver  | Software
   --------+-----------------------------------+------------
           |       Host interface (eth0)       | Hardware
           +-----------------------------------+
                         |        ^
         Tag consumed by |        | Tag added by
         switch hardware |        | switch hardware
                         v        |
           +-----------------------------------+
           |               Switch              |
           |+------+ +------+ +------+ +------+|
           || swp0 | | swp1 | | swp2 | | swp3 ||
           ++------+-+------+-+------+-+------++

Host interface
**************

Host interface network devices use regular and unmodified ethernet driver working
as DSA master port, which manages the switch through processor.

Switch interface
****************

The switch interfaces are also exposed as standard ethernet interfaces in zephyr.
The one connected to master port work as CPU port, and the others for user purpose
work as slave ports.

Switch tagging protocols
************************

Generally, switch tagging protocols are vendor specific. They all contain something which:

- identifies which port the Ethernet frame came from/should be sent to
- provides a reason why this frame was forwarded to the management interface

And tag on the packets to give them a switch frame header. But there are also tag-less case.
It depends on the vendor.

Networking stack process
************************

In order to have the DSA subsystem process the Ethernet switch specific tagging protocol
via master port.

For RX path, put ``dsa_recv()`` at beginning of ``net_recv_data()`` in ``subsys/net/ip/net_core.c``
to handle first for untagging and re-directing interface.

For TX path, the switch interfaces register as standard ethernet devices with ``dsa_xmit()``
as ``ethernet_api->send``. The ``dsa_xmit()`` processes the tagging and re-directing to master
port work.

DSA device driver support
*************************

As the DSA core driver interacts with subsystems/drivers including MDIO, PHY and device tree to
to support the common DSA setup and working process. The device driver support is much easy.

For device tree, the switch description should be following the ``dts/bindings/dsa/dsa.yaml``.

For device driver, all needed to provide are ``dsa_api`` and private data, and then call
``DSA_INIT_INSTANCE(n, _dapi, data)``.

Common pitfalls using DSA setups
********************************

This is copied from Linux DSA documentation. It applies to zephyr too. Although master port and
cpu port exposed as ethernet device in zephyr, they are not able to be used.

.. code-block:: console

   Once a conduit network device is configured to use DSA (dev->dsa_ptr becomes non-NULL), and
   the switch behind it expects a tagging protocol, this network interface can only exclusively
   be used as a conduit interface. Sending packets directly through this interface (e.g.: opening
   a socket using this interface) will not make us go through the switch tagging protocol transmit
   function, so the Ethernet switch on the other end, expecting a tag will typically drop this frame.

TODO work
*********

Comparing to Linux, there are too much features to support in/based on zephyr DSA. But basically
bridge layer should be supported. Then DSA could provide two options for users to use switch ports.

- Standalone mode: all slave ports work as regular ethernet devices. No switching.
- Bridge mode: switch mode enabled with adding slave ports into virtual bridge device. IP address could
  be assigned to the bridge.

.. _Linux DSA documentation:
   https://www.kernel.org/doc/html/latest/networking/dsa/dsa.html
