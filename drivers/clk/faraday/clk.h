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

#ifndef __CLK_FARADAY_H
#define __CLK_FARADAY_H

#include "clk-pll.h"

struct faraday_clock_data {
	struct clk_onecell_data clk_data;
	void __iomem            *base;
	void __iomem            *extbase;
};

struct faraday_fixed_rate_clock {
	unsigned int            id;
	char                    *name;
	const char              *parent_name;
	unsigned long           fixed_rate;
	unsigned long           flags;
};

struct faraday_pll_clock {
	unsigned int            id;
	const char              *name;
	const char              *parent_name;
	unsigned long           offset;
	u8                      en_shift;
	u8                      ms_shift;
	u32                     ms_width;
	u8                      ns_shift;
	u32                     ns_width;
	u8                      ps_shift;
	u32                     ps_width;
	unsigned long           flags;
};

struct samsung_pll_clock {
	unsigned int            id;
	const char              *name;
	const char              *parent_name;
	unsigned long           offset;
	u8                      en_shift;
	u8                      m_shift;
	u32                     m_width;
	u8                      p_shift;
	u32                     p_width;
	u8                      s_shift;
	u32                     s_width;
	unsigned long           flags;
};

struct faraday_fixed_factor_clock {
	unsigned int            id;
	char                    *name;
	const char              *parent_name;
	unsigned int            mult;
	unsigned int            div;
	unsigned long           flags;
};

struct faraday_divider_clock {
	unsigned int            id;
	const char              *name;
	const char              *parent_name;
	unsigned long           flags;
	unsigned long           offset;
	u8                      shift;
	u8                      width;
	u8                      div_flags;
	struct clk_div_table    *table;
};

struct faraday_mux_clock {
	unsigned int            id;
	const char              *name;
	const char *const       *parent_names;
	u8                      num_parents;
	unsigned long           flags;
	unsigned long           offset;
	u8                      shift;
	u8                      width;
	u8                      mux_flags;
	u32                     *table;
};

struct faraday_gate_clock {
	unsigned int            id;
	const char              *name;
	const char              *parent_name;
	unsigned long           flags;
	unsigned long           offset;
	u8                      bit_idx;
	u8                      gate_flags;
};

struct faraday_clock_data *faraday_clk_alloc(struct platform_device *pdev,
		int nr_clks);
int faraday_clk_register_fixed_rates(const struct faraday_fixed_rate_clock *clks,
		int nums, struct faraday_clock_data *data);
int faraday_clk_unregister_fixed_rates(const struct faraday_fixed_rate_clock *clks,
		int nums, struct faraday_clock_data *data);
int faraday_clk_register_plls(const struct faraday_pll_clock *clks,
		int nums, void __iomem *base, struct faraday_clock_data *data);
int faraday_clk_unregister_plls(const struct faraday_pll_clock *clks,
		int nums, struct faraday_clock_data *data);
int samsung_clk_register_plls(const struct samsung_pll_clock *clks,
		int nums, void __iomem *base, struct faraday_clock_data *data);
int samsung_clk_unregister_plls(const struct samsung_pll_clock *clks,
		int nums, struct faraday_clock_data *data);
int faraday_clk_register_fixed_factors(const struct faraday_fixed_factor_clock *clks,
		int nums, struct faraday_clock_data *data);
int faraday_clk_unregister_fixed_factors(const struct faraday_fixed_factor_clock *clks,
		int nums, struct faraday_clock_data *data);
int faraday_clk_register_dividers(const struct faraday_divider_clock *clks,
		int nums, void __iomem *base, struct faraday_clock_data *data);
int faraday_clk_unregister_dividers(const struct faraday_divider_clock *clks,
		int nums, struct faraday_clock_data *data);
int faraday_clk_register_muxs(const struct faraday_mux_clock *clks,
		int nums, void __iomem *base, struct faraday_clock_data *data);
int faraday_clk_unregister_muxs(const struct faraday_mux_clock *clks,
		int nums, struct faraday_clock_data *data);
int faraday_clk_register_gates(const struct faraday_gate_clock *clks,
		int nums, void __iomem *base, struct faraday_clock_data *data);
int faraday_clk_unregister_gates(const struct faraday_gate_clock *clks,
		int nums, struct faraday_clock_data *data);

#endif	/* __CLK_FARADAY_H */