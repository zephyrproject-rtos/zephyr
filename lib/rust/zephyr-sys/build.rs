use std::env;
use std::path::PathBuf;

fn main() {
    let bindings = bindgen::Builder::default()
        .use_core()
        // TODO: Fix include paths
        .clang_arg("-nostdinc")
        .clang_arg("-isystem/opt/zephyr-sdk-0.16.8/arm-zephyr-eabi/lib/gcc/arm-zephyr-eabi/12.2.0/include-fixed")
        .clang_arg("-isystem/opt/zephyr-sdk-0.16.8/arm-zephyr-eabi/lib/gcc/arm-zephyr-eabi/12.2.0/include")
        .clang_arg("-isystem/opt/zephyr-sdk-0.16.8/arm-zephyr-eabi/picolibc/include")
        .clang_arg("-I/home/ubuntu/zephyrproject/zephyr/kernel/include")
        .clang_arg("-I/home/ubuntu/zephyrproject/zephyr/arch/arm/include")
        .clang_arg("-I/home/ubuntu/zephyrproject/zephyr/build/zephyr/include/generated/zephyr")
        .clang_arg("-I/home/ubuntu/zephyrproject/zephyr/include")
        .clang_arg("-I/home/ubuntu/zephyrproject/zephyr/build/zephyr/include/generated")
        .clang_arg("-I/home/ubuntu/zephyrproject/zephyr/soc/st/stm32")
        .clang_arg("-I/home/ubuntu/zephyrproject/zephyr/soc/st/stm32/common")
        .clang_arg("-I/home/ubuntu/zephyrproject/zephyr/drivers")
        .clang_arg("-I/home/ubuntu/zephyrproject/zephyr/soc/st/stm32/stm32f4x")
        .clang_arg("-I/home/ubuntu/zephyrproject/modules/hal/cmsis/CMSIS/Core/Include")
        .clang_arg("-I/home/ubuntu/zephyrproject/zephyr/modules/cmsis")
        .clang_arg("-I/home/ubuntu/zephyrproject/modules/hal/stm32/stm32cube/stm32f4xx/soc")
        .clang_arg("-I/home/ubuntu/zephyrproject/modules/hal/stm32/stm32cube/stm32f4xx/drivers/include")
        .clang_arg("-I/home/ubuntu/zephyrproject/modules/hal/stm32/stm32cube/common_ll/include")
        .clang_arg("-isystem/home/ubuntu/zephyrproject/zephyr/lib/libc/common/include")
        .clang_arg("-imacros/home/ubuntu/zephyrproject/zephyr/build/zephyr/include/generated/zephyr/autoconf.h")
        .clang_arg("-imacros/home/ubuntu/zephyrproject/zephyr/include/zephyr/toolchain/zephyr_stdint.h")
        .clang_arg("-DSTM32F411xE")
        .wrap_static_fns(true)
        .allowlist_function("gpio_.*")
        .allowlist_function("k_.*")
        .header("wrapper.h")
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        .generate()
        .expect("Unable to generate bindings");

    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());

    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");
}
