.. _nrf-clock-skew-sample:

nRF5x Clock Skew Demo
#####################

Overview
********

This sample uses the API for correlating time sources to measure the
skew between HFCLK (used for the CPU) and LFCLK (used for system time).
It also shows how to use this skew to correct durations measured in the
LFCLK domain to durations in the HFCLK domain.

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
   :board: nrf52dk/nrf52832
   :goals: build flash
   :compact:

Running:


Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS build v2.5.0-rc3-94-g06a4b650467b  ***
   Power-up clocks: LFCLK[ON]: Running LFXO ; HFCLK[OFF]: Running HFINT
   Start TIMER_0: 0
   Timer-running clocks: LFCLK[ON]: Running LFXO ; HFCLK[OFF]: Running HFINT
   Checking TIMER_0 at 16000000 Hz against ticks at 32768 Hz
   Timer wraps every 268 s

   Ty  Latest           Base             Span             Err
   HF  00:00:00.015666
   LF  00:00:00.404937
   Started sync: 0

   Ty  Latest           Base             Span             Err
   HF  00:00:09.978436  00:00:00.015666  00:00:09.962769
   LF  00:00:10.414520  00:00:00.404937  00:00:10.009582  00:00:00.046813
   RHF 00:00:09.978435                                   -00:00:00.000000
   Skew 0.995323 ; err 4676818 ppb

   Ty  Latest           Base             Span             Err
   HF  00:00:19.959136  00:00:00.015666  00:00:19.943469
   LF  00:00:20.441589  00:00:00.404937  00:00:20.036651  00:00:00.093182
   RHF 00:00:19.959117                                   -00:00:00.000018
   Skew 0.995349 ; err 4650592 ppb

   Ty  Latest           Base             Span             Err
   HF  00:00:29.937181  00:00:00.015666  00:00:29.921514
   LF  00:00:30.468627  00:00:00.404937  00:00:30.063690  00:00:00.142176
   RHF 00:00:29.937175                                   -00:00:00.000005
   Skew 0.995271 ; err 4729151 ppb

   Ty  Latest           Base             Span             Err
   HF  00:00:39.917347  00:00:00.015666  00:00:39.901680
   LF  00:00:40.495666  00:00:00.404937  00:00:40.090728  00:00:00.189048
   RHF 00:00:39.917339                                   -00:00:00.000008
   Skew 0.995284 ; err 4715502 ppb

   Ty  Latest           Base             Span             Err
   HF  00:00:49.899493  00:00:00.015666  00:00:49.883826
   LF  00:00:50.522674  00:00:00.404937  00:00:50.117736  00:00:00.233910
   RHF 00:00:49.899486                                   -00:00:00.000007
   Skew 0.995333 ; err 4667222 ppb

   Ty  Latest           Base             Span             Err
   HF  00:00:59.878166  00:00:00.015666  00:00:59.862499
   LF  00:01:00.549713  00:00:00.404937  00:01:00.144775  00:00:00.282276
   RHF 00:00:59.878154                                   -00:00:00.000011
   Skew 0.995307 ; err 4693269 ppb
   ...
   Ty  Latest           Base             Span             Err
   HF  00:02:59.654855  00:00:00.015666  00:02:59.639188
   LF  00:03:00.873901  00:00:00.404937  00:03:00.468963  00:00:00.829775
   RHF 00:02:59.654857                                    00:00:00.000001
   Skew 0.995402 ; err 4597902 ppb
   ...
   Ty  Latest           Base             Span             Err
   HF  00:04:59.410593  00:00:00.015666  00:04:59.394926
   LF  00:05:01.198181  00:00:00.404937  00:05:00.793243  00:00:01.398317
   RHF 00:04:59.410594                                    00:00:00.000001
   Skew 0.995351 ; err 4648745 ppb
   ...
   Ty  Latest           Base             Span             Err
   HF  00:09:58.829511  00:00:00.015666  00:09:58.813845
   LF  00:10:02.008911  00:00:00.404937  00:10:01.603973  00:00:02.790128
   RHF 00:09:58.829509                                   -00:00:00.000002
   Skew 0.995362 ; err 4637837 ppb
   ...
   Ty  Latest           Base             Span             Err
   HF  00:29:56.607589  00:00:00.015666  00:29:56.591923
   LF  00:30:05.250732  00:00:00.404937  00:30:04.845794  00:00:08.253871
   RHF 00:29:56.607585                                   -00:00:00.000004
   Skew 0.995427 ; err 4573166 ppb
   ...
   Ty  Latest           Base             Span             Err
   HF  00:59:43.781443  00:00:00.015666  00:59:43.765776
   LF  01:00:00.085113  00:00:00.404937  00:59:59.680175  00:00:15.914399
   RHF 00:59:43.781535                                    00:00:00.000092
   Skew 0.995579 ; err 4421055 ppb
   ...
   Ty  Latest           Base             Span             Err
   HF  01:59:37.666395  00:00:00.015666  01:59:37.650728
   LF  02:00:09.810913  00:00:00.404937  02:00:09.405975  00:00:31.755247
   RHF 01:59:37.666057                                   -00:00:00.000338
   Skew 0.995595 ; err 4404723 ppb
   ...
   Ty  Latest           Base             Span             Err
   HF  05:58:33.905236  00:00:00.015666  05:58:33.889570
   LF  06:00:08.604980  00:00:00.404937  06:00:08.200042  00:01:34.310472
   RHF 05:58:33.904338                                   -00:00:00.000898
   ..
   Skew 0.995635 ; err 4364550 ppb
   Ty  Latest           Base             Span             Err
   HF  11:56:59.503922  00:00:00.015666  11:56:59.488255
   LF  12:00:06.793182  00:00:00.404937  12:00:06.388244  00:03:06.899989
   RHF 11:56:59.503948                                    00:00:00.000025
   Skew 0.995674 ; err 4325747 ppb
