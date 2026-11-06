.. zephyr:code-sample:: zms-physical-delete
   :name: ZMS Physical Delete
   :relevant-api: zms_high_level_api

   Erase the data of all the versions of a ZMS entry from the storage.

Overview
********

:c:func:`zms_delete` only writes a delete marker: the data of the entry and of all its previous
versions stay in the storage until garbage collection erases their sector.
:c:func:`zms_delete_physical` also overwrites the data of every version of the entry, so it
cannot be recovered from the storage afterwards.

The sample:

#. Writes three versions of a secret and one unrelated entry.
#. Deletes the secret with :c:func:`zms_delete` and scans the whole partition to show that the
   data is still there.
#. Deletes the secret with :c:func:`zms_delete_physical` and scans the partition again to show
   that no copy is left, and that the unrelated entry is untouched.

On success the sample prints ``PASS: physical delete succeeded``.

Requirements
************

* A board with flash support or the ``native_sim`` target. The storage must accept programming an
  already written location without an erase (no ECC, no write-once words), for example RRAM or
  NOR flash without ECC.
* :kconfig:option:`CONFIG_ZMS_PHYSICAL_DELETE` enabled.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/kvss/zms/zms_physical_delete
   :host-os: unix
   :board: native_sim
   :goals: run
   :compact:

Sample Output
=============

.. code-block:: console

   3 versions written, 51 copies of the secret in flash
   After zms_delete(): 51 copies of the secret still in flash
   After zms_delete_physical(): 0 copies of the secret in flash
   PASS: physical delete succeeded

The count is the number of byte offsets at which the pattern matches, so it is larger than the number of stored versions.
