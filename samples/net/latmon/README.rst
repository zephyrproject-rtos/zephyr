.. zephyr:code-sample:: latmon-client
   :name: Latmon Client
   :relevant-api: latmon

   Measures delta time between GPIO events and reports the latency metrics via latmon to the latmus
   service executing on the SUT.

Overview
********

This project provides tools to measure the worst-case response time of a system under test (SUT) to
GPIO events using:

- **Latmon**:

  Runs on a Zephyr-based board to generate and monitor GPIO events while collecting metrics

- **Latmus**:

  Runs on the SUT to respond to the falling edge of input GPIO event, displaying the latency metrics
  and generate histogram data.

This project is part of the open-source initiative
`EVL Project - Latmus GPIO Response Time <https://evlproject.org/core/benchmarks/#latmus-gpio-response-time>`_.

The main program is designed to monitor latency using GPIO pins on a Zephyr-based system. It generates a
pulse signal on a GPIO pin and measures the time it takes for the SUT (executing Latmus) to respond to
it.

The SUT must be running Latmus to capture the latency metrics and histogram information reported over the
network. The program uses LEDs to indicate the different states, such as DHCP binding(red), waiting for the
Latmus connection (blue) and sampling (green).

Why Not Just Use a Timer?
=========================

Timer tests miss external factors like GPIO signals, hardware, and interrupt handling.
Latmon and Latmus simulate real-world scenarios, capturing end-to-end latency metrics.
This ensures accurate assessment of real-time responsiveness across the entire system.

- **Real-Time Thread Testing**:

  Evaluates how a user-space thread processes external interrupts.

- **End-to-End Latency Measurement**:

  Captures delays from hardware, drivers, and user-space threads.

- **Versatile Platform Support**:

  Works on EVL, PREEMPT_RT, and other platforms.

Code Structure
==============

The Latmon sample application is divided into two main components:

- **Application Logic** (:zephyr_file:`samples/net/latmon/src/main.c`):

   This file contains the application logic for Latmon.
   It initializes networking, provides the instrumentation mechanism and handles LED indicators for the
   different states.

- **Library** (:zephyr_file:`subsys/net/lib/latmon/latmon.c`):

   This file provides reusable functions and abstractions for latency monitoring via Latmus.
   It includes the core logic for reporting latency metrics and histogram data.

Requirements
************

- **Zephyr-Compatible Board**:

  A board with external GPIO support and an IPv4 network interface (e.g., FRDM-K64F).

- **System under Test**:

  A system with external GPIO pins running the Latmus service and an IPv4 network interface.

- **Network Connection**:

  A DHCP server for IP assignment.

- **Physical Connection**:

  GPIO wires connecting the Zephyr board to the SUT and both systems connected to the network.

Setup and Usage
***************

- **Flash Latmon onto the Zephyr board**:

  The application will connect to the network and wait for a connection from the SUT. The application
  will use DHCP to obtain an IPv4 address.

- **Connect GPIO pins for transmit (Zephyr to SUT) and receive (SUT to Zephyr)**

  On **FRDM-K64F**, the sample code uses the **Arduino header J2**, ``pin 20`` for transmit the pulse to
  the SUT and ``pin 18`` to receive the acknowledgment from the SUT.

- **Run Latmus on the SUT**

  Request the appropriate options with `Latmus <https://evlproject.org/core/testing/#latmus-program>`_. Users
  can for example modify the sampling period with the ``-p`` option or generate histogram data for
  postprocessing with the ``-g`` option,

- **Monitor results from the SUT**

  Latmus will report latency figures and, if requested, generate the histogram data file.

- **Calibrating the Latmus latencies: CONFIG_LATMON_LOOPBACK_CALIBRATION**:

  Users can connect the GPIO pins in loopback mode (transmit to ack) and build the Latmon sample application with
  CONFIG_LATMON_LOOPBACK_CALIBRATION enabled. When connecting to Latmus in this configuration, Latmus is providing
  a calibration value that can be used to adjust the final latencies.

Example
=======

On the host and to build and flash the Zephyr FRDM-K64F board with the Latmon sample:

.. code-block:: console

   user@host:~$ west build -b frdm_k64f samples/net/latmon
   user@host:~$ west flash

On the SUT running on Linux, latmus **MUST** track the falling edge of the signal:

.. code-block:: console

   root@target:~$ latmus -I gpiochip2,23,falling-edge -O gpiochip2,21 -z -g"histogram" "broadcast"

Monitoring both consoles, you should see the following:

.. code-block:: console

  [00:00:03.311,000] <inf> phy_mc_ksz8081: PHY 0 is up
  [00:00:03.311,000] <inf> phy_mc_ksz8081: PHY (0) Link speed 100 Mb, full duplex
  [00:00:03.312,000] <inf> eth_nxp_enet_mac: Link is up
  *** Booting Zephyr OS build v4.1.0-3337-g886443a190b1 ***
  [00:00:03.313,000] <inf> sample_latmon: DHCPv4: binding...
  [00:00:03.313,000] <inf> latmon: Latmon server thread priority: 14
  [00:00:10.964,000] <inf> net_dhcpv4: Received: 192.168.1.58
  [00:00:10.964,000] <inf> sample_latmon: Listening on 192.168.1.58
  [00:00:30.966,000] <inf> latmon: Waiting for Latmus ...
  [00:00:31.356,000] <inf> latmon: Monitor thread priority: -16
  [00:00:31.356,000] <inf> latmon:        monitoring started:
  [00:00:31.356,000] <inf> latmon:         - samples per period: 1000
  [00:00:31.356,000] <inf> latmon:         - period: 1000 usecs
  [00:00:31.356,000] <inf> latmon:         - histogram cells: 200
  [00:00:31.393,000] <inf> latmon: Transfer thread priority: 14

.. code-block:: console

  root@target:~$ latmus -I gpiochip2,23,falling-edge -O gpiochip2,21 -Z -g"histogram" broadcast
  Received broadcast message: 192.168.1.58
  warming up on CPU0 (not isolated)...
  connecting to latmon at 192.168.1.58:2306...
  RTT|  00:00:16  (oob-gpio, 1000 us period, priority 98, CPU0-noisol)
  RTH|----lat min|----lat avg|----lat max|-overrun|---msw|---lat best|--lat worst
  RTD|     26.375|     30.839|     33.508|       0|     0|     26.375|     33.508
  RTD|     26.333|     30.801|     37.633|       0|     0|     26.333|     37.633
  RTD|     26.375|     30.801|     31.966|       0|     0|     26.333|     37.633
  RTD|     26.375|     30.911|     49.675|       0|     0|     26.333|     49.675
  RTD|     26.333|     30.830|     41.658|       0|     0|     26.333|     49.675
  RTD|     26.375|     31.107|     59.216|       0|     0|     26.333|     59.216
  RTD|     26.333|     30.767|     30.925|       0|     0|     26.333|     59.216
  RTD|     26.333|     30.781|     41.616|       0|     0|     26.333|     59.216
  RTD|     26.375|     30.768|     32.925|       0|     0|     26.333|     59.216
  RTD|     26.375|     30.768|     37.633|       0|     0|     26.333|     59.216

On completion and from your host, retrieve the histogram file from the SUT, and generate a plot (a PNG file) using
gnuplot:

.. code-block:: console

   user@host:~$ gnuplot plot_data.gp

The ``plot_data.gp`` script should look like this for a file named ``histogram``:

.. code-block:: gnuplot

   set terminal pngcairo size 800,600
   set output 'plot.png'
   set title "Data Plot"
   set xlabel "Latency (usec)"
   set ylabel "Sample Count"
   set grid
   set style data linespoints
   plot 'histogram' using 1:2 with linespoints title "Data Points"
