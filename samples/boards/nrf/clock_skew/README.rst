.. _nrf-clock-skew-sample:

nRF5x Clock Skew Demo
#####################

Overview
********

This sample uses the API for correlating time sources to measure the
skew between HFCLK (used for the CPU) and LFCLK (used for system time).

The ``CONFIG_APP_ENABLE_HFXO`` Kconfig option can be select to configure
the high frequency clock to use a crystal oscillator rather than the
default RC oscillator.  (Capabilities like Bluetooth that require an
accurate high-frequency clock generally enable this source
automatically.)  The relative error is significantly lower when HFXO is
enabled.

Requirements
************

This application uses any nRF51 DK or nRF52 DK board for the demo.

Building, Flashing and Running
******************************

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nrf/clock_skew
   :board: nrf52dk_nrf52840
   :goals: build flash
   :compact:

Running:


Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS build zephyr-v2.4.0-693-g4a3275faf567  ***
   Power-up clocks: LFCLK[ON]: Running LFXO ; HFCLK[OFF]: Running HFINT
   Start TIMER_0: 0
   Timer-running clocks: LFCLK[ON]: Running LFXO ; HFCLK[OFF]: Running HFINT
   Checking TIMER_0 at 16000000 Hz against ticks at 32768 Hz
   Timer wraps every 268 s

   Ty  Latest           Base             Span             Err
   HF  00:00:00.015667
   LF  00:00:00.404296
   Started sync: 0

   Ty  Latest           Base             Span             Err
   HF  00:00:10.001151  00:00:00.015667  00:00:09.985483
   LF  00:00:10.413818  00:00:00.404296  00:00:10.009521  00:00:00.024038
   Skew 0.997599 ; err 2401411 ppb

   Ty  Latest           Base             Span             Err
   HF  00:00:19.997456  00:00:00.015667  00:00:19.981788
   LF  00:00:20.434265  00:00:00.404296  00:00:20.029968  00:00:00.048180
   Skew 0.997595 ; err 2405464 ppb

   Ty  Latest           Base             Span             Err
   HF  00:00:29.993845  00:00:00.015667  00:00:29.978178
   LF  00:00:30.454650  00:00:00.404296  00:00:30.050354  00:00:00.072176
   Skew 0.997598 ; err 2401828 ppb

   Ty  Latest           Base             Span             Err
   HF  00:00:39.986181  00:00:00.015667  00:00:39.970514
   LF  00:00:40.475036  00:00:00.404296  00:00:40.070739  00:00:00.100225
   Skew 0.997499 ; err 2501189 ppb

   Ty  Latest           Base             Span             Err
   HF  00:00:49.981516  00:00:00.015667  00:00:49.965848
   LF  00:00:50.495422  00:00:00.404296  00:00:50.091125  00:00:00.125277
   Skew 0.997499 ; err 2501010 ppb

   Ty  Latest           Base             Span             Err
   HF  00:00:59.976042  00:00:00.015667  00:00:59.960375
   LF  00:01:00.515808  00:00:00.404296  00:01:00.111511  00:00:00.151136
   Skew 0.997486 ; err 2514243 ppb
   ...
   Ty  Latest           Base             Span             Err
   HF  00:01:59.935661  00:00:00.015667  00:01:59.919994
   LF  00:02:00.638153  00:00:00.404296  00:02:00.233856  00:00:00.313862
   Skew 0.997390 ; err 2610445 ppb
   ...
   Ty  Latest           Base             Span             Err
   HF  00:04:59.769166  00:00:00.015667  00:04:59.753498
   LF  00:05:01.005279  00:00:00.404296  00:05:00.600982  00:00:00.847484
   Skew 0.997181 ; err 2819240 ppb
   ...
   Ty  Latest           Base             Span             Err
   HF  00:09:59.513787  00:00:00.015667  00:09:59.498119
   LF  00:10:01.617156  00:00:00.404296  00:10:01.212860  00:00:01.714741
   Skew 0.997148 ; err 2852201 ppb
   ...
   Ty  Latest           Base             Span             Err
   HF  00:30:08.384536  00:00:00.015667  00:30:08.368868
   LF  00:30:14.084594  00:00:00.404296  00:30:13.680297  00:00:05.311429
   Skew 0.997072 ; err 2928495 ppb
   ...
   Ty  Latest           Base             Span             Err
   HF  00:59:57.353602  00:00:00.015667  00:59:57.337934
   LF  01:00:07.734375  00:00:00.404296  01:00:07.330078  00:00:09.992144
   Skew 0.997230 ; err 2770006 ppb
   ...
   Ty  Latest           Base             Span             Err
   HF  02:59:33.181323  00:00:00.015667  02:59:33.165656
   LF  03:00:03.434265  00:00:00.404296  03:00:03.029968  00:00:29.864312
   Skew 0.997236 ; err 2764463 ppb
   ...
   Ty  Latest           Base             Span             Err
   HF  05:59:55.031709  00:00:00.015667  05:59:55.016042
   LF  06:00:57.120941  00:00:00.404296  06:00:56.716644  00:01:01.700602
   Skew 0.997151 ; err 2849042 ppb
