.. zephyr:code-sample:: ftp-client
   :name: FTP client shell sample
   :relevant-api: ftp_client

   FTP client sample interacting with FTP server over Zephyr shell.

Overview
********

This sample demonstrates the use of the FTP client library to interact with a
FTP server running on a host. The FTP commands are exposed via FTP shell module
provided by Zephyr.

Requirements
************

- :ref:`networking_with_eth_qemu`, :ref:`networking_with_qemu` or :ref:`networking_with_native_sim`
- Linux machine

Building and Running
********************

Build the Zephyr version of the application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/ftp_client
   :board: <board_to_use>
   :goals: build
   :compact:

The sample requires a running FTP server to interact with. This for example can
be achieved by running the Python `pyftpdlib`_ module, which provides built-in
FTP server functionality:

.. code-block:: bash

   pip install pyftpdlib
   mkdir file_root
   echo "some test data" > file_root/test_file.txt
   python3 -m pyftpdlib -p 2121 --write --directory=file_root

This installs the required Python package, creates a root that the FTP server
will expose and creates a dummy test file. Note, that the server will run in a
write-mode, allowing to create, delete and overwrite files and directories - be
careful when using other root locations than the test directory.

Launch :command:`net-setup.sh` in net-tools:

.. code-block:: bash

   ./net-setup.sh

Build and run the FTP client sample application for native_sim:

.. zephyr-app-commands::
   :zephyr-app: samples/net/ftp_client
   :host-os: unix
   :board: native_sim
   :goals: run
   :compact:

The sample prints the greeting and exposes the network shell module to interact with:

.. code-block:: console

   [00:00:00.000,000] <inf> net_config: Initializing network
   [00:00:00.000,000] <inf> net_config: IPv4 address: 192.0.2.1
   [00:00:00.110,000] <inf> net_config: IPv6 address: 2001:db8::1
   [00:00:00.110,000] <inf> net_config: IPv6 address: 2001:db8::1
   [00:00:00.110,000] <inf> ftp_client_sample: Starting FTP client sample
   [00:00:00.110,000] <inf> ftp_client_sample: Network connectivity established.
   [00:00:00.110,000] <inf> ftp_client_sample: You can interact with FTP server via shell.

Connect to the FTP server with ``net ftp connect`` command. The server by default
exposes ``anonymous`` user with dummy password:

.. code-block:: console

   uart:~$ net ftp connect anonymous null 192.0.2.2 2121
   220 pyftpdlib 2.1.0 ready.
   530 Log in with USER and PASS first.
   331 Username ok, send password.
   230 Login successful.

Once successfully connected, a various FTP commands can be executed to interact
with the file system on the host.

List directory contents:

.. code-block:: console

   uart:~$ net ftp ls
   227 Entering passive mode (192,0,2,2,146,7).
   150 File status okay. About to open data connection.
   -rw-r--r--   1 user     domain users       15 Feb 20 11:16 test_file.txt
   226 Transfer complete.

Read file content:

.. code-block:: console

   uart:~$ net ftp get test_file.txt
   227 Entering passive mode (192,0,2,2,222,177).
   150 File status okay. About to open data connection.
   some test data
   226 Transfer complete.

Create a new directory, change working directory and print the current working directory:

.. code-block:: console

   uart:~$ net ftp mkdir test_dir
   257 "/test_dir" directory created.
   uart:~$ net ftp cd test_dir
   250 "/test_dir" is the current directory.
   uart:~$ net ftp pwd
   257 "/test_dir" is the current directory.

Create a new file, print it's content and then remove it:

.. code-block:: console

   uart:~$ net ftp put new_file.txt
   Input mode (max 2048 characters). End the line with \ to start a new one:
   New file content\

   227 Entering passive mode (192,0,2,2,131,169).
   150 File status okay. About to open data connection.
   226 Transfer complete.
   uart:~$ net ftp get new_file.txt
   227 Entering passive mode (192,0,2,2,140,157).
   150 File status okay. About to open data connection.
   New file content
   226 Transfer complete.
   uart:~$ net ftp rm new_file.txt
   250 File removed.

Disconnect from the server:

.. code-block:: console

   uart:~$ net ftp disconnect
   221 Goodbye.

A complete list of supported FTP commands can be listed with ``--help`` parameter:

.. code-block:: console

   uart:~$ net ftp --help
   ftp - FTP client commands
   Subcommands:
     connect     : Connect to FTP server.
                   Usage: connect <username> <password> <hostname> [<port> <sec_tag>]
     disconnect  : Disconnect from FTP server.
                   Usage: disconnect
     status      : Print connection status.
                   Usage: status
     pwd         : Print working directory.
                   Usage: pwd
     ls          : List information about folder or file.
                   Usage: ls [<options> <path>]
     cd          : Change working directory.
                   Usage: cd <path>
     mkdir       : Create directory.
                   Usage: mkdir <dir_name>
     rmdir       : Delete directory.
                   Usage: rmdir <dir_name>
     rename      : Rename a file.
                   Usage: rename <old_name> <new_name>
     rm          : Delete a file.
                   Usage: rm <file_name>
     get         : Read file content.
                   Usage: get <file_name>
     put         : Write to a file.
                   Usage: put <file_name> [<data>]
     append      : Append a file.
                   Usage: append <file_name> <data>
     mode        : Select transfer type.
                   Usage: mode <ascii/binary>

.. _pyftpdlib: https://github.com/giampaolo/pyftpdlib
