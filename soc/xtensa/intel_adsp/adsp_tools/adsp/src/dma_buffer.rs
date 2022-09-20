use std::io;

// A wrapper around a pointer to memory where a DMA friendly buffer lies
pub struct DMABuffer {
    pub(crate) p: *mut u8,
    pub(crate) sz: usize,
    pub(crate) phys_addr: usize,
    pub(crate) pos: usize,
}

impl DMABuffer {
    pub fn reset(&mut self) {
        self.pos = 0;
    }
}

// Buffered Reader for DMA Buf
//impl std::io::BufRead for DMABuffer {
//    fn read(&self mut, buf: &mut [u8]) -> io::Result<usize> {
//
//    }
//}

impl io::Write for DMABuffer {
    fn write(&mut self, buf: &[u8]) -> io::Result<usize> {
        let free_space = self.sz - self.pos;
        let write_len = free_space.min(buf.len());
        unsafe {
            std::ptr::copy_nonoverlapping(
                &buf[0] as *const u8,
                self.p.offset(self.pos as isize),
                write_len,
            );
        }
        self.pos += write_len;
        Ok(write_len)
    }

    fn flush(&mut self) -> io::Result<()> {
        Ok(())
    }
}

// Trait any Stream DMABuf allocator must provide
pub trait DMAAllocator {
    fn alloc(&mut self, sz: usize) -> DMABuffer;

    // Free is optionally defined
    fn free(&mut self, dma_buf: DMABuffer) {}
}
