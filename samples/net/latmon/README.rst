Latency Monitoring with Latmon and Latmus
==========================================

This project provides tools to measure the worst-case response time of a system under test (SUT) to GPIO events using:

- **Latmon**: Runs on a Zephyr-based board to generate and monitor GPIO events while collecting metrics
- **Latmus**: Runs on the SUT to respond to the falling edge of input GPIO event and display metrics.

This project is part of the open-source initiative `EVL Project - Latmus GPIO Response Time <https://evlproject.org/core/benchmarks/#latmus-gpio-response-time>`_.

Why Not Just Use a Timer?
-------------------------
A timer-based test only measures internal response times and often neglects external factors such as GPIO signal propagation, hardware interaction, and interrupt handling.
By using Latmon and Latmus, you can simulate real-world hardware-driven scenarios, capturing comprehensive end-to-end latency metrics, including hardware, drivers, and user-space threads.
This provides a more accurate assessment of the system's real-time responsiveness.

- **Real-Time Thread Testing**: Evaluates how a user-space thread processes external interrupts.
- **End-to-End Latency Measurement**: Captures delays from hardware, drivers, and user-space threads.
- **Versatile Platform Support**: Works on EVL, PREEMPT_RT, and other platforms.

Overview
--------
The main program is designed to monitor latency using GPIO pins on a Zephyr-based system.
It generates a pulse signal on a GPIO pin and measures the time it takes for the SUT (executing Latmus) to respond to the signal.

The SUT must be running Latmus to capture the latency metrics and historgram information reported over the network .
The program uses LEDS to indicate the different states, such as DHCP binding(red), waiting for the Latmus connection (blue) and sampling (green).

Code Structure
--------------

The Latmon sample application is divided into two main components:

1. **Application Logic (`samples/net/latmon/main.c`)**:
   This file contains the main application logic for Latmon. It initializes the hardware, the networking interface, configures the GPIO pins, and manages the state machine for the latency monitoring process.
   It also handles LED indicators for different states (e.g., DHCP binding, waiting for connection, sampling).

2. **Library (`subsys/net/lib/latmon/latmon.c`)**:
   This file provides reusable functions and abstractions for latency monitoring via Latmus.
   It includes the core logic for reporting latency metrics and histogram data.

Requirements
------------

- **Zephyr-Compatible Board**: A board with external GPIO support (e.g., FRDM-K64F).
- **SUT**: A system with external GPIO pins running Latmus.
- **Network Connection**: A DHCP server for IP assignment.
- **Physical Connection**: GPIO wires connecting the Zephyr board to the SUT.

Setup and Usage
---------------

1. Flash Latmon onto the Zephyr board.
2. Connect GPIO pins for TX (Zephyr to SUT) and RX (SUT to Zephyr).
3. Run Latmus on the SUT with appropriate options (e.g., ``-p`` for period, ``-g`` for histogram generation).
4. Monitor results such as min, max, and average latencies.

Example
-------

Run Latmon and Latmus as follows:

   On the host and to flash the Zephyr board::

	$ west flash

   On the SUT, latmus **MUST** track the falling edge of the signal::

	$ latmus -I gpiochip2,23,falling-edge -O gpiochip2,21 -z <ip_address> -g "histogram"

On completion, generate a latency histogram on the host (a PNG file) using gnuplot::

   $ gnuplot plot_data.gp

The `plot_data.gp` script should look like this:

.. code-block:: gnuplot

   set terminal pngcairo size 800,600
   set output 'plot.png'
   set title "Data Plot"
   set xlabel "Latency (usec)"
   set ylabel "Sample Count"
   set grid
   set style data linespoints
   plot 'histogram' using 1:2 with linespoints title "Data Points"

Contributions
-------------

Contributions are welcome. Please submit a pull request or open an issue.
