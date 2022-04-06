/**
 *  drivers/clk/faraday/clk-tx5.c
 *
 *  Faraday TX5 Clock Tree
 *
 *  Copyright (C) 2022 Faraday Technology
 *  Copyright (C) 2022 Bo-Cun Chen <bcchen@faraday-tech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/clkdev.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>

#include <asm/io.h>

#include <dt-bindings/clock/clock-tx5.h>

#include "clk.h"

static const struct faraday_fixed_rate_clock tx5_fixed_rate_clks[] = {
	{ TX5_FIXED_25M,   "osc0", NULL,  25000000, 0 },
	{ TX5_FIXED_40M,   "osc1", NULL,  40000000, 0 },
	{ TX5_FIXED_48M,   "osc2", NULL,  48000000, 0 },
	{ TX5_FIXED_100M,  "osc3", NULL, 100000000, 0 },
	{ TX5_FIXED_32768, "osc4", NULL,     32768, 0 },
};

static const struct faraday_pll_clock tx5_pll_clks[] = {
	{ TX5_PLL_CPU,  "pll_cpu",  "osc2_div4", 0x0030, 0, 8, 3, 16, 9, 12, 4, CLK_IS_CRITICAL },
	{ TX5_PLL_BUS,  "pll_bus",  "osc2_div4", 0x0034, 0, 0, 3,  8, 9,  4, 4, CLK_IS_CRITICAL },
	{ TX5_PLL_LVDS, "pll_lvds", "osc2_div4", 0x0884, 0, 0, 3,  8, 9,  4, 4, CLK_IS_CRITICAL },
	{ TX5_PLL_GMAC, "pll_gmac", "osc2_div4", 0x0888, 0, 0, 3,  8, 9,  4, 4, CLK_IS_CRITICAL },
};

static const struct faraday_fixed_factor_clock tx5_fixed_factor_clks[] = {
	{ TX5_FIXED_48M_DIV4,  "osc2_div4",       "osc2",               1,   4, CLK_SET_RATE_PARENT },
	{ TX5_FIXED_48M_DIV48, "osc2_div48",      "osc2",               1,  48, CLK_SET_RATE_PARENT },
	{ TX5_PLL_CPU_DIV2,    "pll_cpu_div2",    "pll_cpu",            1,   2, CLK_SET_RATE_PARENT },
	{ TX5_PLL_CPU_DIV3,    "pll_cpu_div3",    "pll_cpu",            1,   3, CLK_SET_RATE_PARENT },
	{ TX5_PLL_CPU_DIV6,    "pll_cpu_div6",    "pll_cpu",            1,   6, CLK_SET_RATE_PARENT },
	{ TX5_PLL_BUS_DIV2,    "pll_bus_div2",    "pll_bus",            1,   2, CLK_SET_RATE_PARENT },
	{ TX5_PLL_BUS_DIV4,    "pll_bus_div4",    "pll_bus",            1,   4, CLK_SET_RATE_PARENT },
	{ TX5_PLL_BUS_DIV8,    "pll_bus_div8",    "pll_bus",            1,   8, CLK_SET_RATE_PARENT },
	{ TX5_PLL_BUS_DIV16,   "pll_bus_div16",   "pll_bus",            1,  16, CLK_SET_RATE_PARENT },
	{ TX5_PLL_BUS_DIV10,   "pll_bus_div10",   "pll_bus",            1,  10, CLK_SET_RATE_PARENT },
	{ TX5_PLL_GMAC_DIV5,   "pll_gmac_div5",   "pll_gmac",           1,   5, CLK_SET_RATE_PARENT },
	{ TX5_PLL_GMAC_DIV12,  "pll_gmac_div12",  "pll_gmac",           1,  12, CLK_SET_RATE_PARENT },
	{ TX5_PLL_GMAC_DIV600, "pll_gmac_div600", "pll_gmac",           1, 600, CLK_SET_RATE_PARENT },
	{ TX5_PLL_GMAC_DIV25,  "pll_gmac_div25",  "pll_gmac",           1,  25, CLK_SET_RATE_PARENT },
	{ TX5_PLL_DDR4,        "pll_ddr4",        "osc2",              25,   2, CLK_SET_RATE_PARENT },
	{ TX5_CPU_FCLK,        "cpu_fclk",        "cpu_fclk_mux0",      1,   1, CLK_SET_RATE_PARENT },
	{ TX5_CPU_ACLK,        "cpu_aclk",        "cpu_aclk_mux0",      1,   1, CLK_SET_RATE_PARENT },
	{ TX5_CPU_PCLK,        "cpu_pclk",        "pll_cpu_div6",       1,   1, CLK_SET_RATE_PARENT },
	{ TX5_CPU_TRACECLK,    "cpu_traceclk",    "pll_cpu_div6",       1,   2, CLK_SET_RATE_PARENT },
	{ TX5_GPU_CLK2X,       "gpu_clk2x",       "gpu_clk2x_en",       1,   1, CLK_SET_RATE_PARENT },
	{ TX5_LVDS_PIXEL_CLK,  "lvds_pixel_clk",  "pll_lvds",           1,   1, CLK_SET_RATE_PARENT },
	{ TX5_AXI_CLK,         "axi_clk",         "busclk_mux0",        1,   1, CLK_SET_RATE_PARENT },
	{ TX5_AHB_CLK,         "ahb_clk",         "busclk_mux1",        1,   1, CLK_SET_RATE_PARENT },
	{ TX5_APB_CLK,         "apb_clk",         "busclk_mux2",        1,   1, CLK_SET_RATE_PARENT },
	{ TX5_SD0_SDCLK2X,     "sd0_sdclk2x",     "pll_bus",            1,   1, CLK_SET_RATE_PARENT },
	{ TX5_SD0_SDCLK1X,     "sd0_sdclk1x",     "sd0_sdclk1x_en",     1,   1, CLK_SET_RATE_PARENT },
	{ TX5_SD1_SDCLK2X,     "sd1_sdclk2x",     "pll_bus",            1,   1, CLK_SET_RATE_PARENT },
	{ TX5_SD1_SDCLK1X,     "sd1_sdclk1x",     "sd1_sdclk1x_en",     1,   1, CLK_SET_RATE_PARENT },
	{ TX5_SPICLK,          "spiclk",          "spiclk_en",          1,   1, CLK_SET_RATE_PARENT },
	{ TX5_SSP0_SSPCLK,     "ssp0_sspclk",     "ssp0_sspclk_en",     1,   1, CLK_SET_RATE_PARENT },
	{ TX5_SSP1_SSPCLK,     "ssp1_sspclk",     "ssp1_sspclk_en",     1,   1, CLK_SET_RATE_PARENT },
	{ TX5_SSP2_SSPCLK,     "ssp2_sspclk",     "ssp2_sspclk_en",     1,   1, CLK_SET_RATE_PARENT },
	{ TX5_SSP3_SSPCLK,     "ssp3_sspclk",     "ssp3_sspclk_en",     1,   1, CLK_SET_RATE_PARENT },
	{ TX5_ADC_MCLK,        "adc_mclk",        "adc_mclk_en",        1,   1, CLK_SET_RATE_PARENT },
	{ TX5_PWMTMR0_EXTCLK0, "pwmtmr0_extclk0", "pwmtmr0_extclk_en",  1,   1, CLK_SET_RATE_PARENT },
	{ TX5_PWMTMR0_EXTCLK1, "pwmtmr0_extclk1", "pwmtmr0_extclk_en",  1,   1, CLK_SET_RATE_PARENT },
	{ TX5_PWMTMR0_EXTCLK2, "pwmtmr0_extclk2", "pwmtmr0_extclk_en",  1,   1, CLK_SET_RATE_PARENT },
	{ TX5_PWMTMR0_EXTCLK3, "pwmtmr0_extclk3", "pwmtmr0_extclk_en",  1,   1, CLK_SET_RATE_PARENT },
	{ TX5_PWMTMR0_EXTCLK4, "pwmtmr0_extclk4", "pwmtmr0_extclk_en",  1,   1, CLK_SET_RATE_PARENT },
	{ TX5_PWMTMR0_EXTCLK5, "pwmtmr0_extclk5", "pwmtmr0_extclk_en",  1,   1, CLK_SET_RATE_PARENT },
	{ TX5_PWMTMR0_EXTCLK6, "pwmtmr0_extclk6", "pwmtmr0_extclk_en",  1,   1, CLK_SET_RATE_PARENT },
	{ TX5_PWMTMR0_EXTCLK7, "pwmtmr0_extclk7", "pwmtmr0_extclk_en",  1,   1, CLK_SET_RATE_PARENT },
	{ TX5_PWMTMR1_EXTCLK0, "pwmtmr1_extclk0", "pwmtmr1_extclk_en",  1,   1, CLK_SET_RATE_PARENT },
	{ TX5_PWMTMR1_EXTCLK1, "pwmtmr1_extclk1", "pwmtmr1_extclk_en",  1,   1, CLK_SET_RATE_PARENT },
	{ TX5_PWMTMR1_EXTCLK2, "pwmtmr1_extclk2", "pwmtmr1_extclk_en",  1,   1, CLK_SET_RATE_PARENT },
	{ TX5_PWMTMR1_EXTCLK3, "pwmtmr1_extclk3", "pwmtmr1_extclk_en",  1,   1, CLK_SET_RATE_PARENT },
	{ TX5_PWMTMR1_EXTCLK4, "pwmtmr1_extclk4", "pwmtmr1_extclk_en",  1,   1, CLK_SET_RATE_PARENT },
	{ TX5_PWMTMR1_EXTCLK5, "pwmtmr1_extclk5", "pwmtmr1_extclk_en",  1,   1, CLK_SET_RATE_PARENT },
	{ TX5_PWMTMR1_EXTCLK6, "pwmtmr1_extclk6", "pwmtmr1_extclk_en",  1,   1, CLK_SET_RATE_PARENT },
	{ TX5_PWMTMR1_EXTCLK7, "pwmtmr1_extclk7", "pwmtmr1_extclk_en",  1,   1, CLK_SET_RATE_PARENT },
	{ TX5_TMR_EXTCLK0,     "tmr_extclk0",     "tmr_extclk_en",      1,   1, CLK_SET_RATE_PARENT },
	{ TX5_TMR_EXTCLK1,     "tmr_extclk1",     "tmr_extclk_en",      1,   1, CLK_SET_RATE_PARENT },
	{ TX5_TMR_EXTCLK2,     "tmr_extclk2",     "tmr_extclk_en",      1,   1, CLK_SET_RATE_PARENT },
	{ TX5_TMR_EXTCLK3,     "tmr_extclk3",     "tmr_extclk_en",      1,   1, CLK_SET_RATE_PARENT },
	{ TX5_WDT0_EXTCLK,     "wdt0_extclk",     "wdt_extclk_en",      1,   1, CLK_SET_RATE_PARENT },
	{ TX5_WDT1_EXTCLK,     "wdt1_extclk",     "wdt_extclk_en",      1,   1, CLK_SET_RATE_PARENT },
	{ TX5_GMAC0_ACLK,      "gmac0_aclk",      "axi_clk",            1,   2, CLK_SET_RATE_PARENT },
	{ TX5_GMAC0_CLK25M,    "gmac0_clk25m",    "gmac0_clk25m_en",    1,   1, CLK_SET_RATE_PARENT },
	{ TX5_GMAC0_PTP,       "gmac0_ptp",       "gmac0_ptp_en",       1,   1, CLK_SET_RATE_PARENT },
	{ TX5_GMAC0_CLK125M,   "gmac0_clk125m",   "gmac0_clk125m_en",   1,   1, CLK_SET_RATE_PARENT },
	{ TX5_GMAC0_CLK2P5M,   "gmac0_clk2p5m",   "gmac0_clk2p5m_en",   1,   1, CLK_SET_RATE_PARENT },
	{ TX5_GMAC1_ACLK,      "gmac1_aclk",      "axi_clk",            1,   2, CLK_SET_RATE_PARENT },
	{ TX5_GMAC1_CLK25M,    "gmac1_clk25m",    "gmac1_clk25m_en",    1,   1, CLK_SET_RATE_PARENT },
	{ TX5_GMAC1_PTP,       "gmac1_ptp",       "gmac1_ptp_en",       1,   1, CLK_SET_RATE_PARENT },
	{ TX5_GMAC1_CLK125M,   "gmac1_clk125m",   "gmac1_clk125m_en",   1,   1, CLK_SET_RATE_PARENT },
	{ TX5_GMAC1_CLK2P5M,   "gmac1_clk2p5m",   "gmac1_clk2p5m_en",   1,   1, CLK_SET_RATE_PARENT },
	{ TX5_GPU_ACLK,        "gpu_aclk",        "gpu_aclk_en",        1,   1, CLK_SET_RATE_PARENT },
	{ TX5_USB3_ACLK,       "usb3_aclk",       "usb3_aclk_en",       1,   1, CLK_SET_RATE_PARENT },
	{ TX5_USB3_CLK60M,     "usb3_clk60m",     "usb3_clk60m_en",     1,   1, CLK_SET_RATE_PARENT },
	{ TX5_USB3_LPCLK,      "usb3_lpclk",      "usb3_lpclk_en",      1,   1, CLK_SET_RATE_PARENT },
	{ TX5_DDR_CLK,         "ddr_clk",         "ddr_clk_en",         1,   1, CLK_SET_RATE_PARENT },
	{ TX5_UART0_UCLK,      "uart0_uclk",      "uart0_uclk_en",      1,   1, CLK_SET_RATE_PARENT },
	{ TX5_UART1_UCLK,      "uart1_uclk",      "uart1_uclk_en",      1,   1, CLK_SET_RATE_PARENT },
	{ TX5_UART2_UCLK,      "uart2_uclk",      "uart2_uclk_en",      1,   1, CLK_SET_RATE_PARENT },
	{ TX5_UART3_UCLK,      "uart3_uclk",      "uart3_uclk_en",      1,   1, CLK_SET_RATE_PARENT },
	{ TX5_USART0_UCLK,     "usart0_uclk",     "usart0_uclk_en",     1,   1, CLK_SET_RATE_PARENT },
	{ TX5_USART1_UCLK,     "usart1_uclk",     "usart1_uclk_en",     1,   1, CLK_SET_RATE_PARENT },
	{ TX5_TDC_XCLK,        "tdc_xclk",        "tdc_xclk_en",        1,   1, CLK_SET_RATE_PARENT },
	{ TX5_PCIE_REFCLK,     "pcie_refclk",     "osc0",               1,   1, CLK_SET_RATE_PARENT },
};

static const struct faraday_divider_clock tx5_divider_clks[] = {
	{ TX5_CANFD_DEGLI_CLK_DIV,    "canfd_degli_clk_div",    "osc1",           CLK_SET_RATE_PARENT, 0x0800,  0, 10, 5, NULL },
	{ TX5_CANFD_EXT_CLK_DIV,      "canfd_ext_clk_div",      "osc1",           CLK_SET_RATE_PARENT, 0x0800,  0, 12, 0, NULL },
	{ TX5_LVDS_TV_CLK_DIV,        "lvds_tv_clk_div",        "lvds_pixel_clk", CLK_SET_RATE_PARENT, 0x0874,  0,  4, 0, NULL },
	{ TX5_LVDS_LC_SCALER_CLK_DIV, "lvds_lc_scaler_clk_div", "lvds_pixel_clk", CLK_SET_RATE_PARENT, 0x0874,  4,  4, 0, NULL },
	{ TX5_EXTCLK_DIV,             "extclk_div",             "pll_bus_div16",  CLK_SET_RATE_PARENT, 0x088c,  0, 10, 0, NULL },
};

static const char *const cpu_fclk_mux0[] = { "pll_cpu", "pll_cpu_div2" };
static const char *const cpu_aclk_mux0[] = { "pll_cpu_div3", "pll_cpu_div6" };
static const char *const busclk_mux0[] = { "pll_bus", "pll_bus_div2" };
static const char *const busclk_mux1[] = { "pll_bus_div4", "pll_bus_div8" };
static const char *const busclk_mux2[] = { "pll_bus_div8", "pll_bus_div16" };
static u32 mux_table[] = { 0, 1 };

static const struct faraday_mux_clock tx5_mux_clks[] = {
	{ TX5_CPU_FCLK_MUX0, "cpu_fclk_mux0", cpu_fclk_mux0, ARRAY_SIZE(cpu_fclk_mux0),
	  CLK_SET_RATE_PARENT, 0x0020, 16, 1, 0, mux_table },
	{ TX5_CPU_ACLK_MUX0, "cpu_aclk_mux0", cpu_aclk_mux0, ARRAY_SIZE(cpu_aclk_mux0),
	  CLK_SET_RATE_PARENT, 0x0020, 16, 1, 0, mux_table },
	{ TX5_BUSCLK_MUX0, "busclk_mux0", busclk_mux0, ARRAY_SIZE(busclk_mux0),
	  CLK_SET_RATE_PARENT, 0x0020, 20, 1, 0, mux_table },
	{ TX5_BUSCLK_MUX1, "busclk_mux1", busclk_mux1, ARRAY_SIZE(busclk_mux1),
	  CLK_SET_RATE_PARENT, 0x0020, 21, 1, 0, mux_table },
	{ TX5_BUSCLK_MUX2, "busclk_mux2", busclk_mux2, ARRAY_SIZE(busclk_mux2),
	  CLK_SET_RATE_PARENT, 0x0020, 22, 1, 0, mux_table },
};

static const struct faraday_gate_clock tx5_gate_clks[] = {
	/* hclk & other clock gating control(0x50) */
	{ TX5_DDR_CLK_EN,               "ddr_clk_en",               "pll_ddr4",               0, 0x0050, 28, 0 },
	{ TX5_LVDS_LC_SCALER_CLK_EN,    "lvds_lc_scaler_clk_en",    "lvds_lc_scaler_clk_div", 0, 0x0050, 27, 0 },
	{ TX5_LVDS_TV_CLK_EN,           "lvds_tv_clk_en",           "lvds_tv_clk_div",        0, 0x0050, 26, 0 },
	{ TX5_LVDS_LC_CLK_EN,           "lvds_lc_clk_en",           "lvds_pixel_clk",         0, 0x0050, 25, 0 },
	{ TX5_GPU_ACLK_EN,              "gpu_aclk_en",              "pll_gmac_div5",          0, 0x0050, 24, 0 },
	{ TX5_GPU_CLK2X_EN,             "gpu_clk2x_en",             "pll_cpu_div2",           0, 0x0050, 23, 0 },
	{ TX5_GMAC1_PTP_EN,             "gmac1_ptp_en",             "pll_bus_div16",          0, 0x0050, 22, 0 },
	{ TX5_GMAC1_CLK2P5M_EN,         "gmac1_clk2p5m_en",         "pll_gmac_div600",        0, 0x0050, 21, 0 },
	{ TX5_GMAC1_CLK25M_EN,          "gmac1_clk25m_en",          "pll_bus_div16",          0, 0x0050, 20, 0 },
	{ TX5_GMAC1_CLK125M_EN,         "gmac1_clk125m_en",         "pll_gmac_div12",         0, 0x0050, 19, 0 },
	{ TX5_GMAC0_PTP_EN,             "gmac0_ptp_en",             "pll_bus_div16",          0, 0x0050, 18, 0 },
	{ TX5_GMAC0_CLK2P5M_EN,         "gmac0_clk2p5m_en",         "pll_gmac_div600",        0, 0x0050, 17, 0 },
	{ TX5_GMAC0_CLK25M_EN,          "gmac0_clk25m_en",          "pll_bus_div16",          0, 0x0050, 16, 0 },
	{ TX5_GMAC0_CLK125M_EN,         "gmac0_clk125m_en",         "pll_gmac_div12",         0, 0x0050, 15, 0 },
	{ TX5_CANFD_DEGLI_CLK_EN,       "canfd_degli_clk_en",       "canfd_degli_clk_div",    0, 0x0050, 14, 0 },
	{ TX5_CANFD_EXT_CLK_EN,         "canfd_ext_clk_en",         "canfd_ext_clk_div",      0, 0x0050, 13, 0 },
	{ TX5_CAN2_OSC_EN,              "can2_osc_en",              "osc1",                   0, 0x0050, 12, 0 },
	{ TX5_CAN1_OSC_EN,              "can1_osc_en",              "osc1",                   0, 0x0050, 11, 0 },
	{ TX5_CAN0_OSC_EN,              "can0_osc_en",              "osc1",                   0, 0x0050, 10, 0 },
	{ TX5_TDC_XCLK_EN,              "tdc_xclk_en",              "osc2_div48",             0, 0x0050,  9, 0 },
	{ TX5_ADC_MCLK_EN,              "adc_mclk_en",              "pll_bus_div4",           0, 0x0050,  8, 0 },
	{ TX5_SEC_HCLK_EN,              "sec_hclk_en",              "ahb_clk",                0, 0x0050,  7, 0 },
	{ TX5_SD1_HCLK_EN,              "sd1_hclk_en",              "ahb_clk",                0, 0x0050,  6, 0 },
	{ TX5_SD0_HCLK_EN,              "sd0_hclk_en",              "ahb_clk",                0, 0x0050,  5, 0 },
	{ TX5_NIC_HCLK_EN,              "nic_hclk_en",              "ahb_clk",                0, 0x0050,  4, 0 },
	{ TX5_CAN2_HCLK_EN,             "can2_hclk_en",             "ahb_clk",                0, 0x0050,  3, 0 },
	{ TX5_CAN1_HCLK_EN,             "can1_hclk_en",             "ahb_clk",                0, 0x0050,  2, 0 },
	{ TX5_CAN0_HCLK_EN,             "can0_hclk_en",             "ahb_clk",                0, 0x0050,  1, 0 },
	{ TX5_GPU_HCLK_EN,              "gpu_hclk_en",              "ahb_clk",                0, 0x0050,  0, 0 },
	/* hclk & other clock gating control(0x54) */
	{ TX5_MOTOR_EXADC_SYS40MCLK_EN, "motor_exadc_sys40mclk_en", "pll_bus_div10",          0, 0x0050, 25, 0 },
	{ TX5_MOTOR_EQEP_SYSCLK_EN,     "motor_eqep_sysclk_en",     "pll_bus_div4",           0, 0x0050, 24, 0 },
	{ TX5_MOTOR_EPWM_SYSCLK_EN,     "motor_epwm_sysclk_en",     "pll_bus_div2",           0, 0x0050, 23, 0 },
	{ TX5_MOTOR_ENDAT_SYSCLK_EN,    "motor_endat_sysclk_en",    "pll_bus_div4",           0, 0x0050, 22, 0 },
	{ TX5_PCIE_AUXCLK_EN,           "pcie_auxclk_en",           "osc2_div4",              0, 0x0050, 21, 0 },
	{ TX5_USART1_UCLK_EN,           "usart1_uclk_en",           "osc2",                   0, 0x0054, 20, 0 },
	{ TX5_USART0_UCLK_EN,           "usart0_uclk_en",           "osc2",                   0, 0x0054, 19, 0 },
	{ TX5_UART3_UCLK_EN,            "uart3_uclk_en",            "osc2",                   0, 0x0054, 18, 0 },
	{ TX5_UART2_UCLK_EN,            "uart2_uclk_en",            "osc2",                   0, 0x0054, 17, 0 },
	{ TX5_UART1_UCLK_EN,            "uart1_uclk_en",            "osc2",                   0, 0x0054, 16, 0 },
	{ TX5_UART0_UCLK_EN,            "uart0_uclk_en",            "osc2",                   0, 0x0054, 15, 0 },
	{ TX5_SSP3_SSPCLK_EN,           "ssp3_sspclk_en",           "pll_bus_div2",           0, 0x0054, 14, 0 },
	{ TX5_SSP2_SSPCLK_EN,           "ssp2_sspclk_en",           "pll_bus_div2",           0, 0x0054, 13, 0 },
	{ TX5_SSP1_SSPCLK_EN,           "ssp1_sspclk_en",           "pll_bus_div2",           0, 0x0054, 12, 0 },
	{ TX5_SSP0_SSPCLK_EN,           "ssp0_sspclk_en",           "pll_bus_div2",           0, 0x0054, 11, 0 },
	{ TX5_SEC_RSA_CLK_EN,           "sec_rsa_clk_en",           "pll_bus_div8",           0, 0x0054, 10, 0 },
	{ TX5_SPICLK_EN,                "spiclk_en",                "pll_bus_div2",           0, 0x0054,  9, 0 },
	{ TX5_SD1_SDCLK1X_EN,           "sd1_sdclk1x_en",           "pll_bus_div2",           0, 0x0054,  8, 0 },
	{ TX5_SD0_SDCLK1X_EN,           "sd0_sdclk1x_en",           "pll_bus_div2",           0, 0x0054,  7, 0 },
	{ TX5_WDT_EXTCLK_EN,            "wdt_extclk_en",            "extclk_div",             0, 0x0054,  6, 0 },
	{ TX5_TMR_EXTCLKEN,             "tmr_extclk_en",            "extclk_div",             0, 0x0054,  5, 0 },
	{ TX5_PWMTMR1_EXTCLK_EN,        "pwmtmr1_extclk_en",        "extclk_div",             0, 0x0054,  4, 0 },
	{ TX5_PWMTMR0_EXTCLK_EN,        "pwmtmr0_extclk_en",        "extclk_div",             0, 0x0054,  3, 0 },
	{ TX5_USB3_ACLK_EN,             "usb3_aclk_en",             "pll_gmac_div5",          0, 0x0054,  2, 0 },
	{ TX5_USB3_LPCLK_EN,            "usb3_lpclk_en",            "osc2_div4",              0, 0x0054,  1, 0 },
	{ TX5_USB3_CLK60M_EN,           "usb3_clk60m_en",           "pll_gmac_div25",         0, 0x0054,  0, 0 },
};

static int tx5_clocks_probe(struct platform_device *pdev)
{
	struct faraday_clock_data *clk_data;
	struct resource *res;
	int ret;

	clk_data = faraday_clk_alloc(pdev, FARADAY_NR_CLKS);
	if (!clk_data)
		return -ENOMEM;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "scu");
	if (!res)
		return -ENODEV;
	clk_data->base = devm_ioremap(&pdev->dev,
	                              res->start, resource_size(res));
	if (!clk_data->base)
		return -EBUSY;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "ext_reg");
	if (res) {
		clk_data->extbase = devm_ioremap(&pdev->dev,
		                                 res->start, resource_size(res));
	}

	ret = faraday_clk_register_fixed_rates(tx5_fixed_rate_clks,
	                                       ARRAY_SIZE(tx5_fixed_rate_clks),
	                                       clk_data);
	if (ret)
		return -EINVAL;

	ret = faraday_clk_register_plls(tx5_pll_clks,
	                                ARRAY_SIZE(tx5_pll_clks),
	                                clk_data->base,
	                                clk_data);
	if (ret)
		goto unregister_fixed_rates;

	ret = faraday_clk_register_dividers(tx5_divider_clks,
	                                    ARRAY_SIZE(tx5_divider_clks),
	                                    clk_data->base,
	                                    clk_data);
	if (ret)
		goto unregister_plls;

	ret = faraday_clk_register_fixed_factors(tx5_fixed_factor_clks,
	                                         ARRAY_SIZE(tx5_fixed_factor_clks),
	                                         clk_data);
	if (ret)
		goto unregister_dividers;

	ret = faraday_clk_register_muxs(tx5_mux_clks,
	                                ARRAY_SIZE(tx5_mux_clks),
	                                clk_data->base,
	                                clk_data);
	if (ret)
		goto unregister_fixed_factors;

	ret = faraday_clk_register_gates(tx5_gate_clks,
	                                 ARRAY_SIZE(tx5_gate_clks),
	                                 clk_data->base,
	                                 clk_data);
	if (ret)
		goto unregister_muxs;

	ret = of_clk_add_provider(pdev->dev.of_node,
	                          of_clk_src_onecell_get, &clk_data->clk_data);
	if (ret)
		goto unregister_gates;

	platform_set_drvdata(pdev, clk_data);

	return 0;

unregister_gates:
	faraday_clk_unregister_gates(tx5_gate_clks,
				ARRAY_SIZE(tx5_gate_clks),
				clk_data);
unregister_muxs:
	faraday_clk_unregister_muxs(tx5_mux_clks,
				ARRAY_SIZE(tx5_mux_clks),
				clk_data);
unregister_fixed_factors:
	faraday_clk_unregister_fixed_factors(tx5_fixed_factor_clks,
				ARRAY_SIZE(tx5_fixed_factor_clks),
				clk_data);
unregister_dividers:
	faraday_clk_unregister_dividers(tx5_divider_clks,
				ARRAY_SIZE(tx5_divider_clks),
				clk_data);
unregister_plls:
	faraday_clk_unregister_plls(tx5_pll_clks,
				ARRAY_SIZE(tx5_pll_clks),
				clk_data);
unregister_fixed_rates:
	faraday_clk_unregister_fixed_rates(tx5_fixed_rate_clks,
				ARRAY_SIZE(tx5_fixed_rate_clks),
				clk_data);
	return -EINVAL;
}

static int tx5_clocks_remove(struct platform_device *pdev)
{
	struct faraday_clock_data *clk_data = platform_get_drvdata(pdev);

	of_clk_del_provider(pdev->dev.of_node);

	faraday_clk_unregister_gates(tx5_gate_clks,
				ARRAY_SIZE(tx5_gate_clks),
				clk_data);
	faraday_clk_unregister_muxs(tx5_mux_clks,
				ARRAY_SIZE(tx5_mux_clks),
				clk_data);
	faraday_clk_unregister_fixed_factors(tx5_fixed_factor_clks,
				ARRAY_SIZE(tx5_fixed_factor_clks),
				clk_data);
	faraday_clk_unregister_dividers(tx5_divider_clks,
				ARRAY_SIZE(tx5_divider_clks),
				clk_data);
	faraday_clk_unregister_plls(tx5_pll_clks,
				ARRAY_SIZE(tx5_pll_clks),
				clk_data);
	faraday_clk_unregister_fixed_rates(tx5_fixed_rate_clks,
				ARRAY_SIZE(tx5_fixed_rate_clks),
				clk_data);

	return 0;
}

static const struct of_device_id tx5_clocks_match_table[] = {
	{ .compatible = "faraday,tx5-clk" },
	{ }
};
MODULE_DEVICE_TABLE(of, tx5_clocks_match_table);

static struct platform_driver tx5_clocks_driver = {
	.probe      = tx5_clocks_probe,
	.remove     = tx5_clocks_remove,
	.driver     = {
		.name   = "tx5-clk",
		.owner  = THIS_MODULE,
		.of_match_table = tx5_clocks_match_table,
	},
};

static int __init tx5_clocks_init(void)
{
	return platform_driver_register(&tx5_clocks_driver);
}
core_initcall(tx5_clocks_init);

static void __exit tx5_clocks_exit(void)
{
	platform_driver_unregister(&tx5_clocks_driver);
}
module_exit(tx5_clocks_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("B.C. Chen <bcchen@faraday-tech.com>");
MODULE_DESCRIPTION("Faraday TX5 Clock Driver");
