use std::env;
use std::fs;
use std::io;
use std::path::PathBuf;
use std::str::FromStr;

use regex::Regex;

// Tools to generate nice register access from yaml descriptions
use chiptool::generate::CommonModule;
use chiptool::{generate, ir, transform};
use proc_macro2::TokenStream;

fn main() -> io::Result<()> {
    let out_dir = PathBuf::from(env::var_os("OUT_DIR").unwrap());
    let data_dir = PathBuf::from("../regmaps");

    let mut yaml_paths: Vec<PathBuf> = Vec::new();

    let yaml_ext = std::ffi::OsStr::new("yaml");
    for dir_entry in fs::read_dir(&data_dir)? {
        let dir_entry = dir_entry?;
        if dir_entry.path().extension() == Some(yaml_ext) {
            println!(
                "cargo:warning={}",
                format!("adding yaml path {}", dir_entry.path().display())
            );
            yaml_paths.push(dir_entry.path().to_path_buf());
        }
    }

    // Generate the common module
    fs::write(out_dir.join("common.rs"), chiptool::generate::COMMON_MODULE)?;

    // Generate each register block as its own rust file
    for yaml_path in yaml_paths.iter() {
        let data = fs::read(yaml_path)?;
        let mut ir: ir::IR = serde_yaml::from_slice(&data).unwrap();
        let mut filename = yaml_path.file_stem().unwrap().to_os_string();
        filename.push(".rs");
        let mut out_path = PathBuf::from(&out_dir);
        out_path.push(filename);

        transform::Sanitize {}.run(&mut ir).unwrap();

        transform::sort::Sort {}.run(&mut ir).unwrap();

        let gen_opts = generate::Options {
            common_module: CommonModule::External(TokenStream::from_str("crate::common").unwrap()),
        };

        let data = generate::render(&ir, &gen_opts).unwrap().to_string();

        // Some sane newline additions
        let data = data.replace("] ", "]\n");

        // Replace inner attributes (not allowed)
        let data = Regex::new("# *! *\\[.*\\]").unwrap().replace_all(&data, "");

        println!(
            "cargo:warning={}",
            format!("generating {:?} from {:?}", out_path, yaml_path)
        );
        fs::write(out_path, data.to_string())?;
    }

    println!("cargo:rerun-if-changed=build.rs");
    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=src/hda.rs");
    println!("cargo:rerun-if-changed=src/dsp.rs");
    println!("cargo:rerun-if-changed=src/common.rs");

    for yaml_path in yaml_paths.iter() {
        println!("cargo:rerun-if-changed={}", yaml_path.display());
    }

    Ok(())
}
