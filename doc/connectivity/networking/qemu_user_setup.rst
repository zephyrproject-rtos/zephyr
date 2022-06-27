.. _networking_with_user_qemu:

Networking with QEMU User
#############################

.. contents::
    :local:
    :depth: 2

This page is intended to serve as a starting point for anyone interested in
using QEMU SLIRP with Zephyr.

Introduction
*************

SLIRP is a network backend which provides the complete TCP/IP stack within
QEMU and uses that stack to implement a virtual NAT'd network. As there are
no dependencies on the host, SLIRP is simple to setup.

By default, QEMU uses the ``10.0.2.X/24`` network and runs a gateway at
``10.0.2.2``. All traffic intended for the host network has to travel through
this gateway, which will filter out packets based on the QEMU command line
parameters. This gateway also functions as a DHCP server for all GOS,
allowing them to be automatically assigned with an IP address starting from
``10.0.2.15``.

More details about User Networking can be obtained from here:
https://wiki.qemu.org/Documentation/Networking#User_Networking_.28SLIRP.29

Using SLIRP with Zephyr
************************

In order to use SLIRP with Zephyr, the user has to set the Kconfig option to
enable User Networking.

.. code-block:: console

   CONFIG_NET_QEMU_USER=y

Once this configuration option is enabled, all QEMU launches will use SLIRP.
In the default configuration, Zephyr only enables User Networking, and does
not pass any arguments to it. This means that the Guest will only be able to
communicate to the QEMU gateway, and any data intended for the host machine
will be dropped by QEMU.

In general, QEMU User Networking can take in a lot of arguments including,

* Information about host/guest port forwarding. This must be provided to
  create a communication channel between the guest and host.
* Information about network to use. This may be valuable if the user does
  not want to use the default ``10.0.2.X`` network.
* Tell QEMU to start DHCP server at user-defined IP address.
* ID and other information.

As this information varies with every use case, it is difficult to come up
with good defaults that work for all. Therefore, Zephyr Implementation
offloads this to the user, and expects that they will provide arguments
based on requirements. For this, there is a Kconfig string which can be
populated by the user.

.. code-block:: console

   CONFIG_NET_QEMU_USER_EXTRA_ARGS="net=192.168.0.0/24,hostfwd=tcp::8080-:8080"

This option is appended as-is to the QEMU command line. Therefore, any problems with
this command line will be reported by QEMU only. Here's what this particular
example will do,

* Make QEMU use the ``192.168.0.0/24`` network instead of the default.
* Enable forwarding of any TCP data received from port 8080 of host to port
  8080 of guest, and vice versa.

Limitations
*************

If the user does not have any specific networking requirements other than the
ability to access a web page from the guest, user networking (slirp) is a
good choice. However, it has several limitations

* There is a lot of overhead so the performance is poor.
* The guest is not directly accessible from the host or the external network.
* In general, ICMP traffic does not work (so you cannot use ping within a guest).
* As port mappings need to be defined before launching qemu, clients which use
  dynamically generated ports cannot communicate with external network.
* There is a bug in the SLIRP implementation which filters out all IPv6 packets
  from the guest. See https://bugs.launchpad.net/qemu/+bug/1724590 for details.
  Therefore, IPv6 will not work with User Networking.
