/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdio.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_silabs_siwx91x.h>
#include <zephyr/shell/shell.h>

struct siwx91x_clock_shell_entry {
	const char *domain;
	uint32_t clkid;
	const char *name;
};

static const struct siwx91x_clock_shell_entry siwx91x_clocks[] = {
	/* AON domain */
	{ "AON", SIWX91X_CLK_XTAL_MHZ,        "XTAL_MHZ"        },
	{ "AON", SIWX91X_CLK_XTAL_KHZ,        "XTAL_KHZ"        },
	{ "AON", SIWX91X_CLK_RC_MHZ,          "RC_MHZ"          },
	{ "AON", SIWX91X_CLK_RC_KHZ,          "RC_KHZ"          },
	{ "AON", SIWX91X_CLK_REF_HP,          "REF_HP"          },
	{ "AON", SIWX91X_CLK_REF_ULP,         "REF_ULP"         },
	{ "AON", SIWX91X_CLK_AON_REF_HF,      "AON_REF_HF"      },
	{ "AON", SIWX91X_CLK_AON_REF_LF,      "AON_REF_LF"      },
	{ "AON", SIWX91X_CLK_UULP_GPIO,       "UULP_GPIO"       },
	{ "AON", SIWX91X_CLK_SYSRTC,          "SYSRTC"          },
	{ "AON", SIWX91X_CLK_RTC,             "RTC"             },
	{ "AON", SIWX91X_CLK_WATCHDOG,        "WATCHDOG"        },

	/* HP domain */
	{ "HP",  SIWX91X_CLK_HP_REF_PLL,      "HP_REF_PLL"      },
	{ "HP",  SIWX91X_CLK_PLL_SOC,         "PLL_SOC"         },
	{ "HP",  SIWX91X_CLK_PLL_INTF,        "PLL_INTF"        },
	{ "HP",  SIWX91X_CLK_PLL_I2S,         "PLL_I2S"         },
	{ "HP",  SIWX91X_CLK_CPU,             "CPU"             },
	{ "HP",  SIWX91X_CLK_CPU_LP,          "CPU_LP"          },
	{ "HP",  SIWX91X_CLK_HP_REF_ULP,      "HP_REF_ULP"      },
	{ "HP",  SIWX91X_CLK_UART0,           "UART0"           },
	{ "HP",  SIWX91X_CLK_UART1,           "UART1"           },
	{ "HP",  SIWX91X_CLK_I2C0,            "I2C0"            },
	{ "HP",  SIWX91X_CLK_I2C1,            "I2C1"            },
	{ "HP",  SIWX91X_CLK_GSPI,            "GSPI"            },
	{ "HP",  SIWX91X_CLK_QSPI,            "QSPI"            },
	{ "HP",  SIWX91X_CLK_QSPI2,           "QSPI2"           },
	{ "HP",  SIWX91X_CLK_SSI,             "SSI"             },
	{ "HP",  SIWX91X_CLK_I2S,             "I2S"             },
	{ "HP",  SIWX91X_CLK_STATIC_I2S,      "STATIC_I2S"      },
	{ "HP",  SIWX91X_CLK_PWM,             "PWM"             },
	{ "HP",  SIWX91X_CLK_UDMA,            "UDMA"            },
	{ "HP",  SIWX91X_CLK_GPDMA,           "GPDMA"           },
	{ "HP",  SIWX91X_CLK_RNG,             "RNG"             },
	{ "HP",  SIWX91X_CLK_GPIO,            "GPIO"            },
	{ "HP",  SIWX91X_CLK_ICACHE,          "ICACHE"          },
	{ "HP",  SIWX91X_CLK_PIN_OUT,         "OUT_CLK"         },

	/* ULP domain */
	{ "ULP", SIWX91X_CLK_ULP_REF_CPU,     "ULP_REF_CPU"     },
	{ "ULP", SIWX91X_CLK_ULP_UART,        "ULP_UART"        },
	{ "ULP", SIWX91X_CLK_ULP_I2C,         "ULP_I2C"         },
	{ "ULP", SIWX91X_CLK_ULP_UDMA,        "ULP_UDMA"        },
	{ "ULP", SIWX91X_CLK_ULP_I2S,         "ULP_I2S"         },
	{ "ULP", SIWX91X_CLK_ULP_STATIC_I2S,  "ULP_STATIC_I2S"  },
	{ "ULP", SIWX91X_CLK_ULP_ADC,         "ULP_ADC"         },
	{ "ULP", SIWX91X_CLK_ULP_SSI,         "ULP_SSI"         },
	{ "ULP", SIWX91X_CLK_ULP_TIMER,       "ULP_TIMER"       },
	{ "ULP", SIWX91X_CLK_ULP_REF_AON,     "ULP_REF_AON"     },
	{ "ULP", SIWX91X_CLK_ULP_GPIO,        "ULP_GPIO"        },
};

static const char *siwx91x_shell_clock_status_str(enum clock_control_status status)
{
	switch (status) {
	case CLOCK_CONTROL_STATUS_ON:
		return "on";
	case CLOCK_CONTROL_STATUS_OFF:
		return "off";
	case CLOCK_CONTROL_STATUS_STARTING:
		return "starting";
	default:
		return "unknown";
	}
}

static int siwx91x_shell_clock_query(uint32_t clkid, enum clock_control_status *status,
				     uint32_t *rate_hz)
{
	const struct device *dev = siwx91x_clock_control_get_device(clkid);

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	*status = clock_control_get_status(dev, (clock_control_subsys_t)clkid);
	return clock_control_get_rate(dev, (clock_control_subsys_t)clkid, rate_hz);
}

static int siwx91x_shell_format_rate(char *buf, size_t len, int rate_ret, uint32_t rate_hz)
{
	if (rate_ret != 0) {
		return snprintk(buf, len, "unknown");
	}

	if (rate_hz == 0U) {
		return snprintk(buf, len, "%7s", "Gated");
	}

	if ((rate_hz % 1000000U) == 0U) {
		return snprintk(buf, len, "%7u MHz", rate_hz / 1000000U);
	}

	if ((rate_hz % 1000U) == 0U) {
		return snprintk(buf, len, "%7u kHz", rate_hz / 1000U);
	}

	return snprintk(buf, len, "%7u Hz", rate_hz);
}

static int cmd_clocks(const struct shell *sh, size_t argc, char **argv)
{
	const struct siwx91x_clock_shell_entry *entry;
	enum clock_control_status status;
	char rate_buf[16];
	uint32_t rate_hz;
	int rate_ret;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "clkid domain name              status          rate");
	shell_print(sh, "----- ------ ---------------- ---------- -----------");

	for (size_t i = 0; i < ARRAY_SIZE(siwx91x_clocks); i++) {
		entry = &siwx91x_clocks[i];
		rate_hz = 0;

		rate_ret = siwx91x_shell_clock_query(entry->clkid, &status, &rate_hz);

		if (rate_ret == -ENODEV) {
			shell_print(sh, "%3u %-6s %-16s %-10s %s", entry->clkid, entry->domain,
				    entry->name, "n/a", "device not ready");
			continue;
		}

		siwx91x_shell_format_rate(rate_buf, sizeof(rate_buf), rate_ret, rate_hz);
		shell_print(sh, "%3u %-6s %-16s %-10s %s", entry->clkid, entry->domain, entry->name,
			    siwx91x_shell_clock_status_str(status), rate_buf);
	}

	shell_print(sh, "");
	shell_print(sh, " - Rate \"unknown\":   The clock driver could not determine the "
			"frequency (register return unknown value of the ref clock)");
	shell_print(sh, " - Rate \"Gated\":     The clock is currently disabled (the mux select "
			"the GATED clock)");
	shell_print(sh, " - Status values:");
	shell_print(sh, "      on:        Clock is enabled and running");
	shell_print(sh, "      off:       Clock is disabled/gated");
	shell_print(sh, "      unknown:   Clock status (enable/disable) could not be determined");
	shell_print(sh, "      n/a:       Not applicable (e.g., device or feature not present)");
	shell_print(sh, "");
	shell_print(sh, "Clock wiring diagram: west build -t clock_tree");

	return 0;
}

SHELL_SUBCMD_ADD((silabs), clocks, NULL, "Print all SiWx91x clocks and their status", cmd_clocks,
		 1, 0);
