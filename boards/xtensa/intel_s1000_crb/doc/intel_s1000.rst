.. _Intel_S1000:

Intel S1000
###########

Overview
********

The Intel S1000 ASIC is designed for complex far-field signal processing
algorithms that use high dimensional microphone arrays to do beamforming,
cancel echoes, and reduce noise. It connects to a host processor chip via
simple SPI and I2S interfaces, to the microphone array via I2S or PDM
interfaces, and to speakers via I2S. In addition, it has an I2C interface
for controlling platform components such as ADCs, DACs, CODECs and PMICs.

Intel S1000 is designed to handle speech recognition at the edge (e.g.
locally) so some of your voice commands should still work when not connected
to the network.[1]_

The features include the following:

- Digital Signal Processors

  - Dual Tensilica LX6 cores @ 400 MHz with HiFi3 DSP
  - Single precision scalar floating-point instructions
  - 16KB 4-way I$; 48KB 4-way D$
  - Up to 2400 DMIPS, 3.2 GMACS (16x16), 800 MFLOPS of Compute

- Speech Accelerators

  - A GMM (Gaussian Mixture Model) and neural network accelerator
  - Low power keyboard and limited vocabulary recognition
  - Up to 9.6 GMACS (16x16) of compute

- Internal Memory

  - 4MB shared embedded SRAM
  - 64KB embedded SRAM for streaming samples in low power mode

- External Memory Interfaces

  - Up to 8MB external 16-bit PSRAM
  - Up to 128MB external SPI flash

- I/O Interfaces

  - Host I/O: SPI for command and control, I2S for streaming audio, IRQ, reset, wake, optional USB 2.0 HS device
  - Microphone: I2S/TDM 9.6 MHz max. bit clock
  - Digital Microphone: 4 PDM ports 4.8 MHz max. bit clock
  - Speaker: I2S/TDM 9.6 MHz max. bit clock
  - Instrumentation: I2C master @ 100/400 MHz
  - Debug: UART Tx/Rx/RTS/CTS up to 2.4 Mbaud/s
  - GPIO: 10 mA sink/source, 8x PWM outputs

- Power Management / Consumption

  - Low power idle (memory retention); voice activity detection; play through; full active
  - Clock and power gating support
  - < 20 mW voice activity detection
  - < 250 mW full active

- Package (preliminary): FCCSP132 7.45 x 8.3mm 0.6/0.7mm pitch staggered/orthogonal

- Temperature Range:- Commercial: 0 to 70 degree C; industrial: -40 to +85 degree C

System requirements
*******************

Prerequisites
=============

The Xtensa 'toolchain'_ i.e. XCC is required to build this port. This needs a
license and is available for Linux and Windows. Intel is currently working with Cadence
to make the license available for wider audience. For now, please contact your board
supplier for licensing information and tool chain access.

Set up build environment
========================

To install the Xplorer software, log on to https://xpg.cadence.com/login/gen/ten4genlogin.html.
using your browser.

Once you have entered the login name and password, select version as RF-2015.3 to download
"Xplorer-6.0.3-linux-installer.bin".

Run the installer using these commands:

.. code-block:: console

   cd ~/Downloads
   chmod +x Xplorer-6.0.3-linux-installer.bin
   sudo ./Xplorer-6.0.3-linux-installer.bin

Xplorer software will be installed into the /opt/xtensa folder. Please note a dialogue box
should pop-up after running this command. Otherwise, it means your system is missing some
package which is preventing successful installation, most probably gtk2-i686.  You can
install this missing package with:

.. code-block:: console

   sudo apt-get install gtk2-i686

The Xtensa processor in the S1000 board (cavs21_LX6HiFi3_RF3_WB16) is not
supported by the stock RF-2015.3 (Xplorer-6.0.3) tools installed above. Please
contact your board supplier for downloading the cavs21_LX6HiFi3_RF3_WB16 build.

The cavs21_LX6HiFi3_RF3_WB16 build will be in .tgz format. Once you have downloaded
it, follow the below steps to install the additional needed support:

.. code-block:: console

   tar -xvzf cavs21_LX6HiFi3_RF3_WB16_linux_redist.tgz --directory /opt/xtensa/XtDevTools/install/builds.
   cd /opt/xtensa/XtDevTools/install/builds/RF-2015.3-linux/cavs21_LX6HiFi3_RF3_WB16
   sudo ./install

"install" is the Xtensa Processor Configuration Installation Tool which is required
to update the installation path. When it prompts to enter the path to the Xtensa Tools
directory, enter /opt/xtensa/XtDevTools/install/tools/RF-2015.3-linux/XtensaTools. You
should use the default registry /opt/xtensa/XtDevTools/install/tools/RF-2015.3-linux/XtensaTools/config.

With the XCC toolchain installed, the Zephyr build system must be instructed
to use this particular variant by setting the ``ZEPHYR_TOOLCHAIN_VARIANT``
shell variable. Some more environment variables are also required (see below):

.. code-block:: console

   export XTENSA_PREFER_LICENSE=XTENSA
   export ZEPHYR_TOOLCHAIN_VARIANT=xcc
   export TOOLCHAIN_VER=RF-2015.3-linux
   export XTENSA_CORE=cavs21_LX6HiFi3_RF3_WB16
   export XTENSA_SYSTEM=/opt/xtensa/XtDevTools/install/tools/RF-2015.3-linux/XtensaTools/config/
   export XTENSA_BUILD_PATHS=/opt/xtensa/XtDevTools/install/builds/
   export XTENSA_OCD_PATH=/opt/Tensilica/xocd-12.0.4

Flashing
========

The usual ``flash`` target will work with the ``intel_s1000_crb`` board
configuration. Here is an example for the :ref:`hello_world`
application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: intel_s1000_crb
   :goals: flash

Refer to :ref:`build_an_application` and :ref:`application_run` for
more details.

Setting up UART
===============

We recommend using a "FT232RL FTDI USB To TTL Serial Converter Adapter Module"
to tap the UART data. The J8 Header on S1000 CRB is dedicated for UART.
Connect the J8 header and UART chip as shown below:

+------------+-----------+
| UART chip  | J8 Header |
+============+===========+
| DTR        |           |
+------------+-----------+
| RX         | 2         |
+------------+-----------+
| TX         | 4         |
+------------+-----------+
| VCC        |           |
+------------+-----------+
| CTS        |           |
+------------+-----------+
| GND        | 10        |
+------------+-----------+

Attach one end of the USB cable to the UART chip and the other end to the
Linux system. Use ``minicom`` or another terminal emulator to monitor the
UART data by following these steps:

.. code-block:: console

   dmesg | grep USB
   minicom -D /dev/ttyUSB0

Here, the first command will indicate the tty to which the USB is connected.
The second command assumes it was USB0 and opens up minicom. You can suitably
modify the second command based on the output of the first command. The serial
settings configured in zephyr is "115200 8N1". This is also the default
settings in minicom and can be verified by pressing Ctrl-A Z P.

Using JTAG
==========

For debugging, you can use a flyswatter2 to connect to the S1000 CRB.
The pinouts for flyswatter2 and the corresponding pinouts for CRB are
shown below. Note that pin 6 on CRB is left unconnected.

The corresponding pin mapping is

+-------------+-----------+
| Flyswatter2 |   S1000   |
+=============+===========+
|     1       |     7     |
+-------------+-----------+
|     2       |    NC     |
+-------------+-----------+
|     3       |     4     |
+-------------+-----------+
|     4       |    NC     |
+-------------+-----------+
|     5       |     3     |
+-------------+-----------+
|     6       |     8     |
+-------------+-----------+
|     7       |     2     |
+-------------+-----------+
|     8       |    NC     |
+-------------+-----------+
|     9       |     1     |
+-------------+-----------+
|     10      |    NC     |
+-------------+-----------+
|     11      |    NC     |
+-------------+-----------+
|     12      |    NC     |
+-------------+-----------+
|     13      |     5     |
+-------------+-----------+
|     14      |    NC     |
+-------------+-----------+
|     15      |    NC     |
+-------------+-----------+
|     16      |    NC     |
+-------------+-----------+
|     17      |    NC     |
+-------------+-----------+
|     18      |    NC     |
+-------------+-----------+
|     19      |    NC     |
+-------------+-----------+
|     20      |    NC     |
+-------------+-----------+

Ideally, these connections should have been enough to get the debug working.
However, we need to short 2 pins on Host Connector J3 via a 3.3k resistor
(simple shorting without the resistor will also do) for debugging to work.
Those 2 pins are Pin5 HOST_RST_N_LT_R) and Pin21 (+V_HOST_3P3_1P8).

References
**********

.. target-notes::

.. _`Purchase Intel S1000`: https://click.intel.com/intelr-speech-enabling-developer-kit.html
.. _`Set Up Your Intel Speech Enabling Developer Kit`: https://youtu.be/wGoXiJFkm6k
.. _`FT232 UART`: https://www.amazon.com/FT232RL-Serial-Converter-Adapter-Arduino/dp/B06XDH2VK9
