.. zephyr:code-sample:: system_hashmap
   :name: System hashmap
   :relevant-api: hashmap_apis

   Insert, replace, and remove entries in a hashmap.

Overview
********

This is a simple example that repeatedly:

* inserts up to ``CONFIG_TEST_LIB_HASH_MAP_MAX_ENTRIES``
* replaces up to the same number that were previously inserted
* removes all previously inserted keys

Building
********

This application can be built on :zephyr:board:`native_sim <native_sim>` as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/hash_map
   :host-os: unix
   :board: native_sim
   :goals: build
   :compact:

To build for another board, change "native_sim" above to that board's name.

Additionally, it is possible to use one of the other Hashmap implementations by specifying

* ``CONFIG_SYS_HASH_MAP_CHOICE_SC=y`` (Separate Chaining)
* ``CONFIG_SYS_HASH_MAP_CHOICE_OA_LP=y`` (Open Addressing / Linear Probe)
* ``CONFIG_SYS_HASH_MAP_CHOICE_CXX=y`` (C Wrapper around the C++ ``std::unordered_map``)

To stress the Hashmap implementation, adjust ``CONFIG_TEST_LIB_HASH_MAP_MAX_ENTRIES``.

Running
*******

Run ``build/zephyr/zephyr.exe``

Sample Output
*************

.. code-block:: console

    System Hashmap sample

    [00:00:11.000,000] <inf> hashmap_sample: n_insert: 118200 n_remove: 295500 n_replace: 329061 n_miss: 0 n_error: 0 max_size: 118200
    [00:00:11.010,000] <inf> hashmap_sample: success
