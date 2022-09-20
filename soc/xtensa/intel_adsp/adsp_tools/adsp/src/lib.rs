#[macro_use]
extern crate log;
extern crate env_logger;

use regex::Regex;
use std::fs::OpenOptions;
use std::io::{self, Seek, SeekFrom, Write};
use std::path::{Path, PathBuf};
use std::process::Command;

pub mod dma_buffer;
pub mod dsp;
pub mod hda;
pub mod hda_stream;
pub mod pagealloc;

mod winstream;
pub use winstream::Winstream;

fn check_pci_class(path: &Path) -> std::io::Result<bool> {
    let re = Regex::new(r"PCI_CLASS=40(10|38)0").unwrap();
    let uevent = std::fs::read_to_string(path)?;
    Ok(re.is_match(&uevent))
}

pub fn find_adsp_paths() -> std::io::Result<Vec<PathBuf>> {
    let mut pci_paths: Vec<PathBuf> = Vec::new();
    let pci_device_paths = std::fs::read_dir("/sys/bus/pci/devices/")?;
    for pci_dir_entry in pci_device_paths {
        let pci_dir_entry = pci_dir_entry?;
        let pci_dir_path = pci_dir_entry.path();
        if pci_dir_path.is_dir() {
            let pci_uevent_path = pci_dir_path.join("uevent");
            if let Ok(true) = check_pci_class(&pci_uevent_path) {
                pci_paths.push(PathBuf::from(pci_dir_path));
            }
        }
    }

    Ok(pci_paths)
}

// Enable PCI memory space access and bus mastering are enabled
// Disable interrupts so as not to confuse the kernel
pub fn enable_pci_mapping(pci_path: &Path) -> io::Result<()> {
    info!("Enabling PCI mapping for {:?}", pci_path);
    let mut config = OpenOptions::new()
        .read(true)
        .write(true)
        .open(pci_path.join("config"))?;
    config.seek(SeekFrom::Start(4))?;
    config.write(&[6u8, 4u8])?;
    Ok(())
}

// Needed on linux with the pci device mapping
pub fn enable_hugepages() -> io::Result<()> {
    // Mount if not yet mounted
    Command::new(concat!(
        "mount | grep -q hugetlbfs ||",
        " (mkdir -p /dev/hugepages; ",
        "  mount -t hugetlbfs hugetlbfs /dev/hugepages)"
    ))
    .spawn();

    let free_re = Regex::new(r"HugePages_Free:\s+([0-9])+").unwrap();
    let total_re = Regex::new(r"HugePages_Total:\s+([0-9])+").unwrap();

    // Ensure there's at least one free huge page
    let meminfo = std::fs::read_to_string("/proc/meminfo")?;

    let free_caps = free_re.captures(&meminfo).unwrap();
    if free_caps.get(1).unwrap().as_str().eq("0") {
        // This is a bit much for a one liner... but it does the job.
        let total = u32::from_str_radix(
            total_re
                .captures(&meminfo)
                .unwrap()
                .get(1)
                .unwrap()
                .as_str(),
            10,
        )
        .unwrap();
        let mut nr_hugepages = OpenOptions::new()
            .read(true)
            .write(true)
            .open("/proc/sys/vm/nr_hugepages")?;
        write!(&mut nr_hugepages, "{}", total + 1)?;
    }

    Ok(())
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum DeviceVariant {
    Unknown,
    CAVS15,
    CAVS18,
    CAVS25,
    ACE15,
}

impl DeviceVariant {
    pub fn from_device_id(id: u32) -> Self {
        match id {
            0x5A98 | 0x1A98 | 0x3198 => DeviceVariant::CAVS15,
            0x9DC8 | 0xA348 | 0x02C8 => DeviceVariant::CAVS18,
            0xA0C8 | 0x43C8 | 0x4B55 | 0x4B58 | 0x7AD0 | 0x51C8 => DeviceVariant::CAVS25,
            0x7E28 => DeviceVariant::ACE15,
            _ => DeviceVariant::Unknown,
        }
    }

    pub fn from_pci_device_file(p: &Path) -> std::io::Result<Self> {
        let p0 = p.join("device");
        let dev_file = std::fs::read_to_string(&p0)?;
        let dev_file_hex = &dev_file[2..6];
        debug!("{:?}: hex string {}", &p0, dev_file_hex);
        if let Ok(device_id) = u16::from_str_radix(dev_file_hex, 16) {
            Ok(Self::from_device_id(device_id as u32))
        } else {
            Ok(DeviceVariant::Unknown)
        }
    }
}
