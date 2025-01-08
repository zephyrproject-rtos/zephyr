.. _language_rust:

Rust Language Support
#####################

Rust is a modern systems programming language focused on safety, speed, and concurrency. Unlike
traditional systems programming languages such as C and C++, Rust ensures memory safety without
relying on garbage colleciton, thanks to its ownership model and borrow checker. Rust also offers a
rich type system, expressive syntax, and support for zero-cost abstractions, making it well-suited
for embedded development.

Rust’s emphasis on reliability and performance aligns well with Zephyr. By combining Rust’s
guarantees with Zephyr’s robust ecosystem, developers can create efficient and maintainable embedded
applications. The integration of Rust with Zephyr allows developers to leverage Rust’s language
features while benefiting from Zephyr’s drivers, APIs, and multi-threading capabilities.

At this point, Rust support in Zephyr is entirely for those wishing to write applications in Rust
that run *on* Zephyr.  Any efforts to add code *to* Zephyr, written in Rust (such as device drivers)
would be an independent effort.

Enabling Rust Support
*********************

Both Rust and Zephyr have their own build systems, along with an ecosystem around this.  For Rust,
this is the "crate" system of managing external dependencies.  Because this support is for
applications written in Rust, Zephyr's rust support leverages crates, and the cargo tool to build
the Rust part of the application.

Enabling the module
-------------------

Before getting very far, it is important to make sure that the Rust support module is included.  By
default, rust support is listed in the project manifest, but is marked as optional.  It is easy to
enable the module using ``west``:

.. code-block:: console

   west config manifest.project-filter +zephyr-lang-rust
   west update

This should enable the module, and then sync modules, which will add the module to your modules
directory.

Setting up the environment
--------------------------

Before starting with Rust, make sure you are able to build a simple Zephyr application, such as
"blinky". It is easier to debug Zephyr build issues before introducing Rust.

After this, you will need to have a recent Rust toolchain install, as well as target support for
your desired target.  It is generally easiest to use `Rustup`_ to install an maintain a rust
installation.  After following these instructions, you will need to install target support for your
target.

Because Zephyr and LLVM use different naming conventions for targets, and can be a little
challenging to determine the right target to install.  Sometimes, it is easiest to proceed with
building a Rust sample with your desired target, and the toolchain will emit an error message giving
the appropriate command to run.  The target needed will depend both on the board/soc selected, as
well as certain configuration choices (such as whether floating point is enabled).  Please see the
function ``_rust_map_target`` in :module_file:`zephyr-lang-rust:CMakeLists.txt` for details on how
this is computed.  As an example, a particular Cortex-M target may need the following command:

.. code-block:: console

   rustup target add thumbv7m-none-eabi

.. _`Rustup`: https://rustup.rs/

Building a sample
-----------------

.. This file (a directory in samples) and the above CMakeLists.txt should be done with the
   module_file reference.  However, the current doc build doesn't seem to be including modules, so
   for now, just make these regular file references.

The directory :file:`zephyr-lang-rust:samples` contains some samples that can be useful for
testing.  Hello world is a good basic test, although blinky may also be useful if your target has an
LED defined.  Please make sure that :file:`samples/basic/blink` works with your board,
however, before trying it with Rust.

Following from the getting-started guide, you can build the hello world sample in rust with:

.. code-block: console

   cd ~/zephyrproject/modules/lang/rust
   west build -p always -b <your-board-name> samples/blinky

Provided this is successful, the image can be flashed and tested as per the getting started guide.
