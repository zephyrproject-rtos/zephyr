#[macro_use]
extern crate log;
extern crate env_logger;
use std::io::{self, Write};
use std::{thread, time::Duration};

use adsp::{self, dsp, Winstream};

fn main() -> std::io::Result<()> {
    env_logger::init();

    let pci_paths = adsp::find_adsp_paths()?;
    let pci_path = pci_paths.first().unwrap();
    let dev_variant = adsp::DeviceVariant::from_pci_device_file(&pci_path)?;
    info!(
        "Found audio device at {:?}, variant {:?}",
        &pci_path, &dev_variant
    );
    let dsp = dsp::DSP::open(pci_path, dev_variant.clone())?;
    let win3 = dsp.window(3, 0);
    let winstream = Winstream::open(win3);
    for msg in winstream {
        let mut stdout = io::stdout().lock();

        stdout.write_all(&msg)?;
        stdout.flush()?;

        thread::sleep(Duration::from_millis(5));
    }

    Ok(())
}
