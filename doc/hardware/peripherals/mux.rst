.. _mux_api:

Multiplexer (MUX)
#################

Overview
********

The MUX subsystem provides a unified API for hardware signal
multiplexers, it lets consumer drivers route an input signal to an output
by referencing a MUX controller through standard devicetree ``mux-controls``
or ``mux-states`` phandle-array properties, without depending on any vendor-specific HAL.

The subsystem distinguishes two consumer-side patterns:

* ``mux-controls`` — the consumer references a controller and supplies the
  *state* (which input to route, or which output pattern to write) at
  runtime via :c:func:`mux_control_set`. Use this when the routing changes
  during normal operation.

* ``mux-states`` — the consumer references a controller *and* a fixed state
  in devicetree (the trailing cell of the specifier is the state value).
  The runtime call :c:func:`mux_state_apply` programs that fixed state in
  one line. Use this when the routing is decided at integration time and
  never changes after init.

Devicetree bindings
*******************

Controller side
===============

Every MUX controller binding includes ``mux-controller.yaml`` (in
``dts/bindings/mux/``) and declares two cell-count properties:

* ``#mux-control-cells`` — number of cells in a ``mux-controls`` specifier,
  used purely to address the desired control line within the controller.
* ``#mux-state-cells`` — number of cells in a ``mux-states`` specifier;
  equals ``#mux-control-cells + 1``. The trailing cell carries the state
  value and is extracted by the framework into ``mux_state::state`` at
  compile time.

Backends are free to name the addressing cells after their hardware
(``channel``, ``output``, ``index``, ``device``/``input``, ...) and to name
the trailing state cell after its hardware semantics
(``state``, ``input``, ``connection``, ``source``, ...).

Consumer side
=============

Consumers include ``mux-consumer.yaml`` and may set:

* ``mux-controls`` — list of phandle + addressing-cells tuples.
* ``mux-control-names`` — optional names matching ``mux-controls`` entries.
* ``mux-states`` — list of phandle + addressing-cells + trailing state
  cell tuples.
* ``mux-state-names`` — optional names matching ``mux-states`` entries.

Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_MUX`

API Reference
*************

.. doxygengroup:: mux_interface
