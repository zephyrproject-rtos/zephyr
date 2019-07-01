This sample has been tested with qemu_x64 board where it produces the
following traces, building with qemu_xtensa is also possible but requires
commenting out dai_get and dma_get function on SOF src/audio/dai.c:

[00:00:00.000,000] <inf> sof: module 0x0000f0ac init
[00:00:00.000,000] <inf> sof: module 0x0000f0b0 init
[00:00:00.000,000] <inf> sof: module 0x0000f0b4 init
[00:00:00.000,000] <inf> sof: module 0x0000f0b8 init
[00:00:00.000,000] <inf> sof: module 0x0000f0bc init
[00:00:00.000,000] <inf> sof: module 0x0000f0c0 init
[00:00:00.000,000] <inf> sof: module 0x0000f0c4 init
[00:00:00.000,000] <inf> sof: module 0x0000f0c8 init
[00:00:00.000,000] <inf> sof: module 0x0000f0cc init
[00:00:00.000,000] <inf> sof: module 0x0000f0d0 init
[00:00:00.000,000] <inf> sof: IPC ipc_init()
[00:00:00.000,000] <inf> sof: IPC initialized
[00:00:00.000,000] <inf> sof: unknown edf_scheduler_init()
[00:00:00.000,000] <inf> sof: scheduler initialized
[00:00:00.000,000] <inf> sof: HOST host_new()
[00:00:00.000,000] <inf> sof: HOST host_new()
[00:00:00.000,000] <inf> sof: HOST host_new()
[00:00:00.000,000] <inf> sof: VOLUME volume_new()
[00:00:00.000,000] <inf> sof: VOLUME vol->initial_ramp = 0, vol->ramp = 0
[00:00:00.000,000] <inf> sof: VOLUME volume_new()
[00:00:00.000,000] <inf> sof: VOLUME vol->initial_ramp = 0, vol->ramp = 0
[00:00:00.000,000] <inf> sof: VOLUME volume_new()
[00:00:00.010,000] <inf> sof: VOLUME vol->initial_ramp = 0, vol->ramp = 0
[00:00:00.010,000] <inf> sof: VOLUME volume_new()
[00:00:00.010,000] <inf> sof: VOLUME vol->initial_ramp = 0, vol->ramp = 0
[00:00:00.010,000] <inf> sof: DAI dai_new()
[00:00:00.010,000] <inf> sof: DAI dai_new()
[00:00:00.010,000] <inf> sof: MIXER mixer_new()
[00:00:00.010,000] <inf> sof: PIPE pipeline_new()
[00:00:00.010,000] <inf> sof: BUFFER buffer_new()
[00:00:00.010,000] <inf> sof: BUFFER buffer_new()
[00:00:00.010,000] <inf> sof: BUFFER buffer_new()
[00:00:00.010,000] <inf> sof: BUFFER buffer_new()
[00:00:00.010,000] <inf> sof: BUFFER buffer_new()
[00:00:00.010,000] <inf> sof: BUFFER buffer_new()
[00:00:00.010,000] <inf> sof: BUFFER buffer_new()
[00:00:00.010,000] <inf> sof: BUFFER buffer_new()
[00:00:00.010,000] <inf> sof: PIPE pipeline: connect comp 0 and buffer 16
[00:00:00.010,000] <inf> sof: PIPE pipeline: connect comp 1 and buffer 16
[00:00:00.010,000] <inf> sof: PIPE pipeline: connect comp 2 and buffer 17
[00:00:00.010,000] <inf> sof: PIPE pipeline: connect comp 3 and buffer 17
[00:00:00.010,000] <inf> sof: PIPE pipeline: connect comp 1 and buffer 18
[00:00:00.010,000] <inf> sof: PIPE pipeline: connect comp 4 and buffer 18
[00:00:00.010,000] <inf> sof: PIPE pipeline: connect comp 3 and buffer 19
[00:00:00.010,000] <inf> sof: PIPE pipeline: connect comp 4 and buffer 19
[00:00:00.010,000] <inf> sof: PIPE pipeline: connect comp 4 and buffer 20
[00:00:00.010,000] <inf> sof: PIPE pipeline: connect comp 5 and buffer 20
[00:00:00.010,000] <inf> sof: PIPE pipeline: connect comp 5 and buffer 21
[00:00:00.010,000] <inf> sof: PIPE pipeline: connect comp 6 and buffer 21
[00:00:00.010,000] <inf> sof: PIPE pipeline: connect comp 7 and buffer 22
[00:00:00.010,000] <inf> sof: PIPE pipeline: connect comp 8 and buffer 22
[00:00:00.020,000] <inf> sof: PIPE pipeline: connect comp 8 and buffer 23
[00:00:00.020,000] <inf> sof: PIPE pipeline: connect comp 9 and buffer 23
[00:00:00.020,000] <inf> sof: pipeline initialized
SOF on qemu_x86
