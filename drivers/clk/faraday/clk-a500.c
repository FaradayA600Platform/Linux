/**
 *  drivers/clk/faraday/clk-a500.c
 *
 *  Faraday A500 Clock Tree
 *
 *  Copyright (C) 2021 Faraday Technology
 *  Copyright (C) 2021 Bo-Cun Chen <bcchen@faraday-tech.com>
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
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>

#include <asm/io.h>

#include <dt-bindings/clock/clock-a500.h>

#include "clk.h"

static const struct faraday_fixed_rate_clock a500_fixed_rate_clks[] = {
	{ A500_FIXED_30M, "osc0", NULL, 30000000, 0 },
};

static const struct faraday_pll_clock a500_pll_clks[] = {
	/* PLL0 Control Register */
	{ A500_PLL0, "pll0", "osc0", 0x0030, 0, 16, 3, 23, 9,  8, 4, CLK_IS_CRITICAL },
	/* PLL1 Control Register */
	{ A500_PLL1, "pll1", "osc0", 0x8330, 0,  8, 3, 16, 9, 28, 4, CLK_IS_CRITICAL },
	/* PLL2 Control Register */
	{ A500_PLL2, "pll2", "osc0", 0x8338, 0,  8, 3, 16, 9, 28, 4, 0 },
	/* PLL3 Control Register */
	{ A500_PLL3, "pll3", "osc0", 0x8340, 0,  8, 3, 16, 9, 28, 4, CLK_IS_CRITICAL },
};

static const struct faraday_fixed_factor_clock a500_fixed_factor_clks[] = {
	{ A500_CA53_0_CLK,   "ca53_0_clk",   "pll0_div", 1, 1, CLK_SET_RATE_PARENT },
	{ A500_CA53_1_CLK,   "ca53_1_clk",   "pll1_div", 1, 1, CLK_SET_RATE_PARENT },
	{ A500_SDCLK1X,      "sdclk1x",      "sdc_clk",  1, 2, CLK_SET_RATE_PARENT },
	{ A500_USB3_CORECLK, "usb3_coreclk", "usb3_mux", 1, 1, CLK_SET_RATE_PARENT },
};

static const struct faraday_divider_clock a500_divider_clks[] = {
	/* CA53 Clock Counter Register 0 */
	{ A500_CS_ACLK,       "cs_aclk",       "cs_gic_mux",        CLK_SET_RATE_PARENT, 0x8100, 28, 3, 0, NULL },
	{ A500_CA53_1_ACLK,   "ca53_1_aclk",   "pll1_div",          CLK_SET_RATE_PARENT, 0x8100, 24, 3, 0, NULL },
	{ A500_CA53_0_ACLK,   "ca53_0_aclk",   "pll0_div",          CLK_SET_RATE_PARENT, 0x8100, 20, 3, 0, NULL },
	{ A500_SYS_2_CLK,     "sys_2_clk",     "pll1",              CLK_SET_RATE_PARENT, 0x8100, 16, 3, 0, NULL },
	{ A500_SYS_1_CLK,     "sys_1_clk",     "pll1",              CLK_SET_RATE_PARENT, 0x8100, 12, 3, 0, NULL },
	{ A500_SYS_0_CLK,     "sys_0_clk",     "pll0",              CLK_SET_RATE_PARENT, 0x8100,  8, 3, 0, NULL },
	{ A500_PLL1_DIV,      "pll1_div",      "pll1",              CLK_SET_RATE_PARENT, 0x8100,  4, 3, 0, NULL },
	{ A500_PLL0_DIV,      "pll0_div",      "pll0",              CLK_SET_RATE_PARENT, 0x8100,  0, 3, 0, NULL },
	/* CA53 Clock Counter Register 1 */
	{ A500_X2P_PCLK,      "x2p_pclk",      "axic_1_aclk",       CLK_SET_RATE_PARENT, 0x8104, 28, 3, 0, NULL },
	{ A500_H2P_PCLK,      "h2p_pclk",      "axic_1_aclk",       CLK_SET_RATE_PARENT, 0x8104, 24, 3, 0, NULL },
	{ A500_SYS_HCLK,      "sys_hclk",      "axic_1_aclk",       CLK_SET_RATE_PARENT, 0x8104, 20, 3, 0, NULL },
	{ A500_PERI_1_CLK,    "peri_1_clk",    "axic_1_bypass_mux", CLK_SET_RATE_PARENT, 0x8104, 16, 3, 0, NULL },
	{ A500_PERI_0_CLK,    "peri_0_clk",    "axic_0_bypass_mux", CLK_SET_RATE_PARENT, 0x8104, 12, 3, 0, NULL },
	{ A500_AXIC_1_ACLK,   "axic_1_aclk",   "axic_1_bypass_mux", CLK_SET_RATE_PARENT, 0x8104,  8, 3, 0, NULL },
	{ A500_AXIC_0_ACLK,   "axic_0_aclk",   "axic_0_bypass_mux", CLK_SET_RATE_PARENT, 0x8104,  4, 3, 0, NULL },
	{ A500_GIC_ACLK,      "gic_aclk",      "cs_gic_mux",        CLK_SET_RATE_PARENT, 0x8104,  0, 3, 0, NULL },
	/* CA53 Clock Counter Register 2 */
	{ A500_GMAC0_CLK2P5M, "gmac0_clk2p5m", "gmac0_clk125m",     CLK_SET_RATE_PARENT, 0x8108, 24, 6, 0, NULL },
	{ A500_GMAC1_CLK25M,  "gmac1_clk25m",  "gmac1_g",           CLK_SET_RATE_PARENT, 0x8108, 16, 6, 0, NULL },
	{ A500_GMAC0_CLK25M,  "gmac0_clk25m",  "gmac0_g",           CLK_SET_RATE_PARENT, 0x8108,  8, 6, 0, NULL },
	{ A500_GMAC1_CLK125M, "gmac1_clk125m", "gmac1_g",           CLK_SET_RATE_PARENT, 0x8108,  4, 4, 0, NULL },
	{ A500_GMAC0_CLK125M, "gmac0_clk125m", "gmac0_g",           CLK_SET_RATE_PARENT, 0x8108,  0, 4, 0, NULL },
	/* CA53 Clock Counter Register 3 */
	{ A500_USB3_CLK60M,   "usb3_clk60m",   "usb3_g",            CLK_SET_RATE_PARENT, 0x810c, 24, 6, 0, NULL },
	{ A500_USB3_LP_CLK,   "usb3_lp_clk",   "usb3_g",            CLK_SET_RATE_PARENT, 0x810c, 16, 6, 0, NULL },
	{ A500_USB3_CLK30M,   "usb3_clk30m",   "usb3_g",            CLK_SET_RATE_PARENT, 0x810c,  8, 6, 0, NULL },
	{ A500_GMAC1_CLK2P5M, "gmac1_clk2p5m", "gmac1_clk125m",     CLK_SET_RATE_PARENT, 0x810c,  0, 6, 0, NULL },
	/* CA53 Clock Counter Register 4 */
	{ A500_TDC_XCLK,      "tdc_xclk",      "tdc_xclk_g",        CLK_SET_RATE_PARENT, 0x8110, 24, 6, 0, NULL },
	{ A500_EXT_AHB_CLK,   "ext_ahb_clk",   "ext_ahb_clk_g",     CLK_SET_RATE_PARENT, 0x8110, 16, 6, 0, NULL },
	{ A500_SPI_CLK,       "spi_clk",       "spi_clk_g",         CLK_SET_RATE_PARENT, 0x8110,  8, 6, 0, NULL },
	{ A500_ADC_MCLK,      "adc_mclk",      "adc_mclk_g",        CLK_SET_RATE_PARENT, 0x8110,  0, 6, 0, NULL },
	/* CA53 Clock Counter Register 5 */
	{ A500_TRACE_CLK,     "trace_clk",     "cs_gic_mux",        CLK_SET_RATE_PARENT, 0x8114, 28, 3, 0, NULL },
	{ A500_EAHB_ACLK,     "eahb_aclk",     "eahb_aclk_g",       CLK_SET_RATE_PARENT, 0x8114, 24, 3, 0, NULL },
	{ A500_PCIE_LP_CLK,   "pcie_lp_clk",   "pcie_lp_clk_g",     CLK_SET_RATE_PARENT, 0x8114, 18, 6, 0, NULL },
	{ A500_LCD_SCACLK,    "lcd_scaclk",    "lcd_scaclk_g",      CLK_SET_RATE_PARENT, 0x8114, 12, 6, 0, NULL },
	{ A500_LCD_PIXCLK,    "lcd_pixclk",    "lcd_pixclk_g",      CLK_SET_RATE_PARENT, 0x8114,  6, 6, 0, NULL },
	{ A500_SDC_CLK,       "sdc_clk",       "sdc_clk_g",         CLK_SET_RATE_PARENT, 0x8114,  0, 6, 0, NULL },
};

static const char *const otg_ck_sel_mux[] = { "sys_1_clk", "sys_2_clk" };
static const char *const axic_1_bypass_mux[] = { "axic_1_mux", "osc0" };
static const char *const axic_1_mux[] = { "sys_0_clk", "sys_1_clk" };
static const char *const eahb_mux[] = { "sys_0_clk", "sys_1_clk" };
static const char *const axic_0_bypass_mux[] = { "axic_0_mux", "osc0" };
static const char *const axic_0_mux[] = { "sys_0_clk", "sys_1_clk" };
static const char *const cs_gic_mux[] = { "ca53_0_aclk", "ca53_1_aclk" };
static const char *const pcie_lp_clk_mux[] = { "peri_0_clk", "peri_1_clk" };
static const char *const sdc_mux[] = { "sys_1_clk", "sys_2_clk" };
static const char *const ext_ahb_mux[] = { "peri_0_clk", "peri_1_clk" };
static const char *const spi_mux[] = { "peri_0_clk", "peri_1_clk" };
static const char *const adc_mux[] = { "peri_0_clk", "peri_1_clk" };
static const char *const usb3_mux[] = { "usb3_clk30m", "osc0" };
static const char *const gmac_mux[] = { "peri_0_clk", "peri_1_clk" };
static u32 mux_table[] = { 0, 1 };

static const struct faraday_mux_clock a500_mux_clks[] = {
	/* Clock Select Control Register */
	{ A500_OTG_CK_SEL_MUX, "otg_ck_sel_mux", otg_ck_sel_mux, ARRAY_SIZE(otg_ck_sel_mux),
	  CLK_SET_RATE_PARENT, 0x812c, 31, 1, 0, mux_table },
	{ A500_AXIC1_BYPASS_MUX, "axic_1_bypass_mux", axic_1_bypass_mux, ARRAY_SIZE(axic_1_bypass_mux),
	  CLK_SET_RATE_PARENT, 0x812c, 21, 1, 0, mux_table },
	{ A500_AXIC1_MUX, "axic_1_mux", axic_1_mux, ARRAY_SIZE(axic_1_mux),
	  CLK_SET_RATE_PARENT, 0x812c, 20, 1, 0, mux_table },
	{ A500_AXIC1_MUX, "eahb_mux", eahb_mux, ARRAY_SIZE(eahb_mux),
	  CLK_SET_RATE_PARENT, 0x812c, 19, 1, 0, mux_table },
	{ A500_AXIC0_BYPASS_MUX, "axic_0_bypass_mux", axic_0_bypass_mux, ARRAY_SIZE(axic_0_bypass_mux),
	  CLK_SET_RATE_PARENT, 0x812c, 18, 1, 0, mux_table },
	{ A500_AXIC0_MUX, "axic_0_mux", axic_0_mux, ARRAY_SIZE(axic_0_mux),
	  CLK_SET_RATE_PARENT, 0x812c, 17, 1, 0, mux_table },
	{ A500_CS_GIC_MUX, "cs_gic_mux", cs_gic_mux, ARRAY_SIZE(cs_gic_mux),
	  CLK_SET_RATE_PARENT, 0x812c, 16, 1, 0,mux_table },
	{ A500_PCIE_LP_CLK_MUX, "pcie_lp_clk_mux", pcie_lp_clk_mux, ARRAY_SIZE(pcie_lp_clk_mux),
	  CLK_SET_RATE_PARENT, 0x812c,  8, 1, 0, mux_table },
	{ A500_SDC_MUX, "sdc_mux", sdc_mux, ARRAY_SIZE(sdc_mux),
	  CLK_SET_RATE_PARENT, 0x812c,  7, 1, 0, mux_table },
	{ A500_EXT_AHB_MUX, "ext_ahb_mux", ext_ahb_mux, ARRAY_SIZE(ext_ahb_mux),
	  CLK_SET_RATE_PARENT, 0x812c,  6, 1, 0, mux_table },
	{ A500_SPI_MUX, "spi_mux", spi_mux, ARRAY_SIZE(spi_mux),
	  CLK_SET_RATE_PARENT, 0x812c,  5, 1, 0, mux_table },
	{ A500_ADC_MUX, "adc_mux", adc_mux, ARRAY_SIZE(adc_mux),
	  CLK_SET_RATE_PARENT, 0x812c,  4, 1, 0, mux_table },
	{ A500_USB3_MUX, "usb3_mux", usb3_mux, ARRAY_SIZE(usb3_mux),
	  CLK_SET_RATE_PARENT, 0x812c,  3, 1, 0, mux_table },
	{ A500_GMAC_MUX, "gmac_mux", gmac_mux, ARRAY_SIZE(gmac_mux),
	  CLK_SET_RATE_PARENT, 0x812c,  0, 1, 0, mux_table },
};

static const struct faraday_gate_clock a500_gate_clks[] = {
	/* AXI Clock Gated Control Register */
	{ A500_PCIE1_ACLK_G,  "pcie1_aclk_g",  "axic_1_aclk",     0,               0x8118,  7, 0 },
	{ A500_PCIE0_ACLK_G,  "pcie0_aclk_g",  "axic_1_aclk",     0,               0x8118,  6, 0 },
	{ A500_USB3_ACLK_G,   "usb3_aclk_g",   "axic_1_aclk",     0,               0x8118,  5, 0 },
	{ A500_GMAC1_ACLK_G,  "gmac1_aclk_g",  "axic_1_aclk",     0,               0x8118,  4, 0 },
	{ A500_GMAC0_ACLK_G,  "gmac0_aclk_g",  "axic_1_aclk",     0,               0x8118,  3, 0 },
	{ A500_LCDC_ACLK_G,   "lcdc_aclk_g",   "axic_1_aclk",     0,               0x8118,  2, 0 },
	{ A500_EAHB_ACLK_G,   "eahb_aclk_g",   "eahb_mux",        0,               0x8118,  1, 0 },
	{ A500_DDR_ACLK_G,    "ddr_aclk_g",    "pll3",            CLK_IS_CRITICAL, 0x8118,  0, 0 },
	/* AHB Clock Gated Control Register */
	{ A500_DMAC_HCLK_G,   "dmac_hclk_g",   "sys_hclk",        0,               0x811c,  3, 0 },
	{ A500_AES_HCLK_G,    "aes_hclk_g",    "sys_hclk",        0,               0x811c,  2, 0 },
	{ A500_SDC_HCLK_G,    "sdc_hclk_g",    "sys_hclk",        0,               0x811c,  1, 0 },
	{ A500_EAHB_HCLK_G,   "eahb_hclk_g",   "sys_hclk",        0,               0x811c,  0, 0 },
	/* X2P’s APB Clock Gated Control Register */
	{ A500_MISC_PCLK_G,   "misc_pclk_g",   "x2p_pclk",        0,               0x8124, 10, 0 },
	{ A500_RSA_PCLK_G,    "rsa_pclk_g",    "x2p_pclk",        0,               0x8124,  9, 0 },
	{ A500_SHA_PCLK_G,    "sha_pclk_g",    "x2p_pclk",        0,               0x8124,  8, 0 },
	{ A500_TRNG_PCLK_G,   "trng_pclk_g",   "x2p_pclk",        0,               0x8124,  7, 0 },
	{ A500_ADC_PCLK_G,    "adc_pclk_g",    "x2p_pclk",        0,               0x8124,  6, 0 },
	{ A500_GMAC1_PCLK_G,  "gmac1_pclk_g",  "x2p_pclk",        0,               0x8124,  5, 0 },
	{ A500_GMAC0_PCLK_G,  "gmac0_pclk_g",  "x2p_pclk",        0,               0x8124,  4, 0 },
	{ A500_LCDC_PCLK_G,   "lcdc_pclk_g",   "x2p_pclk",        0,               0x8124,  3, 0 },
	{ A500_PCIE1_PCLK_G,  "pcie1_pclk_g",  "x2p_pclk",        0,               0x8124,  2, 0 },
	{ A500_PCIE0_PCLK_G,  "pcie0_pclk_g",  "x2p_pclk",        0,               0x8124,  1, 0 },
	{ A500_DDR_PCLK_G,    "ddr_pclk_g",    "x2p_pclk",        CLK_IS_CRITICAL, 0x8124,  0, 0 },
	/* Peripheral Clock Gated Control Register */
	{ A500_PCIE_LP_CLK_G, "pcie_lp_clk_g", "pcie_lp_clk_mux", 0,               0x8128, 10, 0 },
	{ A500_LCD_SCACLK_G,  "lcd_scaclk_g",  "pll2",            0,               0x8128,  9, 0 },
	{ A500_LCD_PIXCLK_G,  "lcd_pixclk_g",  "pll2",            0,               0x8128,  8, 0 },
	{ A500_SDC_CLK_G,     "sdc_clk_g",     "sdc_mux",         0,               0x8128,  7, 0 },
	{ A500_TDC_XCLK_G,    "tdc_xclk_g",    "osc0",            0,               0x8128,  6, 0 },
	{ A500_EXT_AHB_CLK_G, "ext_ahb_clk_g", "ext_ahb_mux",     0,               0x8128,  5, 0 },
	{ A500_SPI_CLK_G,     "spi_clk_g",     "spi_mux",         0,               0x8128,  4, 0 },
	{ A500_ADC_MCLK_G,    "adc_mclk_g",    "adc_mux",         0,               0x8128,  3, 0 },
	{ A500_USB3_G,        "usb3_g",        "otg_ck_sel_mux",  0,               0x8128,  2, 0 },
	{ A500_GMAC1_G,       "gmac1_g",       "gmac_mux",        0,               0x8128,  1, 0 },
	{ A500_GMAC0_G,       "gmac0_g",       "gmac_mux",        0,               0x8128,  0, 0 },
};

static int a500_clocks_probe(struct platform_device *pdev)
{
	struct faraday_clock_data *clk_data;
	int ret;

	clk_data = faraday_clk_alloc(pdev, FARADAY_NR_CLKS);
	if (!clk_data)
		return -ENOMEM;

	ret = faraday_clk_register_fixed_rates(a500_fixed_rate_clks,
	                                       ARRAY_SIZE(a500_fixed_rate_clks),
	                                       clk_data);
	if (ret)
		return -EINVAL;

	ret = faraday_clk_register_plls(a500_pll_clks,
	                                ARRAY_SIZE(a500_pll_clks),
                                    clk_data->base,
	                                clk_data);
	if (ret)
		goto unregister_fixed_rates;

	ret = faraday_clk_register_dividers(a500_divider_clks,
	                                    ARRAY_SIZE(a500_divider_clks),
                                        clk_data->base,
	                                    clk_data);
	if (ret)
		goto unregister_plls;

	ret = faraday_clk_register_fixed_factors(a500_fixed_factor_clks,
	                                         ARRAY_SIZE(a500_fixed_factor_clks),
	                                         clk_data);
	if (ret)
		goto unregister_dividers;
	
	ret = faraday_clk_register_muxs(a500_mux_clks,
	                                ARRAY_SIZE(a500_mux_clks),
                                    clk_data->base,
	                                clk_data);
	if (ret)
		goto unregister_fixed_factors;

	ret = faraday_clk_register_gates(a500_gate_clks,
	                                 ARRAY_SIZE(a500_gate_clks),
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
	faraday_clk_unregister_gates(a500_gate_clks,
				ARRAY_SIZE(a500_gate_clks),
				clk_data);
unregister_muxs:
	faraday_clk_unregister_muxs(a500_mux_clks,
				ARRAY_SIZE(a500_mux_clks),
				clk_data);
unregister_fixed_factors:
	faraday_clk_unregister_fixed_factors(a500_fixed_factor_clks,
				ARRAY_SIZE(a500_fixed_factor_clks),
				clk_data);
unregister_dividers:
	faraday_clk_unregister_dividers(a500_divider_clks,
				ARRAY_SIZE(a500_divider_clks),
				clk_data);
unregister_plls:
	faraday_clk_unregister_plls(a500_pll_clks,
				ARRAY_SIZE(a500_pll_clks),
				clk_data);
unregister_fixed_rates:
	faraday_clk_unregister_fixed_rates(a500_fixed_rate_clks,
				ARRAY_SIZE(a500_fixed_rate_clks),
				clk_data);
	return -EINVAL;
}

static int a500_clocks_remove(struct platform_device *pdev)
{
	struct faraday_clock_data *clk_data = platform_get_drvdata(pdev);

	of_clk_del_provider(pdev->dev.of_node);

	faraday_clk_unregister_gates(a500_gate_clks,
				ARRAY_SIZE(a500_gate_clks),
				clk_data);
	faraday_clk_unregister_muxs(a500_mux_clks,
				ARRAY_SIZE(a500_mux_clks),
				clk_data);
	faraday_clk_unregister_fixed_factors(a500_fixed_factor_clks,
				ARRAY_SIZE(a500_fixed_factor_clks),
				clk_data);
	faraday_clk_unregister_dividers(a500_divider_clks,
				ARRAY_SIZE(a500_divider_clks),
				clk_data);
	faraday_clk_unregister_plls(a500_pll_clks,
				ARRAY_SIZE(a500_pll_clks),
				clk_data);
	faraday_clk_unregister_fixed_rates(a500_fixed_rate_clks,
				ARRAY_SIZE(a500_fixed_rate_clks),
				clk_data);

	return 0;
}

static const struct of_device_id a500_clocks_match_table[] = {
	{ .compatible = "faraday,a500-clk" },
	{ }
};
MODULE_DEVICE_TABLE(of, a500_clocks_match_table);

static struct platform_driver a500_clocks_driver = {
	.probe      = a500_clocks_probe,
	.remove     = a500_clocks_remove,
	.driver     = {
		.name   = "a500-clk",
		.owner  = THIS_MODULE,
		.of_match_table = a500_clocks_match_table,
	},
};

static int __init a500_clocks_init(void)
{
	return platform_driver_register(&a500_clocks_driver);
}
core_initcall(a500_clocks_init);

static void __exit a500_clocks_exit(void)
{
	platform_driver_unregister(&a500_clocks_driver);
}
module_exit(a500_clocks_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("B.C. Chen <bcchen@faraday-tech.com>");
MODULE_DESCRIPTION("Faraday A500 Clock Driver");
