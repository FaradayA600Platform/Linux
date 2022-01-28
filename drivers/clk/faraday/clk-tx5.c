/**
 *  drivers/clk/faraday/clk-tx5.c
 *
 *  Faraday TX5 Clock Tree
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
#include <linux/clk-provider.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/of_address.h>

#define CONFIG_MACH_TX5_VP

struct tx5_clk {
	void __iomem	*scu_base;
	void __iomem	*scu_ext_base;
};

struct tx5_clk *hw;

unsigned int tx5_clk_get_pll_ns(const char *name)
{
#ifndef CONFIG_MACH_TX5_VP
	void __iomem *scu_va;
#endif
	unsigned int ns = 0;

	if (!strcmp(name, "pll0")) {
#ifndef CONFIG_MACH_TX5_VP
		scu_va = hw->scu_base;
		ns = ((readl(scu_va + 0x30) >> 15) & 0x07E) | ((readl(scu_va + 0x30) >> 27) & 0x001);
#else
		ns = 100;
#endif
	} else if (!strcmp(name, "pll1")) {
#ifndef CONFIG_MACH_TX5_VP
		scu_va = hw->scu_base;
		ns = ((readl(scu_va + 0x30) >>  8) & 0x007) | ((readl(scu_va + 0x30) >> 28) & 0x00F);
#else
		ns = 99;
#endif
	} else if (!strcmp(name, "pll2")) {
#ifndef CONFIG_MACH_TX5_VP
		scu_va = hw->scu_ext_base;
		ns = ((readl(scu_va + 0x14) >> 12) & 0x3FF);
#else
		ns = 400;
#endif
	} else if (!strcmp(name, "pll3")) {
#ifndef CONFIG_MACH_TX5_VP
		scu_va = hw->scu_ext_base;
		ns = ((readl(scu_va + 0x1C) >> 12) & 0x3FF);
#else
		ns = 125;
#endif
	} else if (!strcmp(name, "pll4")) {
#ifndef CONFIG_MACH_TX5_VP
		scu_va = hw->scu_ext_base;
		ns = ((readl(scu_va + 0x1C) >> 12) & 0x3FF);
#else
		ns = 25;
#endif
	}

	return ns;
}

unsigned int tx5_clk_get_cpu_div(struct device_node *np, const char **parent_name)
{
	void __iomem *scu_va;
	unsigned int sel, div;

	scu_va = hw->scu_base;

#ifndef CONFIG_MACH_TX5_VP
	sel = (readl(scu_va + 0x30) >> 4) & 0x1;
#else
	sel = 1;
#endif
	div = 1;

	*parent_name = of_clk_get_parent_name(np, sel);

	return div;
}

unsigned int tx5_clk_get_ddrmclk_div(struct device_node *np, const char **parent_name)
{
	void __iomem *scu_va;
	unsigned int sel, div;

	scu_va = hw->scu_ext_base;
	
#ifndef CONFIG_MACH_TX5_VP
	sel = (readl(scu_va + 0x148) >> 2) & 0x1;
#else
	sel = 0;
#endif
	div = 1;

	*parent_name = of_clk_get_parent_name(np, sel);

	return div;
}

unsigned int tx5_clk_get_axi_div(struct device_node *np, const char **parent_name)
{
	void __iomem *scu_va;
	unsigned int sel, div;

	scu_va = hw->scu_base;

#ifndef CONFIG_MACH_TX5_VP
	sel = (readl(scu_va + 0x30) >> 5) & 0x1;
#else
	sel = 1;
#endif
	div = 1;

	*parent_name = of_clk_get_parent_name(np, 0);

	return div;
}

unsigned int tx5_clk_get_ahb_div(struct device_node *np, const char **parent_name)
{
	void __iomem *scu_va;
	unsigned int sel, div;

	scu_va = hw->scu_base;

#ifndef CONFIG_MACH_TX5_VP
	sel = (readl(scu_va + 0x30) >> 5) & 0x1;
#else
	sel = 1;
#endif
	div = 4;

	*parent_name = of_clk_get_parent_name(np, 0);

	return div;
}

unsigned int tx5_clk_get_apb_div(struct device_node *np, const char **parent_name)
{
	void __iomem *scu_va;
	unsigned int sel, div;

	scu_va = hw->scu_base;

#ifndef CONFIG_MACH_TX5_VP
	sel = (readl(scu_va + 0x30) >> 5) & 0x1;
#else
	sel = 1;
#endif
	div = 8;

	*parent_name = of_clk_get_parent_name(np, 0);

	return div;
}

unsigned int tx5_clk_get_div(struct device_node *np, const char **parent_name)
{
	const char *name = np->name;
	unsigned int div = 1;

	if (!strcmp(name, "cpu")) {
		div = tx5_clk_get_cpu_div(np, parent_name);
	} else if (!strcmp(name, "ddrmclk")) {
		div = tx5_clk_get_ddrmclk_div(np, parent_name);
	} else if (!strcmp(name, "AXI") || !strcmp(name, "axi") || !strcmp(name, "aclk")) {
		div = tx5_clk_get_axi_div(np, parent_name);
	} else if (!strcmp(name, "AHB") || !strcmp(name, "ahb") || !strcmp(name, "hclk")) {
		div = tx5_clk_get_ahb_div(np, parent_name);
	} else if (!strcmp(name, "APB") || !strcmp(name, "apb") || !strcmp(name, "pclk")) {
		div = tx5_clk_get_apb_div(np, parent_name);
	}

	return div;
}

#ifdef CONFIG_OF
static void __init of_tx5_faraday_pll_setup(struct device_node *np)
{
	struct clk *clk;
	const char *clk_name = np->name;
	const char *parent_name;
	u32 div, mult;

	if (of_property_read_u32(np, "clock-div", &div)) {
		pr_err("%s Fixed factor clock <%s> must have a"\
			"clock-div property\n",
			__func__, np->name);
		return;
	}

	of_property_read_string(np, "clock-output-names", &clk_name);

	mult = tx5_clk_get_pll_ns(clk_name);

	parent_name = of_clk_get_parent_name(np, 0);

	clk = clk_register_fixed_factor(NULL, clk_name, parent_name, 0,
					mult, div);
	if (!IS_ERR(clk)) {
		of_clk_add_provider(np, of_clk_src_simple_get, clk);
	}
}

static void __init of_tx5_faraday_mux_setup(struct device_node *np)
{
	struct clk *clk;
	const char *clk_name = np->name;
	const char *parent_name;
	u32 div, mult;

	if (of_property_read_u32(np, "clock-div", &div)) {
		pr_err("%s Fixed factor clock <%s> must have a"\
			"clock-div property\n",
			__func__, np->name);
		return;
	}

	of_property_read_string(np, "clock-output-names", &clk_name);

	mult = 1;

	div = tx5_clk_get_div(np, &parent_name);

	clk = clk_register_fixed_factor(NULL, clk_name, parent_name, 0,
					mult, div);
	if (!IS_ERR(clk)) {
		of_clk_add_provider(np, of_clk_src_simple_get, clk);
	}
}

static const __initconst struct of_device_id tx5_clk_match[] = {
	{ .compatible = "tx5,osc0", .data = of_fixed_clk_setup, },
	{ .compatible = "tx5,osc0_div4", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "tx5,osc1", .data = of_fixed_clk_setup, },
	{ .compatible = "tx5,pll0", .data = of_tx5_faraday_pll_setup, },
	{ .compatible = "tx5,pll0_div3", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "tx5,pll1", .data = of_tx5_faraday_pll_setup, },
	{ .compatible = "tx5,pll2", .data = of_tx5_faraday_pll_setup, },
	{ .compatible = "tx5,pll2_div2", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "tx5,pll2_div4", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "tx5,pll2_div8", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "tx5,pll2_div16", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "tx5,pll2_div32", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "tx5,pll3", .data = of_tx5_faraday_pll_setup, },
	{ .compatible = "tx5,pll3_div5", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "tx5,pll3_div12", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "tx5,pll3_div25", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "tx5,pll3_div30", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "tx5,pll3_div60", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "tx5,pll3_div600", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "tx5,pll4", .data = of_tx5_faraday_pll_setup, },
	{ .compatible = "tx5,cpu", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "tx5,ddrmclk", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "tx5,axi", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "tx5,aclk", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "tx5,ahb", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "tx5,hclk", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "tx5,apb", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "tx5,pclk", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "tx5,sdclk", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "tx5,spiclk", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "tx5,sspclk", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "tx5,sspclk_i2s", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "tx5,gmacclk", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "tx5,lcclk", .data = of_fixed_factor_clk_setup, },
};

void __init of_tx5_clocks_init(struct device_node *n)
{
	struct device_node *node;
	struct of_phandle_args clkspec;
	struct clk *clk;
	unsigned long pll0, pll1, pll2, pll3, pll4;
	unsigned long cpuclk, mclk, aclk, hclk, pclk;
	unsigned long sdclk, spiclk, sspclk, gmacclk, lcclk;

	pll0 = 0; pll1 = 0; pll2 = 0; pll3 = 0; pll4 = 0;
	cpuclk = 0; aclk = 0; hclk = 0; pclk = 0; mclk = 0;
	sdclk = 0; spiclk = 0; sspclk = 0; gmacclk = 0; lcclk = 0;

	hw = kzalloc(sizeof(*hw), GFP_KERNEL);
	if (!hw)
		return;

	hw->scu_base = of_iomap(n, 0);
	if (WARN_ON(!hw->scu_base))
		return;

	of_clk_init(tx5_clk_match);

	for (node = of_find_matching_node(NULL, tx5_clk_match);
	     node; node = of_find_matching_node(node, tx5_clk_match)) {
		clkspec.np = node;
		clk = of_clk_get_from_provider(&clkspec);
		of_node_put(clkspec.np);

		if (!strcmp(__clk_get_name(clk), "pll0"))
			pll0 = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "pll1"))
			pll1 = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "pll2"))
			pll2 = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "pll3"))
			pll3 = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "pll4"))
			pll4 = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "cpu"))
			cpuclk = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "ddrmclk"))
			mclk = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "aclk"))
			aclk = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "hclk"))
			hclk = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "pclk"))
			pclk = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "sdclk"))
			sdclk = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "spiclk"))
			spiclk = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "sspclk"))
			sspclk = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "gmacclk"))
			gmacclk = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "lcclk"))
			lcclk = clk_get_rate(clk);
	}

	printk(KERN_INFO "PLL0: %ld MHz, PLL1: %ld MHz, PLL2: %ld MHz, PLL3: %ld MHz, PLL4: %ld MHz\n",
	       pll0/1000/1000, pll1/1000/1000, pll2/1000/1000, pll3/1000/1000, pll4/1000/1000);
	printk(KERN_INFO "CPU: %ld MHz, DDR MCLK: %ld MHz, ACLK: %ld MHz, HCLK: %ld MHz PCLK: %ld MHz\n",
	       cpuclk/1000/1000, mclk/1000/1000, aclk/1000/1000, hclk/1000/1000, pclk/1000/1000);
	printk(KERN_INFO "SDCLK: %ld MHz, SPICLK: %ld MHz, SSPCLK: %ld MHz, GMACCLK: %ld MHz, LCCLK: %ld MHz\n",
	       sdclk/1000/1000, spiclk/1000/1000, sspclk/1000/1000, gmacclk/1000/1000, lcclk/1000/1000);
}
CLK_OF_DECLARE(tx5_clocks_init, "faraday,tx5-clk", of_tx5_clocks_init);
#endif
