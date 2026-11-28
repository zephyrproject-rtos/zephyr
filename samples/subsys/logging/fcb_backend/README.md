# Zepyhr FCB logger sample 

This folder contains example for interfacing with FCB log backend flash.

This sample is tested with:
  for Zephyr version: 4.1.99 
	  nrf52840dk_nrf52840 - mx25r64
  NCS 2.6.1 :  
	  nrf9152dk_nrf9151	- gd25wb256
	#tested custom boards  
	sentek_iot_probe_wifi_nrf52840 - en25qh32
  	sentek_sm200_modem_nrf52840 - at45db641 with modified driver

## Setup

Before you get started, you'll need to install the nRF Connect SDK an fcb-log module.

1. nrf52840dk_nrf52840
  select prj.conf as base configuragion file
  select cnf/nrf52840dk_nrf52840_qspi.conf as extra devicetree conf
  select cnf/nrf52840dk_nrf52840_qspi.overlay as extra devicetree overlay
  select cnf/log_shell_uart.conf  to use uart as log and shell
  select cnf/log_shell_rtt.conf  to use rtt as log and shell

2. nrf9151dk_nrf9151
  select prj.conf as base configuragion file
  select cnf/nrf9151dk_nrf9151_qspi.conf as extra devicetree conf
  select cnf/nrf9151dk_nrf9151_qspi.overlay as extra devicetree overlay
  select cnf/log_shell_uart.conf  to use uart as log and shell
  select cnf/log_shell_rtt.conf  to use rtt as log and shell



1. Using PARTITION MANAGER
Note. Cross check pm.config file containg FCB_STORAGE entries and enabled.

  zephyr.dtc contains external flash entries:
    i.e. for at45db641
    nordic,pm-ext-flash = &at45db641;
    at45db641: at45db641e@0 {
				...
        status = "okay";
				partitions {
					compatible = "fixed-partitions";
					...
					lfs1_part: partition@0 {
						label = "fcb_storage";
						reg = < 0x0 0x10000 >;
					};

      i.e. gd25wb256
      nordic,pm-ext-flash = &gd25wb256;
      gd25wb256: gd25wb256e3ir@1 {
					status = "okay";
          ...
          partitions {
						compatible = "fixed-partitions";
						#address-cells = < 0x1 >;
						#size-cells = < 0x1 >;
						fcb_part: partition@0 {
							label = "fcb_storage";
							reg = < 0x10000 0x20000 >;
						};


      i.e. en25qh32b
      nordic,pm-ext-flash = &en25qh32b;
      en25qh32b: en25qh32b@0 { 
				status = "okay";
        ...
        partitions {
					compatible = "fixed-partitions";
					#address-cells = < 0x1 >;
					#size-cells = < 0x1 >;
					lfs1_part: partition@0 {
						label = "fcb_storage";
						reg = < 0x0 0x10000 >;
					};

      i.e.mx25r64
      nordic,pm-ext-flash = &mx25r64;
      mx25r64: mx25r6435f@0 {
				status = "okay";
        ...
      partitions {
					compatible = "fixed-partitions";
					#address-cells = < 0x1 >;
					#size-cells = < 0x1 >;
					lfs1_part: partition@0 {
						label = "fcb_storage";
						reg = < 0x0 0x10000 >;
					};
  
2. Using DTS

provide  overlay for external flash device.
i.e. for nrf52840dk_nrf52840_qspi.overlay

&mx25r64 {
	status = "okay";
	partitions {
		compatible = "fixed-partitions";
		#address-cells = <1>;
		#size-cells = <1>;

		fcb_storage: partition@0 {
			label = "fcb_storage";
			reg = <0x00000000 0x00010000>;
		};
	};
};


Build examples:

west build --build-dir ./build . --pristine --board nrf52840dk/nrf52840 -- -DNCS_TOOLCHAIN_VERSION="NONE" -DEXTRA_CONF_FILE=conf/nrf52840dk_nrf52840_qspi.conf -DCONF_FILE="prj.conf" -DDTC_OVERLAY_FILE=conf/nrf52840dk_nrf52840_qspi.overlay -DDEBUG_THREAD_INFO=Off

west build --build-dir ./build_1 . --pristine --board nrf9151dk/nrf9151 -- -DNCS_TOOLCHAIN_VERSION="NONE" -DEXTRA_CONF_FILE=conf/nrf9151dk_nrf9151_qspi.conf -DCONF_FILE="prj.conf" -DDTC_OVERLAY_FILE=conf/nrf9151dk_nrf9151_qspi.overlay -DDEBUG_THREAD_INFO=Off


Note. Could not submit build version containg MCU boot as it require NRF repository in a workspace 
      which is not possible in zephyr PR but it was successfully tested with NRF NCS 2.6.1 builds.

