.. zephyr:code-sample:: tftp-server
   :name: TFTP server
   :relevant-api: tftp_client

   Start a TFTP server and verify PUT/GET transfers on a mounted file system.

Overview
********

This sample starts a Zephyr TFTP server that listens for incoming PUT and GET
requests from TFTP clients. It uses a RAM disk with an ``ext2`` file system
to store transferred files.

The server API is transport-agnostic. This sample uses the built-in UDP
transport helper.

Manual Interaction
******************

To interact with the server from your host machine using ``native_sim``:

1. **Setup Network Interface**:
   Zephyr's ``native_sim`` uses a TAP interface. You need to set this up on your host:

   .. code-block:: bash

      sudo <NET_TOOLS_PATH>/net-setup.sh start

2. **Build and Run the Sample**:

   .. zephyr-app-commands::
      :zephyr-app: samples/net/tftp_server
      :board: native_sim
      :goals: run

3. **Use a TFTP Client**:
   From another terminal on your host, use a TFTP client (e.g., ``tftp-hpa``)
   to transfer files to/from the server (IP: ``192.0.2.1``):

   .. code-block:: bash

      # Put a file
      echo "hello zephyr" > test.txt
      tftp 192.0.2.1 -m octet -c put test.txt

      # Get the file back
      tftp 192.0.2.1 -m octet -c get test.txt

Typical Output
**************

When the server is running and a transfer occurs, logs will show:

.. code-block:: none

   *** Booting Zephyr OS build v3.x.x ***
   === TFTP Server Sample ===
   Initializing UDP transport on port 69...
   Formatting FS (RAM disk)...
   Mounting FS...
   Starting TFTP server thread...
   TFTP Server started on port 69.
   The server is now ready to receive connections.

   [00:00:10.000,000] <inf> tftp_server: WRQ: filename='test.txt', mode='octet'
   [00:00:10.050,000] <inf> tftp_server: Write transfer complete: 13 bytes
   [00:00:15.000,000] <inf> tftp_server: RRQ: filename='test.txt', mode='octet'
   [00:00:15.020,000] <inf> tftp_server: Transfer complete: 13 bytes
