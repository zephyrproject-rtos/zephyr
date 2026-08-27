.. zephyr:code-sample:: t1s
   :name: 10BASE-T1S tests (OA_TC6)

   Setup tests to examine long term stability of 10BASE-T1S communication


10BASE-T1S Network (OA_TC6) Tests
#################################

Overview
********

This test setup checks if long term stability and operation of the
T1S communication is preserved.

Prerequisites
*************

HW prerequisites:
=================

* `Raspberry PI4 <rpi4-spec_>`_ with Debian 11.9 (v.6.1 kernel) and
  `LAN8670-USB dongle <evb-lan8670_>`_ setup as 10 BASE T1S master (HOST)
* Nucleo (:zephyr:board:`nucleo_g474re`) connected to `LAN8651 "micro BUS" module from
  microe.com <microe-lan8651_>`_ (one or more) (DUTs)

.. note::

    The lan867x-linux-driver-2v2/linux-v6.1.21-support IPv6 is not working on
    PC's Debian 12. Hence, the move to RPI4 is recommended.

Connections and IOs
===================

+-------------------------------------------------------+
| :zephyr:board:`nucleo_g474re`  | `microe-lan8651`_    |
+================================+======================+
| 3V3 (CN 6)                     | 3V3                  |
+--------------------------------+----------------------+
| GND (CN 6)                     | GND                  |
+--------------------------------+----------------------+
| SCK/D13 (CN 5)                 | SCK                  |
+--------------------------------+----------------------+
| MISO/D12 (CN 5)                | SDO                  |
+--------------------------------+----------------------+
| PWM/MOSI/D11 (CN 5)            | SDI                  |
+--------------------------------+----------------------+
| PWM/CS/D10 (CN 5)              | CS                   |
+--------------------------------+----------------------+
| D8 (CN 5)                      | RST                  |
+--------------------------------+----------------------+
| D7 (CN 9)                      | IRQ                  |
+--------------------------------+----------------------+


Software prerequisites:
=======================

* Compile `udp-perf`_ program (both for HOST and DUTs)

* `The udp-perf Zephyr sources <udp-perf-zephyr_>`_ should be cloned to:
  ``modules/udp-perf/zephyr``

* Extend ``west.yml``:

.. code-block:: shell

   - name: udp-perf
     path: modules/udp-perf

* Enable config :code:`UDPPERF` in the config file for   :code:`echo_server`
  or :code:`zperf`

* Extend ``boards/nucleo_g474re.conf`` for per setup IP configuration:

.. code-block:: shell

  CONFIG_NET_CONFIG_MY_IPV4_ADDR="192.0.2.1"
  CONFIG_NET_CONFIG_PEER_IPV4_ADDR="192.0.2.3"
  CONFIG_NET_CONFIG_MY_IPV6_ADDR="2001:db8::1"
  CONFIG_NET_CONFIG_PEER_IPV6_ADDR="2001:db8::3"


Testing and validation:
***********************

:zephyr:code-sample:`sockets-echo-server` with `udp-perf`_:
===========================================================

* Build for the :zephyr:board:`nucleo_g474re` board

.. code-block:: shell

   west build -p always -b nucleo_g474re samples/net/sockets/echo_server/

Default configuration:

.. code-block:: console

   Nucleo ip:  192.0.2.1      RPI4: 192.0.2.3
   Nucleo ipv6 2001:db8::1      RPI4: 2001:db8::3


* Use RPI4 to connect the USB <-> T1S dongle (connect to USB 2.0):

.. code-block:: shell

   uname -a # Linux raspberrypi 6.1.21
   cd ~/work/Zephyr/lan867x-linux-driver-2v2/linux-v6.1.21-support
   ./load.sh

Add default route to Nucleo (on RPI4):

.. code-block:: shell

   ip -6 route add 2001:db8::1 dev eth0

* Run test programs (on RPI4) - log via SSH to have separate consoles for each ``while``:

.. code-block:: shell

   cd ~/work/Zephyr/net-tools $
   while :; do ./echo-client -r -i eth1 192.0.2.1; sleep 1; ping -c 3 192.0.2.1; done
   while :; do ./echo-client -r -i eth1 2001:db8::1; sleep 1; ping -6 -c 3 2001:db8::1; done
   cd ~/work/Zephyr/udp-perf $
   while :; do ./target/armv7-unknown-linux-gnueabihf/release/udp-perf client --ip 192.0.2.1 --packets 100 --mode reduced --runs 5; [ $? -ne 0 ] && echo "Error!"; done


:zephyr:code-sample:`zperf`:
============================

* Build for the :zephyr:board:`nucleo_g474re` board

.. code-block:: shell

   west build -p always -b nucleo_g474re samples/net/zperf/

* Zephyr module (nucleo board) as CLIENT

.. code-block:: shell

   zperf udp upload 192.0.2.3 5001 10 1460 10M # -> nucleo shell
   iperf -s -l 1460 -u -B 192.0.2.3 -i 1 # -> RPI4

* Zephyr module (nucleo board) as SERVER

.. code-block:: shell

   zperf udp download 5001 # -> nucleo shell
   iperf -l 1460 -u -c 192.0.2.1 -i 1 -b 10M -p 5001 # -> RPI4


Running more device instances:
******************************

It is possible to run more than one instance of DUTs:

* Adjust IP addresses in ``nucleo_g474re.conf``

* In ``boards/nucleo_g474re.overlay`` adjust :code:`local-mac-address` and
  :code:`plca-node-id` properties.


.. _microe-lan8651:
   https://www.mikroe.com/two-wire-eth-click?srsltid=AfmBOoouUPjUzmLu19iA8wRez6GyJp5Mf_mMjckgRcvtqrBA9INjaKQe

.. _evb-lan8670:
   https://www.microchip.com/en-us/development-tool/ev08l38a

.. _rpi4-spec:
   https://www.raspberrypi.com/products/raspberry-pi-4-model-b/specifications/

.. _udp-perf:
   https://gitlab.com/network851614/udp-perf/-/tree/main/zephyr?ref_type=heads

.. _udp-perf-zephyr:
   https://gitlab.com/network851614/udp-perf/-/tree/main/zephyr?ref_type=heads
