.. zephyr:code-sample:: zms
   :name: Zephyr Memory Storage (ZMS)
   :relevant-api: zms_high_level_api

   Store and retrieve data from storage using the ZMS API.

Overview
********
 The sample shows how to use ZMS to store ID/VALUE pairs and reads them back.
 Deleting an ID/VALUE pair is also shown in this sample.

 The sample stores the following items:

 #. A string representing an IP address: stored at id=1, data="192.168.1.1"
 #. A binary blob representing a key/value pair: stored at id=0xbeefdead,
    data={0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF}
 #. A variable (32bit): stored at id=2, data=cnt
 #. A long set of data (128 bytes)

 A loop is executed where we mount the storage system, and then write all set
 of data.

 Each DELETE_ITERATION period, we delete all set of data and verify that it has been deleted.
 We generate as well incremented ID/value pairs, we store them until storage is full, then we
 delete them and verify that storage is empty.

Requirements
************

* A board with flash support or native_sim target

Building and Running
********************

This sample can be found under :zephyr_file:`samples/subsys/fs/zms` in the Zephyr tree.

The sample can be built for several platforms, but for the moment it has been tested only
on native_sim target

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/fs/zms
   :goals: build
   :compact:

After running the generated image on a native_sim target, the output on the console shows the
multiple Iterations of read/write/delete exectuted.

Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS build v3.7.0-2383-g624f75400242 ***
   [00:00:00.000,000] <inf> fs_zms: 3 Sectors of 4096 bytes
   [00:00:00.000,000] <inf> fs_zms: alloc wra: 0, fc0
   [00:00:00.000,000] <inf> fs_zms: data wra: 0, 0
   ITERATION: 0
   Adding IP_ADDRESS 172.16.254.1 at id 1
   Adding key/value at id beefdead
   Adding counter at id 2
   Adding Longarray at id 3
   [00:00:00.000,000] <inf> fs_zms: 3 Sectors of 4096 bytes
   [00:00:00.000,000] <inf> fs_zms: alloc wra: 0, f80
   [00:00:00.000,000] <inf> fs_zms: data wra: 0, 8c
   ITERATION: 1
   ID: 1, IP Address: 172.16.254.1
   Adding IP_ADDRESS 172.16.254.1 at id 1
   Id: beefdead, Key: de ad be ef de ad be ef
   Adding key/value at id beefdead
   Id: 2, loop_cnt: 0
   Adding counter at id 2
   Id: 3, Longarray: 0 1 2 3 4 5 6 7 8 9 a b c d e f 10 11 12 13 14 15 16 17 18 19 1a 1b 1c 1d 1e 1f 20 21 22 23 24 25 26 27 28 29 2a 2b 2c 2d 2e 2f 30 31 32 33 34 35 36 37 38 39 3a 3b 3c 3d 3e 3f 40 41 42 43 44 45 46 47 48 49 4a 4b 4c 4d 4e 4f 50 51 52 53 5
   4 55 56 57 58 59 5a 5b 5c 5d 5e 5f 60 61 62 63 64 65 66 67 68 69 6a 6b 6c 6d 6e 6f 70 71 72 73 74 75 76 77 78 79 7a 7b 7c 7d 7e 7f
   Adding Longarray at id 3
   .
   .
   .
   .
   .
   .
   [00:00:00.000,000] <inf> fs_zms: 3 Sectors of 4096 bytes
   [00:00:00.000,000] <inf> fs_zms: alloc wra: 0, f40
   [00:00:00.000,000] <inf> fs_zms: data wra: 0, 80
   ITERATION: 299
   ID: 1, IP Address: 172.16.254.1
   Adding IP_ADDRESS 172.16.254.1 at id 1
   Id: beefdead, Key: de ad be ef de ad be ef
   Adding key/value at id beefdead
   Id: 2, loop_cnt: 298
   Adding counter at id 2
   Id: 3, Longarray: 0 1 2 3 4 5 6 7 8 9 a b c d e f 10 11 12 13 14 15 16 17 18 19 1a 1b 1c 1d 1e 1f 20 21 22 23 24 25 26 27 28 29 2a 2b 2c 2d 2e 2f 30 31 32 33 34 35 36 37 38 39 3a 3b 3c 3d 3e 3f 40 41 42 43 44 45 46 47 48 49 4a 4b 4c 4d 4e 4f 50 51 52 53 5
   4 55 56 57 58 59 5a 5b 5c 5d 5e 5f 60 61 62 63 64 65 66 67 68 69 6a 6b 6c 6d 6e 6f 70 71 72 73 74 75 76 77 78 79 7a 7b 7c 7d 7e 7f
   Adding Longarray at id 3
   Memory is full let's delete all items
   Free space in storage is 8064 bytes
   Sample code finished Successfully
