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

Use the interactive search form below to quickly navigate through the list of
supported boards.

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
