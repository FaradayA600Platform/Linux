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

#ifndef __CLK_FARADAY_PLL_H
#define __CLK_FARADAY_PLL_H

struct faraday_clk_pll {
	struct clk_hw           hw;
	const char              *name;
	void __iomem            *reg;
	u8                      en_shift;
	u8                      ms_shift;
	u32                     ms_mask;
	u8                      ns_shift;
	u32                     ns_mask;
	u8                      ps_shift;
	u32                     ps_mask;
	unsigned long           flags;
};

struct samsung_clk_pll {
	struct clk_hw           hw;
	const char              *name;
	void __iomem            *reg;
	u8                      en_shift;
	u8                      m_shift;
	u32                     m_mask;
	u8                      p_shift;
	u32                     p_mask;
	u8                      s_shift;
	u32                     s_mask;
	unsigned long           flags;
};

struct clk * faraday_clk_register_pll(struct device *dev, const char *name,
		const char *parent_name, unsigned long flags,
		void __iomem *reg, unsigned int en_shift,
		unsigned int ms_shift, unsigned int ms_mask,
		unsigned int ns_shift, unsigned int ns_mask,
		unsigned int ps_shift, unsigned int ps_mask);
void faraday_clk_unregister_pll(struct clk *clk);

struct clk * samsung_clk_register_pll(struct device *dev, const char *name,
		const char *parent_name, unsigned long flags,
		void __iomem *reg, unsigned int en_shift,
		unsigned int m_shift, unsigned int m_mask,
		unsigned int p_shift, unsigned int p_mask,
		unsigned int s_shift, unsigned int s_mask);
void samsung_clk_unregister_pll(struct clk *clk);

#endif	/* __CLK_FARADAY_PLL_H */