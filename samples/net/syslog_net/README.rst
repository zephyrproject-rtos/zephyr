.. _syslog-net-sample:

Syslog net Application
######################

Overview
********

This sample application enables a remote syslog service that will
send syslog messages to a remote server, as configured in ``prj.conf``.
See https://tools.ietf.org/html/rfc5424 and https://tools.ietf.org/html/rfc5424
for more details about syslog protocol over UDP.

The source code for this sample application can be found at:
:file:`samples/net/syslog_net`.

Requirements
************

- :ref:`networking_with_qemu`

Building and Running
********************

For configuring the remote IPv6 syslog server, set the following
variables in prj.conf file:

.. code-block:: console

	CONFIG_SYS_LOG_BACKEND_NET=y
	CONFIG_SYS_LOG_BACKEND_NET_SERVER="[2001:db8::2]:514"

Default port number is 514 if user does not specify a value.
The following syntax is supported for the server address
and port:

.. code-block:: console

	192.0.2.1:514
	192.0.2.42
	[2001:db8::1]:514
	[2001:db8::2]
	2001:db::42

Build syslog_net sample application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/syslog_net
   :board: <board to use>
   :conf: <config file to use>
   :goals: build
   :compact:
