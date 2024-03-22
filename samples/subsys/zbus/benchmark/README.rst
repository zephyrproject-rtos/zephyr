.. zephyr:code-sample:: zbus-benchmark
   :name: Benchmarking
   :relevant-api: zbus_apis

   Measure the time for sending 256KB from a producer to N consumers.

This sample implements an application to measure the time for sending 256KB from the producer to the
consumers.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/zbus/dyn_channel
   :host-os: unix
   :board: qemu_cortex_m3
   :gen-args: -DCONFIG_BM_MESSAGE_SIZE=512 -DCONFIG_BM_ONE_TO=1 -DCONFIG_BM_LISTENERS=y
   :goals: build run

Notice we have the following parameters:

* **CONFIG_BM_MESSAGE_SIZE** the size of the message to be transferred (2 to 4096 bytes);
* **CONFIG_BM_ONE_TO** number of consumers to send (1 up to 8 consumers);
* **CONFIG_BM_LISTENERS** Use y to perform the benchmark listeners;
* **CONFIG_BM_SUBSCRIBERS** Use y to perform the benchmark subscribers;
* **CONFIG_BM_MSG_SUBSCRIBERS** Use y to perform the benchmark message subscribers.

Sample Output
=============
The result would be something like:

.. code-block:: console

   *** Booting Zephyr OS build zephyr-vX.Y.Z ***
   I: Benchmark 1 to 1 using LISTENERS to transmit with message size: 512 bytes
   I: Bytes sent = 262144, received = 262144
   I: Average data rate: 12.62MB/s
   I: Duration: 0.019805908s

   @19805


Running the benchmark automatically
===================================

There is a `Robot framework <https://robotframework.org/>`_ script called ``benchmark_256KB.robot`` which runs all the input combinations as the complete benchmark.
The resulting file, ``zbus_dyn_benchmark_256KB.csv`` is generated in the project root folder. It takes a long time to execute. In the CSV file, we have the following columns:

+-----------------+---------------------+--------------------------+---------------+-------------+-------------+
| Observer type   | Number of consumers | Message size (bytes)     | Duration (ns) | RAM (bytes) | ROM (bytes) |
+=================+=====================+==========================+===============+=============+=============+
| LIS/SUB/MSG_SUB | 1,4,8               | 2,8,32,128,512           | float         | int         | int         |
+-----------------+---------------------+--------------------------+---------------+-------------+-------------+

The complete benchmark command using Robot framework is:

.. code-block:: console

    robot --variable serial_port:/dev/ttyACM0 --variable board:nrf52dk/nrf52832 -d /tmp/benchmark_out   benchmark_256KB.robot

An example of execution using the ``nrf52dk/nrf52832`` board would generate a file like this:

.. code-block::

   LISTENERS,1,2,890787.3333333334,9247,23091
   LISTENERS,1,8,237925.0,9253,23091
   LISTENERS,1,32,74513.0,9277,23151
   LISTENERS,1,128,33813.0,9565,23231
   LISTENERS,1,512,35746.0,10717,23623
   LISTENERS,4,2,314198.3333333333,9274,23142
   LISTENERS,4,8,82244.33333333333,9280,23142
   LISTENERS,4,32,24057.333333333332,9304,23202
   LISTENERS,4,128,9816.0,9592,23282
   LISTENERS,4,512,9277.0,10744,23674
   LISTENERS,8,2,211465.66666666666,9310,23202
   LISTENERS,8,8,56294.0,9316,23210
   LISTENERS,8,32,15635.0,9340,23270
   LISTENERS,8,128,5818.0,9628,23350
   LISTENERS,8,512,4862.0,10780,23742
   SUBSCRIBERS,1,2,7804351.333333333,9927,23463
   SUBSCRIBERS,1,8,1978179.3333333333,9933,23463
   SUBSCRIBERS,1,32,514139.3333333333,9957,23523
   SUBSCRIBERS,1,128,146759.0,10309,23603
   SUBSCRIBERS,1,512,55104.0,11845,23995
   SUBSCRIBERS,4,2,5551961.0,11994,24134
   SUBSCRIBERS,4,8,1395009.0,12000,24134
   SUBSCRIBERS,4,32,354583.3333333333,12024,24194
   SUBSCRIBERS,4,128,92976.66666666667,12568,24274
   SUBSCRIBERS,4,512,28015.0,15256,24666
   SUBSCRIBERS,8,2,5449839.0,14750,24858
   SUBSCRIBERS,8,8,1321766.6666666667,14756,24866
   SUBSCRIBERS,8,32,332804.0,14780,24926
   SUBSCRIBERS,8,128,85489.33333333333,15580,25006
   SUBSCRIBERS,8,512,23905.0,19804,25398
   MSG_SUBSCRIBERS,1,2,8783538.333333334,10371,25615
   MSG_SUBSCRIBERS,1,8,2249592.6666666665,10377,25615
   MSG_SUBSCRIBERS,1,32,610168.0,10401,25675
   MSG_SUBSCRIBERS,1,128,207295.0,10753,25755
   MSG_SUBSCRIBERS,1,512,143584.66666666666,12289,26147
   MSG_SUBSCRIBERS,4,2,5787699.0,12318,26126
   MSG_SUBSCRIBERS,4,8,1473907.0,12324,26126
   MSG_SUBSCRIBERS,4,32,396127.6666666667,12348,26186
   MSG_SUBSCRIBERS,4,128,126362.66666666667,12892,26266
   MSG_SUBSCRIBERS,4,512,59040.666666666664,15580,26658
   MSG_SUBSCRIBERS,8,2,5453999.333333333,14914,26610
   MSG_SUBSCRIBERS,8,8,1356312.3333333333,14920,26650
   MSG_SUBSCRIBERS,8,32,361368.3333333333,14944,26710
   MSG_SUBSCRIBERS,8,128,113148.66666666667,15744,26790
   MSG_SUBSCRIBERS,8,512,51218.333333333336,19968,27182
