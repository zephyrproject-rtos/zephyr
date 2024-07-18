use alloc::ffi::CString;
use alloc::string::ToString;
use core::ffi::c_char;
use core::panic::PanicInfo;
use zephyr_sys::printk;

extern "C" {
    fn k_panic__extern() -> !;
}

#[panic_handler]
unsafe fn panic(_info: &PanicInfo) -> ! {
    if let Ok(message) = CString::new(_info.to_string()) {
        printk("%s\n\0".as_ptr() as *const c_char, message.as_ptr());
    }

    k_panic__extern();
}
