.. _boards:

Supported Boards and Shields
############################

This page lists all the boards and shields that are currently supported in Zephyr.

If you are looking to add Zephyr support for a new board, please start with the
:ref:`board_porting_guide`. When adding support documentation for a board, remember to use the
template available under :zephyr_file:`doc/templates/board.tmpl`.

Shields are hardware add-ons that can be stacked on top of a board to add extra functionality.
Refer to the :ref:`shield_porting_guide` for more information on how to port a shield.

.. admonition:: Search Tips
   :class: dropdown

   * Use the form below to filter the list of supported boards and shields. If a field is left
     empty, it will not be used in the filtering process.

   * Filtering by name and vendor is available for both boards and shields. The rest of the fields
     apply only to boards.

   * A board/shield must meet **all** criteria selected across different fields. For example, if you
     select both a vendor and an architecture, only boards that match both will be displayed. Within
     a single field, selecting multiple options (such as two architectures) will show boards
     matching **either** option.

   * The list of supported hardware features for each board is automatically generated using
     information from the Devicetree. It may not be reflecting the full list of supported features
     since some of them may not be enabled by default.

   * Can't find your exact board? Don't worry! If a similar board with the same or a closely related
     MCU exists, you can use it as a :ref:`starting point <create-your-board-directory>` for adding
     support for your own board.

.. toctree::
   :maxdepth: 2
   :glob:
   :hidden:

   */index

.. zephyr:board-catalog::
