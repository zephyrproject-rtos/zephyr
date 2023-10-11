.. _zbus-benchmark-sample:

Benchmark sample
################

This sample implements an application to measure the time for sending 256KB from the producer to the consumers.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/zbus/dyn_channel
   :host-os: unix
   :board: qemu_cortex_m3
   :gen-args: -DCONFIG_BM_MESSAGE_SIZE=1 -DCONFIG_BM_ONE_TO=1 -DCONFIG_BM_ASYNC=0
   :goals: build run

Notice we have the following parameters:

* **CONFIG_BM_MESSAGE_SIZE** the size of the message to be transferred;
* **CONFIG_BM_ONE_TO** number of consumers to send;
* **CONFIG_BM_ASYNC** if the execution must be asynchronous or synchronous. Use y to async and n to sync;

Sample Output
=============
The result would be something like:

.. code-block:: console

    *** Booting Zephyr OS build zephyr-v3.2.0  ***
    I: Benchmark 1 to 8: Dynamic memory, ASYNC transmission and message size 256
    I: Bytes sent = 262144, received = 262144
    I: Average data rate: 1872457.14B/s
    I: Duration: 140ms

    @140


Running the benchmark automatically
===================================

There is a Robot script called ``benchmark_256KB.robot`` which runs all the input combinations as the complete benchmark.
The resulting file, ''zbus_dyn_benchmark_256KB.csv`` is generated in the project root folder. It takes a long time to execute. In the CSV file, we have the following columns:

+------------+---------------------+--------------------------+---------------+-------------+-------------+
| Style      | Number of consumers | Message size (bytes)     | Duration (ms) | RAM (bytes) | ROM (bytes) |
+============+=====================+==========================+===============+=============+=============+
| SYNC/ASYNC | 1,2,4,8             | 1,2,4,8,16,32,64,128,256 | float         | int         | int         |
+------------+---------------------+--------------------------+---------------+-------------+-------------+

The complete benchmark command using Robot framework is:

.. code-block:: console

    robot --variable serial_port:/dev/ttyACM0 -d /tmp/benchmark_out   benchmark_256KB.robot

An example of execution using the ``hifive1_revb`` board would generate a file like this:

.. code-block::

    SYNC,1,1,8534.0,6856,17434
    SYNC,1,2,4469.0,6856,17440
    SYNC,1,4,2362.3333333333335,6856,17438
    SYNC,1,8,1307.6666666666667,6864,17448
    SYNC,1,16,768.6666666666666,6872,17478
    SYNC,1,32,492.0,6888,17506
    SYNC,1,64,391.0,6920,17540
    SYNC,1,128,321.0,6984,17600
    SYNC,1,256,258.0,7112,17758
    SYNC,2,1,4856.666666666667,6880,17490
    SYNC,2,2,2464.0,6880,17494
    SYNC,2,4,1367.0,6880,17494
    SYNC,2,8,778.6666666666666,6888,17504
    SYNC,2,16,477.0,6896,17534
    SYNC,2,32,321.0,6912,17562
    SYNC,2,64,266.0,6944,17592
    SYNC,2,128,203.0,7008,17662
    SYNC,2,256,188.0,7136,17814
    SYNC,4,1,3021.3333333333335,6920,17536
    SYNC,4,2,1505.3333333333333,6920,17542
    SYNC,4,4,860.0,6920,17542
    SYNC,4,8,521.3333333333334,6928,17552
    SYNC,4,16,328.0,6936,17582
    SYNC,4,32,227.0,6952,17606
    SYNC,4,64,180.0,6984,17646
    SYNC,4,128,164.0,7048,17710
    SYNC,4,256,149.0,7176,17854
    SYNC,8,1,2044.3333333333333,7000,17632
    SYNC,8,2,1002.6666666666666,7000,17638
    SYNC,8,4,586.0,7000,17638
    SYNC,8,8,383.0,7008,17648
    SYNC,8,16,250.0,7016,17674
    SYNC,8,32,203.0,7032,17708
    SYNC,8,64,156.0,7064,17742
    SYNC,8,128,141.0,7128,17806
    SYNC,8,256,133.0,7256,17958
    ASYNC,1,1,22187.666666666668,7312,17776
    ASYNC,1,2,11424.666666666666,7312,17782
    ASYNC,1,4,5823.0,7312,17778
    ASYNC,1,8,3071.0,7312,17790
    ASYNC,1,16,1625.0,7328,17832
    ASYNC,1,32,966.3333333333334,7344,17862
    ASYNC,1,64,578.0,7376,17896
    ASYNC,1,128,403.6666666666667,7440,17956
    ASYNC,1,256,352.0,7568,18126
    ASYNC,2,1,18547.333333333332,7792,18030
    ASYNC,2,2,9563.0,7792,18034
    ASYNC,2,4,4831.0,7792,18030
    ASYNC,2,8,2497.3333333333335,7792,18044
    ASYNC,2,16,1377.6666666666667,7824,18098
    ASYNC,2,32,747.3333333333334,7856,18132
    ASYNC,2,64,492.0,7920,18162
    ASYNC,2,128,321.0,8048,18232
    ASYNC,2,256,239.33333333333334,8304,18408
    ASYNC,4,1,16823.0,8744,18474
    ASYNC,4,2,8604.333333333334,8744,18480
    ASYNC,4,4,4325.666666666667,8744,18472
    ASYNC,4,8,2258.0,8744,18490
    ASYNC,4,16,1198.3333333333333,8808,18572
    ASYNC,4,32,696.0,8872,18610
    ASYNC,4,64,430.0,9000,18650
    ASYNC,4,128,289.0,9256,18714
    ASYNC,4,256,227.0,9768,18906
    ASYNC,8,1,15976.666666666666,10648,19366
    ASYNC,8,2,7929.666666666667,10648,19372
    ASYNC,8,4,4070.6666666666665,10648,19356
    ASYNC,8,8,2158.6666666666665,10648,19382
    ASYNC,8,16,1119.6666666666667,10776,19506
    ASYNC,8,32,619.6666666666666,10904,19566
    ASYNC,8,64,391.0,11160,19600
    ASYNC,8,128,273.0,11672,19686
    ASYNC,8,256,211.0,12696,19934
