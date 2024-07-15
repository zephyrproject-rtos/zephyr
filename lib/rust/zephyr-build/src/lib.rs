// Copyright (c) 2024 Linaro LTD
// SPDX-License-Identifier: Apache-2.0

// Pre-build code for zephyr module.

// This module makes the values from the generated .config available as conditional compilation.
// Note that this only applies to the zephyr module, and the user's application will not be able to
// see these definitions.  To make that work, this will need to be moved into a support crate which
// can be invoked by the user's build.rs.

// This builds a program that is run on the compilation host before the code is compiled.  It can
// output configuration settings that affect the compilation.

use std::io::{BufRead, BufReader};
use std::env;
use std::fs::File;

use regex::Regex;

/// Export boolean Kconfig entries.  This must happen in any crate that wishes to access the
/// configuration settings.
pub fn export_bool_kconfig() {
    let dotconfig = env::var("DOTCONFIG").expect("DOTCONFIG must be set by wrapper");

    // Ensure the build script is rerun when the dotconfig changes.
    println!("cargo:rerun-if-env-changed=DOTCONFIG");
    println!("cargo-rerun-if-changed={}", dotconfig);

    let config_y = Regex::new(r"^(CONFIG_.*)=y$").unwrap();

    let file = File::open(&dotconfig).expect("Unable to open dotconfig");
    for line in BufReader::new(file).lines() {
        let line =  line.expect("reading line from dotconfig");
        if let Some(caps) = config_y.captures(&line) {
            println!("cargo:rustc-cfg={}", &caps[1]);
        }
    }
}
