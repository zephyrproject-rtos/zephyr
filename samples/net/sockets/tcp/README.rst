.. zephyr:code-sample:: sockets-tcp-sample
   :name: TCP sample for TTCN-3 based sanity check
   :relevant-api: bsd_sockets

   Use TTCN-3 to validate the functionality of the TCP stack.

Overview
********

This application is used together with the TTCN-3 based sanity check
to validate the functionality of the TCP.

Building, Running and executing TTCN-3 based Sanity Check for TCP
*****************************************************************

Compile and start the `net-test-tools`_:

.. code-block:: console

   ./autogen.sh
   make
   ./loop-slipcat.sh

Build the TCP sample app:

.. code-block:: console

   cd samples/net/sockets/tcp
   mkdir build && cd build
   cmake -DBOARD=qemu_x86 -DEXTRA_CONF_FILE="overlay-slip.conf" ..
   make run

Compile and run the TCP sanity check `net-test-suites`_:

.. code-block:: console

   . titan-install.sh
   . titan-env.sh
   cd src
   . make.sh
   ttcn3_start test_suite tcp2_check_3_runs.cfg

.. _`net-test-tools`: https://github.com/intel/net-test-tools
.. _`net-test-suites`: https://github.com/intel/net-test-suites
