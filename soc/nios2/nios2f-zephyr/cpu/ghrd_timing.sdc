#**************************************************************
# Create Clock
#**************************************************************

derive_pll_clocks

# JTAG Signal Constraints constrain the TCK port, assuming a 10MHz JTAG clock and 3ns delays
create_clock -name {altera_reserved_tck} -period 41.667 [get_ports { altera_reserved_tck }]
set_input_delay -clock altera_reserved_tck -clock_fall -max 5 [get_ports altera_reserved_tdi]
set_input_delay -clock altera_reserved_tck -clock_fall -max 5 [get_ports altera_reserved_tms]
set_output_delay -clock altera_reserved_tck 5 [get_ports altera_reserved_tdo]

create_clock -name {clk_50} -period 20.000 {clk_50}

set_false_path -to [get_ports {user_led[*]}]
set_false_path -to [get_ports {fpga_reset_n}]
set_false_path -from [get_ports {fpga_reset_n}]

derive_clock_uncertainty

# QSPI interface
set_output_delay -clock {clk_50 } -rise -min 11 [get_ports {qspi_io[*]}]
set_output_delay -clock {clk_50 } -rise -min 11 [get_ports {qspi_clk}]
set_output_delay -clock {clk_50 } -rise -min 11 [get_ports {qspi_csn}]
set_input_delay  -clock {clk_50 } -rise -min 10 [get_ports {qspi_io[*]}]

# UART
set_false_path -from * -to [get_ports {uart_tx}]
