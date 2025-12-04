.. _dt-writing-bindings:

Rules for upstream bindings
###########################

This section includes general rules for writing bindings that you want to
submit to the upstream Zephyr Project. (You don't need to follow these rules
for bindings you don't intend to contribute to the Zephyr Project, but it's a
good idea.)

Decisions made by the Zephyr devicetree maintainer override the contents of
this section. If that happens, though, please let them know so they can update
this page, or you can send a patch yourself.

.. contents:: Contents
   :local:

Always check for existing bindings
**********************************

Zephyr aims for devicetree :ref:`dt-source-compatibility`. Therefore, if there
is an existing binding for your device in an authoritative location, you should
try to replicate its properties when writing a Zephyr binding, and you must
justify any Zephyr-specific divergences.

In particular, this rule applies if:

- There is an existing binding in the mainline Linux kernel. See
  :file:`Documentation/devicetree/bindings` in `Linus's tree`_ for existing
  bindings and the `Linux devicetree documentation`_ for more information.

- Your hardware vendor provides an official binding outside of the Linux
  kernel.

.. _Linus's tree:
   https://github.com/torvalds/linux/

.. _Linux devicetree documentation:
   https://www.kernel.org/doc/html/latest/devicetree/index.html

General rules
*************

Wherever possible, when writing Devicetree bindings for Zephyr, try to follow
the same `design guidelines laid out by Linux`_.

.. _design guidelines laid out by Linux:
   https://docs.kernel.org/devicetree/bindings/writing-bindings.html

File names
==========

Bindings which match a compatible must have file names based on the compatible.

- For example, a binding for compatible ``vnd,foo`` must be named ``vnd,foo.yaml``.
- If the binding is bus-specific, you can append the bus to the file name;
  for example, if the binding YAML has ``on-bus: bar``, you may name the file
  ``vnd,foo-bar.yaml``.

Recommendations are requirements
================================

All recommendations in :ref:`dt-bindings-default` are requirements when
submitting the binding.

In particular, if you use the ``default:`` feature, you must justify the
value in the property's description.

Descriptions
============

There are only two acceptable ways to write property ``description:``
strings.

If your description is short, it's fine to use this style:

.. code-block:: yaml

   description: my short string

If your description is long or spans multiple lines, you must use this
style:

.. code-block:: yaml

   title: I'm sure you need a short title.

   description: |
     My very long string
     goes here.
     Look at all these lines!

This ``|`` style prevents YAML parsers from removing the newlines in
multi-line descriptions. This in turn makes these long strings
display properly in the :ref:`devicetree_binding_index`.

If using the binding’s properties gets complicated, you can use examples
to provide a minimal node. e.g.:

.. code-block:: yaml

   description: ...

   properties:
    ...

   examples:
     - |
       leds {
         compatible = "gpio-leds";

         uled: led {
         gpios = <&gpioe 12 GPIO_ACTIVE_HIGH>;
         };
       };

Naming conventions
==================

Do not use uppercase letters (``A`` through ``Z``) or underscores (``_``) in
property names. Use lowercase letters (``a`` through ``z``) instead of
uppercase. Use dashes (``-``) instead of underscores. (The one exception to
this rule is if you are replicating a well-established binding from somewhere
like Linux.)

Rules for vendor prefixes
*************************

The following general rules apply to vendor prefixes in :ref:`compatible
<dt-important-props>` properties.

- If your device is manufactured by a specific vendor, then its compatible
  should have a vendor prefix.

  If your binding describes hardware with a well known vendor from the list in
  :zephyr_file:`dts/bindings/vendor-prefixes.txt`, you must use that vendor
  prefix.

- If your device is not manufactured by a specific hardware vendor, do **not**
  invent a vendor prefix. Vendor prefixes are not mandatory parts of compatible
  properties, and compatibles should not include them unless they refer to an
  actual vendor. There are some exceptions to this rule, but the practice is
  strongly discouraged.

- Do not submit additions to Zephyr's :file:`dts/bindings/vendor-prefixes.txt`
  file unless you also include users of the new prefix. This means at least a
  binding and a devicetree using the vendor prefix, and should ideally include
  a device driver handling that compatible.

  For custom bindings, you can add a custom
  :file:`dts/bindings/vendor-prefixes.txt` file to any directory in your
  :ref:`DTS_ROOT <dts_root>`. The devicetree tooling will respect these
  prefixes, and will not generate warnings or errors if you use them in your
  own bindings or devicetrees.

- We sometimes synchronize Zephyr's vendor-prefixes.txt file with the Linux
  kernel's equivalent file; this process is exempt from the previous rule.

- If your binding is describing an abstract class of hardware with Zephyr
  specific drivers handling the nodes, it's usually best to use ``zephyr`` as
  the vendor prefix. See :ref:`dt_vendor_zephyr` for examples.

.. _dt-bindings-default-rules:

Rules for default values
************************

In any case where ``default:`` is used in a devicetree binding, the
``description:`` for that property **must** explain *why* the value was
selected and any conditions that would make it necessary to provide a different
value. Additionally, if changing one property would require changing another to
create a consistent configuration, then those properties should be made
required.

There is no need to document the default value itself; this is already present
in the :ref:`devicetree_binding_index` output.

There is a risk in using ``default:`` when the value in the binding may be
incorrect for a particular board or hardware configuration.  For example,
defaulting the capacity of the connected power cell in a charging IC binding
is likely to be incorrect.  For such properties it's better to make the
property ``required: true``, forcing the user to make an explicit choice.

Driver developers should use their best judgment as to whether a value can be
safely defaulted. Candidates for default values include:

- delays that would be different only under unusual conditions
  (such as intervening hardware)
- configuration for devices that have a standard initial configuration (such as
  a USB audio headset)
- defaults which match the vendor-specified power-on reset value
  (as long as they are independent from other properties)

Examples of how to write descriptions according to these rules:

.. code-block:: yaml

   properties:
     cs-interval:
       type: int
       default: 0
       description: |
         Minimum interval between chip select deassertion and assertion.
         The default corresponds to the reset value of the register field.
     hold-time-ms:
       type: int
       default: 20
       description: |
         Amount of time to hold the power enable GPIO asserted before
         initiating communication. The default was recommended in the
         manufacturer datasheet, and would only change under very
         cold temperatures.

Some examples of what **not** to do, and why:

.. code-block:: yaml

   properties:
     # Description doesn't mention anything about the default
     foo:
       type: int
       default: 1
       description: number of foos

     # Description mentions the default value instead of why it
     # was chosen
     bar:
       type: int
       default: 2
       description: bar size; default is 2

     # Explanation of the default value is in a comment instead
     # of the description. This won't be shown in the bindings index.
     baz:
       type: int
       # This is the recommended value chosen by the manufacturer.
       default: 2
       description: baz time in milliseconds

The ``zephyr,`` prefix
**********************

You must add this prefix to property names in the following cases:

- Zephyr-specific extensions to bindings we share with upstream Linux. One
  example is the ``zephyr,vref-mv`` ADC channel property which is common to ADC
  controllers defined in :zephyr_file:`dts/bindings/adc/adc-controller.yaml`.
  This channel binding is partially shared with an analogous Linux binding, and
  Zephyr-specific extensions are marked as such with the prefix.

- Configuration values that are specific to a Zephyr device driver. One example
  is the ``zephyr,lazy-load`` property in the :dtcompatible:`ti,bq274xx`
  binding. Though devicetree in general is a hardware description and
  configuration language, it is Zephyr's only mechanism for configuring driver
  behavior for an individual ``struct device``. Therefore, as a compromise,
  we do allow some software configuration in Zephyr's devicetree bindings, as
  long as they use this prefix to show that they are Zephyr specific.

You may use the ``zephyr,`` prefix when naming a devicetree compatible that is
specific to Zephyr. One example is
:dtcompatible:`zephyr,ipc-openamp-static-vrings`. In this case, it's permitted
but not required to add the ``zephyr,`` prefix to properties defined in the
binding.
