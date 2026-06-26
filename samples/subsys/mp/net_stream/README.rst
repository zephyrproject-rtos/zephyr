.. zephyr:code-sample:: mp-net-stream
   :name: MediaPipe Network Streaming
   :relevant-api: mp

   Stream MJPEG video over TCP using a libMP pipeline.

Overview
********

This sample demonstrates a three-element MediaPipe (libMP) pipeline that
reads an MJPEG file from the filesystem, parses individual JPEG frames, and
streams them over a TCP socket to a remote client.

.. code-block:: none

   zfilesrc --> zjpeg_parser --> ztcpsink
   (file)       (frame split)   (TCP server)

The ``ztcpsink`` element from the **znet** plugin opens a TCP server socket,
waits for exactly one client to connect, then forwards each JPEG frame as
raw bytes.  Any player that understands raw MJPEG over TCP can receive the
stream.

Requirements
************

* A FAT-formatted storage volume with a file named ``sample.mjpeg``
* ``CONFIG_NETWORKING``, ``CONFIG_NET_TCP``, and ``CONFIG_NET_SOCKETS`` enabled

Building and Running on native_sim
***********************************

The sample uses native_sim's flash simulator to emulate an SD card and the
``eth_native_posix`` driver for networking.

1. Create a TAP interface (one time, requires root):

   .. code-block:: console

      sudo ip tuntap add zeth0 mode tap
      sudo ip addr add 192.0.2.2/24 dev zeth0
      sudo ip link set zeth0 up

2. Prepare a test MJPEG file and write it to the flash image.  For a quick
   test, convert any video or image sequence with ffmpeg:

   .. code-block:: console

      ffmpeg -i input.mp4 -vf scale=320:240 -vcodec mjpeg -f mjpeg sample.mjp

   Then inject it into the flash.bin that Zephyr will use:

   .. code-block:: console

      # Build first to get flash.bin generated, then write the file.
      # The helper script below mounts flash.bin as a FAT volume:
      sudo mount -o loop,offset=$((0x200000)) flash.bin /mnt/tmp
      sudo cp sample.mjpeg /mnt/tmp/
      sudo umount /mnt/tmp

3. Build and run:

   .. code-block:: console

      west build -b native_sim/native/64 samples/subsys/mp/net_stream \
          -- -DEXTRA_CONF_FILE=boards/native_sim_64.conf \
             -DEXTRA_DTC_OVERLAY_FILE=boards/native_sim_64.overlay
      ./build/zephyr/zephyr.exe

4. In a second terminal, connect a player:

   .. code-block:: console

      ffplay -f mjpeg tcp://192.0.2.1:5000

Sample Output
*************

.. code-block:: none

   [00:00:00.000] <inf> main: Filesystem mounted at /SD:
   [00:00:00.001] <inf> main: Pipeline: filesrc -> zjpeg_parser -> ztcpsink (port 5000)
   [00:00:00.001] <inf> main: Connect with: ffplay -f mjpeg tcp://localhost:5000
   [00:00:00.002] <inf> mp_ztcpsink: Waiting for client on port 5000 ...
   [00:00:01.234] <inf> mp_ztcpsink: Client connected, streaming started
   ...
   [00:00:05.000] <inf> main: End of stream
