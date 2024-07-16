.. _language_rust:

Rust Language Support
#####################

Rust is a systems programming language focused on safety, speed, and
concurrency. Designed to prevent common programming errors such as
null pointer dereferencing and buffer overflows, Rust emphasizes
memory safety without sacrificing performance. Its powerful type
system and ownership model ensure thread-safe programming, making it
an ideal choice for developing reliable and efficient low-level code.
Rust's expressive syntax and modern features make it a robust
alternative for developers working on embedded systems, operating
systems, and other performance-critical applications.

Enabling Rust Support
*********************

Currently, Zephyr supports applications written in Rust and C.  The
enable Rust support, you must select the :kconfig:option:`CONFIG_RUST`
in the application configuration file.

The rust toolchain is separate from the rest of the Zephyr SDK.   It
is recommended to use the `rustup`_ tool to install the rust
toolchain.  In addition to the base compiler, you will need to install
core libraries for the target(s) you wish to compile on.  It is
easiest to determine what needs to be installed by trying a build.
The diagnostics from the rust compilation will indicate the rust
command needed to install the appropriate target support:

.. _rustup: https://rustup.rs/

.. code-block:: shell

   $ west build ...
   ...
   error[E0463]: cant find crate for `core`
     |
   = note: the `thumbv7m-none-eabi` target may not be installed
   = help: consider downloading the target with `rustup target add thumb7m-none-eabi`

In this case, the given ``rustup`` command will install the needed
target support.  The target needed will depend on both the board
selected, as well as certain configuration choices (such as whether
floating point is enabled).

Writing a Rust Application
**************************

See :zephyr_file:`samples/rust` for examples of an application.

CMake files
-----------

The application should contain a :file:`CMakeFiles.txt`, similar to
the following:

.. code-block:: cmake

   cmake_minimum_required(VERSION 3.20.0)

   find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
   project(hello_world)

   rust_cargo_application()

Cargo files
-----------

Rust applications are built with Cargo.  The Zephyr build system will
configure and invoke cargo in your application directory, with options
set so that it can find the Zephyr support libraries, and that the
output will be contained within the Zephyr build directory.

The :file:`Cargo.toml` will need to have a ``[lib]`` section that sets
``crate-type = ["staticlib"]``, and will need to include ``zephyr =
"0.1.0"`` as a dependency.  You can use crates.io and the Crate
ecosystem to include any other dependencies you need.  Just make sure
that you use crates that support building with no-std.

Application
-----------

The application source itself should live in file:`src/lib.rs`.  A few
things are needed.  A minimal file would be:

.. code-block:: rust

   #![no_std]

   extern crate zephyr;

   #[no_mangle]
   extern "C" fn rust_main() {
   }

The ``no_std`` declaration is needed to prevent the code from
referencing the ``std`` library.  The extern reference will cause the
zephyr crate to be brought in, even if nothing from it is used.
Practically, any meaningful Rust application on Zephyr will use
something from this crate, and this line is not necessary.  Lastly,
the main declaration exports the main symbol so that it can be called
by C code.  The build ``rust_cargo_application()`` cmake function will
include a small C file that will call into this from the C main
function.

Zephyr Functionality
********************

The bindings to Zephyr for Rust are under development, and are
currently rather minimalistic.

Bool Kconfig settings
---------------------

Boolean Kconfig settings can be used from within Rust code.  Due to
design constraints by the Rust language, settings that affect
compilation must be determined before the build is made.  In order to
use this in your application, you will need to use the
``zephyr-build`` crate, provided, to make these symbols available.

To your ``Cargo.toml`` file, add the following:

.. code-block:: toml

   [build-dependencies]
   zephyr-build = "0.1.0"

Then, you will need a ``build.rs`` file to call the support function.
The following will work:

.. code-block:: rust

   fn main() {
       zephyr_build::export_bool_kconfig();
   }

At this point, it will be possible to use the ``cfg`` directive in
Rust on boolean Kconfig values.  For example:

.. code-block:: rust

   #[cfg(CONFIG_MY_SETTING)]
   one_declaration;

   #[cfg(not(CONFIG_MY_SETTING)]
   other_declaration;

Other Kconfig settings
----------------------

All bool, numeric and string Kconfig settings are accessible from the
``zephyr::kconfig`` module.  For example:

.. code-block:: rust

   let ram_size = zephyr::kconfig::CONFIG_RAM_SIZE * 1024;

Other functionality
-------------------

Access to other functionality within zephyr is a work-in-progress, and
this document will be updated as that is done.
