:orphan:

.. _intel_adsp_generic:

Intel ADSP CAVS and ACE
#######################

Intel's Audio and Digital Signal Processing (ADSP) hardware offerings
include the Converged Audio Video Sensing (CAVS) series and its successor,
the Audio and Context Engine (ACE). These Xtensa-based ADSPs can be integrated
into a variety of Intel products. The below table lists (some of) the Intel
microprocessor(s) that each version of the Intel ADSP is compatible with.

+----------+-----------------------------+
| ADSP     | Microprocessor              |
+==========+=============================+
| CAVS 1.5 | Apollo Lake                 |
+----------+-----------------------------+
| CAVS 1.8 | Whiskey Lake                |
+----------+-----------------------------+
| CAVS 2.5 | Tiger Lake                  |
+----------+-----------------------------+
| ACE 1.5  | Meteor Lake                 |
+----------+-----------------------------+

Intel open-sources firmware for its ADSP hardware under the Sound Open Firmware
(`SOF`_) project. SOF can be built with either Zephyr or Cadence's proprietary
Xtensa OS (XTOS) and run on a variety of Intel and non-Intel platforms.

In general, the Intel `UP2`_ and `UP Xtreme`_ product lines are the publicly
available reference boards for Zephyr's Intel ADSP support. This guide uses the
`UP Xtreme i11-0001 series`_ (:ref:`intel_adsp_cavs25`) board as an example.
However, the instructions are generic and will work on other boards unless
otherwise stated. You will be referred to the documentation for your specific
board in these cases.

System requirements
*******************

Setting Up Target Board
-----------------------

You can only flash Zephyr to the ADSP by using Zephyr's Python tool in a Linux
host running on the board's main CPU. It is possible (and recommended) for users
to build the binary locally on their development machine and flash remotely,
but the board itself must be capable of running the Python script that receives
the binary sent over the network by West and flashes it. You should install a
version of Linux that supports or comes with the current version of Python that
Zephyr requires. Consider using Ubuntu 20.04, which comes with Python 3.8
installed.

Note that if you plan to use SOF on your board, you will need to build and
install the modified Linux SOF kernel instead of the default kernel. It is
recommended you follow the `SOF instructions`_ to build and run SOF on Zephyr.

UP2 and UP Xtreme users can refer to the `UP Community wiki`_ for help installing a
Linux operating system on their board.

Signing Tool
------------

As firmware binary signing is mandatory on Intel products from Skylake onwards,
you will also need to set up the SOF rimage signing tool and key.

.. code-block:: shell

   cd zephyrproject/modules/audio/sof/
   git clone https://github.com/thesofproject/rimage --recurse-submodules
   cd rimage

Follow the instructions in the :file:`README.md` to build and install the tool
globally on your system. You should be able to invoke the binary from the
command line with the name "rimage". For example:

.. code-block:: shell

   which rimage
   /usr/local/bin/rimage

If you are unable to install it, you can also pass the path to the tool binary
as an argument to ``west flash``, though this is not recommended.

Xtensa Toolchain (Optional)
---------------------------

The Zephyr SDK provides GCC-based toolchains necessary to build Zephyr for
the CAVS and ACE boards. However, users seeking greater levels of optimization
may desire to build with the proprietary Xtensa toolchain distributed by
`Cadence`_ instead. The following instructions assume you have purchased and
installed the toolchain(s) and core(s) for your board following their
instructions.

First, make sure to set ``XTENSAD_LICENSE_FILE`` as instructed by Cadence.
Next, set the following environment variables:

.. code-block:: shell

   export XTENSA_TOOLCHAIN_PATH=$HOME/xtensa/XtDevTools/install/tools
   export XTENSA_BUILDS_DIR=$XTENSA_TOOLCHAIN_PATH/../builds
   export ZEPHYR_TOOLCHAIN_VARIANT=xcc
   export TOOLCHAIN_VER=RG-2017.8-linux
   export XTENSA_CORE=cavs2x_LX6HiFi3_2017_8

The bottom three variables are specific to each version of CAVS / ACE; refer to
your board's documentation for their values.

Programming and Debugging
*************************

Building
--------

Build as usual.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: intel_adsp_cavs25
   :goals: build

Signing
-------

West automatically signs the binary as the first step of flashing, but if you
need to sign the binary yourself without flashing, you can invoke the west sign
command directly. Read the output of a ``west flash`` command to find the
``west sign`` invocation. You can copy and modify it for your own purposes.

As mentioned previously, if you're unable to install the rimage tool
globally, you can pass it the path to the tool binary as an argument to
``west flash`` if the flash target exists for your board. To see a list
of all arguments to the Intel ADSP runner, run the following after you have
built the binary. There are multiple arguments related to signing, including a
key argument.

.. code-block:: console

   west flash --context

Remote Flashing to CAVS-based ADSP
----------------------------------

As mentioned previously, the recommended way to run and monitor the output of
Zephyr on CAVS boards is remotely. The Linux host on the main CPU may freeze up
and need to be restarted if a flash or runtime error occurs on the ADSP. From
this point onward, we will refer to the board as the "remote host" and your
development machine as the "local host".

Copy the below scripts to the CAVS board.
:zephyr_file:`soc/xtensa/intel_adsp/tools/remote-fw-service.py` will receive
the binary sent over the network by West and invoke
:zephyr_file:`soc/xtensa/intel_adsp/tools/cavstool.py` (referred to as the
"CAVS tool"), which performs the flash and captures the log. Start
:file:`remote-fw-service.py`.

.. code-block:: console

   scp -r $ZEPHYR_BASE/soc/xtensa/intel_adsp/tools/cavstool.py username@remotehostname
   scp -r $ZEPHYR_BASE/soc/xtensa/intel_adsp/tools/remote-fw-service.py username@remotehostname
   ssh username@remotehostname
   sudo ./remote-fw-service.py

:file:`remote-fw-service.py` uses ports 9999 and 10000 on the remote host to
communicate. It forwards logs collected by :file:`cavstool.py` on port 9999
(referred to as its "log port") and services requests on port 10000
(its "requests port"). When you run West or Twister on your local host,
it sends requests using the :zephyr_file:`soc/xtensa/intel_adsp/tools/cavstool_client.py`
script (referred to as "CAVS tool client"). It also uses ports 9999 and 10000 on
your local host, so be sure those ports are free.

Flashing with West is simple.

.. code-block:: console

   west flash --remote-host remotehostname --pty remotehostname

Running tests with Twister is slightly more complicated.

.. code-block:: console

   twister -p intel_adsp_cavs25 --device-testing --device-serial-pty="$ZEPHYR_BASE/soc/xtensa/intel_adsp/tools/cavstool_client.py,-s,remotehostname,-l" --west-flash="--remote-host=remotehostname" -T samples/hello_world

If your network is set up such that the TCP connection from
:file:`cavstool_client.py` to :file:`remote-fw-service.py` is forwarded through
an intermediate host, you may need to tell :file:`cavstool_client.py` to connect
to different ports as well as a different hostname. You can do this by appending
the port numbers to the intermediate host name.

.. code-block:: console

   west flash --remote-host intermediatehost:reqport --pty remotehostname:logport
   twister -p intel_adsp_cavs25 --device-testing --device-serial-pty="$ZEPHYR_BASE/soc/xtensa/intel_adsp/tools/cavstool_client.py,-s,remotehostname:logport,-l" --west-flash="--remote-host=remotehostname:reqport" -T samples/hello_world

You can also save this information to a hardware map file and pass that to
Twister.

.. code-block:: console

   twister -p intel_adsp_cavs25 --hardware-map cavs.map --device-testing -T samples/hello_world

Here's a sample ``cavs.map``:

.. code-block:: console

   - connected: true
     id: None
     platform: intel_adsp_cavs25
     product: None
     runner: intel_adsp
     serial_pty: "/home/zephyrus/zephyrproject/zephyr/soc/xtensa/intel_adsp/tools/cavstool_client.py,-s,remotehostname:logport,-l"
     runner_params:
     - --remote-host=remotehostname:reqport

Any of the arguments you would pass to Twister or West, you can pass with the
hardware map. As mentioned previously, you can see the Intel ADSP runner
arguments by passing the ``--context`` flag while flashing with West.

Refer to :ref:`twister_script` for more information on hardware maps.

Local Flashing to CAVS-based ADSP
---------------------------------

You can also directly flash the signed binary with the CAVS tool on the board.
This may be useful for debugging.

.. code-block:: console

   sudo ./cavstool.py zephyr.ri

You should see the following at the end of the log if you are successful:

.. code-block:: console

   ***** Booting Zephyr OS vx.x.x-xxx-gxxxxxxxxxxxx *****
   Hello World! intel_adsp_cavs25

Flashing to ACE-based ADSP
--------------------------

Flashing is not yet supported for platforms with ACE-based ADSP, as these
platforms are not yet publicly available.

Debugging
---------

As Zephyr doesn't (yet) support GDB for the Intel ADSP platforms, users are
recommended to take advantage of Zephyr's built-in :ref:`coredump` and
:ref:`logging_api` features.

Troubleshooting
***************

You can pass verbose flags directly to the Intel ADSP scripts:

.. code-block:: console

   sudo ./remote-fw-service.py -v
   sudo ./cavstool.py zephyr.ri -v

To see a list of their arguments:

.. code-block:: console

   sudo ./remote-fw-service.py --help
   sudo ./cavstool.py --help

If flashing fails at ``west sign`` with errors related to unparsed keys, try
reinstalling the latest version of the signing tool. For example:

.. code-block:: shell

   error: 1 unparsed keys left in 'adsp'
   error: 1 unparsed arrays left in 'adsp'

If you get an "Address already in use" error when starting
:file:`remote-fw-service.py` on the board, you may have another instance of the
script running. Kill it.

.. code-block:: console

   $ sudo netstat -peanut | grep 9999
   tcp   0   0 0.0.0.0:9999   0.0.0.0:*   LISTEN   0   289788   14795/python3
   $ sudo kill 14795

If West or Twister successfully sign and establish TCP connections
with :file:`remote-fw-service.py` but hang with no output afterwards,
there are two possibilities: either :file:`remote-fw-service.py` failed
to communicate, or :file:`cavstool.py` failed to flash. Log into
the remote host and check the output of :file:`remote-fw-service.py`.

If a message about "incorrect communication" appears, you mixed up the port
numbers for logging and requests in your command or hardware map. Switch them
and try again.

.. code-block:: shell

   ERROR:remote-fw:incorrect monitor communication!

If a "load failed" message appears, that means the flash failed. Examine the
log of ``west flash`` and carefully check that the arguments to ``west sign``
are correct.

.. code-block:: console

   WARNING:cavs-fw:Load failed?  FW_STATUS = 0x81000012
   INFO:cavs-fw:cAVS firmware load complete
   --

Sometimes a flash failure or network miscommunication corrupts the state of
the ADSP or :file:`remote-fw-service.py`. If you are unable to identify a
cause of repeated failures, try restarting the scripts and / or power cycling
your board to reset the state.

Users - particularly, users of the Xtensa toolchain - should also consider
clearing their Zephyr cache, as caching issues can occur from time to time.
Delete the cache as well as any applicable build directories and start from
scratch. You can try using the "pristine" option of West first, if you like.

.. code-block:: console

   rm -rf build twister-out*
   rm -rf ~/.ccache ~/.cache/zephyr

Xtensa toolchain users can get more detailed logs from the license server by
exporting ``FLEXLM_DIAGNOSTICS=3``.

.. _SOF: https://thesofproject.github.io/latest/index.html

.. _Chromebooks: https://www.hp.com/us-en/shop/pdp/hp-chromebook-x360-14c-cc0047nr

.. _UP2: https://up-board.org/upsquared/specifications/

.. _UP Xtreme: https://up-board.org/up-xtreme/

.. _UP Xtreme i11-0001 series: https://up-shop.org/up-xtreme-i11-boards-0001-series.html

.. _SOF instructions: https://thesofproject.github.io/latest/getting_started/build-guide/build-with-zephyr.html

.. _UP Community wiki: https://github.com/up-board/up-community/wiki/Ubuntu

.. _Cadence: https://www.cadence.com/en_US/home/tools/ip/tensilica-ip.html
