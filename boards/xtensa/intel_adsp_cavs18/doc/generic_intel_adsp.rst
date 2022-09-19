.. _Intel_Adsp_Generic_Running_Guide:

Intel Adsp Generic Running Guide
################################

This documentation describes how to run the intel_adsp_cavs boards. Including:

- intel_adsp_cavs15

- intel_adsp_cavs18

- intel_adsp_cavs20

- intel_adsp_cavs25


Set up the environment
**********************

1. Copy following two tools to the $HOME directory of the target machine (DUT):

- soc/xtensa/intel_adsp/tools/cavstool.py
   (The firmware loader)

- soc/xtensa/intel_adsp/tools/remote-fw-service.py
   (The remote service provider)

   You can use scp command to copy them to DUT, Ex.

.. code-block:: console

   $scp boards/xtensa/intel_adsp/tools/cavstool.py user@myboard:~
   $scp boards/xtensa/intel_adsp/tools/remote-fw-service.py user@myboard:~

2. In your build machine, install the rimage tool, the signed key and
   the toml config file. Please refer to please refer:


     https://github.com/thesofproject/rimage.


How Remote Service works
************************

The CAVS remote service runs on the target board and interacts with
west. Two services working on the server:

- Run Sevice
  Run Service (or Request Service)  works as a flasher. It will receive and
  download the firmware to the intel_adsp_cavs boards then starts the Zephyr
  Application. It starts at port 10000 by default.

- Log Service
  Log Service redirect the remote target board's /dev/tty console. It will
  output Zephyr's log message to user via network. It starts at port 9999
  by default.

The --remote-host parameter specify the network address which Run Service
provided, and the --pty parameter specifies the network address of log
output service.


Build and run the tests
***********************

1. In the remote target machine, starting the service by:

.. code-block:: console

   sudo ./remote-fw-service.py

2. Build the application. Take hello world as an example:

.. code-block:: console

   west build -b intel_adsp_cavs15 samples/hello_world

3. Run the test by:

.. code-block:: console

   west flash --remote-host {host}:{port} \
              --pty {host}:{port}

Ex.

.. code-block:: console

   west flash --remote-host 192.168.0.1 --pty

   # with specifying the port
   west flash --remote-host 192.168.0.1:12345 \
              --pty 192.168.0.1:54321


Now you can see the outout log in your terminal.


If you don't want to use the default location of rimage tools, you can
also specify the rimage tool, config and key by:

.. code-block:: console

   west flash --remote-host {host}:{port} \
              --pty {host}:{port} \
              --rimage-tool [path to the rimage tool] \
              --config-dir [path to dir of .toml config file] \
              --key [path to signing key]


The cavstool server will listen to the available network interfaces on
port 9999 and 10000 by default. In some case you might need to specify
it only listen on a dedicate IP address, or change the default ports
using, you can do it with following parameters:

.. code-block:: console

   # with specifying the port
   sudo ./remote-fw-service.py --log-port 54321 --req-port 12345

   # can be simplified with
   sudo ./remote-fw-service -p 54321 -r 12345

   # with specifying a IP address
   sudo ./remote-fw-service -s 192.168.0.2

   # with specifying the IP address with a log port
   sudo ./remote-fw-service -s 192.168.0.2:54321

   # with specifying the IP, log and request port
   sudo ./remote-fw-service -s 192.168.0.2:54321 -r 12345

   # Also works in this way
   sudo ./remote-fw-service -s 192.168.0.2 -p 54321 -r 12345


Run by twister
**************

For running by twister, the --remote-host parameter needs to be added into
the content of the --west-flash parameter. Assume the IP address of your CAVS
boarad is 192.168.1.2, the port of the Request Service is 12345, the port of
the Log Service is 54321, this is an example of the twister command:

.. code-block:: console

   twister -p intel_adsp_cavs25 --device-testing \
     --device-serial-pty="$ZEPHYR_BASE/soc/xtensa/intel_adsp/tools/cavstool_client.py,-s,192.168.1.2:54321,-l" \
     --west-flash="--remote-host=192.168.1.4:12345"


Like we run tests by west, if you don't want to use the default location of
SOF tools, you can also specify the rimage tool, config and key by:

.. code-block:: console

   twister -p intel_adsp_cavs15 --device-testing \
     --device-serial-pty="$ZEPHYR_BASE/soc/xtensa/intel_adsp/tools/cavstool_client.py,-s,192.168.1.2:54321,-l" \
     --west-flash="--remote-host=192.168.1.2:12345,\
     --rimage-tool=$HOME/sof/rimage/rimage,\
     --config-dir=$HOME/sof/rimage/config/,\
     --key=$HOME/sof/keys/otc_private_key.pem" \
     -T tests/kernel/semaphore/semaphore/ -vv


Note that there should be no space between the arguments in --west-flash,
it use comma to separate the parameters.


Run one or multiple boards
**************************

In the above example, there are many parameters need to be keying in when
running by twister. You can reduce it is by writing a hardware map file.
Ruuning twister with the hardware map file also support you running tests
on single/multiple ADSP boards parallelly.

Let see how to use a hardware map file by twister to run a single board,
this is the content of the hardware map file cavs.map:

.. code-block:: console

   - connected: true
     id: None
     platform: intel_adsp_cavs25
     product: None
     runner: intel_adsp
     serial_pty: "/home/user/zephyrproject/zephyr/soc/xtensa/intel_adsp/tools/cavstool_client.py,-s,192.168.1.4,-l"
     runner_params:
       - --remote-host=192.168.1.4


If you need to specify the port using, you can write the hardware map file
like following example. Assume you have a log port of 54321 and a req port
12345:

.. code-block:: console

   - connected: true
     id: None
     platform: intel_adsp_cavs25
     product: None
     runner: intel_adsp
     serial_pty: "/home/user/zephyrproject/zephyr/soc/xtensa/intel_adsp/tools/cavstool_client.py,-s,192.168.1.4,--log-port,54321,-l"
     runner_params:
       - --remote-host=192.168.1.4
       - --tool-opt=--req-port
       - --tool-opt=12345


And another simplified form of the port specifying is to use {host}:{port}
for the --remote-host of the runner params and -s of the serial-pty, Ex.

.. code-block:: console

   - connected: true
     id: None
     platform: intel_adsp_cavs25
     product: None
     runner: intel_adsp
     serial_pty: "/home/user/zephyrproject/zephyr/soc/xtensa/intel_adsp/tools/cavstool_client.py,-s,192.168.1.4:54321,-l"
     runner_params:
       - --remote-host=192.168.1.4:12345


Then you can run twister with fewer parameters:

.. code-block:: console

   twister --hardware-map ./cavs.map --device-testing -T samples/hello_world -vv


And below example of the hardware map file shows you how to run tests in
mulitple boards:

.. code-block:: console

   - connected: true
     id: None
     platform: intel_adsp_cavs15
     product: None
     runner: intel_adsp
     serial_pty: "/home/user/zephyrproject/zephyr/soc/xtensa/intel_adsp/tools/cavstool_client.py,-s,192.168.1.2,-l"
     runner_params:
       - --remote-host=192.168.1.2

   - connected: true
     id: None
     platform: intel_adsp_cavs18
     product: None
     runner: intel_adsp
     serial_pty: "/home/user/zephyrproject/zephyr/soc/xtensa/intel_adsp/tools/cavstool_client.py,-s,192.168.1.3,-l"
     runner_params:
       - --remote-host=192.168.1.3

   - connected: true
     id: None
     platform: intel_adsp_cavs25
     product: None
     runner: intel_adsp
     serial_pty: "/home/user/zephyrproject/zephyr/soc/xtensa/intel_adsp/tools/cavstool_client.py,-s,192.168.1.4,-l"
     runner_params:
       - --remote-host=192.168.1.4

If you don't want to run certain platform in this file, just make
the "connected" field from "true" to "false", it will be skip.

Again, if you don't use the default location of the SOF tools, you
can remove the --rimage-tool, --config-dir and --key in the extra_params
field. For example:

.. code-block:: console

   - connected: true
     id: None
     platform: intel_adsp_cavs25
     product: None
     runner: intel_adsp
     serial_pty: "/home/user/zephyrproject/zephyr/soc/xtensa/intel_adsp/tools/cavstool_client.py,-s,192.168.1.4,-l"
     runner_params:
       - --remote-host=192.168.1.4
       - --rimage-tool=/home/user/sof/rimage/rimage
       - --config-dir=/home/user/sof/rimage/config/
       - --key=/home/user/sof/keys/otc_private_key_3k.pem


To run multiple boards does also work when specifying the ports.


Passing extra parameter to tools
********************************

wwe can pass parameters to run/require service by the --tool-opt
option. This is for possible extending in the future. For example:

.. code-block:: console

   west flash --remote-host=192.168.0.1 --pty=192.168.0.1 \
              --tool-opt=--arg='white space' --tool-opt=-r --tool-opt=12345

That means our optional parameters will be parsed as:

.. code-block:: console

   ['--arg=white space', '-r', '12345']

Then cavs request service tool can get them.
