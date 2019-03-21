.. _flash_map_flash_area:

Flash map (flash_map)
##########################

Flash map is a way for storing flash partitioning information in one central
location in flash_area structures array form.

Flash map is generated from DTS based on content of :ref:`flash_partitions`
nodes.
The flash_area API provides a way to access data in the flash map.
The flash_area_open() API is the interface for obtaining the flash partitions
flash_area from the flash map.


Flash Area API (flash_area)
###########################

The flash_area concept combines methods for operating on a flash chunk
together with a description of this chunk. Its methods are basically wrappers
around the flash API, with input parameter range checks. Not all flash
operation are wrapped so an API call to retrieve the flash area driver is
included as well. The flash area methods are designed to be used along with
the flash_area structures of flash_map and user-specific flash_areas, with
the exception of the area_open API used to fetch a flash_area from
the flash_map.


API Reference
*************

flash_area API
==============

.. doxygengroup:: flash_area_api
   :project: Zephyr

