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
   :gen-args: -DCONFIG_BM_MESSAGE_SIZE=1 -DCONFIG_BM_ONE_TO=1 -DCONFIG_BM_ASYNC=y
   :goals: build run

Notice we have the following parameters:

* **CONFIG_BM_MESSAGE_SIZE** the size of the message to be transferred (1, 2, 4, 8, 16, 32, 64, 128, or 256);
* **CONFIG_BM_ONE_TO** number of consumers to send (1, 2, 4, or 8);
* **CONFIG_BM_ASYNC** if the execution must be asynchronous or synchronous. Use y to async and n to sync;

Sample Output
=============
The result would be something like:

.. code-block:: console

    *** Booting Zephyr OS build zephyr-v3.3.0 ***
    I: Benchmark 1 to 1: Dynamic memory, SYNC transmission and message size 1
    I: Bytes sent = 262144, received = 262144
    I: Average data rate: 0.6MB/s
    I: Duration: 4.72020167s

    @4072020167


Running the benchmark automatically
===================================

There is a `Robot framework <https://robotframework.org/>`_ script called ``benchmark_256KB.robot`` which runs all the input combinations as the complete benchmark.
The resulting file, ``zbus_dyn_benchmark_256KB.csv`` is generated in the project root folder. It takes a long time to execute. In the CSV file, we have the following columns:

+------------+---------------------+--------------------------+---------------+-------------+-------------+
| Style      | Number of consumers | Message size (bytes)     | Duration (ns) | RAM (bytes) | ROM (bytes) |
+============+=====================+==========================+===============+=============+=============+
| SYNC/ASYNC | 1,2,4,8             | 1,2,4,8,16,32,64,128,256 | float         | int         | int         |
+------------+---------------------+--------------------------+---------------+-------------+-------------+

The complete benchmark command using Robot framework is:

.. code-block:: console

    robot --variable serial_port:/dev/ttyACM0 --variable board:nrf52833dk_nrf52833 -d /tmp/benchmark_out   benchmark_256KB.robot

An example of execution using the ``nrf52833dk_nrf52833`` board would generate a file like this:

.. code-block::

    SYNC,1,1,2312815348.3333335,7286,23752
    SYNC,1,2,1172444661.3333333,7287,23760
    SYNC,1,4,602284749.6666666,7289,23768
    SYNC,1,8,323750814.0,7293,23772
    SYNC,1,16,175120035.66666666,7301,23776
    SYNC,1,32,103942871.33333333,7317,23776
    SYNC,1,64,68318685.0,7349,23776
    SYNC,1,128,50567627.333333336,7477,23776
    SYNC,1,256,41656494.0,7733,23776
    SYNC,2,1,1277842204.3333333,7298,23768
    SYNC,2,2,647094726.6666666,7299,23776
    SYNC,2,4,329559326.3333333,7301,23784
    SYNC,2,8,170979817.66666666,7305,23796
    SYNC,2,16,95174153.66666667,7313,23792
    SYNC,2,32,55786133.0,7329,23792
    SYNC,2,64,36173502.333333336,7361,23792
    SYNC,2,128,26326497.666666668,7489,23792
    SYNC,2,256,21280924.333333332,7745,23792
    SYNC,4,1,745513916.0,7322,23800
    SYNC,4,2,374755859.6666667,7323,23808
    SYNC,4,4,191497802.66666666,7325,23816
    SYNC,4,8,101399739.66666667,7329,23820
    SYNC,4,16,54026286.0,7337,23824
    SYNC,4,32,31097412.0,7353,23824
    SYNC,4,64,19643148.333333332,7385,23824
    SYNC,4,128,13936360.333333334,7513,23824
    SYNC,4,256,11047363.333333334,7769,23824
    SYNC,8,1,477518717.3333333,7370,23864
    SYNC,8,2,240773518.66666666,7371,23872
    SYNC,8,4,121897379.33333333,7373,23880
    SYNC,8,8,64015706.333333336,7377,23884
    SYNC,8,16,33681234.0,7385,23888
    SYNC,8,32,18880208.333333332,7401,23888
    SYNC,8,64,11505127.0,7433,23888
    SYNC,8,128,7781982.333333333,7561,23888
    SYNC,8,256,5940755.333333333,7817,23888
    ASYNC,1,1,9422749837.333334,7962,24108
    ASYNC,1,2,4728759765.333333,7963,24116
    ASYNC,1,4,2380554199.3333335,7965,24124
    ASYNC,1,8,1225118001.6666667,7969,24128
    ASYNC,1,16,618764241.6666666,7977,24132
    ASYNC,1,32,326253255.3333333,7993,24132
    ASYNC,1,64,179473876.66666666,8025,24132
    ASYNC,1,128,106170654.33333333,8217,24132
    ASYNC,1,256,69386800.33333333,8601,24136
    ASYNC,2,1,8347330729.0,8650,24288
    ASYNC,2,2,4186747233.3333335,8651,24296
    ASYNC,2,4,2092895507.3333333,8653,24304
    ASYNC,2,8,1049245198.6666666,8657,24316
    ASYNC,2,16,541544596.6666666,8665,24312
    ASYNC,2,32,281127929.6666667,8681,24312
    ASYNC,2,64,150746663.66666666,8713,24312
    ASYNC,2,128,85662842.0,8969,24312
    ASYNC,2,256,48909505.0,9481,24320
    ASYNC,4,1,7854085286.666667,10026,24652
    ASYNC,4,2,3935852050.3333335,10027,24660
    ASYNC,4,4,1972869873.0,10029,24668
    ASYNC,4,8,979451497.6666666,10033,24672
    ASYNC,4,16,499348958.0,10041,24676
    ASYNC,4,32,253712972.0,10057,24676
    ASYNC,4,64,131022135.33333333,10089,24676
    ASYNC,4,128,69610595.66666667,10473,24676
    ASYNC,4,256,38706461.666666664,11241,24692
    ASYNC,8,1,7590311686.666667,12778,25220
    ASYNC,8,2,3800333658.6666665,12779,25228
    ASYNC,8,4,1900014241.6666667,12781,25236
    ASYNC,8,8,940419515.0,12785,25240
    ASYNC,8,16,478739420.6666667,12793,25244
    ASYNC,8,32,241465250.66666666,12809,25244
    ASYNC,8,64,122701009.0,12841,25244
    ASYNC,8,128,63405355.0,13481,25244
    ASYNC,8,256,33752441.666666664,14761,25244
