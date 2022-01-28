// SPDX-License-Identifier: GPL-2.0
/*
 *  Faraday JohnDeere TS711 Evaluation Board Clock Tree
 *
 *  Copyright (C) 2021 Faraday Technology
 *  Copyright (C) 2021 Guo-Cheng Lee <gclee@faraday-tech.com>
 */
#include <linux/clk-provider.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/of_address.h>

#include <mach/hardware.h>
#include <mach/ftscu100.h>

struct ts711_clk {
	char *name;
	char *parent;
	unsigned long rate;
	unsigned int mult;
	unsigned int div;
};

unsigned int ts711_clk_get_pll_ns(const char *name)
{
	void __iomem *scu_va;
	unsigned int ns = 0;

    scu_va = ioremap(PLAT_SCU_PA_BASE, SZ_4K);

	if (!strcmp(name, "pll0_pre_ckout2")) {
		ns = FTSCU100_PLL0_PRE_NS(scu_va+FTSCU100_OFFSET_PLLCTL);
	} else if (!strcmp(name, "pll0_ckout0b")) {
		ns = FTSCU100_PLL0_NS(scu_va+FTSCU100_OFFSET_PLLCTL);
	} else if (!strcmp(name, "pll1_ckout2")) {
		ns = FTSCU100_PLL1_NS(scu_va+FTSCU100_OFFSET_PLL2_CTRL);
	} else if (!strcmp(name, "pll2")) {
		ns = 16;
	}

	return ns;
}

#ifdef CONFIG_OF
static void __init of_ts711_faraday_pll_setup(struct device_node *np)
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

	mult = ts711_clk_get_pll_ns(clk_name);

	parent_name = of_clk_get_parent_name(np, 0);

	clk = clk_register_fixed_factor(NULL, clk_name, parent_name, 0,
					mult, div);
	if (!IS_ERR(clk)) {
		of_clk_add_provider(np, of_clk_src_simple_get, clk);
	}
}

static const __initconst struct of_device_id ts711_clk_match[] = {
	{ .compatible = "ts711evb,osc0", .data = of_fixed_clk_setup, },
	{ .compatible = "ts711evb,osc1", .data = of_fixed_clk_setup, },
	{ .compatible = "ts711evb,osc2", .data = of_fixed_clk_setup, },
    { .compatible = "ts711evb,pll0_pre_ckout2", .data = of_ts711_faraday_pll_setup, },
	{ .compatible = "ts711evb,pll0_ckout0b", .data = of_ts711_faraday_pll_setup, },    
	{ .compatible = "ts711evb,pll1_ckout2", .data = of_ts711_faraday_pll_setup, },
	{ .compatible = "ts711evb,pll2", .data = of_ts711_faraday_pll_setup, },
	{ .compatible = "ts711evb,aclk", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "ts711evb,hclk", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "ts711evb,pclk", .data = of_fixed_factor_clk_setup, },
	{ .compatible = "ts711evb,cpu", .data = of_fixed_factor_clk_setup, },
    { .compatible = "ts711evb,ddrmclk", .data = of_fixed_factor_clk_setup, },
};

void __init of_ts711_clocks_init(struct device_node *n)
{
	struct device_node *node;
	struct of_phandle_args clkspec;
	struct clk *clk;
	unsigned long cpuclk, aclk, hclk, pclk, mclk;

	cpuclk = 0; aclk = 0; hclk = 0; pclk = 0; mclk = 0;

	of_clk_init(ts711_clk_match);

	for (node = of_find_matching_node(NULL, ts711_clk_match);
	     node; node = of_find_matching_node(node, ts711_clk_match)) {
		clkspec.np = node;
		clk = of_clk_get_from_provider(&clkspec);
		of_node_put(clkspec.np);

		if (!strcmp(__clk_get_name(clk), "cpu"))
			cpuclk = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "aclk"))
			aclk = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "hclk"))
			hclk = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "pclk"))
			pclk = clk_get_rate(clk);
		else if (!strcmp(__clk_get_name(clk), "ddrmclk"))
			mclk = clk_get_rate(clk);
	}

	printk(KERN_INFO "CPU: %ld, DDR MCLK: %ld\nACLK: %ld, HCLK: %ld, PCLK: %ld\n",
	       cpuclk, mclk, aclk, hclk, pclk);
}
CLK_OF_DECLARE(of_ts711_clocks_init, "faraday,ts711evb-clk", of_ts711_clocks_init);
#endif
