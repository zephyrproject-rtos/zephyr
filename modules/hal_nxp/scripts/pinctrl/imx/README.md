# i.MX Pin Control Generation Scripts

These scripts aid in the generation of pin control data for i.MX platforms,
with the exception of iMX RT600 and RT500 (supported by the LPC
generation scripts). The following scripts are provided:

| Script               | Functionality                |
| -------------------- | ---------------------------- |
| `imx_gen_gpio.py`    | Generates GPIO to pin mappings for SOC DTSI files |
| `imx_fixup_pinmux.py`| Corrects pinmux errors present in source data |
| `imx_cfg_utils.py`   | support library              |

Note that users seeking to generate board pin control files should use
`gen_board_pinctrl.py`, the additional scripts in this directory are
only intended for SOC maintainers seeing to create pin control header files.
