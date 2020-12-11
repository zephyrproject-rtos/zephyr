.. _flash_api:

Flash
#####

Overview
********

**Flash offset concept**

Offsets used by the user API are expressed in relation to
the flash memory beginning address. This rule shall be applied to
all flash controller regular memory that layout is accessible via
API for retrieving the layout of pages (see option:`CONFIG_FLASH_PAGE_LAYOUT`).

An exception from the rule may be applied to a vendor-specific flash
dedicated-purpose region (such a region obviously can't be covered under
API for retrieving the layout of pages).



User API Reference
******************
.. doxygengroup:: flash_interface
   :project: Zephyr

Implementation interface API Reference
**************************************
.. doxygengroup:: flash_internal_interface
   :project: Zephyr
