use adsp_regmap::hda;
use memmap2::{MmapOptions, MmapRaw};
use std::fs::OpenOptions;
use std::path::Path;
use std::thread;
use std::time::Duration;

pub struct HDA {
    bar_map: MmapRaw,
    reg_map: hda::Hda,
}

impl HDA {
    pub fn open(pci_path: &Path) -> std::io::Result<Self> {
        let bar_path = pci_path.join("resource0");
        info!("Mapping {:?} as HDA", bar_path);
        let bar_file = OpenOptions::new().read(true).write(true).open(&bar_path)?;
        let bar_map = MmapOptions::new().map_raw(&bar_file)?;
        let reg_map = hda::Hda(bar_map.as_mut_ptr());

        Ok(HDA { bar_map, reg_map })
    }

    // Regmap
    pub fn reg_map(&self) -> &hda::Hda {
        &self.reg_map
    }

    // Input stream count
    pub fn input_streams(&self) -> u8 {
        unsafe { self.reg_map.gcap().read().iss() }
    }

    // Output stream count
    pub fn output_streams(&self) -> u8 {
        unsafe { self.reg_map.gcap().read().oss() }
    }

    // Reset the audio device and wait for it
    // TODO in theory this should be &mut self
    // or use interior mutability with RefCell... but whatever
    // its all pointer manipulation and so long as we do it "safely"
    // its "safe"
    pub fn reset(&self) {
        unsafe {
            debug!("HDA Reset... GCTL {:X}", self.reg_map.gctl().read().0);
            self.reg_map.gctl().modify(|r| r.set_crst(false));
            debug!(
                "Waiting on CRST to be unset, GCTL {:X}",
                self.reg_map.gctl().read().0
            );
            while self.reg_map.gctl().read().crst() {
                debug!(
                    "Waiting on CRST to be unset, GCTL {:X}",
                    self.reg_map.gctl().read().0
                );
                thread::sleep(Duration::from_millis(100));
            }
            self.reg_map.gctl().modify(|r| r.set_crst(true));
            while !self.reg_map.gctl().read().crst() {
                debug!(
                    "Waiting on CRST to be set, GCTL {:X}",
                    self.reg_map.gctl().read().0
                );
                thread::sleep(Duration::from_millis(100));
            }
            debug!("HDA Reset Done, GCTL {:X}", self.reg_map.gctl().read().0);
        }
    }

    // TODO use &mut self
    pub fn enable_dsp(&self) {
        unsafe {
            debug!(
                "HDA DSP Enable... PPCTL {:X}",
                self.reg_map.ppctl().read().0
            );
            self.reg_map.ppctl().modify(|r| r.set_gprocen(true));
            while !self.reg_map.ppctl().read().gprocen() {
                debug!(
                    "Waiting on GPROCEN to be set, PPCTL {:X}",
                    self.reg_map.ppctl().read().0
                );
                thread::sleep(Duration::from_millis(100));
            }
            debug!(
                "HDA DSP Enable Done PPCTL {:X}",
                self.reg_map.ppctl().read().0
            );
        }
    }
}
