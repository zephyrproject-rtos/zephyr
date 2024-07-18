use std::env;
use std::path::PathBuf;

fn main() {
    let bindings = bindgen::Builder::default()
        .clang_arg("-I/home/ubuntu/zephyrproject/zephyr/include")
        .clang_arg("-I/home/ubuntu/zephyrproject/zephyr/build/zephyr/include/generated")
        .clang_arg("-imacros/home/ubuntu/zephyrproject/zephyr/build/zephyr/include/generated/zephyr/autoconf.h")
        .wrap_static_fns(true)
        .allowlist_function("gpio_.*")
        .header("wrapper.h")
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        .generate()
        .expect("Unable to generate bindings");

    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());

    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");
}
