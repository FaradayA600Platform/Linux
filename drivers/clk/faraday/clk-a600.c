/**
 *  drivers/clk/faraday/clk-a600.c
 *
 *  Faraday A600 Clock Tree
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
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>

#include <asm/io.h>

#include <dt-bindings/clock/clock-a600.h>

#include "clk.h"

static const struct faraday_fixed_rate_clock a600_fixed_rate_clks[] = {
	{ A600_FIXED_12M,    "osc0",  NULL, 12000000, 0 },
	{ A600_FIXED_12288K, "audio", NULL, 12288000, 0 },
};

static const struct samsung_pll_clock a600_pll_clks[] = {
	{ A600_PLL2, "pll2", "osc0", 0x80014, 0, 12, 10, 4, 6, 1, 3, 0 },
	{ A600_PLL3, "pll3", "osc0", 0x8001C, 0, 12, 10, 4, 6, 1, 3, CLK_IS_CRITICAL },
};

static const struct faraday_fixed_factor_clock a600_fixed_factor_clks[] = {
	{ A600_PLL0,         "pll0",         "osc0",            500,  4, CLK_SET_RATE_PARENT },
	{ A600_PLL1,         "pll1",         "osc0",            400,  6, CLK_SET_RATE_PARENT },
//	{ A600_PLL2,         "pll2",         "osc0",            396,  8, CLK_SET_RATE_PARENT },
//	{ A600_PLL3,         "pll3",         "osc0",            500,  6, CLK_SET_RATE_PARENT },
	{ A600_PLL3_DIV2,    "pll3_div2",    "pll3",              1,  2, CLK_SET_RATE_PARENT },
	{ A600_PLL3_DIV4,    "pll3_div4",    "pll3_div2",         1,  2, CLK_SET_RATE_PARENT },
	{ A600_CA53_CLK,     "ca53_clk",     "pll0_bypass_mux",   1,  1, CLK_SET_RATE_PARENT },
	{ A600_CA53_ACLK,    "ca53_aclk",    "pll0_bypass_mux",   1,  2, CLK_SET_RATE_PARENT },
	{ A600_CS_CLK,       "cs_clk",       "pll0_bypass_mux",   1,  4, CLK_SET_RATE_PARENT },
	{ A600_AXI_CLK,      "axi_clk",      "pll1_bypass_mux",   1,  1, CLK_SET_RATE_PARENT },
	{ A600_AXI_CLK_DIV2, "axi_clk_div2", "pll1_bypass_mux",   1,  2, CLK_SET_RATE_PARENT },
	{ A600_AHB_CLK,      "ahb_clk",      "pll1_bypass_mux",   1,  4, CLK_SET_RATE_PARENT },
	{ A600_APB_CLK,      "apb_clk",      "pll1_bypass_mux",   1,  8, CLK_SET_RATE_PARENT },
	{ A600_SD0_SDCLK1X,  "sd0_sdclk1x",  "sd0_ck_en",         1,  2, CLK_SET_RATE_PARENT },
	{ A600_SD1_SDCLK1X,  "sd1_sdclk1x",  "sd1_ck_en",         1,  2, CLK_SET_RATE_PARENT },
	{ A600_ADC_CLK,      "adc_clk",      "adc_clk_div",       1,  2, CLK_SET_RATE_PARENT },
	{ A600_TDC_CLK,      "tdc_clk",      "tdc_clk_div",       1,  2, CLK_SET_RATE_PARENT },
	{ A600_WDT_EXTCLK,   "wdt_extclk",   "osc0",              1, 16, CLK_SET_RATE_PARENT },
	{ A600_LOWFREQ_CLK,  "lowfreq_clk",  "osc0",              1, 16, CLK_SET_RATE_PARENT },
};

static const struct faraday_divider_clock a600_divider_clks[] = {
	/* IP clock divider Setting Register (EXT_REG) */
	{ A600_TRACE_CLK,     "trace_clk",     "cs_clk",           CLK_SET_RATE_PARENT, 0x30,  8, 3, 0, NULL },
	{ A600_GIC_ACLK,      "gic_aclk",      "ca53_aclk",        CLK_SET_RATE_PARENT, 0x30,  0, 3, 0, NULL },
	/* Clock Divider Register 0 (EXT_REG) */
	{ A600_LCD_SCACLK,    "lcd_scaclk",    "pll2",             CLK_SET_RATE_PARENT, 0x54, 24, 4, 0, NULL },
	{ A600_LCD_PIXCLK,    "lcd_pixclk",    "pll2",             CLK_SET_RATE_PARENT, 0x54, 20, 4, 0, NULL },
	{ A600_ADC_CLK_DIV,   "adc_clk_div",   "ahb_clk",          CLK_SET_RATE_PARENT, 0x54, 16, 4, 0, NULL },
	{ A600_GMAC0_CLK2P5M, "gmac0_clk2p5m", "gmac0_clk125m",    CLK_SET_RATE_PARENT, 0x54, 10, 6, 0, NULL },
	{ A600_GMAC0_CLK25M,  "gmac0_clk25m",  "gmac0_ck_en",      CLK_SET_RATE_PARENT, 0x54,  4, 6, 0, NULL },
	{ A600_GMAC0_CLK125M, "gmac0_clk125m", "gmac0_ck_en",      CLK_SET_RATE_PARENT, 0x54,  0, 4, 0, NULL },
	/* Clock Divider Register 1 (EXT_REG) */
	{ A600_HB_CLK,        "hb_clk",        "pll1_bypass_mux",  CLK_SET_RATE_PARENT, 0x58, 24, 4, 0, NULL },
	{ A600_TDC_CLK_DIV,   "tdc_clk_div",   "apb_clk",          CLK_SET_RATE_PARENT, 0x58, 16, 6, 0, NULL },
	{ A600_GMAC1_CLK2P5M, "gmac1_clk2p5m", "gmac1_clk125m",    CLK_SET_RATE_PARENT, 0x58, 10, 6, 0, NULL },
	{ A600_GMAC1_CLK25M,  "gmac1_clk25m",  "gmac1_ck_en",      CLK_SET_RATE_PARENT, 0x58,  4, 6, 0, NULL },
	{ A600_GMAC1_CLK125M, "gmac1_clk125m", "gmac1_ck_en",      CLK_SET_RATE_PARENT, 0x58,  0, 4, 0, NULL },
};

static const char *const pll0_bypass_mux[] = { "osc0", "pll0" };
static const char *const pll1_bypass_mux[] = { "osc0", "pll1" };
static const char *const ddr_clk_sel_mux[] = { "axi_clk_div2", "pll3_div2" };
static u32 mux_table[] = { 0, 1 };

static const struct faraday_mux_clock a600_mux_clks[] = {
	/* PLL Control Register (SCU) */
	{ A600_PLL0_BYPASS_MUX, "pll0_bypass_mux", pll0_bypass_mux, ARRAY_SIZE(pll0_bypass_mux),
	  CLK_SET_RATE_PARENT, 0x00030, 4, 1, 0, mux_table },
	{ A600_PLL1_BYPASS_MUX, "pll1_bypass_mux", pll1_bypass_mux, ARRAY_SIZE(pll1_bypass_mux),
	  CLK_SET_RATE_PARENT, 0x00030, 5, 1, 0, mux_table },
	/* PLL Control Register (SCU secure extension) */
	{ A600_DDR_CLK_SEL_MUX, "ddr_clk_sel_mux", ddr_clk_sel_mux, ARRAY_SIZE(ddr_clk_sel_mux),
	  CLK_SET_RATE_PARENT, 0x80148, 2, 1, 0, mux_table },
};

static const struct faraday_gate_clock a600_gate_busclks[] = {
	/* AHB Clock Control Register (SCU) */
	{ A600_GMAC1_ACLK_EN,     "gmac1_aclk_en",     "ahb_clk",         0,               0x00050, 5, 0 },
	{ A600_GMAC0_ACLK_EN,     "gmac0_aclk_en",     "ahb_clk",         0,               0x00050, 4, 0 },
	{ A600_USB2_1_HCLK_EN,    "usb2_1_hclk_en",    "ahb_clk",         0,               0x00050, 3, 0 },
	{ A600_USB2_0_HCLK_EN,    "usb2_0_hclk_en",    "ahb_clk",         0,               0x00050, 2, 0 },
	{ A600_SD1_HCLK_EN,       "sd1_hclk_en",       "ahb_clk",         0,               0x00050, 1, 0 },
	{ A600_SD0_HCLK_EN,       "sd0_hclk_en",       "ahb_clk",         0,               0x00050, 0, 0 },
	/* APB Clock Control Register (SCU) */
	{ A600_WDT1_PCLK_EN,      "wdt1_pclk_en",      "apb_clk",         0,               0x00060, 26, 0 },
	{ A600_WDT0_PCLK_EN,      "wdt0_pclk_en",      "apb_clk",         0,               0x00060, 25, 0 },
	{ A600_PCIE1_PCLK_EN,     "pcie1_pclk_en",     "apb_clk",         0,               0x00060, 24, 0 },
	{ A600_PCIE0_PCLK_EN,     "pcie0_pclk_en",     "apb_clk",         0,               0x00060, 23, 0 },
	{ A600_DMAC1_PCLK_EN,     "dmac1_pclk_en",     "apb_clk",         0,               0x00060, 22, 0 },
	{ A600_DMAC0_PCLK_EN,     "dmac0_pclk_en",     "apb_clk",         0,               0x00060, 21, 0 },
	{ A600_DDR_PCLK_EN,       "ddr_pclk_en",       "apb_clk",         CLK_IS_CRITICAL, 0x00060, 20, 0 },
	{ A600_I2C3_PCLK_EN,      "i2c3_pclk_en",      "apb_clk",         0,               0x00060, 19, 0 },
	{ A600_I2C2_PCLK_EN,      "i2c2_pclk_en",      "apb_clk",         0,               0x00060, 18, 0 },
	{ A600_I2C1_PCLK_EN,      "i2c1_pclk_en",      "apb_clk",         0,               0x00060, 17, 0 },
	{ A600_I2C0_PCLK_EN,      "i2c0_pclk_en",      "apb_clk",         0,               0x00060, 16, 0 },
//	{ A600_UART3_PCLK_EN,     "uart3_pclk_en",     "apb_clk",         0,               0x00060, 15, 0 },
//	{ A600_UART2_PCLK_EN,     "uart2_pclk_en",     "apb_clk",         0,               0x00060, 14, 0 },
//	{ A600_UART1_PCLK_EN,     "uart1_pclk_en",     "apb_clk",         0,               0x00060, 13, 0 },
//	{ A600_UART0_PCLK_EN,     "uart0_pclk_en",     "apb_clk",         0,               0x00060, 12, 0 },
	{ A600_PWM1_PCLK_EN,      "pwm1_pclk_en",      "apb_clk",         0,               0x00060, 11, 0 },
	{ A600_SPI1_PCLK_EN,      "spi1_pclk_en",      "apb_clk",         0,               0x00060, 10, 0 },
	{ A600_SPI0_PCLK_EN,      "spi0_pclk_en",      "apb_clk",         0,               0x00060,  9, 0 },
	{ A600_I2S_PCLK_EN,       "i2s_pclk_en",       "apb_clk",         0,               0x00060,  8, 0 },
	{ A600_H265_PCLK_EN,      "h265_pclk_en",      "apb_clk",         0,               0x00060,  7, 0 },
	{ A600_LCDC_PCLK_EN,      "lcdc_pclk_en",      "apb_clk",         0,               0x00060,  6, 0 },
	{ A600_GMAC1_PCLK_EN,     "gmac1_pclk_en",     "apb_clk",         0,               0x00060,  5, 0 },
	{ A600_GMAC0_PCLK_EN,     "gmac0_pclk_en",     "apb_clk",         0,               0x00060,  4, 0 },
	{ A600_GPIO_PCLK_EN,      "gpio_pclk_en",      "apb_clk",         0,               0x00060,  3, 0 },
	{ A600_ADC_PCLK_EN,       "adc_pclk_en",       "apb_clk",         0,               0x00060,  2, 0 },
	{ A600_TDC_PCLK_EN,       "tdc_pclk_en",       "apb_clk",         0,               0x00060,  1, 0 },
	{ A600_PWM0_PCLK_EN,      "pwm0_pclk_en",      "apb_clk",         0,               0x00060,  0, 0 },
	/* AXI Clock Control Register (SCU) */
	{ A600_HB_ACLK_EN,        "hb_aclk_en",        "axi_clk",         0,               0x00080, 10, 0 },
	{ A600_PCIE1_ACLK_EN,     "pcie1_aclk_en",     "pll3_div4",       0,               0x00080,  7, 0 },
	{ A600_PCIE0_ACLK_EN,     "pcie0_aclk_en",     "pll3_div4",       0,               0x00080,  6, 0 },
	{ A600_DDR_ACLK_EN,       "ddr_aclk_en",       "axi_clk",         CLK_IS_CRITICAL, 0x00080,  5, 0 },
	{ A600_H265_ACLK_EN,      "h265_aclk_en",      "axi_clk_div2",    0,               0x00080,  4, 0 },
	{ A600_ID_MAPPER_ACLK_EN, "id_mapper_aclk_en", "axi_clk",         CLK_IS_CRITICAL, 0x00080,  3, 0 },
	{ A600_LCDC_ACLK_EN,      "lcdc_aclk_en",      "axi_clk_div2",    0,               0x00080,  2, 0 },
	{ A600_DMAC1_ACLK_EN,     "dmac1_aclk_en",     "axi_clk",         0,               0x00080,  1, 0 },
	{ A600_DMAC0_ACLK_EN,     "dmac0_aclk_en",     "axi_clk",         0,               0x00080,  0, 0 },
	/* IP CLOCK ENABLE Register (SCU secure extension) */
	{ A600_TRACE_CK_EN,       "trace_ck_en",       "trace_clk",       0,               0x80020, 11, 0 },
	{ A600_GIC_CK_EN,         "gic_ck_en",         "gic_aclk",        CLK_IS_CRITICAL, 0x80020, 10, 0 },
	{ A600_PWM1_CK8_EN,       "pwm1_ck8_en",       "osc0",            0,               0x80020,  9, 0 },
	{ A600_PWM1_CK7_EN,       "pwm1_ck7_en",       "osc0",            0,               0x80020,  8, 0 },
	{ A600_PWM1_CK6_EN,       "pwm1_ck6_en",       "osc0",            0,               0x80020,  7, 0 },
	{ A600_PWM1_CK5_EN,       "pwm1_ck5_en",       "osc0",            0,               0x80020,  6, 0 },
	{ A600_PWM1_CK4_EN,       "pwm1_ck4_en",       "osc0",            0,               0x80020,  5, 0 },
	{ A600_PWM1_CK3_EN,       "pwm1_ck3_en",       "osc0",            0,               0x80020,  4, 0 },
	{ A600_PWM1_CK2_EN,       "pwm1_ck2_en",       "osc0",            0,               0x80020,  3, 0 },
	{ A600_PWM1_CK1_EN,       "pwm1_ck1_en",       "osc0",            0,               0x80020,  2, 0 },
	{ A600_EFUSE_CK_EN,       "efuse_ck_en",       "osc0",            0,               0x80020,  1, 0 },
	{ A600_SPICLK_CK_EN,      "spiclk_ck_en",      "ahb_clk",         0,               0x80020,  0, 0 },
};

static const struct faraday_gate_clock a600_gate_clks[] = {
	/* IP CLOCK ENABLE Register (EXT_REG) */
//	{ A600_UART3_CK_EN,       "uart3_ck_en",       "pll3_div4",       0,               0x0050, 24, 0 },
//	{ A600_UART2_CK_EN,       "uart2_ck_en",       "pll3_div4",       0,               0x0050, 23, 0 },
//	{ A600_UART1_CK_EN,       "uart1_ck_en",       "pll3_div4",       0,               0x0050, 22, 0 },
//	{ A600_UART0_CK_EN,       "uart0_ck_en",       "pll3_div4",       0,               0x0050, 21, 0 },
	{ A600_PWM0_CK8_EN,       "pwm0_ck8_en",       "osc0",            0,               0x0050, 20, 0 },
	{ A600_PWM0_CK7_EN,       "pwm0_ck7_en",       "osc0",            0,               0x0050, 19, 0 },
	{ A600_PWM0_CK6_EN,       "pwm0_ck6_en",       "osc0",            0,               0x0050, 18, 0 },
	{ A600_PWM0_CK5_EN,       "pwm0_ck5_en",       "osc0",            0,               0x0050, 17, 0 },
	{ A600_PWM0_CK4_EN,       "pwm0_ck4_en",       "osc0",            0,               0x0050, 16, 0 },
	{ A600_PWM0_CK3_EN,       "pwm0_ck3_en",       "osc0",            0,               0x0050, 15, 0 },
	{ A600_PWM0_CK2_EN,       "pwm0_ck2_en",       "osc0",            0,               0x0050, 14, 0 },
	{ A600_PWM0_CK1_EN,       "pwm0_ck1_en",       "osc0",            0,               0x0050, 13, 0 },
	{ A600_SPI1_SSPCLK_CK_EN, "spi1_sspclk_ck_en", "ahb_clk",         0,               0x0050, 12, 0 },
	{ A600_SPI0_SSPCLK_CK_EN, "spi0_sspclk_ck_en", "ahb_clk",         0,               0x0050, 11, 0 },
	{ A600_I2S_SSPCLK_CK_EN,  "i2s_sspclk_ck_en",  "audio",           0,               0x0050, 10, 0 },
	{ A600_H265_CK_EN,        "h265_ck_en",        "axi_clk_div2",    0,               0x0050,  9, 0 },
	{ A600_LC_CK_EN,          "lc_ck_en",          "lcd_pixclk",      0,               0x0050,  8, 0 },
	{ A600_LC_SCALER_CK_EN,   "lc_scaler_ck_en",   "lcd_scaclk",      0,               0x0050,  7, 0 },
	{ A600_SD0_CK_EN,         "sd0_ck_en",         "axi_clk_div2",    0,               0x0050,  6, 0 },
	{ A600_SD1_CK_EN,         "sd1_ck_en",         "axi_clk_div2",    0,               0x0050,  5, 0 },
	{ A600_GMAC1_CK_EN,       "gmac1_ck_en",       "pll3_div2",       0,               0x0050,  4, 0 },
	{ A600_GMAC0_CK_EN,       "gmac0_ck_en",       "pll3_div2",       0,               0x0050,  3, 0 },
	{ A600_ADC_CK_EN,         "adc_ck_en",         "adc_clk",         0,               0x0050,  2, 0 },
	{ A600_TDC_CK_EN,         "tdc_ck_en",         "tdc_clk",         0,               0x0050,  1, 0 },
	{ A600_HB_CK_EN,          "hb_ck_en",          "hb_clk",          0,               0x0050,  0, 0 },
};

static int a600_clocks_probe(struct platform_device *pdev)
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

	ret = faraday_clk_register_fixed_rates(a600_fixed_rate_clks,
	                                       ARRAY_SIZE(a600_fixed_rate_clks),
	                                       clk_data);
	if (ret)
		return -EINVAL;

	ret = samsung_clk_register_plls(a600_pll_clks,
	                                ARRAY_SIZE(a600_pll_clks),
	                                clk_data->base,
	                                clk_data);
	if (ret)
		goto unregister_fixed_rates;

	ret = faraday_clk_register_dividers(a600_divider_clks,
	                                    ARRAY_SIZE(a600_divider_clks),
	                                    clk_data->extbase,
	                                    clk_data);
	if (ret)
		goto unregister_plls;

	ret = faraday_clk_register_fixed_factors(a600_fixed_factor_clks,
	                                         ARRAY_SIZE(a600_fixed_factor_clks),
	                                         clk_data);
	if (ret)
		goto unregister_dividers;
	
	ret = faraday_clk_register_muxs(a600_mux_clks,
	                                ARRAY_SIZE(a600_mux_clks),
	                                clk_data->base,
	                                clk_data);
	if (ret)
		goto unregister_fixed_factors;

	ret = faraday_clk_register_gates(a600_gate_busclks,
	                                 ARRAY_SIZE(a600_gate_busclks),
	                                 clk_data->base,
	                                 clk_data);
	if (ret)
		goto unregister_muxs;

	ret = faraday_clk_register_gates(a600_gate_clks,
	                                 ARRAY_SIZE(a600_gate_clks),
	                                 clk_data->extbase,
	                                 clk_data);
	if (ret)
		goto unregister_gates_busclk;

	ret = of_clk_add_provider(pdev->dev.of_node,
	                          of_clk_src_onecell_get, &clk_data->clk_data);
	if (ret)
		goto unregister_gates;

	platform_set_drvdata(pdev, clk_data);

	return 0;

unregister_gates:
	faraday_clk_unregister_gates(a600_gate_clks,
				ARRAY_SIZE(a600_gate_clks),
				clk_data);
unregister_gates_busclk:
	faraday_clk_unregister_gates(a600_gate_busclks,
				ARRAY_SIZE(a600_gate_busclks),
				clk_data);
unregister_muxs:
	faraday_clk_unregister_muxs(a600_mux_clks,
				ARRAY_SIZE(a600_mux_clks),
				clk_data);
unregister_fixed_factors:
	faraday_clk_unregister_fixed_factors(a600_fixed_factor_clks,
				ARRAY_SIZE(a600_fixed_factor_clks),
				clk_data);
unregister_dividers:
	faraday_clk_unregister_dividers(a600_divider_clks,
				ARRAY_SIZE(a600_divider_clks),
				clk_data);
unregister_plls:
	samsung_clk_unregister_plls(a600_pll_clks,
				ARRAY_SIZE(a600_pll_clks),
				clk_data);
unregister_fixed_rates:
	faraday_clk_unregister_fixed_rates(a600_fixed_rate_clks,
				ARRAY_SIZE(a600_fixed_rate_clks),
				clk_data);
	return -EINVAL;
}

static int a600_clocks_remove(struct platform_device *pdev)
{
	struct faraday_clock_data *clk_data = platform_get_drvdata(pdev);

	of_clk_del_provider(pdev->dev.of_node);

	faraday_clk_unregister_gates(a600_gate_clks,
				ARRAY_SIZE(a600_gate_clks),
				clk_data);
	faraday_clk_unregister_gates(a600_gate_busclks,
				ARRAY_SIZE(a600_gate_busclks),
				clk_data);
	faraday_clk_unregister_muxs(a600_mux_clks,
				ARRAY_SIZE(a600_mux_clks),
				clk_data);
	faraday_clk_unregister_fixed_factors(a600_fixed_factor_clks,
				ARRAY_SIZE(a600_fixed_factor_clks),
				clk_data);
	faraday_clk_unregister_dividers(a600_divider_clks,
				ARRAY_SIZE(a600_divider_clks),
				clk_data);
	samsung_clk_unregister_plls(a600_pll_clks,
				ARRAY_SIZE(a600_pll_clks),
				clk_data);
	faraday_clk_unregister_fixed_rates(a600_fixed_rate_clks,
				ARRAY_SIZE(a600_fixed_rate_clks),
				clk_data);

	return 0;
}

static const struct of_device_id a600_clocks_match_table[] = {
	{ .compatible = "faraday,a600-clk" },
	{ }
};
MODULE_DEVICE_TABLE(of, a600_clocks_match_table);

static struct platform_driver a600_clocks_driver = {
	.probe      = a600_clocks_probe,
	.remove     = a600_clocks_remove,
	.driver     = {
		.name   = "a600-clk",
		.owner  = THIS_MODULE,
		.of_match_table = a600_clocks_match_table,
	},
};

static int __init a600_clocks_init(void)
{
	return platform_driver_register(&a600_clocks_driver);
}
core_initcall(a600_clocks_init);

static void __exit a600_clocks_exit(void)
{
	platform_driver_unregister(&a600_clocks_driver);
}
module_exit(a600_clocks_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("B.C. Chen <bcchen@faraday-tech.com>");
MODULE_DESCRIPTION("Faraday A500 Clock Driver");
