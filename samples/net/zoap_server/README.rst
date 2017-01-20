CoAP Server
###########

Overview
********

A simple CoAP server showing how to expose a simple resource.

This demo assumes that the platform of choice has networking support,
some adjustments to the configuration may be needed.

The sample will listen for requests in the CoAP UDP port (5683) in the
site-local IPv6 multicast address reserved for CoAP nodes.

The exported resource, with path '/test', will just respond any GET to
that path with the the type, code and message identification retrieved
from the request. The response will have this format:

.. code-block:: none

  Type: <type>
  Code: <code>
  MID: <message id>

Building And Running
********************

This project has no output in case of success, the correct
functionality can be verified by using some external tool like tcpdump
or wireshark.

See the `net-tools`_ project for more details

It can be built and executed on QEMU as follows:

.. code-block:: console

    make run

.. _`net-tools`: https://gerrit.zephyrproject.org/r/gitweb?p=net-tools.git;a=tree
