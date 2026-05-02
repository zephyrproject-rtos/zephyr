.. _boards-silabs:

Silicon Labs
############

.. toctree::
   :maxdepth: 2
   :titlesonly:
   :glob:

   */*

Silicon Labs development hardware is represented in Zephyr by mapping
Silicon Labs *kits* to Zephyr *boards*. The name used is the orderable product
number (OPN) of the kit, as found on the packaging and on the Silicon Labs
website. The board name in Zephyr is created by normalizing the OPN to lowercase
and replacing dashes with underscores.

You may find multiple other number and letter sequences silk-screened or lasered
onto Silicon Labs boards, including a PCB* number and a BRD* number. In most
cases, the digits of these sequences correspond to the numerical part of the kit
OPN. For instance, the kit ``xg24_dk2601b``, which is a Dev Kit for the
EFR32xG24 SoC, uses board BRD2601B, which again uses PCB2601A. It is possible to
use the ``west boards`` command to search for board names if you have a PCB and
you don't know what board name to use:

  .. code-block:: console

   west boards -n 2601
