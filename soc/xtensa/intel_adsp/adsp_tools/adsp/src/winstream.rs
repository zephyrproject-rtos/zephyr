use std::ptr;

pub struct Winstream {
    p: *const u8,
    last_seq: u32,
}

impl Winstream {
    pub fn open(p: *const u8) -> Self {
        Winstream { p, last_seq: 0 }
    }

    fn header(&self) -> (u32, u32, u32, u32) {
        let header = unsafe { std::slice::from_raw_parts(self.p as *const u32, 16) };
        (header[0], header[1], header[2], header[3])
    }

    // Read from the ring of buf_len starting at start_idx and ending at end_idx
    fn read_ring(&self, start_idx: u32, end_idx: u32, buf_len: u32) -> Vec<u8> {
        // check for a wrap

        let read_len = if end_idx < start_idx {
            (end_idx + buf_len) - start_idx
        } else {
            end_idx - start_idx
        };
        let mut v: Vec<u8> = Vec::with_capacity(read_len as usize);

        let mut read_lens: [u32; 2] = [read_len, 0];
        if start_idx + read_lens[0] >= buf_len {
            read_lens[0] = buf_len - start_idx;
            read_lens[1] = read_len - read_lens[0];
        }
        let start_offset = start_idx + 16;

        debug!(
            "winstream reading from offset {} len {}, offset {} len {}",
            start_offset, read_lens[0], 16, read_lens[1]
        );
        for i in 0..read_lens[0] {
            v.push(unsafe {
                ptr::read_volatile(self.p.offset(start_offset as isize + i as isize))
            });
        }
        for i in 0..read_lens[1] {
            v.push(unsafe { ptr::read_volatile(self.p.offset(16 as isize + i as isize)) });
        }
        v
    }
}

// Implement winstream as an iterator that always returns more strings
impl Iterator for Winstream {
    type Item = Vec<u8>;
    fn next(&mut self) -> Option<Vec<u8>> {
        // Retry a few times before giving up on reading
        for _ in 0..2 {
            let (wlen, start, end, seq) = self.header();
            if self.last_seq == 0 {
                self.last_seq = seq;
            }

            debug!(
                "winstream last_seq {}, wlen {}, start {}, end {}, seq {}",
                self.last_seq, wlen, start, end, seq
            );

            // Bytes we are behind is seq - last_seq
            let bytes_behind = seq - self.last_seq;

            // If we fell behind too far (larger than wlen)
            // then just drop everything
            if bytes_behind > wlen {
                warn!("Winstream fell behind, dropped {} bytes", bytes_behind);
                self.last_seq = seq;
                return Some(Vec::new());
            }

            let start_idx = if end < bytes_behind {
                (end + wlen) - bytes_behind
            } else {
                end - bytes_behind
            };

            // Otherwise we read the ring from last_seq to seq wrapping if needed
            // the actual start index is in fact seq
            let msg = self.read_ring(start_idx, end, wlen);
            debug!("Winstream read {} bytes", msg.len());

            // Verify the buffer hasn't changed while we read
            let (_, start1, end1, seq1) = self.header();
            if seq == seq1 && start == start1 && end == end1 {
                self.last_seq = seq;
                return Some(msg);
            } else {
                debug!("Winstream modified while reading, retrying...");
            }
        }

        warn!("Winstream buffer changing while reading, gave up");
        Some(Vec::new())
    }
}
