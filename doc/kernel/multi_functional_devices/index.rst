.. _multi_functional_devices:

Multi-functional devices
########################

Introduction
************

Multi-functional devices (MFDs) are devices which require two or more distinct
interfaces to be completely utilized.

The :ref:`device_model_api` presents only one interface per device,
consequenctly, MFDs must consist of multiple devices. The following segments
describe the design guidelines for creating MFD drivers, bindings and devicetree
layouts.

MFD devicetree layout
*********************

An MFD consists of one parent device and one to multiple child devices. The
parent and child devices are individually accessible from the application,
and each provide a single interface to be used from it. In the device tree,
all child device nodes are subnodes of the single parent device node.

.. code-block:: devicetree

   bus {
       parent {
          child1 {
          }; 
          child2 {
          };
          child3 {
          };
       };
   };

MFD bindings
************

In the parent device's bindings file, the ``bus`` property is assigned the
parent's ``compatible`` property value.

.. code-block:: yaml

   # Parent bindings file

   compatible: "foo-company,parent"

   bus: foo-company,parent

In the child devices bindings files, the ``on-bus`` property is assigned the
parent's ``compatible`` property value.

.. code-block:: yaml

   # Child bindings file

   compatible: "foo-company,parent-child"

   on-bus: foo-company,parent

The childs ``compatible`` properties are assigned the parent's ``compatible``
property value, dash, the childs name.

.. note::

   Properties specific to a child device must be placed in the child's bindings
   file.

MFD devicetree layout in bindings file
**************************************

The MFDs devicetree layout must be presented in a commented out section at the 
end of the parent device's bindings file. The layout must display at minimum all
required subnodes and properties with example values to be used as reference.

.. code-block:: yaml

   # Parent bindings file

   compatible: "foo-company,parent"

   properties:
     a:
       type: phandle
       required: true

   bus: foo-company,parent

   on-bus: bus

   # Example
   #
   # bus {
   #     parent: parent {
   #         compatible = "foo-company,parent";
   #         a = <100>;
   #
   #         child1: child1 {
   #             compatible = "foo-company,parent-child1";
   #             b = <200>;
   #         }; 
   #
   #         child2: child2 {
   #             compatible = "foo-company,parent-child2";
   #             c;
   #         };
   #
   #         child3: child3 {
   #             compatible = "foo-company,parent-child3";
   #         };
   #     };
   # };

MFD instances
*************

Each MFD has a single driver which supports multiple interfaces. This driver
first instanciates all child devices, followed by the parent device. All child
devices store a pointer to the parent's struct device in the :c:struct:`device`
data member.

.. note::

   Use the :c:macro:`MFD_DT_PARENT(node_id, compat)` and
   :c:macro:`MFD_DT_CHILD(node_id, child, compat)` macros to validate and get
   the parent and child nodes.

MFD API implementation
**********************

The parent and child API implementations only differ in how the parent
instance data is retrieved.

.. code-block:: c

   int parent_api_func1_impl(const struct device *dev, uint8_t value)
   {
      struct parent_data *data = (struct parent_data *)dev->data;
      ...
   }

   int child1_api_func1_impl(const struct device *dev, int8_t value)
   {
      /* Pointer to parent stored in child1's data pointer */
      const struct device *parent = (const struct device *)dev->data;
      struct parent_data *data = (struct parent_data *)parent->data;
      ...
   }

For MFDs that have multiple child devices implementing the same API,
an index can be stored in the child devices config struct.

.. code-block:: c

   int child_api_func1_impl(const struct device *dev, int8_t value)
   {
      /* Pointer to parent stored in child's data pointer */
      const struct device *parent = (const struct device *)dev->data;
      struct parent_data *data = (struct parent_data *)parent->data;

      /* Index stored in child cfg */
      const struct child_cfg *child_cfg = (const struct child_cfg *)dev->cfg;
      uint8_t child_index = child_cfg->index;
      ...
   }

MFD power management
********************

All devices which constitude the MFD are registered as pm devices. See
:ref:`pm-device-runtime-pm` and :ref:`pm-device-system-pm`. The application
interacts with each device independantly while the driver internally manages
potential dependencies between the devices during runtime.

.. doxygengroup:: device_model
