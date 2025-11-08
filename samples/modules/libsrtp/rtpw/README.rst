.. zephyr:code-sample:: rtpw
   :name: Rtpw

   Send and receive data using (S)RTP protocol.

Overveiw
********

A simple demo of using (S)RTP protocol to send and receive data over network. The sample could be sender or receiver.
To test (S)RTP via a Linux machine, you should build the libsrtp.
To test the sender while PC is receiver, the peer IP address should be set in Kconfig file. Next open a terminal and enter command:

.. code-block:: console
``~/libsrtp/test$ ./rtpw -r -k c1eec3717da76195bb878578790af71c4ee9f859e197a414a78d5abc7451 -e 128 -a 0.0.0.0 9999``
You should see in PC side:
.. code-block::
``Using libsrtp3 3.0.0-pre [0x3000000]
security services: confidentiality message authentication
set master key/salt to c1eec3717da76195bb878578790af71c/4ee9f859e197a414a78d5abc7451
	word: SRTP test0.
	word: SRTP test1.
	word: SRTP test2.
	word: SRTP test3.``

You should see in device side:
.. code-block::
``*** Booting Zephyr OS build v4.2.0-5032-g84d1da7ea2a6 ***
[00:00:00.060,000] <inf> net_config: Initializing network
[00:00:00.068,000] <inf> net_config: Waiting interface 1 (0x341b0380) to be up...
[00:00:01.651,000] <inf> phy_mii: PHY (0) Link speed 100 Mb, full duplex
[00:00:01.660,000] <inf> net_config: Interface 1 (0x341b0380) coming up
[00:00:01.670,000] <inf> net_config: IPv4 address: 10.42.0.2
[00:00:01.678,000] <inf> rtpw_sample: Using Zephyr 1.0.0 [0x1000000]

``[00:00:01.770,000] <inf> rtpw_sample: peer IPv4 address: 10.42.0.1.
[00:00:01.779,000] <inf> rtpw_sample: my IPv4 address: 10.42.0.2.
[00:00:01.788,000] <inf> rtpw_sample: sending word: SRTP test0.
[00:00:02.297,000] <inf> rtpw_sample: sending word: SRTP test1.
[00:00:02.806,000] <inf> rtpw_sample: sending word: SRTP test2.
[00:00:03.315,000] <inf> rtpw_sample: sending word: SRTP test3.``

To test (S)RTP via a Linux machine, you should build the libsrtp.
To test the receiver while PC is sender, the peer IP address should be set in Kconfig file. Next open a terminal and enter command:

.. code-block:: console
``~/libsrtp/test$ ./rtpw -s -k c1eec3717da76195bb878578790af71c4ee9f859e197a414a78d5abc7451 -e 128 -a <CONFIG_NET_CONFIG_MY_IPV4_ADDR> 9999``
for example:
``~/libsrtp/test$ ./rtpw -s -k c1eec3717da76195bb878578790af71c4ee9f859e197a414a78d5abc7451 -e 128 -a 10.42.0.2 9999``

You should see in PC side:
.. code-block:: console
``Using libsrtp3 3.0.0-pre [0x3000000]
security services: confidentiality message authentication
set master key/salt to c1eec3717da76195bb878578790af71c/4ee9f859e197a414a78d5abc7451
sending word: abducing
sending word: acidheads
sending word: acidness
sending word: actons``

You should see in device side:
.. code-block:: console
``*** Booting Zephyr OS build v4.2.0-5032-g84d1da7ea2a6 ***
[00:00:00.060,000] <inf> net_config: Initializing network
[00:00:00.068,000] <inf> net_config: Waiting interface 1 (0x341b04c0) to be up...
[00:00:01.651,000] <inf> phy_mii: PHY (0) Link speed 100 Mb, full duplex
[00:00:01.660,000] <inf> net_config: Interface 1 (0x341b04c0) coming up
[00:00:01.670,000] <inf> net_config: IPv4 address: 10.42.0.2
[00:00:01.678,000] <inf> rtpw_sample: Using Zephyr 1.0.0 [0x1000000]``

``[00:00:01.770,000] <inf> rtpw_sample: peer IPv4 address: 0.0.0.0.
[00:00:01.779,000] <inf> rtpw_sample: my IPv4 address: 10.42.0.2.
[00:00:18.010,000] <inf> rtpw_sample: receiving word: abducing``

``[00:00:18.510,000] <inf> rtpw_sample: receiving word: acidheads``

``[00:00:19.010,000] <inf> rtpw_sample: receiving word: acidness``

``[00:00:19.510,000] <inf> rtpw_sample: receiving word: actons``

Configuration
*************

The sender/receiver type and the SRTP key can be changed via Kconfig file.

Building and Running
********************

This application can be built as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/rtpw
   :host-os: unix
   :board: frdm_mcxn947/mcxn947/cpu0
