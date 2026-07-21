.. zephyr:code-sample:: virtiofs
   :name: virtiofs filesystem
   :relevant-api: file_system_api

   Use file system API over virtiofs.

Overview
********

This sample app demonstrates the use of Zephyr's :ref:`file system API
<file_system_api>` over `virtiofs <https://virtio-fs.gitlab.io/>`_ by reading, creating and listing files and directories.
In the case of virtiofs the mounted filesystem is a directory on the host.

Requirements
************
This sample requires `virtiofsd <https://gitlab.com/virtio-fs/virtiofsd>`_ to run.

Building
********
.. zephyr-app-commands::
   :zephyr-app: samples/subsys/fs/virtiofs
   :board: qemu_x86_64
   :goals: build
   :compact:


Running
*******
Before launching QEMU ``virtiofsd`` has to be running. The static
``vhost-user-fs-pci`` device is embedded in the image through
:code:`CONFIG_QEMU_EXTRA_FLAGS`, but the matching ``-chardev socket`` flag that
selects the socket path is supplied at run time so it can be chosen freely.
Launch ``virtiofsd`` on a socket of your choice:

.. code-block::

   virtiofsd --socket-path=/tmp/vhostqemu --shared-dir shared_dir_path

where :code:`shared_dir_path` is a directory that will be mounted on Zephyr side.
Then point QEMU at the same socket and launch it:

.. code-block::

   export QEMU_EXTRA_FLAGS="-chardev socket,id=char0,path=/tmp/vhostqemu"
   west build -t run

Running with twister
====================
The sample can also be executed automatically with the ``virtiofs`` twister
harness, which starts ``virtiofsd`` on a per-run socket for you (skipping the
test when ``virtiofsd`` is not installed):

.. code-block::

   scripts/twister -p qemu_x86_64 -T samples/subsys/fs/virtiofs

This sample will list the files and directories in the mounted filesystem and print the contents of the file :code:`file` in the mounted directory.
This sample will also create some files and directories.
You can create the sample directory using :code:`prepare_sample_directory.sh`.

Example output:

.. code-block::

   *** Booting Zephyr OS build v4.1.0-rc1-28-gc6816316fc50 ***
   /virtiofs directory tree:
   - dir2 (type=dir)
     - b (type=file, size=3)
     - a (type=file, size=2)
     - c (type=file, size=4)
   - dir (type=dir)
     - some_file (type=file, size=0)
     - nested_dir (type=dir)
        - some_other_file (type=file, size=0)
   - file (type=file, size=27)

   /virtiofs/file content:
   this is a file on the host


After running the sample you can check the created files:

.. code-block:: console

   shared_dir_path$ cat file_created_by_zephyr
   hello world
   shared_dir_path$ cat second_file_created_by_zephyr
   lorem ipsum
