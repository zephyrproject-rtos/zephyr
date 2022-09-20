use crate::dma_buffer::*;
use memmap2::{MmapOptions, MmapRaw};
use std::fs::OpenOptions;
use std::io::{self, Read, Seek, SeekFrom};
use std::path::PathBuf;

pub struct HugePage {
    phys_addr: usize,
    virt_addr: usize,
    mem_map: MmapRaw,
    used: usize,
}

const PAGE_SIZE: u64 = 4096;
const HUGEPAGE_SIZE: u64 = 2 * 1024 * 1024;

impl HugePage {
    pub fn open(name: &str) -> io::Result<HugePage> {
        let mut path = PathBuf::from("/dev/hugepages");
        path.push(name);

        // TODO check memory budget to ensure we have enough for another huge page
        // if not then increase the number of huge pages available

        debug!("Opening hugepage file {:?}", &path);
        let huge_file = OpenOptions::new()
            .read(true)
            .write(true)
            .create(true)
            .open(&path)?;

        debug!("Mapping hugepage file");
        let mem_map = MmapOptions::new().map_raw(&huge_file)?;

        debug!("Faulting page");
        // Fault the page in so it has an address
        unsafe {
            std::ptr::write_volatile(mem_map.as_mut_ptr(), 0);
        }

        let virt_addr = mem_map.as_ptr() as usize;
        let virt_page_num = virt_addr >> 12;
        let mut phys_page_entry: [u8; 8] = [0; 8];
        let mut page_map = OpenOptions::new().read(true).open("/proc/self/pagemap")?;
        page_map.seek(SeekFrom::Start((virt_page_num * 8) as u64))?;
        page_map.read_exact(&mut phys_page_entry)?;
        let phys_page_entry = u64::from_le_bytes(phys_page_entry);
        let phys_addr = ((phys_page_entry & ((1 << 55) - 1)) * PAGE_SIZE) as usize;

        info!(
            "Mapped in huge page, {} bytes , virtual address {:#}, page number {:#}, physical address {:#}",
            HUGEPAGE_SIZE,
            virt_addr,
            virt_page_num,
            phys_addr
        );

        Ok(HugePage {
            phys_addr,
            virt_addr,
            mem_map,
            used: 0,
        })
    }
}

impl DMAAllocator for HugePage {
    fn alloc(&mut self, sz: usize) -> DMABuffer {
        // pointer offset by used
        let p = unsafe { self.mem_map.as_mut_ptr().offset(self.used as isize) };

        self.used += sz;

        let phys_addr = self.phys_addr + sz;

        info!(
            "HugePage alloc, used {} of {} bytes",
            self.used, HUGEPAGE_SIZE,
        );

        DMABuffer {
            p,
            sz,
            phys_addr,
            pos: 0,
        }
    }
}
