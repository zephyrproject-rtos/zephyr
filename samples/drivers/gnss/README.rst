.. zephyr:code-sample:: gnss
   :name: GNSS
   :relevant-api: gnss_interface navigation

   Connect to a GNSS device to obtain time, navigation data, and satellite information.

Overview
********
This sample demonstrates how to use a GNSS device implementing the
GNSS device driver API.

Requirements
************

This sample requires a board with a GNSS device present and enabled
in the devicetree.

Sample Output
*************

.. code-block:: console

    gnss: gnss_info: {satellites_cnt: 14, hdop: 0.850, fix_status: GNSS_FIX, fix_quality: GNSS_SPS}
    gnss: navigation_data: {latitude: 57.162331699, longitude : 9.961104199, bearing 12.530, speed 0.25, altitude: 42.372}
    gnss: gnss_time: {hour: 16, minute: 17, millisecond 36000, month_day 3, month: 10, century_year: 23}
    gnss has fix!
    gnss: gnss_satellite: {prn: 1, snr: 30, elevation 71, azimuth 276, system: GLONASS, is_tracked: 1}
    gnss: gnss_satellite: {prn: 11, snr: 31, elevation 62, azimuth 221, system: GLONASS, is_tracked: 1}
    gnss reported 2 satellites (of which 2 tracked, of which 0 has RTK corrections)!

Real-Time Kinematics (RTK)
**************************

This sample may also be configured to enable Real-Time Kinematics (RTK) positioning for
enhanced accuracy, with the assistance of a local base station.

RTK Requirements
****************

This sample requires the following setup to work with RTK:

* A UBlox F9P GNSS module connected to your board to act as a rover
* A second UBlox F9P module connected to a PC to act as a base station

Base Station Setup
******************

To enable RTK functionality:

1. Connect the base station F9P module to your PC via USB.
2. Also, connect the rover's serial port (running the sample) to your PC via USB.
3. Note the serial port the rover is connected through (e.g., /dev/ttyUSB0)
4. Run the base station script:

.. code-block:: console

	python3 base_station/base_station_f9p.py --port /dev/ttyUSB0

The script configures the F9P module as a base station and streams RTCM3
correction data to the rover. The base station will perform a survey-in
process to determine its precise position before starting to transmit
corrections.
