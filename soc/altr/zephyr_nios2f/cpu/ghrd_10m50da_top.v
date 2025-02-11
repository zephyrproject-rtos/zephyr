module ghrd_10m50da_top (
  //Clock and Reset
  input  wire       clk_50,
  //input  wire       clk_ddr3_100_p,
  input  wire       fpga_reset_n,
  //QSPI
  output wire     	 qspi_clk,
  inout  wire[3:0]    qspi_io,
  output wire         qspi_csn,
	//ddr3
   //output wire [13:0] mem_a,
   //output wire [2:0]  mem_ba,
   //inout  wire [0:0]  mem_ck,
   //inout  wire [0:0]  mem_ck_n,
   //output wire [0:0]  mem_cke,
   //output wire [0:0]  mem_cs_n,
   //output wire [0:0]  mem_dm,
   //output wire [0:0]  mem_ras_n,
   //output wire [0:0]  mem_cas_n,
   //output wire [0:0]  mem_we_n,
   //output wire        mem_reset_n,
   ///inout  wire [7:0]  mem_dq,
   //inout  wire [0:0]  mem_dqs,
   //inout  wire [0:0]  mem_dqs_n,
   //output wire [0:0]  mem_odt,
   //i2c
	inout  wire         i2c_sda,
   inout  wire         i2c_scl,
   //spi
	input wire 			spi_miso,
	output wire			spi_mosi,
	output wire 		spi_sclk,
	output wire 		spi_ssn,
  //16550 UART
  input wire	     uart_rx,
  output wire 		  uart_tx,
  output wire [4:0] user_led
);
//Heart-beat counter
reg   [25:0]  heart_beat_cnt;

//DDR3 interface assignments
//wire          local_init_done;
//wire          local_cal_success;
//wire          local_cal_fail;

//i2c interface
wire i2c_serial_sda_in ;
wire i2c_serial_scl_in ;
wire i2c_serial_sda_oe ;
wire i2c_serial_scl_oe ;
assign i2c_serial_scl_in = i2c_scl;
assign i2c_scl = i2c_serial_scl_oe ? 1'b0 : 1'bz;

assign i2c_serial_sda_in = i2c_sda;
assign i2c_sda = i2c_serial_sda_oe ? 1'b0 : 1'bz;

//assign system_resetn = fpga_reset_n & local_init_done;

// SoC sub-system module
ghrd_10m50da ghrd_10m50da_inst (
		.clk_clk                                                  		(clk_50),
		//.ref_clock_bridge_in_clk_clk												(clk_ddr3_100_p),
		.reset_reset_n                                           		(fpga_reset_n),
		//.mem_resetn_in_reset_reset_n                                   (fpga_reset_n               ), //                 mem_resetn_in_reset.reset_n
		.ext_flash_qspi_pins_data                       					(qspi_io),
		.ext_flash_qspi_pins_dclk                     						(qspi_clk),
		.ext_flash_qspi_pins_ncs                               			(qspi_csn),
		//.memory_mem_a                                                  (mem_a[12:0]               ), //                              memory.mem_a
      //.memory_mem_ba                                                 (mem_ba                    ), //                                    .mem_ba
      //.memory_mem_ck                                                 (mem_ck                    ), //                                    .mem_ck
      //.memory_mem_ck_n                                               (mem_ck_n                  ), //                                    .mem_ck_n
      //.memory_mem_cke                                                (mem_cke                   ), //                                    .mem_cke
      //.memory_mem_cs_n                                               (mem_cs_n                  ), //                                    .mem_cs_n
      //.memory_mem_dm                                                 (mem_dm                    ), //                                    .mem_dm
      //.memory_mem_ras_n                                              (mem_ras_n                 ), //                                    .mem_ras_n
      //.memory_mem_cas_n                                              (mem_cas_n                 ), //                                    .mem_cas_n
      //.memory_mem_we_n                                               (mem_we_n                  ), //                                    .mem_we_n
      //.memory_mem_reset_n                                            (mem_reset_n               ), //                                    .mem_reset_n
      //.memory_mem_dq                                                 (mem_dq                    ), //                                    .mem_dq
      //.memory_mem_dqs                                                (mem_dqs                   ), //                                    .mem_dqs
      //.memory_mem_dqs_n                                              (mem_dqs_n                 ), //                                    .mem_dqs_n
      //.memory_mem_odt                                                (mem_odt                   ), //                                    .mem_odt
      //.mem_if_ddr3_emif_0_status_local_init_done                     (local_init_done           ), //           mem_if_ddr3_emif_0_status.local_init_done
      //.mem_if_ddr3_emif_0_status_local_cal_success                   (local_cal_success         ), //                                    .local_cal_success
      //.mem_if_ddr3_emif_0_status_local_cal_fail                      (local_cal_fail            ),  //                                    .local_cal_fail
		//i2c
		.i2c_0_i2c_serial_sda_in													(i2c_serial_sda_in),
		.i2c_0_i2c_serial_scl_in													(i2c_serial_scl_in),
		.i2c_0_i2c_serial_sda_oe													(i2c_serial_sda_oe),
		.i2c_0_i2c_serial_scl_oe													(i2c_serial_scl_oe),
		//spi
		.spi_0_external_MISO                           						(spi_miso),								//                           spi_0_external.MISO
		.spi_0_external_MOSI                          						(spi_mosi),								//                                         .MOSI
		.spi_0_external_SCLK                           						(spi_sclk),								//                                         .SCLK
		.spi_0_external_SS_n                            					(spi_ssn),								//                                         .SS_n
		//pio
		.led_external_connection_export											(user_led[3:0]),
		//16550 UART
		.a_16550_uart_0_rs_232_serial_sin (uart_rx),                    //           a_16550_uart_0_rs_232_serial.sin
		.a_16550_uart_0_rs_232_serial_sout (uart_tx),                   //                                       .sout
		.a_16550_uart_0_rs_232_serial_sout_oe ()                //                                       .sout_oe

);

//DDR3 Address Bit #13 is not available for DDR3 SDRAM A (64Mx16)
//assign mem_a[13] = 1'b0;

//Heart beat by 50MHz clock
always @(posedge clk_50 or negedge fpga_reset_n)
  if (!fpga_reset_n)
      heart_beat_cnt <= 26'h0; //0x3FFFFFF
  else
      heart_beat_cnt <= heart_beat_cnt + 1'b1;

assign user_led[4] = heart_beat_cnt[25];
 

endmodule


