module ghrd_10m50da_top (
  //Clock and Reset
  input  wire       clk_50,
  input  wire       fpga_reset_n,
  //QSPI
//  output wire     	 qspi_clk,
//  inout  wire[3:0]    qspi_io,
//  output wire         qspi_csn, 
  //16550 UART
  input wire	     uart_rx,
  output wire 		  uart_tx,
  output wire [4:0] user_led
);
//Heart-beat counter
reg   [25:0]  heart_beat_cnt;

// SoC sub-system module
ghrd_10m50da ghrd_10m50da_inst (
		.clk_clk                                                  (clk_50), 
		.reset_reset_n                                            (fpga_reset_n),
//		.ext_flash_flash_dataout_conduit_dataout                       (qspi_io),             
//		.ext_flash_flash_dclk_out_conduit_dclk_out                     (qspi_clk),            
//		.ext_flash_flash_ncs_conduit_ncs                               (qspi_csn), 
		//16550 UART
		.a_16550_uart_0_rs_232_serial_sin (uart_rx),                    //           a_16550_uart_0_rs_232_serial.sin
		.a_16550_uart_0_rs_232_serial_sout (uart_tx),                   //                                       .sout
		.a_16550_uart_0_rs_232_serial_sout_oe ()                //                                       .sout_oe
);  



//Heart beat by 50MHz clock
always @(posedge clk_50 or negedge fpga_reset_n)
  if (!fpga_reset_n)
      heart_beat_cnt <= 26'h0; //0x3FFFFFF
  else
      heart_beat_cnt <= heart_beat_cnt + 1'b1;

assign user_led = {4'hf,heart_beat_cnt[25]};
 

endmodule


