.. _language_rust:

Rust Language Support
#####################

Rust is a modern systems programming language designed to provide memory safety, concurrency, and
performance without sacrificing low-level control. It achieves this through a unique ownership model
that eliminates common bugs like null pointer dereferencing and data races at compile time.

Rust's emphasis on safety and correctness makes it particularly well-suited for embedded systems and
environments where reliability is critical.
Additionally, Rust offers powerful abstractions without a runtime or garbage collector, allowing
developers to write both high-level code and low-level hardware interactions with confidence and
efficiency.

These attributes make Rust a strong choice for projects on Zephyr, where resource constraints and
system stability are paramount.

Enabling Rust Support
*********************

In order to enable Rust support in a Zephyr application, a few things need to be done:

1.  As Rust is currently an optional module, the module needs to be enabled.  The easiest way to do
    this is with west:

    .. code-block:: shell

       west config manifest.project-filter +zephyr-lang-rust
       west update

    This should cause the Rust language support to be placed in :samp:`modules/lang/rust` in your
    Zephyr workspace.

2.  Enable Rust support, via :kconfig:option:`CONFIG_RUST` in the :file:`prj.conf`. The easiest way
    to do this (as well as the CMake setup from the next step) is to start with one of the samples
    in :module_file:`modules/lang/rust/samples <zephyr-lang-rust:samples>`.

3.  Configure the application's :file:`CMakeLists.txt` file to support Rust.  Again this is easiest
    to copy from a sample, but this will look something like:

    .. code-block:: cmake

       cmake_minimum_required(VERSION 3.20.0)

       find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

       project(my_app)
       rust_cargo_application()

4.  Create a :file:`Cargo.toml` that describes how to build the Rust application. From the Hello
    World sample:

    .. code-block:: toml

       [package]
       # This must be rustapp for now.
       name = "rustapp"
       version = "0.1.0"
       edition = "2021"
       description = "The description of my app"
       license = "Apache-2.0 or MIT"

       [lib]
       crate-type = ["staticlib"]

       [dependencies]
       zephyr = "0.1.0"
       log = "0.4.22"

    The only required dependency is ``zephyr`` which provides the zephyr crate that is used to
    interface with Zephyr.

5.  Build as you would any other Zephyr application.  Only a few targets currently support Rust
    (these can be seen in the
    :module_file:`modules/lang/rust/etc/platforms.txt <zephyr-lang-rust:etc/platforms.txt>` file).

API Documentation
*****************

The `API Documentation`_ for the latest version in the module is kept on gh-pages.

.. _`API Documentation`:
   https://zephyrproject-rtos.github.io/zephyr-lang-rust/nostd/zephyr/index.html

This documentation is generated for a general target, with all features enabled.  Once you have an
application that is buildable, you can generate documentation specifically for your target:

.. code-block:: shell

   west build -t rustdoc

   ...

   Generated /my/path/app/zephyr/build/doc/rust/target/riscv32i-unknown-none-elf/doc/rustapp/index.html

The path printed at the end can be opened in a browser.  This top level docs will be for your
application itself.  Look for the 'zephyr' crate on the left side bar, and this will take you to the
docs for Zephyr.  This page will also generate local docs for any dependencies used by your
application, directly or indirectly.
