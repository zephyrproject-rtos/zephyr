:orphan:

.. _zephyr_licensing:

Licensing of Zephyr Project components
######################################

Zephyr as a whole is licensed under the `Apache 2.0 License`_. It does, however,
import or reuse a small number of packages, scripts and other files that are
covered by other licenses. In some cases there is no way to add a license header
to those files, so their licensing is declared centrally, in a machine-readable
form, in the :zephyr_file:`REUSE.toml` file at the root of the repository
(following the `REUSE specification`_).

The sections below are **generated automatically** from that metadata, so they
always reflect the actual state of the tree. To add, update or remove an entry,
edit the corresponding ``[[annotations]]`` block in :zephyr_file:`REUSE.toml`
rather than this page (see :ref:`external-contributions`).

.. note::

   This page lists licensing *exceptions* only. It does **not** define the
   license of the Zephyr project itself, which is Apache 2.0 as specified in the
   :zephyr_file:`LICENSE` file at the root of the repository.

.. contents:: Documented components
   :local:
   :depth: 1

.. zephyr-licensing-exceptions::

.. _Apache 2.0 License:
   https://github.com/zephyrproject-rtos/zephyr/blob/main/LICENSE

.. _REUSE specification:
   https://reuse.software/spec/
