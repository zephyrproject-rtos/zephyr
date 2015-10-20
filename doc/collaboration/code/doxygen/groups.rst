.. _groups:

Doxygen Groups
##############

The Zephyr Kernel's in-code documentation implements Doxygen groups.
The comments regarding the various services are contained in their respective groups.
The services groups are then contained in one of two main groups: either microkernel or nanokernel.
Each group can contain other groups, functions, variables, enums, typedefs and defines.

.. note:: Compound entities such as: clases, files and namespaces can be put into multiple groups;
          however, variables, functions, typedefs and enums can only be member of one group.

Defining Groups
***************

There are different ways to define groups and members.
You must define a group using the directive :literal:`@defgroup <GroupLabel>`,
where :literal:`<GroupLabel>` is replaced with the name of the specific group.
This label serves as reference when adding or pulling information to that group.
Keep in mind that the label must be a single word.
The desired entities must be inside the comment block that uses the syntax :literal:`@{` &
:literal:`@}`


.. code-block:: C

      /** @defgroup <GroupLabel> Group Name
       *  Detailed Description of the Group
       *  @{
       */

       /* @brief Brief description of the entity*/
       <entity1>

       /* @brief Brief description of the entity*/
       <entity2>

      /** @} */ // End of GroupLabel

If you need to add more information that is not listed inside the group block,
you must use :literal:`@addtogroup <GroupLabel>`.


Example:

.. code-block:: C

      /** @addtogroup <GroupLabel>
       *  Optional: This line adds information to the GroupLabel Detailed Description of the Group
       *  @{
       */

       /* The entities you want to add must be inserted inside before the close block*/

      /** @} // End of addtogroup GroupLabel