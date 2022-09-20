use super::DeviceVariant;
use adsp_regmap::dsp;
use memmap2::{MmapOptions, MmapRaw};
use std::fs::OpenOptions;
use std::path::Path;
use std::thread;
use std::time::Duration;

pub struct DSP {
    bar_map: MmapRaw,
    reg_map: dsp::Dsp,
    variant: DeviceVariant,
    window_base: usize,
    window_size: usize,
}

pub const IPC_FW_PURGE: u32 = (1 << 24) | (1 << 14);

impl DSP {
    pub fn open(pci_path: &Path, variant: DeviceVariant) -> std::io::Result<Self> {
        let bar_path = pci_path.join("resource4");
        info!("Mapping {:?} as DSP", bar_path);
        let bar_file = OpenOptions::new().read(true).write(true).open(&bar_path)?;
        let bar_map = MmapOptions::new().map_raw(&bar_file)?;

        // TODO register map for ACE is different!
        let reg_map = dsp::Dsp(bar_map.as_mut_ptr());

        let window_base = if variant == DeviceVariant::ACE15 {
            0x180000
        } else {
            0x80000
        };

        let window_size = 0x20000;

        Ok(DSP {
            bar_map,
            reg_map,
            variant,
            window_base, // 512KB offset
            window_size, // 128KB size
        })
    }

    // Get adspcs for easy debugging
    pub fn adspcs(&self) -> u32 {
        unsafe { self.reg_map.adspcs().read().0 }
    }

    // Stall a core
    pub fn stall_core(&mut self, core_id: u8) {
        unsafe {
            self.reg_map
                .adspcs()
                .modify(|r| r.set_cstall(r.cstall() | (1 << core_id)));
            while !self.is_core_stalled(core_id) {
                debug!(
                    "Waiting for core stall , ADSPCS: {:X}",
                    self.reg_map.adspcs().read().0
                );
                thread::sleep(Duration::from_millis(100));
            }
        }
    }
    // Unstall a core
    pub fn unstall_core(&mut self, core_id: u8) {
        unsafe {
            self.reg_map
                .adspcs()
                .modify(|r| r.set_cstall(r.cstall() & !(1 << core_id)));
            while self.is_core_stalled(core_id) {
                debug!(
                    "Waiting for core unstall , ADSPCS: {:X}",
                    self.reg_map.adspcs().read().0
                );
                thread::sleep(Duration::from_millis(100));
            }
        }
    }

    pub fn is_core_stalled(&self, core_id: u8) -> bool {
        unsafe { self.reg_map.adspcs().read().cstall() & (1 << core_id) != 0 }
    }

    // Reset a core
    pub fn reset_core(&mut self, core_id: u8) {
        unsafe {
            self.reg_map
                .adspcs()
                .modify(|r| r.set_crst(r.crst() | (1 << core_id)));
            while !self.is_core_reset(core_id) {
                debug!(
                    "Waiting for core reset off, ADSPCS: {:X}",
                    self.reg_map.adspcs().read().0
                );
                thread::sleep(Duration::from_millis(100));
            }
        }
    }

    // Unreset a core
    pub fn unreset_core(&mut self, core_id: u8) {
        unsafe {
            self.reg_map
                .adspcs()
                .modify(|r| r.set_crst(r.crst() & !(1 << core_id)));
            while self.is_core_reset(core_id) {
                debug!(
                    "Waiting for core unreset off, ADSPCS: {:X}",
                    self.reg_map.adspcs().read().0
                );
                thread::sleep(Duration::from_millis(100));
            }
        }
    }

    pub fn is_core_reset(&self, core_id: u8) -> bool {
        unsafe { self.reg_map.adspcs().read().crst() & (1 << core_id) != 0 }
    }

    // Power Off a Core
    pub fn power_off_core(&mut self, core_id: u8) {
        unsafe {
            self.reg_map
                .adspcs()
                .modify(|r| r.set_spa(r.spa() & !(1 << core_id)));
            while self.is_core_on(core_id) {
                debug!(
                    "Waiting for core power off, ADSPCS: {:X}",
                    self.reg_map.adspcs().read().0
                );
                thread::sleep(Duration::from_millis(100));
            }
        }
    }

    // Power On a Core
    pub fn power_on_core(&mut self, core_id: u8) {
        unsafe {
            self.reg_map
                .adspcs()
                .modify(|r| r.set_spa(r.spa() | (1 << core_id)));
            while !self.is_core_on(core_id) {
                debug!(
                    "Waiting for core power on, ADSPCS: {:X}",
                    self.reg_map.adspcs().read().0
                );
                thread::sleep(Duration::from_millis(100));
            }
        }
    }

    pub fn is_core_on(&self, core_id: u8) -> bool {
        unsafe { self.reg_map.adspcs().read().cpa() & (1 << core_id) == 1 }
    }

    // Get a memory window pointer with an offset
    pub fn window(&self, window: u8, offset: u32) -> *const u8 {
        let win_offset: isize = (self.window_base as isize)
            + (window as isize) * (self.window_size as isize)
            + (offset as isize);
        unsafe { self.bar_map.as_ptr().offset(win_offset) }
    }

    // Wait for the rom status to be 0x5 which means "ROM OK"
    pub fn wait_for_rom(&self) {
        // ROM Status is the first word of window 0
        unsafe {
            loop {
                let rom_status: u32 = std::ptr::read_volatile(self.window(0, 0) as *const u32);

                if (rom_status >> 24) == 5 {
                    break;
                }
                debug!(
                    "Waiting for ROM Status to show BOOTED, currently {:X}",
                    rom_status
                );
                thread::sleep(Duration::from_millis(100));
            }
        }
    }

    // Wait for the rom status to be FW_ENTERED
    pub fn wait_for_fw(&self) {
        unsafe {
            loop {
                let rom_status: u32 = std::ptr::read_volatile(self.window(0, 0) as *const u32);
                if rom_status & ((1 << 28) - 1) == 5 {
                    break;
                }
                debug!(
                    "Waiting for ROM Status to show FW_ENTERED, currentl {:X}",
                    rom_status
                );
                thread::sleep(Duration::from_millis(100));
            }
        }
    }

    pub fn ipc_send(&mut self, msg: u32, msg_ext: u32) {
        unsafe {
            self.reg_map.hipcidr().write(|_| msg | 1 << 31);
        }
    }

    pub fn is_ipc_done(&mut self) -> bool {
        unsafe { self.reg_map.hipcida().read().0 & (1 << 31) != 0 }
    }
}
