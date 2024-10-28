.. zephyr:code-sample:: fxos8700
   :name: FXOS8700 Accelerometer/Magnetometer Sensor
   :relevant-api: sensor_interface

   Get accelerometer and magnetometer data from an FXOS8700 sensor (polling & trigger mode).

Overview
********

This sample application shows how to use the FXOS8700 driver.
The driver supports FXOS8700 accelerometer/magnetometer and
MMA8451Q, MMA8652FC, MMA8653FC accelerometers.

Building and Running
********************

This project outputs sensor data to the console. FXOS8700
sensor is present on the :zephyr:board:`frdm_k64f`, :zephyr:board:`frdm_k22f`,
:zephyr:board:`frdm_kw41z`, :ref:`hexiwear`, and :zephyr:board:`twr_ke18f` boards.
Accelerometer only devices are present on the :zephyr:board:`frdm_kl25z`,
:zephyr:board:`bbc_microbit`, and :ref:`reel_board` boards. It does not work on
QEMU.

Building and Running for FRDM-K64F
==================================
FRDM-K64F is equipped with FXOS8700CQ accelerometer and magnetometer.
Sample can be built and executed for the FRDM-K64F as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/fxos8700
   :board: frdm_k64f
   :goals: build flash
   :compact:

Example building for the FRDM-K64F with motion detection support:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/fxos8700
   :board: frdm_k64f
   :conf: "prj.conf overlay-motion.conf"
   :goals: build flash
   :compact:

Building and Running for FRDM-K22F
==================================
FRDM-K22F is equipped with FXOS8700CQ accelerometer and magnetometer.
Sample can be built and executed for the FRDM-K22F as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/fxos8700
   :board: frdm_k22f
   :goals: build flash
   :compact:

Example building for the FRDM-K22F with motion detection support:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/fxos8700
   :board: frdm_k22f
   :conf: "prj.conf overlay-motion.conf"
   :goals: build flash
   :compact:

Building and Running for TWR-KE18F
==================================
TWR-KE18F is equipped with FXOS8700CQ accelerometer and magnetometer.
The FXOS8700CQ IRQ lines, however, are not connected by default, so
motion detection is not supported.

Sample can be built and executed for the TWR-KE18F as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/fxos8700
   :board: twr_ke18f
   :goals: build flash
   :compact:

Building and Running for FRDM-KL25Z
===================================
FRDM-KL25Z is equipped with MMA8451Q accelerometer.
Sample can be built and executed for the FRDM-KL25Z as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/fxos8700
   :board: frdm_kl25z
   :conf: "prj_accel.conf"
   :goals: build flash
   :compact:

Building and Running for Micro Bit
==================================
Micro Bit is equipped with MMA8653FC accelerometer.
Sample can be built and executed for the Micro Bit as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/fxos8700
   :board: bbc_microbit
   :conf: "prj_accel.conf"
   :goals: build flash
   :compact:

Building and Running for reel board
===================================
The reel board is equipped with MMA8652FC accelerometer.
Sample can be built and executed for the reel board as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/fxos8700
   :board: reel_board
   :conf: "prj_accel.conf"
   :goals: build flash
   :compact:

Building and Running for MIMXRT685-EVK
======================================
MIMXRT685-EVK is equipped with FXOS8700CQ accelerometer and magnetometer.
Sample can be built and executed for the MIMXRT685-EVK as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/fxos8700
   :board: mimxrt685_evk/mimxrt685s/cm33
   :goals: build flash
   :compact:

Building and Running for MIMXRT595-EVK
======================================
MIMXRT595-EVK is optionally equipped with FXOS8700CQ accelerometer and magnetometer.
Please confirm the FXOS8700CQ(U6) is populated on your board.
Sample can be built and executed for the MIMXRT595-EVK as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/fxos8700
   :board: mimxrt595_evk/mimxrt595s/cm33
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

   AX= -0.191537 AY=  0.067037 AZ=  9.902418 MX=  0.379000 MY=  0.271000 MZ= -0.056000 T= 22.080000
   AX= -0.162806 AY=  0.143652 AZ=  9.940725 MX=  0.391000 MY=  0.307000 MZ= -0.058000 T= 22.080000
   AX= -0.172383 AY=  0.134075 AZ=  9.969455 MX=  0.395000 MY=  0.287000 MZ= -0.017000 T= 22.080000
   AX= -0.210690 AY=  0.105344 AZ=  9.911994 MX=  0.407000 MY=  0.306000 MZ= -0.068000 T= 22.080000
   AX= -0.153229 AY=  0.124498 AZ=  9.950302 MX=  0.393000 MY=  0.301000 MZ= -0.021000 T= 22.080000
   AX= -0.153229 AY=  0.095768 AZ=  9.921571 MX=  0.398000 MY=  0.278000 MZ= -0.040000 T= 22.080000
   AX= -0.162806 AY=  0.105344 AZ=  9.902418 MX=  0.372000 MY=  0.300000 MZ= -0.046000 T= 22.080000

<repeats endlessly>
