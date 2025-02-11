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
    gnss reported 2 satellites!
