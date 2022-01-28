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

#include <linux/clk-provider.h>
#include <linux/slab.h>
#include <linux/io.h>

#include "clk-pll.h"

#define to_faraday_clk_pll(_hw) container_of(_hw, struct faraday_clk_pll, hw)
#define to_samsung_clk_pll(_hw) container_of(_hw, struct samsung_clk_pll, hw)

int power(int base, int exponent)
{
	int result = 1;
	int i;

	for (i = 0; i < exponent; i++)
	{
		result = result * base;
	}

	return result;
}

int faraday_clk_pll_prepare(struct clk_hw *hw)
{
	struct faraday_clk_pll *pll = to_faraday_clk_pll(hw);
	unsigned int reg;

	reg = readl(pll->reg);
	reg |= BIT(pll->en_shift);
	writel(reg, pll->reg);

	return 0;
}

void faraday_clk_pll_unprepare(struct clk_hw *hw)
{
	struct faraday_clk_pll *pll = to_faraday_clk_pll(hw);
	unsigned int reg;

	reg = readl(pll->reg);
	reg &= ~BIT(pll->en_shift);
	writel(reg, pll->reg);
}

int faraday_clk_pll_is_prepared(struct clk_hw *hw)
{
	struct faraday_clk_pll *pll = to_faraday_clk_pll(hw);
	unsigned int reg;

	reg = readl(pll->reg) & BIT(pll->en_shift);

	return reg ? 1 : 0;
}

unsigned long faraday_clk_pll_recalc_rate(struct clk_hw *hw,
		unsigned long parent_rate)
{
	struct faraday_clk_pll *pll = to_faraday_clk_pll(hw);
	unsigned long long int rate;
	unsigned int reg, ms, ns, ps = 0;
	unsigned int mult, div;

	reg = readl(pll->reg);
	ns = (reg >> pll->ns_shift) & pll->ns_mask;
	ms = (reg >> pll->ms_shift) & pll->ms_mask;
	if (pll->ps_mask)
		ps = (reg >> pll->ps_shift) & pll->ps_mask;

	mult = ns;
	div = ms * (ps == 0 ? 1 : 2 * ps);

	rate = (unsigned long long int)parent_rate * mult;
	do_div(rate, div);

	return (unsigned long)rate;
}

const struct clk_ops faraday_clk_pll_ops = {
	.prepare = faraday_clk_pll_prepare,
	.unprepare = faraday_clk_pll_unprepare,
	.is_prepared = faraday_clk_pll_is_prepared,
	.recalc_rate = faraday_clk_pll_recalc_rate,
};

struct clk * faraday_clk_register_pll(struct device *dev, const char *name,
		const char *parent_name, unsigned long flags,
		void __iomem *reg, unsigned int en_shift,
		unsigned int ms_shift, unsigned int ms_mask,
		unsigned int ns_shift, unsigned int ns_mask,
		unsigned int ps_shift, unsigned int ps_mask)
{
	struct faraday_clk_pll *pll;
	struct clk_init_data init = {};
	struct clk *hw;

	pll = kzalloc(sizeof(pll), GFP_KERNEL);
	if (!pll)
		return ERR_PTR(-ENOMEM);

	pll->name = name;
	pll->reg = reg;
	pll->en_shift = en_shift;
	pll->ms_shift = ms_shift;
	pll->ms_mask = ms_mask;
	pll->ns_shift = ns_shift;
	pll->ns_mask = ns_mask;
	pll->ps_shift = ps_shift;
	pll->ps_mask = ps_mask;
	pll->hw.init = &init;

	init.name = name;
	init.ops = &faraday_clk_pll_ops;
	init.flags = flags;
	init.parent_names = parent_name ? &parent_name : NULL;
	init.num_parents = 1;

	hw = clk_register(NULL, &pll->hw);
	if (IS_ERR(hw))
		kfree(pll);

	return hw;
}

void faraday_clk_unregister_pll(struct clk *clk)
{
	struct clk_hw *hw;

	hw = __clk_get_hw(clk);
	if (!hw)
		return;

	clk_unregister(clk);
	kfree(to_faraday_clk_pll(hw));
}

int samsung_clk_pll_prepare(struct clk_hw *hw)
{
	struct samsung_clk_pll *pll = to_samsung_clk_pll(hw);
	unsigned int reg;

	reg = readl(pll->reg);
	reg |= BIT(pll->en_shift);
	writel(reg, pll->reg);

	return 0;
}

void samsung_clk_pll_unprepare(struct clk_hw *hw)
{
	struct samsung_clk_pll *pll = to_samsung_clk_pll(hw);
	unsigned int reg;

	reg = readl(pll->reg);
	reg &= ~BIT(pll->en_shift);
	writel(reg, pll->reg);
}

int samsung_clk_pll_is_prepared(struct clk_hw *hw)
{
	struct samsung_clk_pll *pll = to_samsung_clk_pll(hw);
	unsigned int reg;

	reg = readl(pll->reg) & BIT(pll->en_shift);

	return reg ? 1 : 0;
}

unsigned long samsung_clk_pll_recalc_rate(struct clk_hw *hw,
		unsigned long parent_rate)
{
	struct samsung_clk_pll *pll = to_samsung_clk_pll(hw);
	unsigned long long int rate;
	unsigned int reg, m, p, s;
	unsigned int mult, div;

	reg = readl(pll->reg);
	m = (reg >> pll->m_shift) & pll->m_mask;
	p = (reg >> pll->p_shift) & pll->p_mask;
	s = (reg >> pll->s_shift) & pll->s_mask;

	mult = m;
	div = p * power(2, s);

	rate = (unsigned long long int)parent_rate * mult;
	do_div(rate, div);

	return (unsigned long)rate;
}

const struct clk_ops samsung_clk_pll_ops = {
	.prepare = samsung_clk_pll_prepare,
	.unprepare = samsung_clk_pll_unprepare,
	.is_prepared = samsung_clk_pll_is_prepared,
	.recalc_rate = samsung_clk_pll_recalc_rate,
};

struct clk * samsung_clk_register_pll(struct device *dev, const char *name,
		const char *parent_name, unsigned long flags,
		void __iomem *reg, unsigned int en_shift,
		unsigned int m_shift, unsigned int m_mask,
		unsigned int p_shift, unsigned int p_mask,
		unsigned int s_shift, unsigned int s_mask)
{
	struct samsung_clk_pll *pll;
	struct clk_init_data init = {};
	struct clk *hw;

	pll = kzalloc(sizeof(pll), GFP_KERNEL);
	if (!pll)
		return ERR_PTR(-ENOMEM);

	pll->name = name;
	pll->reg = reg;
	pll->en_shift = en_shift;
	pll->m_shift = m_shift;
	pll->m_mask = m_mask;
	pll->p_shift = p_shift;
	pll->p_mask = p_mask;
	pll->s_shift = s_shift;
	pll->s_mask = s_mask;
	pll->hw.init = &init;

	init.name = name;
	init.ops = &samsung_clk_pll_ops;
	init.flags = flags;
	init.parent_names = parent_name ? &parent_name : NULL;
	init.num_parents = 1;

	hw = clk_register(NULL, &pll->hw);
	if (IS_ERR(hw))
		kfree(pll);

	return hw;
}

void samsung_clk_unregister_pll(struct clk *clk)
{
	struct clk_hw *hw;

	hw = __clk_get_hw(clk);
	if (!hw)
		return;

	clk_unregister(clk);
	kfree(to_faraday_clk_pll(hw));
}