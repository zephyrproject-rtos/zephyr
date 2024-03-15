.. _intel_adsp_generic:

Intel ADSP cAVS and ACE
#######################

Intel's Audio and Digital Signal Processing (ADSP) hardware offerings
include the Converged Audio Voice Speech (cAVS) series and its successor,
the Audio and Context Engine (ACE). These Xtensa-based ADSPs can be integrated
into a variety of Intel products. The below table lists (some of) the Intel
microprocessor(s) that each version of the Intel ADSP is compatible with.

+----------+-----------------------------+
| ADSP     | Microprocessor              |
+==========+=============================+
| cAVS 1.5 | Apollo Lake                 |
+----------+-----------------------------+
| cAVS 1.8 | Whiskey Lake                |
+----------+-----------------------------+
| cAVS 2.5 | Tiger Lake                  |
+----------+-----------------------------+
| ACE 1.5  | Meteor Lake                 |
+----------+-----------------------------+

Intel open-sources firmware for its ADSP hardware under the Sound Open Firmware
(`SOF`_) project. SOF can be built with either Zephyr or Cadence's proprietary
Xtensa OS (XTOS) and run on a variety of Intel and non-Intel platforms.

The Intel `UP Xtreme`_ product line has the publicly
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

UP Xtreme users can refer to the `UP Community wiki`_ for help installing a
Linux operating system on their board.

Signing Tool
------------

As firmware binary signing is mandatory on Intel products from Skylake onwards,
you will also need to set up the SOF rimage signing tool and key.

.. code-block:: shell

   cd zephyrproject
   west config manifest.project-filter -- +sof
   west update
   cd modules/audio/sof/tools/rimage

Follow the instructions in the rimage :file:`README.md` to build the tool on
your system. You can either copy the executable to a directory in your PATH or
use ``west config rimage.path /path/to/rimage-build/rimage``; see more details
in the output of ``west sign -h``. Running directly from the build directory
makes you less likely to use an obsolete rimage version by mistake.

Platform-specific configuration files are located in the ``rimage/config/``
subdirectory. For a different configuration directory you can use:
``west config build.cmake-args -- -DRIMAGE_CONFIG_PATH=/path/to/my/rimage/config``.


Xtensa Toolchain (Optional)
---------------------------

The Zephyr SDK provides GCC-based toolchains necessary to build Zephyr for
the cAVS and ACE boards. However, users seeking greater levels of optimization
may desire to build with the proprietary Xtensa toolchain distributed by
`Cadence`_ instead. The following instructions assume you have purchased and
installed the toolchain(s) and core(s) for your board following their
instructions.

First, make sure to set the ``$HOME/.flexlmrc`` file or
``XTENSAD_LICENSE_FILE`` variable as instructed by Cadence.
Next, set the following environment variables:

.. code-block:: shell

   export XTENSA_TOOLCHAIN_PATH=$HOME/xtensa/XtDevTools/install/tools
   export XTENSA_BUILDS_DIR=$XTENSA_TOOLCHAIN_PATH/../builds
   export ZEPHYR_TOOLCHAIN_VARIANT=xcc
   export TOOLCHAIN_VER=RG-2017.8-linux
   export XTENSA_CORE=cavs2x_LX6HiFi3_2017_8

The bottom three variables are specific to each version of cAVS / ACE; refer to
your board's documentation for their values.

Programming and Debugging
*************************

Building
--------

Build as usual.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: intel_adsp/cavs25
   :goals: build

Signing
-------

``west build`` tries to sign the binary at the end of the build. If you need
to sign the binary yourself, you can invoke ``west sign`` directly. Read the
``west`` logs to find the ``west sign`` invocation; you can copy and modify
the command logged for your own purposes. Run ``west sign -h`` for more
details.

The build tries to provide as many default rimage parameters are possible. If
needed, there are several ways to override them depending on your specific
situation and use case. They're often not mutually exclusive but to avoid
undocumented rimage precedence rules it's best to use only one way at a time.

- For local, interactive use prefer ``rimage.extra-args`` in west config, see
  ``west sign -h``. The WEST_CONFIG_LOCAL environment variable can point at a
  different west configuration file if needed.

- You can add or overwrite a ``$platform.toml`` file(s) in your
  ``rimage/config/`` directory

- For board-specific needs you can define WEST_SIGN_OPTS in
  ``boards/my/board/board.cmake``, see example in
  ``soc/intel/adsp/common/CMakeLists.txt``

Starting with Zephyr 3.6.0, ``west flash`` does not invoke ``west sign``
anymore and you cannot pass rimage parameters to ``west flash`` anymore. To
see an up-to-date list of all arguments to the Intel ADSP runner, run the
following after you have built the binary:

.. code-block:: console

   west flash --context

Remote Flashing to cAVS-based ADSP
----------------------------------

As mentioned previously, the recommended way to run and monitor the output of
Zephyr on cAVS boards is remotely. The Linux host on the main CPU may freeze up
and need to be restarted if a flash or runtime error occurs on the ADSP. From
this point onward, we will refer to the board as the "remote host" and your
development machine as the "local host".

Copy the below scripts to the cAVS board.
:zephyr_file:`soc/intel/intel_adsp/tools/remote-fw-service.py` will receive
the binary sent over the network by West and invoke
:zephyr_file:`soc/intel/intel_adsp/tools/cavstool.py` (referred to as the
"cAVS tool"), which performs the flash and captures the log. Start
:file:`remote-fw-service.py`.

.. code-block:: console

   scp -r $ZEPHYR_BASE/soc/intel/intel_adsp/tools/cavstool.py username@remotehostname
   scp -r $ZEPHYR_BASE/soc/intel/intel_adsp/tools/remote-fw-service.py username@remotehostname
   ssh username@remotehostname
   sudo ./remote-fw-service.py

:file:`remote-fw-service.py` uses ports 9999 and 10000 on the remote host to
communicate. It forwards logs collected by :file:`cavstool.py` on port 9999
(referred to as its "log port") and services requests on port 10000
(its "requests port"). When you run West or Twister on your local host,
it sends requests using the :zephyr_file:`soc/intel/intel_adsp/tools/cavstool_client.py`
script (referred to as "cAVS tool client"). It also uses ports 9999 and 10000 on
your local host, so be sure those ports are free.

Flashing with West is simple.

.. code-block:: console

   west flash --remote-host remotehostname --pty remotehostname

Running tests with Twister is slightly more complicated.

.. code-block:: console

   twister -p intel_adsp/cavs25 --device-testing --device-serial-pty="$ZEPHYR_BASE/soc/intel/intel_adsp/tools/cavstool_client.py,-s,remotehostname,-l" --west-flash="--remote-host=remotehostname" -T samples/hello_world

If your network is set up such that the TCP connection from
:file:`cavstool_client.py` to :file:`remote-fw-service.py` is forwarded through
an intermediate host, you may need to tell :file:`cavstool_client.py` to connect
to different ports as well as a different hostname. You can do this by appending
the port numbers to the intermediate host name.

.. code-block:: console

   west flash --remote-host intermediatehost:reqport --pty remotehostname:logport
   twister -p intel_adsp/cavs25 --device-testing --device-serial-pty="$ZEPHYR_BASE/soc/intel/intel_adsp/tools/cavstool_client.py,-s,remotehostname:logport,-l" --west-flash="--remote-host=remotehostname:reqport" -T samples/hello_world

You can also save this information to a hardware map file and pass that to
Twister.

.. code-block:: console

   twister -p intel_adsp/cavs25 --hardware-map cavs.map --device-testing -T samples/hello_world

Here's a sample ``cavs.map``:

.. code-block:: console

   - connected: true
     id: None
     platform: intel_adsp/cavs25
     product: None
     runner: intel_adsp
     serial_pty: "/home/zephyrus/zephyrproject/zephyr/soc/intel/intel_adsp/tools/cavstool_client.py,-s,remotehostname:logport,-l"
     runner_params:
     - --remote-host=remotehostname:reqport

Any of the arguments you would pass to Twister or West, you can pass with the
hardware map. As mentioned previously, you can see the Intel ADSP runner
arguments by passing the ``--context`` flag while flashing with West.

Refer to :ref:`twister_script` for more information on hardware maps.

Local Flashing to cAVS-based ADSP
---------------------------------

You can also directly flash the signed binary with the cAVS tool on the board.
This may be useful for debugging.

.. code-block:: console

   sudo ./cavstool.py zephyr.ri

You should see the following at the end of the log if you are successful:

.. code-block:: console

   ***** Booting Zephyr OS vx.x.x-xxx-gxxxxxxxxxxxx *****
   Hello World! intel_adsp

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

.. _UP Xtreme: https://up-board.org/up-xtreme/

.. _UP Xtreme i11-0001 series: https://up-shop.org/up-xtreme-i11-boards-0001-series.html

.. _SOF instructions: https://thesofproject.github.io/latest/getting_started/build-guide/build-with-zephyr.html

.. _UP Community wiki: https://github.com/up-board/up-community/wiki/Ubuntu

.. _Cadence: https://www.cadence.com/en_US/home/tools/ip/tensilica-ip.html
