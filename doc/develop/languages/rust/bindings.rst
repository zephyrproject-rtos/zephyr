.. _rust_bindings:

Bindings to Zephyr for Rust
###########################

Zephyr is written in C, and its primary API is also made available to C.  It is written as a
mixture of types, function prototypes, inline functions, and macros.

Although Rust interfaces fairly easily with C at an ABI level, the compiler does not have any
support for generating the code necessary for these bindings. In order to call C code, the types
must be reproduced in Rust, function prototypes written in Rust, and often wrappers made for inline
functions and macros (or this functionality replicated in Rust).

This is a tedious, and error-prone process, and the end result would quickly get out of date with
the development of Zephyr.

The `bindgen`_ project seeks to address much of this issue by automatically generating bindings to C
code for Rust.  It makes use of ``libclang`` to parse and interpret a given set of headers, and
generate the Rust code needed to call this code.

.. _bindgen: https://github.com/rust-lang/rust-bindgen

Because the Zephyr headers use numerous conditional compilation macros, the bindings needed will be
very specialized to a given board, and even a given configuration.  To do this, the file
:file:`lib/rust/zephyr-sys/build.rs`, which ``cargo`` knows to run at build time, will
generate the bindings for the particular build of Zephyr being used by the current Rust application.

These bindings will be made available in the ``zephyr-sys`` crate, as well as under ``zephyr::raw``.

Using the Bindings
******************

In general, using direct bindings to C functions from Rust is a bit more difficult than calling them
from C.  Although all of these calls are considered "unsafe", and must be placed in an ``unsafe``
block, the Rust language has stricter constraints on what is allowed, even by unsafe code.  The
intent of supporting the Rust Language on Zephyr project is to allow full use of Zephyr, without
needing to resort to unsafe code, it is understandable that this implementation will be incomplete,
and some applications may require resorting to, what is sometimes referred to as "The Dark Arts".
The `Rustinomicon`_ is a good resource for those wishing to delve into this area.

.. _Rustinomicon: https://doc.rust-lang.org/nomicon/

In addition, the code in :file:`zephyr/lib/rust/zephyr` that provides the higher level abstractions
will primarily be implemented using these auto-generated bindings.

Generation
**********

The generation of the bindings is controlled by the ``build.rs`` program.  This attempts to capture
the symbols that are needed for successful bindings, but there may be things missing.  Additional
bindings can be included by adding their patterns to the list of patterns in ``build.rs``, and
possibly adding additional ``#include`` directives to :file:`lib/rust/zephyr-sys/wrapper.h`.
