use std::env;
use std::path::PathBuf;

fn main() {
    let bindings = bindgen::Builder::default()
        .use_core()
        // TODO: Fix include paths
        .clang_arg("-nostdinc")
        .clang_arg("-isystem/opt/zephyr-sdk-0.16.8/arm-zephyr-eabi/lib/gcc/arm-zephyr-eabi/12.2.0/include")
        .clang_arg("-isystem/opt/zephyr-sdk-0.16.8/arm-zephyr-eabi/picolibc/include")
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
