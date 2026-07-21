.. _devicetree_style:

Devicetree Style Guidelines
###########################

  * Indent with tabs.
  * Tab size is 8 characters.
  * Follow the Devicetree specification conventions and rules.
  * If the Linux kernel rules in
    `Devicetree Sources (DTS) Coding Style <https://docs.kernel.org/devicetree/bindings/dts-coding-style.html>`_
    make a recommendation, it's the preferred style in Zephyr too.
  * You can split related groups of properties into "paragraphs" by
    separating them with one empty line (two newline characters) if it aids
    readability.
  * Use dashes (``-``) as word separators for node and property names.
  * Use underscores (``_``) as word separators in node labels.
  * Leave a single space on each side of the equal sign (``=``) in property
    definitions.
  * Don't insert empty lines before a dedenting ``};``.
  * Insert a single empty line to separate nodes at the same hierarchy level.
  * When splitting long property values across multiple lines, specify the first
    value on the same line as the opening bracket (``<`` or ``[``). Place the
    closing bracket and semicolon (``>;`` or ``];``) on the same line after
    the final value of the property.

Examples:

.. literalinclude:: style-example.dts
  :language: devicetree
  :start-after: start-after-here
