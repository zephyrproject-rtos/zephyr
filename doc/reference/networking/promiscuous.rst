.. _promiscuous_interface:

Promiscuous Mode
################

.. contents::
    :local:
    :depth: 2

Overview
********

Promiscuous mode is a mode for a network interface controller that
causes it to pass all traffic it receives to the application rather than
passing only the frames that the controller is specifically programmed
to receive. This mode is normally used for packet sniffing as used
to diagnose network connectivity issues by showing an application
all the data being transferred over the network.  (See the
`Wikipedia article on promiscuous mode
<https://en.wikipedia.org/wiki/Promiscuous_mode>`_ for more information.)

The network promiscuous APIs are used to enable and disable this mode,
and to wait for and receive a network data to arrive. Not all network
technologies or network device drivers support promiscuous mode.

Sample usage
************

First the promiscuous mode needs to be turned ON by the application like this:

.. code-block:: c

	ret = net_promisc_mode_on(iface);
	if (ret < 0) {
		if (ret == -EALREADY) {
			printf("Promiscuous mode already enabled\n");
		} else {
			printf("Cannot enable promiscuous mode for "
			       "interface %p (%d)\n", iface, ret);
		}
	}


If there is no error, then the application can start to wait for network data:

.. code-block:: c

	while (true) {
		pkt = net_promisc_mode_wait_data(K_FOREVER);
		if (pkt) {
			print_info(pkt);
		}

		net_pkt_unref(pkt);
	}


Finally the promiscuous mode can be turned OFF by the application like this:

.. code-block:: c

	ret = net_promisc_mode_off(iface);
	if (ret < 0) {
		if (ret == -EALREADY) {
			printf("Promiscuous mode already disabled\n");
		} else {
			printf("Cannot disable promiscuous mode for "
			       "interface %p (%d)\n", iface, ret);
		}
	}


See :ref:`net-promiscuous-mode-sample` for a more comprehensive example.


API Reference
*************

.. doxygengroup:: promiscuous
   :project: Zephyr
