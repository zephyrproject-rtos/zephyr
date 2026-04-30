.. _nxp_ias_camera_to_rpi_camera_adapter:

NXP IAS Camera to RPi Camera Adapter
###################################

Overview
********

This shield describes an AP1302-based camera module connected through the
"IAS Camera to RPi Camera Adapter".

The shield is intentionally minimal and only adds the camera sensor device node
(AP1302) and its outgoing CSI-2 endpoint.

The full pipeline wiring (enabling the SoC MIPI CSI-2 receiver / ISI, choosing
which capture device to use via ``zephyr,camera``, reserved-memory for firmware,
and board-specific power supplies) should be provided by an application or
sample devicetree overlay.

Board requirements
******************

Boards using this shield must provide the following devicetree labels:

- ``nxp_cam_i2c``: I2C bus connected to the camera module.
- ``nxp_cam_connector``: GPIO nexus providing camera control pins (compatible
  ``nxp,ias-camera-to-rpi-camera-adapter``).
