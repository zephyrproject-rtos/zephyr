.. zephyr:code-sample:: bridge
   :name: bridge
   :relevant-api: ethernet net_if

   Test and debug Ethernet bridge

Overview
********

Example on testing/debugging Ethernet bridge

The source code for this sample application can be found at:
:zephyr_file:`samples/net/ethernet/bridge`.

Building and Running
********************

Follow these steps to build the bridge sample application:

.. zephyr-app-commands::
   :zephyr-app: samples/net/ethernet/bridge
   :board: <board to use>
   :conf: prj.conf
   :goals: build
   :compact:

A quick method to verify the bridge without hardware board is using native_sim application.
The :zephyr_file:`samples/net/ethernet/bridge/bridge-test-setup.sh` provides a script for host set-up.
The steps are as below.

1. Run script for pre-setup. Two tap interfaces zeth0 and zeth1 will be created.

.. code-block:: console

    bridge-test-setup.sh pre-setup

2. Run native_sim application.

3. Run script for post-setup. zeth1 will be moved to another net name space. (We couldn't complete this
   post-setup before running native_sim application, because it seems zephyr iface will not connect
   interface in net name space.)

.. code-block:: console

    bridge-test-setup.sh post-setup

4. Test the bridging between zeth0 and zeth1. The IP will be configured as below.
   Ping zeth1 in another net name space.

   - zeth0: IPv4 192.0.2.2, IPv6 2001:db8::2
   - zeth1: IPv4 192.0.2.3, IPv6 2001:db8::3

.. code-block:: console

    ping 192.0.2.3
    ping 2001:db8::3

5. Clean up after testing.

.. code-block:: console

    bridge-test-setup.sh clean
