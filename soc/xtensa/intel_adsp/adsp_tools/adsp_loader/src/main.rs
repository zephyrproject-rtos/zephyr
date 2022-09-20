#[macro_use]
extern crate log;
extern crate env_logger;

use std::fs::OpenOptions;
use std::io::{self, Write};
use std::path::PathBuf;
use std::rc::Rc;

use adsp::{self, dma_buffer::*, dsp, hda, hda_stream, pagealloc};

fn main() -> io::Result<()> {
    env_logger::Builder::from_env(env_logger::Env::default().default_filter_or("info")).init();

    let pci_paths = adsp::find_adsp_paths().unwrap();
    let pci_path = pci_paths.first().unwrap();
    let dev_variant = adsp::DeviceVariant::from_pci_device_file(&pci_path)?;

    info!(
        "Found audio device at {:?}, variant {:?}",
        &pci_path, &dev_variant
    );

    adsp::enable_pci_mapping(&pci_path)?;
    adsp::enable_hugepages();

    let hda = Rc::new(hda::HDA::open(pci_path)?);

    info!(
        "ADSP GCAP read from pci bar input streams {}, output streams {}",
        hda.input_streams(),
        hda.output_streams(),
    );

    info!("Enable global processing");
    hda.enable_dsp();

    info!("HDA Reset");
    hda.reset();

    let mut dsp = dsp::DSP::open(pci_path, dev_variant.clone())?;

    info!("Core 0 Set Stall");
    dsp.stall_core(0);

    info!("Core 0 Set Reset");
    dsp.reset_core(0);

    info!("Core 0 Set Power Off");
    dsp.power_off_core(0);

    info!("Creating DMA friendly allocator from a Huge Page");
    let mut huge_page = pagealloc::HugePage::open("adsp_loader")?;

    // Pick the stream_id (output stream) to use
    // The first output stream comes after the last input stream
    let stream_id = hda.input_streams();
    let fw_file_path = PathBuf::from("zephyr.ri");
    let mut fw_file = OpenOptions::new().read(true).open(&fw_file_path)?;
    let fw_file_size = fw_file.metadata()?.len();

    info!(
        "Configuring stream {} for uploading firmware file {:?}, size {}",
        stream_id, fw_file_path, fw_file_size
    );

    // TODO replace all of the below core on/off/reset/stall stream load, etc
    //  business into an interface, trait FWLoad which is impl'ed by each DSP variant.
    // dsp.load_firmware(fw_file); where fw_file is impl BufReader and can be a buffer
    // network socket, or file.

    let stream = hda_stream::HDAStream::<hda_stream::OutStream, hda_stream::Reset>::open(
        hda.clone(),
        stream_id,
    )?;
    let mut stream = stream.configure(
        &mut huge_page,
        &[hda_stream::align_up(fw_file_size as u32), 128u32],
    );

    info!("Stream configured writing firmware");
    std::io::copy(&mut fw_file, &mut stream)?;

    // Power on the core(s)
    info!("Powering on core(s)");
    dsp.power_on_core(0);
    while !dsp.is_core_on(0) {}

    // Inform the stream to read from with ipc
    info!("Sending IPC to Purge FW");
    dsp.ipc_send(dsp::IPC_FW_PURGE | ((stream_id as u32) << 9), 0);

    info!("Unreset for core(s)");
    dsp.unreset_core(0);
    while dsp.is_core_reset(0) {}

    info!("Unstall core(s)");
    dsp.unstall_core(0);
    while dsp.is_core_stalled(0) {}

    // Watch for rom boot signal
    info!("Wait for ROM_STATUS to show ROM_OK");
    dsp.wait_for_rom();

    info!("Starting stream");
    let stream = stream.start();

    info!("Waiting for ROM_STATUS to show FW_ENTERED");
    dsp.wait_for_fw();

    info!("Resetting stream");
    stream.reset();

    info!("Firmware load complete!");

    Ok(())
}
