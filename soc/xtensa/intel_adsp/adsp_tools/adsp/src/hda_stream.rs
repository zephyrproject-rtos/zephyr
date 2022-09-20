use crate::{
    dma_buffer::{DMAAllocator, DMABuffer},
    hda::HDA,
};
use std::io;
use std::marker::PhantomData;
use std::ptr;
use std::rc::Rc;

pub enum Reset {}
pub enum Configured {}
pub enum Streaming {}

pub trait HDAStreamState {}
impl HDAStreamState for Reset {}
impl HDAStreamState for Configured {}
impl HDAStreamState for Streaming {}

pub enum InStream {}
pub enum OutStream {}
pub trait HDAStreamDirection {}
impl HDAStreamDirection for InStream {}
impl HDAStreamDirection for OutStream {}

pub struct HDAStream<D: HDAStreamDirection, S: HDAStreamState> {
    hda: Rc<HDA>,
    stream_id: usize,
    buf_idx: usize,
    bufs: Vec<DMABuffer>,
    _state: PhantomData<S>,
    _direction: PhantomData<D>,
}

impl<D, S> HDAStream<D, S>
where
    D: HDAStreamDirection,
    S: HDAStreamState,
{
    // Reset the stream
    pub fn reset(self) -> HDAStream<D, Reset> {
        let reg_map = self.hda.reg_map();
        unsafe {
            reg_map.sdctl(self.stream_id).modify(|r| r.set_run(false));
            // TODO sleep
            reg_map.sdctl(self.stream_id).modify(|r| r.set_srst(true));
            // TODO spin on reset == 0
            reg_map.sdctl(self.stream_id).modify(|r| r.set_srst(false));
            // TODO spin on reset == 1
            //TODO disable spib and set position to 0
            // enable dsp capture (hda.ppctl |= (1 << self.stream_id))
        }
        HDAStream {
            hda: self.hda,
            stream_id: self.stream_id,
            buf_idx: 0,
            bufs: Vec::new(),
            _direction: self._direction,
            _state: PhantomData,
        }
    }
}

#[repr(C)]
struct BufferDescriptor {
    address: u64,
    length: u32,
    options: u32,
}

pub const fn align_up(len: u32) -> u32 {
    let rem = len % 128;
    len + (128 - rem)
}

impl<D> HDAStream<D, Reset>
where
    D: HDAStreamDirection,
{
    pub fn open(hda: Rc<HDA>, stream_id: u8) -> std::io::Result<Self> {
        Ok(HDAStream {
            hda,
            stream_id: stream_id as usize,
            buf_idx: 0,
            bufs: Vec::new(),
            _state: PhantomData,
            _direction: PhantomData,
        })
    }

    // Configure the Stream
    pub fn configure<A: DMAAllocator>(
        self,
        allocator: &mut A,
        buf_lens: &[u32],
    ) -> HDAStream<D, Configured> {
        let mut buffer_length = 0;
        let mut dma_bufs: Vec<DMABuffer> = Vec::new();

        assert!(buf_lens.len() >= 2, "Must have at least 2 buffer lengths");

        for buf_len in buf_lens.iter() {
            assert!(
                buf_len % 128 == 0,
                "Buffer length must be 128 byte aligned, buf_len {} % 128 = {} != 0",
                buf_len,
                buf_len % 128,
            );

            buffer_length += buf_len;
            dma_bufs.push(allocator.alloc(*buf_len as usize))
        }

        // Create buffer descriptor list, each bdl entry is 16 bytes
        // the first 8 bytes are the physical address of the buffer
        // the following 4 bytes are the size of the buffer
        // the last 4 bytes are unused but contain bit flags for
        // interrupt on completion (no interrupts here)
        let bdl_buf = allocator.alloc(buf_lens.len() * 16);
        let mut bdl_offs = 0;
        for dma_buf in dma_bufs.iter() {
            unsafe {
                ptr::copy_nonoverlapping(
                    ptr::addr_of!(dma_buf.phys_addr) as *const u8,
                    bdl_buf.p.offset(bdl_offs),
                    8,
                );
                ptr::copy_nonoverlapping(
                    ptr::addr_of!(dma_buf.sz) as *const u8,
                    bdl_buf.p.offset(bdl_offs + 8),
                    4,
                );
            }
            bdl_offs += 16;
        }

        let reg_map = self.hda.reg_map();
        unsafe {
            reg_map.sdctl(self.stream_id).modify(|r| {
                r.set_strm(self.stream_id as u8);
                r.set_tp(true);
            });
            reg_map
                .sdbdpu(self.stream_id)
                .write(|_| (bdl_buf.phys_addr >> 32) & 0xFFFFFFFF);
            reg_map
                .sdbdpl(self.stream_id)
                .write(|_| (bdl_buf.phys_addr & 0xFFFFFFFF));
            reg_map.sdcbl(self.stream_id).write(|_| buffer_length);
            reg_map
                .sdlvi(self.stream_id)
                .write(|_| (dma_bufs.len() - 1));
            reg_map
                .ppctl()
                .modify(|r| r.set_procen(self.stream_id, true));
            reg_map
                .spbfctl()
                .modify(|r| r.set_spibe(self.stream_id, true));
        }

        HDAStream {
            hda: self.hda,
            stream_id: self.stream_id,
            buf_idx: 0,
            bufs: dma_bufs,
            _direction: self._direction,
            _state: PhantomData,
        }
    }
}

impl<D> HDAStream<D, Configured>
where
    D: HDAStreamDirection,
{
    // Start the stream
    pub fn start(self) -> HDAStream<D, Streaming> {
        let reg_map = self.hda.reg_map();
        unsafe {
            reg_map.sdctl(self.stream_id).modify(|r| {
                r.set_run(true);
            });
        }

        HDAStream {
            hda: self.hda,
            stream_id: self.stream_id,
            buf_idx: self.buf_idx,
            bufs: self.bufs,
            _direction: self._direction,
            _state: PhantomData,
        }
    }
}

impl std::io::Write for HDAStream<OutStream, Configured> {
    fn write(&mut self, buf: &[u8]) -> io::Result<usize> {
        let mut write_len = buf.len();
        let mut buf_idx = self.buf_idx;
        loop {
            write_len -= self.bufs[buf_idx].write(buf)?;
            if write_len == 0 || buf_idx >= self.bufs.len() - 1 {
                break;
            }
            buf_idx -= 1;
        }
        unsafe {
            self.hda
                .reg_map()
                .spib(self.stream_id)
                .modify(|r| *r += buf.len() as u32);
        }
        Ok(buf.len() - write_len)
    }

    fn flush(&mut self) -> io::Result<()> {
        Ok(())
    }
}
