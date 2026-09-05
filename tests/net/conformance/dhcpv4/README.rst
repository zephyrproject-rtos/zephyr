.. _net_conformance_dhcpv4:

DHCPv4 client conformance test
##############################

Overview
********

This runs a TTCN-3 conformance suite against Zephyr's DHCPv4 client. The
application here is the system under test: it starts the client and leaves it
alone. The suite is the server, and lives in the ``net-tools`` repository under
``ttcn3/suites/dhcpv4``.

Requirements
************

The same as :ref:`net_conformance_mdns`, and additionally the suite has to run
as root. DHCP is defined on ports 67 and 68 and there is no way to move it
elsewhere, so the tester cannot avoid binding a privileged port. The test skips
itself with a reason when it is not run with enough privilege.

Running
*******

.. code-block:: console

   export TTCN3_DIR=/usr
   sudo -E ./scripts/twister -p native_sim --enable-slow \
     -T tests/net/conformance/dhcpv4

What is covered
***************

The shape of a discover, that an unanswered discover is repeated rather than
given up on, and that an offer brings back a request naming both the offered
address and the server that offered it, which an acknowledgment then completes.

Only one exchange can be completed in a run: a client that has been given an
address stops asking for one, so the test case that completes the exchange runs
last.

See :ref:`ttcn3_known_gaps` for what is deliberately not tested and why.
