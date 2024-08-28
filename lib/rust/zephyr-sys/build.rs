// Copyright (c) 2024 Linaro LTD
// SPDX-License-Identifier: Apache-2.0

// Pre-build code for zephyr module.

// This module makes the values from the generated .config available as conditional compilation.
// Note that this only applies to the zephyr module, and the user's application will not be able to
// see these definitions.  To make that work, this will need to be moved into a support crate which
// can be invoked by the user's build.rs.

// This builds a program that is run on the compilation host before the code is compiled.  It can
// output configuration settings that affect the compilation.

use anyhow::Result;

use bindgen::Builder;

use std::env;
use std::path::{Path, PathBuf};

fn main() -> Result<()> {
    // Determine which version of Clang we linked with.
    let version = bindgen::clang_version();
    println!("Clang version: {:?}", version);

    // Pass in the target used to build the native code.
    let target = env::var("TARGET")?;

    // Rustc uses some complex target tuples for the riscv targets, whereas clang uses other
    // options.  Fortunately, these variants shouldn't affect the structures generated, so just
    // turn this into a generic target.
    let target = if target.starts_with("riscv32") {
        "riscv32-unknown-none-elf".to_string()
    } else {
        target
    };

    // Likewise, do the same with RISCV-64.
    let target = if target.starts_with("riscv64") {
        "riscv64-unknown-none-elf".to_string()
    } else {
        target
    };

    let target_arg = format!("--target={}", target);

    // println!("includes: {:?}", env::var("INCLUDE_DIRS"));
    // println!("defines: {:?}", env::var("INCLUDE_DEFINES"));

    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    let wrapper_path = PathBuf::from(env::var("WRAPPER_FILE").unwrap());

    // Bindgen everything.
    let bindings = Builder::default()
        .header(Path::new("wrapper.h").canonicalize().unwrap().to_str().unwrap())
        .use_core()
        .clang_arg(&target_arg);
    let bindings = define_args(bindings, "-I", "INCLUDE_DIRS");
    let bindings = define_args(bindings, "-D", "INCLUDE_DEFINES");
    let bindings = bindings
        .wrap_static_fns(true)
        .wrap_static_fns_path(wrapper_path)
        // <inttypes.h> seems to come from somewhere mysterious in Zephyr.  For us, just pull in the
        // one from the minimal libc.
        .clang_arg("-DRUST_BINDGEN")
        .clang_arg("-I../../../lib/libc/minimal/include")
        .derive_copy(false)
        .allowlist_function("k_.*")
        .allowlist_function("gpio_.*")
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        .generate()
        .expect("Unable to generate bindings");

    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");

    Ok(())
}

fn define_args(bindings: Builder, prefix: &str, var_name: &str) -> Builder {
    let text = env::var(var_name).unwrap();
    let mut bindings = bindings;
    for entry in text.split(" ") {
        if entry.is_empty() {
            continue;
        }
        println!("Entry: {}{}", prefix, entry);
        let arg = format!("{}{}", prefix, entry);
        bindings = bindings.clang_arg(arg);
    }
    bindings
}
