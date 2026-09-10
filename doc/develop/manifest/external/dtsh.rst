.. _external_module_dtsh:

DTSh
####

Introduction
************

`DTSh <DTSh-Handbook_>`_ is an interactive DTS file viewer with
a shell-like command line interface:

- easily *navigate* and *visualize* the devicetree
- find nodes based on e.g. supported bus protocols, bindings, generated IRQs,
  memory size, or keywords like "sensor" or "PWM"
- redirect commands output to files (text, HTML, SVG)
  to document hardware configurations or simply take notes
- contextual auto-completion, commands history, semantic highlighting, user profiles
- script-able (aka batch modes)

This Zephyr module adds DTSh as a West extension to your Zephyr workspace.


Usage with Zephyr
*****************

To install the DTSh module, you will need to define your own manifest file,
or pull it in by adding a submanifest.

E.g. assuming the same paths as in `Zephyr Getting Started Guide`_,
create ``zephyrproject/zephyr/submanifests/dtsh.yaml`` with the following content:

.. code-block:: yaml

   manifest:
     projects:
       - name: dtsh
         url: https://github.com/dottspina/dtsh.git
         revision: zephyr
         path: modules/tools/dtsh
         west-commands: scripts/west-commands.yml


Then update the workspace and install DTSh requirements:

.. code-block:: sh

   west update dtsh
   west packages -m dtsh pip --install

.. note::

   ``west update dtsh`` will fetch all tags from the `DTSh project <DTSh-project_>`_: ignore them,
   they don't point to versions of this Zephyr module, and are entirely irrelevant here.


The West command
*****************

Once installed, this project/module should provide the ``west dtsh`` command.

.. code-block:: console

   $ west build
   $ west dtsh
   dtsh (0.2.5-zephyr): Shell-like interface with Devicetree
   How to exit: q, or quit, or exit, or press Ctrl-D

   /
   > cd &flash_controller

   /soc/flash-controller@4001e000
   > find -E --also-known-as (image|storage).* --format NKd -T
                                Also Known As               Description
                                ───────────────────────────────────────────────────────────────────────────────────
   flash-controller@4001e000    flash_controller            Nordic NVMC (Non-Volatile Memory Controller)
   └── flash@0                  flash0                      Flash node
       └── partitions                                       This binding is used to describe fixed partitions…
           ├── partition@c000   image-0, slot0_partition    Each child node of the fixed-partitions node represents…
           ├── partition@82000  image-1, slot1_partition    Each child node of the fixed-partitions node represents…
           └── partition@f8000  storage, storage_partition  Each child node of the fixed-partitions node represents…


For the full West command synopsis, run ``west dtsh -h``.

.. note::

   It's recommended to install the module to its default location, ``modules/tools/dtsh``.
   Otherwise, be sure to set the ``ZEPHYR_BASE`` environment variable before running ``west dtsh``.


Reference
*********

- `DTSh project <DTSh-project_>`_
- `DTSh Handbook <DTSh-Handbook_>`_
- `dtsh module <dtsh-module_>`_


.. _DTSh-project: https://github.com/dottspina/dtsh

.. _DTSh-Handbook: https://dottspina.github.io/dtsh/handbook.html

.. _dtsh-module: https://github.com/dottspina/dtsh/tree/zephyr

.. _Zephyr Getting Started Guide: https://docs.zephyrproject.org/latest/develop/getting_started/
