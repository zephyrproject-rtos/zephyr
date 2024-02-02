.. _nrf53_sync_rtc_sample:

Synchronized RTC
################

Overview
********

Sample is showing how RTC clocks used for system clock on application and network
core are synchronized. The result of synchronization is an offset value on network
core which can be applied to the system tick for logging timestamping.

Sample is using IPM driver and IPC HAL to produce events which occur at same time on
both cores. Application core periodically reads current system tick and stores it in
the shared memory and triggers IPC task which results in the interrupt on the network
core. In the context of the IPC interrupt handler, network core is logging timestamp
from shared memory and local system tick updated by the offset. User can observe
that before synchronization procedure is completed, timestamps differ significantly
and once procedure is completed timestamps are synchronized. Network core timestamp
may be slightly behind (usually 1 tick) due to latency introduced by the
interrupt handling.

For simplicity and low latency sample is not using more sophisticated IPM protocols.

Building the application for nrf5340dk/nrf5340/cpuapp
*****************************************************

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nrf/nrf53_sync_rtc
   :board: nrf5340dk/nrf5340/cpuapp
   :goals: flash
   :flash-args: --hex-file build/nrf53_sync_rtc/zephyr/zephyr.hex
   :west-args: --sysbuild

Open a serial terminals (for example Minicom or PuTTY) and connect the board with the
following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

When you reset the development kit, the following messages (one for master and one for remote) will appear on the corresponding serial ports:

.. code-block:: console

   *** Booting Zephyr OS build v2.7.0-rc1-89-ge755863d66c9  ***
   [00:00:00.306,915] <inf> main: IPC send at 10056 ticks
   [00:00:00.356,903] <inf> main: IPC send at 11695 ticks
   [00:00:00.406,921] <inf> main: IPC send at 13334 ticks
   [00:00:00.456,939] <inf> main: IPC send at 14973 ticks
   [00:00:00.506,958] <inf> main: IPC send at 16612 ticks
   [00:00:00.556,976] <inf> main: IPC send at 18251 ticks
   [00:00:00.606,994] <inf> main: IPC send at 19890 ticks
   [00:00:00.657,012] <inf> main: IPC send at 21529 ticks
   [00:00:00.707,031] <inf> main: IPC send at 23168 ticks
   [00:00:00.757,049] <inf> main: IPC send at 24807 ticks


.. code-block:: console

   *** Booting Zephyr OS build v2.7.0-rc1-89-ge755863d66c9  ***
   [00:00:00.054,534] <inf> main: Local timestamp: 1787, application core timestamp: 10056
   [00:00:00.104,553] <inf> main: Local timestamp: 3426, application core timestamp: 11695
   [00:00:00.154,571] <inf> main: Local timestamp: 5065, application core timestamp: 13334
   [00:00:00.204,589] <inf> main: Local timestamp: 6704, application core timestamp: 14973
   [00:00:00.254,608] <inf> main: Local timestamp: 8343, application core timestamp: 16612
   [00:00:00.514,892] <inf> sync_rtc: Updated timestamp to synchronized RTC by 8270 ticks (252380us)
   [00:00:00.557,006] <inf> main: Local timestamp: 18252, application core timestamp: 18251
   [00:00:00.607,025] <inf> main: Local timestamp: 19891, application core timestamp: 19890
   [00:00:00.657,043] <inf> main: Local timestamp: 21530, application core timestamp: 21529
   [00:00:00.707,061] <inf> main: Local timestamp: 23169, application core timestamp: 23168
   [00:00:00.757,080] <inf> main: Local timestamp: 24807, application core timestamp: 24807

Observe that initially logging timestamps for the corresponding events on both cores
do not match. Same with local and remote timestamps reported on network core. After
RTC synchronization is completed they start to match.

.. _nrf53_sync_rtc_sample_build_bsim:

Building the application for the simulated nrf5340bsim
******************************************************

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nrf/nrf53_sync_rtc
   :host-os: unix
   :board: nrf5340bsim_nrf5340_cpuapp
   :goals: build
   :west-args: --sysbuild

Then you can execute your application using:

.. code-block:: console

   $ ./build/zephyr/zephyr.exe -nosim
   # Press Ctrl+C to exit

You can expect a similar output as in the real HW in the invoking console.
