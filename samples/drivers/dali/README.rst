.. zephyr:code-sample:: dali
   :name: Digital Addressable Lighting Interface (DALI)
   :relevant-api: dali

   Blink LED controllers connected to a DALI bus.

Overview
********

This sample utilizes the :ref:`dali <dali_api>` driver API to blink DALI enabled LED drivers.

Building and Running
********************

The interface to the DALI bus is defined in the board's devicetree.

The board's devictree must have a ``dali0`` node that provides the
access to the DALI bus. See the predefined overlays in
:zephyr_file:`samples/drivers/dali/boards` for examples.

.. note:: For proper operation a DALI specific physcial interface is required.

Sample outout
=============

You should see DALI frames transferred every 2 seconds to the DALI bus.
The frames are alternating. One frame is a DALI OFF command broadcasted to
all control gears. The other frame is a DALI RECALL MAX command broadcasted
to all control gears. When a control gear is connected it will alternate
between no light output from the attached LED and maximum output of the LED.
