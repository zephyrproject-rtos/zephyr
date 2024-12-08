.. _boards:

Supported Boards and Shields
############################

If you are looking to add Zephyr support for a new board, please start with the
:ref:`board_porting_guide`.

When adding support documentation for a board, remember to use the template
available under :zephyr_file:`doc/templates/board.tmpl`.

Shields are hardware add-ons that can be stacked on top of a board to add extra
functionality. They are listed separately from boards, towards :ref:`the end of
this page <boards-shields>`.

.. admonition:: Search Tips
   :class: dropdown

   * Use the form below to filter the list of supported boards. If a field is left empty, it will
     not be used in the filtering process.

   * A board must meet **all** criteria selected across different fields. For example, if you select
     both a vendor and an architecture, only boards that match both will be displayed. Within a
     single field, selecting multiple options (such as two architectures) will show boards matching
     **either** option.

   * Can't find your exact board? Don't worry! If a similar board with the same or a closely related
     MCU exists, you can use it as a :ref:`starting point <create-your-board-directory>` for adding
     support for your own board.

.. toctree::
   :maxdepth: 2
   :glob:
   :hidden:

   */index

.. zephyr:board-catalog::

.. _boards-shields:

Shields
#######

.. toctree::
   :maxdepth: 1
   :glob:

   shields/**/*
