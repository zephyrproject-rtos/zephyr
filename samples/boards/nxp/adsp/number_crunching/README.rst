.. zephyr:code-sample:: number_crunching
   :name: Number crunching using optimized library

   Set up and use different backends for complex math operations.

Overview
********

Number crunching sample does vector operations, Fast Fourier Transformation and filtering.
This example demonstrates how to include a proprietary static library into the Zephyr build system.
The library is in an out of tree location.

To use this sample, with an out of tree library, one needs to define an environment variable
``LIB_LOCATION``.
In that location, one needs to have a ``CMakeLists.txt`` that defines:

.. code-block:: cmake

	# Link with the external 3rd party library.
	set(LIB_DIR         "lib"                   CACHE STRING "")
	set(INCLUDE_DIR     "include"               CACHE STRING "")
	set(LIB_NAME        "proprietary_lib.a"     CACHE STRING "")

If the environment variable ``LIB_LOCATION`` is not defined, a default Zephyr API is used instead.

This sample executes some mathematical functions that can be used for audio processing like
filtering (Fast Fourier Transform (FFT)) or echo cancellation (Least Mean Square (LMS) filter
algorithm).

The sample has:

- :file:`main.c`: calls the generic math functions;
- :file:`math_ops.c`: executes the math functions, computes the cycles it took to execute and checks the output;
- :file:`cmsis_dsp_wrapper.c`: calls the exact math functions from CMSIS-DSP if :kconfig:option:`CONFIG_CMSIS_DSP` is defined and ``LIB_LOCATION`` is not defined;
- :file:`nature_dsp_wrapper`: if ``LIB_LOCATION`` is defined and points to an out of tree location where that NatureDSP lib and headers can be found, calls the exact math functions from NatureDSP library.

If one wants to include a new backend it needs to create a new wrapper for that library or backend.

Requirements
************

CMSIS-DSP is an optional module and needs to be added explicitly to your Zephyr workspace:

.. code-block:: shell

   west config manifest.project-filter -- +cmsis-dsp
   west update cmsis-dsp

NatureDSP can be taken from Github: https://github.com/foss-xtensa/ndsplib-hifi4/tree/main.
To compile it use the steps described in the repository.

Building and Running
*********************

To build the sample with ``west`` for the ``imx8mp_evk/mimx8ml8/adsp``, which is the HiFi4 DSP core
from NXP i.MX8M Plus board, run:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nxp/adsp/number_crunching/
   :board: imx8mp_evk/mimx8ml8/adsp
   :goals: build run
   :compact:

An output example, for CMSIS-DSP is:

.. code-block:: console

	*** Booting Zephyr OS build v3.7.0-2815-g9018e424d7a1 ***

	Proprietary library example!

	[Library Test] == Vector Sum test  ==
	[Backend] CMSIS-DSP module
	[Library Test] Vector Sum takes 6886 cycles
	[Library Test] == Vector Sum test end with 1 ==

	[Library Test] == Vector power sum test  ==
	[Backend] CMSIS-DSP module
	[Library Test] Vector power sum takes 6659 cycles
	[Library Test] == Vector power sum test end with 1 ==

	[Library Test] == Vector power sum test  ==
	[Backend] CMSIS-DSP module
	[Library Test] Vector power sum takes 3681 cycles
	[Library Test] == Vector power sum test end ==

	[Library Test] == Fast Fourier Transform on Real Data test  ==
	[Backend] CMSIS-DSP module
	[Library Test] Fast Fourier Transform on Real Data takes 67956 cycles
	[Library Test] == Fast Fourier Transform on Real Data test end ==

	[Library Test] == Bi-quad Real Block IIR test  ==
	[Backend] CMSIS-DSP module
	[Library Test] Bi-quad Real Block IIR takes 506702 cycles
	[Library Test] == Bi-quad Real Block IIR end ==

	[Library Test] == Least Mean Square (LMS) Filter for Real Data test  ==
	[Backend] CMSIS-DSP module
	[Library Test] Least Mean Square (LMS) Filter for Real Data test takes 184792 cycles
	[Library Test] == Least Mean Square (LMS) Filter for Real Data test end ==

For NatureDSP, the output looks like this:

.. code-block:: console

	*** Booting Zephyr OS build v3.7.0-2815-g9018e424d7a1 ***

	Proprietary library example!

	[Library Test] == Vector Sum test  ==
	[Backend] NatureDSP library
	[Library Test] Vector Sum takes 3829 cycles
	[Library Test] == Vector Sum test end with 1 ==

	[Library Test] == Vector power sum test  ==
	[Backend] NatureDSP library
	[Library Test] Vector power sum takes 2432 cycles
	[Library Test] == Vector power sum test end with 1 ==

	[Library Test] == Vector power sum test  ==
	[Backend] NatureDSP library
	[Library Test] Vector power sum takes 2594 cycles
	[Library Test] == Vector power sum test end ==

	[Library Test] == Fast Fourier Transform on Real Data test  ==
	[Backend] NatureDSP library
	[Library Test] Fast Fourier Transform on Real Data takes 3338 cycles
	[Library Test] == Fast Fourier Transform on Real Data test end ==

	[Library Test] == Bi-quad Real Block IIR test  ==
	[Backend] NatureDSP library
	[Library Test] Bi-quad Real Block IIR takes 13501 cycles
	[Library Test] == Bi-quad Real Block IIR end ==

	[Library Test] == Least Mean Square (LMS) Filter for Real Data test  ==
	[Backend] NatureDSP library
	[Backend] NatureDSP library
	[Library Test] Least Mean Square (LMS) Filter for Real Data test takes 7724 cycles
	[Library Test] == Least Mean Square (LMS) Filter for Real Data test end ==
