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

1. Copy soc/xtensa/intel_adsp/tools/cavstool.py to the target
   host machine (DUT).

2. In your build machine, install the rimage tool, the signed key and
   the toml config file. Please refer to please refer:


     https://github.com/thesofproject/rimage.


Build and run the tests
***********************

1. In the remote target machine, starting the service by:

.. code-block:: console

   sudo ./cavstool.py

2. Build the application. Take semaphore as an example:

.. code-block:: console

   west build -b intel_adsp_cavs15 samples/hello_world

3. Run the test by:

.. code-block:: console

   west flash --remote-host [remote hostname or ip addr]

Now you can see the outout log in your terminal.


If you don't want to use the default location of rimage tools, you can
also specify the rimage tool, config and key by:

.. code-block:: console

   west flash --remote-host [remote hostname or ip addr] \
              --rimage-tool [path to the rimage tool] \
              --config-dir [path to dir of .toml config file] \
              --key [path to signing key]

Run by twister
**************

Assume the remote ADSP host's ip address is 192.168.1.2, you can run the
twister by following command:

.. code-block:: console

   twister -p intel_adsp_cavs15 --device-testing \
     --device-serial-pty="$ZEPHYR_BASE/soc/xtensa/intel_adsp/tools/cavstool_client.py,-s,192.168.1.2,-l" \
     --west-flash="--remote-host=192.168.1.2,--pty"


Like we run test by west, if you don't want to use the default location of
SOF tools, you can also specify the rimage tool, config and key by:

.. code-block:: console

   twister -p intel_adsp_cavs15 --device-testing \
     --device-serial-pty="$ZEPHYR_BASE/soc/xtensa/intel_adsp/tools/cavstool_client.py,-s,192.168.1.2,-l" \
     --west-flash="--remote-host=192.168.1.2,--pty,\
     --rimage-tool=$HOME/sof/rimage/rimage,\
     --config-dir=$HOME/sof/rimage/config/,\
     --key=$HOME/sof/keys/otc_private_key.pem" \
     -T tests/kernel/semaphore/semaphore/ -vv


Note that there should be no space between the arguments in --west-flash,
it use comma to separate the parameters.


Run multiple boards
*******************

In the above example, there are many parameters need to be key in when
running it. There is a more easy way to make you to key in less, and
it also support running multiple boards at the same time.

Ex.
  twister --hardware-map cavs.map --device-testing -T tests/ -v


Run it this way we have to make a hardware map file first. Edit a
hardware map file like below example, you can run one/multiple tests
on one/multiple ADSP boards parallelly.

And if you don't want to run it in certain platform, just make
the "connected" field from "true" to "false", it will be skip.

Here is a example of the hardware map file:

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


By the way, if you don't use the default location of the SOF tools, you
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
