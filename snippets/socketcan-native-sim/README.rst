.. _snippet-socketcan-native-sim:

SocketCAN on Native Simulator Snippet (socketcan-native-sim)
############################################################

.. code-block:: console

   west build -S socketcan-native-sim [...]

Overview
********

This snippet allows to configure Controller Area Network (CAN) samples with Linux SocketCAN support
on :zephyr:board:`native_sim`.

By default, the native simulator expects a SocketCAN network device called ``zcan0`` (specified in
:zephyr_file:`boards/native/native_sim/native_sim.dts`). This name can be added as an alternative
name for an existing SocketCAN network device (here, a newly created virtual CAN network device
``vcan0``) using a command like the following:

.. code-block:: console

   sudo modprobe vcan
   sudo ip link add dev vcan0 type vcan
   sudo ip link property add dev vcan0 altname zcan0

The SocketCAN device must be configured and brought up before running the native simulator:

.. code-block:: console

   sudo ip link set vcan0 up
