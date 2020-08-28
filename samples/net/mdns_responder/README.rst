.. _mdns-responder-sample:

mDNS Responder Application
##########################

Overview
********

This application will wait mDNS queries for a pre-defined hostname and
respond to them. The default hostname is **zephyr** and it is set in the
:file:`prj.conf` file.

Requirements
************

- :ref:`networking_with_host`

- avahi or similar mDNS capable application that is able to query mDNS
  information.

Building and Running
********************

Build and run the mdns-responder sample application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/mdns_responder
   :board: <board to use>
   :conf: <config file to use>
   :goals: build
   :compact:

After the mdns-responder sample application is started, it will wait queries
from the network.

Open a terminal window in your host and type:

.. code-block:: console

    $ avahi-resolve -4 -n zephyr.local

If the query is successful, then following information is printed:

.. code-block:: console

    zephyr.local	192.0.2.1

For a IPv6 query, type this:

.. code-block:: console

    $ avahi-resolve -6 -n zephyr.local

If the query is successful, then following information is printed:

.. code-block:: console

    zephyr.local	2001:db8::1
