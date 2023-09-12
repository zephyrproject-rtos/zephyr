.. _bluetooth_mesh_lcd_cli:

Large Composition Data Client
#############################

The Large Composition Data Client model is a foundation model defined by the Bluetooth mesh
specification. The model is optional, and is enabled through the
:kconfig:option:`CONFIG_BT_MESH_LARGE_COMP_DATA_CLI` option.

The Large Composition Data Client model was introduced in the Bluetooth Mesh Protocol Specification
version 1.1, and supports the functionality of reading pages of Composition Data that do not fit in
a Config Composition Data Status message and reading the metadata of the model instances on a node
that supports the :ref:`bluetooth_mesh_lcd_srv` model.

The Large Composition Data Client model communicates with a Large Composition Data Server model
using the device key of the node containing the target Large Composition Data Server model instance.

If present, the Large Composition Data Client model must only be instantiated on the primary
element.

API reference
*************

.. doxygengroup:: bt_mesh_large_comp_data_cli
   :project: Zephyr
   :members:
