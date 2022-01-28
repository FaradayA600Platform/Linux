/*
 * Faraday Clock Framework
 *
 * (C) Copyright 2021-2022 Faraday Technology
 * Bo-Cun Chen <bcchen@faraday-tech.com>
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/clkdev.h>
#include <linux/clk-provider.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/slab.h>

#include "clk.h"

static DEFINE_SPINLOCK(faraday_clk_lock);

struct faraday_clock_data *faraday_clk_alloc(struct platform_device *pdev,
		int nr_clks)
{
	struct faraday_clock_data *clk_data;
	struct resource *res;
	struct clk **clk_table;

	clk_data = devm_kmalloc(&pdev->dev, sizeof(*clk_data), GFP_KERNEL);
	if (!clk_data)
		return NULL;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return NULL;

	clk_data->base = devm_ioremap(&pdev->dev,
	                              res->start, resource_size(res));
	if (!clk_data->base)
		return NULL;

	clk_table = devm_kmalloc_array(&pdev->dev, nr_clks,
	                               sizeof(*clk_table),
	                               GFP_KERNEL);
	if (!clk_table)
		return NULL;

	clk_data->clk_data.clks = clk_table;
	clk_data->clk_data.clk_num = nr_clks;

	return clk_data;
}

int faraday_clk_register_fixed_rates(const struct faraday_fixed_rate_clock *clks,
		int nums, struct faraday_clock_data *data)
{
	struct clk *clk;
	int i;

	for (i = 0; i < nums; i++) {
		clk = clk_register_fixed_rate(NULL, clks[i].name,
		                              clks[i].parent_name,
		                              clks[i].flags,
		                              clks[i].fixed_rate);
		if (IS_ERR(clk)) {
			pr_err("%s: failed to register clock %s\n",
			       __func__, clks[i].name);
			goto err;
		}

		data->clk_data.clks[clks[i].id] = clk;
	}

	return 0;

err:
	while (i--)
		clk_unregister_fixed_rate(data->clk_data.clks[clks[i].id]);

	return PTR_ERR(clk);
}

int faraday_clk_unregister_fixed_rates(const struct faraday_fixed_rate_clock *clks,
		int nums, struct faraday_clock_data *data)
{
	int i;

	i = nums - 1;
	while (i--)
		clk_unregister_fixed_rate(data->clk_data.clks[clks[i].id]);

	return 0;
}

int faraday_clk_register_plls(const struct faraday_pll_clock *clks,
		int nums, void __iomem *base, struct faraday_clock_data *data)
{
	struct clk *clk;
	int i;

	for (i = 0; i < nums; i++) {
		clk = faraday_clk_register_pll(NULL, clks[i].name,
		                               clks[i].parent_name,
		                               clks[i].flags,
		                               base + clks[i].offset,
		                               clks[i].en_shift,
		                               clks[i].ms_shift,
		                               BIT(clks[i].ms_width) - 1,
		                               clks[i].ns_shift,
		                               BIT(clks[i].ns_width) - 1,
		                               clks[i].ps_shift,
		                               BIT(clks[i].ps_width) - 1
		);
		if (IS_ERR(clk)) {
			pr_err("%s: failed to register clock %s\n",
			       __func__, clks[i].name);
			goto err;
		}

		data->clk_data.clks[clks[i].id] = clk;
	}

	return 0;

err:
	while (i--)
		faraday_clk_unregister_pll(data->clk_data.clks[clks[i].id]);

	return PTR_ERR(clk);
}

int faraday_clk_unregister_plls(const struct faraday_pll_clock *clks,
		int nums, struct faraday_clock_data *data)
{
	int i;

	i = nums - 1;
	while (i--)
		faraday_clk_unregister_pll(data->clk_data.clks[clks[i].id]);

	return 0;
}

int samsung_clk_register_plls(const struct samsung_pll_clock *clks,
		int nums, void __iomem *base, struct faraday_clock_data *data)
{
	struct clk *clk;
	int i;

	for (i = 0; i < nums; i++) {
		clk = samsung_clk_register_pll(NULL, clks[i].name,
		                               clks[i].parent_name,
		                               clks[i].flags,
		                               base + clks[i].offset,
		                               clks[i].en_shift,
		                               clks[i].m_shift,
		                               BIT(clks[i].m_width) - 1,
		                               clks[i].p_shift,
		                               BIT(clks[i].p_width) - 1,
		                               clks[i].s_shift,
		                               BIT(clks[i].s_width) - 1
		);
		if (IS_ERR(clk)) {
			pr_err("%s: failed to register clock %s\n",
			       __func__, clks[i].name);
			goto err;
		}

		data->clk_data.clks[clks[i].id] = clk;
	}

	return 0;

err:
	while (i--)
		samsung_clk_unregister_pll(data->clk_data.clks[clks[i].id]);

	return PTR_ERR(clk);
}

int samsung_clk_unregister_plls(const struct samsung_pll_clock *clks,
		int nums, struct faraday_clock_data *data)
{
	int i;

	i = nums - 1;
	while (i--)
		samsung_clk_unregister_pll(data->clk_data.clks[clks[i].id]);

	return 0;
}

int faraday_clk_register_fixed_factors(const struct faraday_fixed_factor_clock *clks,
		int nums, struct faraday_clock_data *data)
{
	struct clk *clk;
	int i;

	for (i = 0; i < nums; i++) {
		clk = clk_register_fixed_factor(NULL, clks[i].name,
		                                clks[i].parent_name,
		                                clks[i].flags,
		                                clks[i].mult,
		                                clks[i].div);
		if (IS_ERR(clk)) {
			pr_err("%s: failed to register clock %s\n",
			       __func__, clks[i].name);
			goto err;
		}

		data->clk_data.clks[clks[i].id] = clk;
	}

	return 0;

err:
	while (i--)
		clk_unregister_fixed_factor(data->clk_data.clks[clks[i].id]);

	return PTR_ERR(clk);
}

int faraday_clk_unregister_fixed_factors(const struct faraday_fixed_factor_clock *clks,
		int nums, struct faraday_clock_data *data)
{
	int i;

	i = nums - 1;
	while (i--)
		clk_unregister_fixed_factor(data->clk_data.clks[clks[i].id]);

	return 0;
}

int faraday_clk_register_dividers(const struct faraday_divider_clock *clks,
		int nums, void __iomem *base, struct faraday_clock_data *data)
{
	struct clk *clk;
	int i;

	for (i = 0; i < nums; i++) {
		clk = clk_register_divider_table(NULL, clks[i].name,
		                                 clks[i].parent_name,
		                                 clks[i].flags,
		                                 base + clks[i].offset,
		                                 clks[i].shift,
		                                 clks[i].width,
		                                 clks[i].div_flags,
		                                 clks[i].table,
		                                 &faraday_clk_lock);
		if (IS_ERR(clk)) {
			pr_err("%s: failed to register clock %s\n",
			       __func__, clks[i].name);
			goto err;
		}

		data->clk_data.clks[clks[i].id] = clk;
	}

	return 0;

err:
	while (i--)
		clk_unregister_divider(data->clk_data.clks[clks[i].id]);

	return PTR_ERR(clk);
}

int faraday_clk_unregister_dividers(const struct faraday_divider_clock *clks,
		int nums, struct faraday_clock_data *data)
{
	int i;

	i = nums - 1;
	while (i--)
		clk_unregister_divider(data->clk_data.clks[clks[i].id]);

	return 0;
}

int faraday_clk_register_muxs(const struct faraday_mux_clock *clks,
		int nums, void __iomem *base, struct faraday_clock_data *data)
{
	struct clk *clk;
	int i;

	for (i = 0; i < nums; i++) {
		clk = clk_register_mux_table(NULL, clks[i].name,
		                             clks[i].parent_names,
		                             clks[i].num_parents,
		                             clks[i].flags,
		                             base + clks[i].offset,
		                             clks[i].shift,
		                             BIT(clks[i].width) - 1,
		                             clks[i].mux_flags,
		                             clks[i].table,
		                             &faraday_clk_lock);
		if (IS_ERR(clk)) {
			pr_err("%s: failed to register clock %s\n",
			       __func__, clks[i].name);
			goto err;
		}

		data->clk_data.clks[clks[i].id] = clk;
	}

	return 0;

err:
	while (i--)
		clk_unregister_mux(data->clk_data.clks[clks[i].id]);

	return PTR_ERR(clk);
}

int faraday_clk_unregister_muxs(const struct faraday_mux_clock *clks,
		int nums, struct faraday_clock_data *data)
{
	int i;

	i = nums - 1;
	while (i--)
		clk_unregister_mux(data->clk_data.clks[clks[i].id]);

	return 0;
}

int faraday_clk_register_gates(const struct faraday_gate_clock *clks,
		int nums, void __iomem *base, struct faraday_clock_data *data)
{
	struct clk *clk;
	int i;

	for (i = 0; i < nums; i++) {
		clk = clk_register_gate(NULL, clks[i].name,
		                        clks[i].parent_name,
		                        clks[i].flags,
		                        base + clks[i].offset,
		                        clks[i].bit_idx,
		                        clks[i].gate_flags,
		                        &faraday_clk_lock);
		if (IS_ERR(clk)) {
			pr_err("%s: failed to register clock %s\n",
			       __func__, clks[i].name);
			goto err;
		}

		data->clk_data.clks[clks[i].id] = clk;
	}

	return 0;

err:
	while (i--)
		clk_unregister_gate(data->clk_data.clks[clks[i].id]);

	return PTR_ERR(clk);
}

int faraday_clk_unregister_gates(const struct faraday_gate_clock *clks,
		int nums, struct faraday_clock_data *data)
{
	int i;

	i = nums - 1;
	while (i--)
		clk_unregister_gate(data->clk_data.clks[clks[i].id]);

	return 0;
}