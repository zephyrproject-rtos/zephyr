.. zephyr:code-sample:: fs-unmount-path
   :name: fs_unmount_path
   :relevant-api: file_system_api

   Mount a RAM disk as FAT and unmount it with :c:func:`fs_unmount_path`.

Overview
********

Validates path-based unmount used for hot-unplug flows (for example USB host MSC).

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/fs/unmount_path
   :board: native_sim
   :goals: build run
   :compact:
