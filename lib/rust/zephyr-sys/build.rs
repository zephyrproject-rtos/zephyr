use std::{env, fs};
use std::path::PathBuf;

fn main() {
    let input_header = env::current_dir().unwrap().join("bindgen_input.h");
    let out_path = PathBuf::from(env::var("OUT_DIR").expect("OUT_DIR must be set!"));
    let wrap_static_fns = PathBuf::from(env::var("BINDGEN_WRAP_STATIC_FNS").expect("BINDGEN_WRAP_STATIC_FNS must be set!"));
    let clang_args_path = PathBuf::from(env::var("BINDGEN_CLANG_ARGS").expect("BINDGEN_CLANG_ARGS must be set!"));
    let clang_args = fs::read_to_string(clang_args_path).expect("Failed to read BINDGEN_CLANG_ARGS file!");

    let bindings = bindgen::Builder::default()
        .use_core()
        .detect_include_paths(false)
        .wrap_static_fns(true)
        .wrap_static_fns_path(wrap_static_fns)
        .allowlist_function("gpio_.*")
        .allowlist_function("k_.*")
        .allowlist_function("printk")
        .clang_args(clang_args.split(';'))
        .header(input_header.to_str().unwrap())
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        .generate()
        .expect("Unable to generate bindings!");

    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");
}
