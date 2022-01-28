/*
 * Core driver for the Faraday pin controller
 *
 * (C) Copyright 2014 Faraday Technology
 * Yan-Pai Chen <ypchen@faraday-tech.com>
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

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/pinctrl/machine.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinmux.h>
#include "../core.h"
#include "pinctrl-ftscu010.h"

struct ftscu010_pmx {
	struct device *dev;
	struct pinctrl_dev *pctl;
	const struct ftscu010_pinctrl_soc_data *soc;
	void __iomem *base;
};

/*
 * Pin group operations
 */
static int ftscu010_pinctrl_get_groups_count(struct pinctrl_dev *pctldev)
{
	struct ftscu010_pmx *pmx = pinctrl_dev_get_drvdata(pctldev);

	return pmx->soc->ngroups;
}

static const char *ftscu010_pinctrl_get_group_name(struct pinctrl_dev *pctldev,
						   unsigned selector)
{
	struct ftscu010_pmx *pmx = pinctrl_dev_get_drvdata(pctldev);

	if (selector >= pmx->soc->ngroups)
		return NULL;

	return pmx->soc->groups[selector].name;
}

static int ftscu010_pinctrl_get_group_pins(struct pinctrl_dev *pctldev,
					   unsigned selector,
					   const unsigned **pins,
					   unsigned *npins)
{
	struct ftscu010_pmx *pmx = pinctrl_dev_get_drvdata(pctldev);

	if (selector >= pmx->soc->ngroups)
		return -EINVAL;

	*pins = pmx->soc->groups[selector].pins;
	*npins = pmx->soc->groups[selector].npins;
	return 0;
}

#ifdef CONFIG_DEBUG_FS
static void ftscu010_pinctrl_pin_dbg_show(struct pinctrl_dev *pctldev,
				       struct seq_file *s,
				       unsigned offset)
{
	seq_printf(s, " %s", dev_name(pctldev->dev));
}
#endif

static int ftscu010_pinctrl_dt_node_to_map(struct pinctrl_dev *pctldev,
					   struct device_node *np,
					   struct pinctrl_map **map,
					   unsigned *num_maps)
{
	struct ftscu010_pmx *pmx = pinctrl_dev_get_drvdata(pctldev);
	struct pinctrl_map *new_map;
	struct property *function;
	int map_num = 1, err;
	u32 func;

	new_map = kmalloc(sizeof(struct pinctrl_map) * map_num, GFP_KERNEL);
	if (!new_map)
		return -ENOMEM;

	*map = new_map;
	*num_maps = 1;

	function = of_find_property(np, "scu010,function", NULL);
	if (!function) {
		dev_err(pmx->dev, "%pOF: missing scu010,function property\n", np);
		return -EINVAL;
	}

	err = of_property_read_u32(np, "scu010,function", &func);
	if (err) {
		dev_err(pmx->dev, "%pOF: read property value error\n", np);
		return -EINVAL;
	}

	new_map[0].type = PIN_MAP_TYPE_MUX_GROUP;
	new_map[0].data.mux.function = pmx->soc->functions[func].name;
	new_map[0].data.mux.group = NULL;

	dev_info(pmx->dev, "maps: function %s(#%d) group %s num %d\n",
	         (*map)->data.mux.function, func, (*map)->data.mux.group, map_num);

	return 0;
}

static void ftscu010_pinctrl_dt_free_map(struct pinctrl_dev *pctldev,
					 struct pinctrl_map *map,
					 unsigned num_maps)
{
	kfree(map);
}

static struct pinctrl_ops ftscu010_pinctrl_ops = {
	.get_groups_count = ftscu010_pinctrl_get_groups_count,
	.get_group_name = ftscu010_pinctrl_get_group_name,
	.get_group_pins = ftscu010_pinctrl_get_group_pins,
#ifdef CONFIG_DEBUG_FS
	.pin_dbg_show = ftscu010_pinctrl_pin_dbg_show,
#endif
	.dt_node_to_map = ftscu010_pinctrl_dt_node_to_map,
	.dt_free_map = ftscu010_pinctrl_dt_free_map,
};

#define FTSCU010_PIN_MASK	0x3			/* 2-bit per pin */
#define FTSCU010_PIN_SHIFT(pin)	((pin) % 16) << 1

/*
 * pinmux operations
 */
static int ftscu010_pinctrl_get_funcs_count(struct pinctrl_dev *pctldev)
{
	struct ftscu010_pmx *pmx = pinctrl_dev_get_drvdata(pctldev);

	return pmx->soc->nfunctions;
}

static const char *ftscu010_pinctrl_get_func_name(struct pinctrl_dev *pctldev,
						  unsigned selector)
{
	struct ftscu010_pmx *pmx = pinctrl_dev_get_drvdata(pctldev);

	return pmx->soc->functions[selector].name;
}

static int ftscu010_pinctrl_get_func_groups(struct pinctrl_dev *pctldev,
					     unsigned selector,
					     const char * const **groups,
					     unsigned * const ngroups)
{
	struct ftscu010_pmx *pmx = pinctrl_dev_get_drvdata(pctldev);

	*groups = pmx->soc->functions[selector].groups;
	*ngroups = pmx->soc->functions[selector].ngroups;
	return 0;
}

static unsigned int ftscu010_pinmux_setting(const struct ftscu010_pin *pin,
					    unsigned selector)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(pin->functions); ++i) {
		if (pin->functions[i] == selector)
			break;
	}

	WARN_ON(i == ARRAY_SIZE(pin->functions));

	return i;
}

static void ftscu010_pinctrl_setup(struct pinctrl_dev *pctldev,
				   unsigned selector,
				   unsigned gselector,
				   bool enable)
{
	struct ftscu010_pmx *pmx = pinctrl_dev_get_drvdata(pctldev);
	const struct ftscu010_pinctrl_soc_data *soc = pmx->soc;
	const struct ftscu010_pin_group *group;
	int i, npins;

	dev_info(pmx->dev, "setup: %s function %s(#%d) group %s(#%d)\n",
		enable ? "enable" : "disable",
		soc->functions[selector].name, selector,
		soc->groups[gselector].name, gselector);

	group = &pmx->soc->groups[gselector];
	npins = group->npins;

	/* for each pin in the group */
	for (i = 0; i < npins; ++i) {
		unsigned int id = group->pins[i];
		const struct ftscu010_pin *pin = &pmx->soc->map[id];
#ifdef CONFIG_PINCTRL_FTSCU010_GROUP
		int shift = 0;
#else
		int shift = FTSCU010_PIN_SHIFT(id);
#endif
		unsigned int val;

		val = readl(pmx->base + pin->offset);
		val &= ~(FTSCU010_PIN_MASK << shift);
		if (enable)
			val |= ftscu010_pinmux_setting(pin, selector) << shift;
		writel(val, pmx->base + pin->offset);
	}

}

static int ftscu010_pinctrl_set_mux(struct pinctrl_dev *pctldev,
				    unsigned selector, unsigned group)
{
	ftscu010_pinctrl_setup(pctldev, selector, group, true);
	return 0;
}

static struct pinmux_ops ftscu010_pinmux_ops = {
	.get_functions_count = ftscu010_pinctrl_get_funcs_count,
	.get_function_name = ftscu010_pinctrl_get_func_name,
	.get_function_groups = ftscu010_pinctrl_get_func_groups,
	.set_mux = ftscu010_pinctrl_set_mux,
};

static struct pinctrl_desc ftscu010_pinctrl_desc = {
	.pctlops = &ftscu010_pinctrl_ops,
	.pmxops = &ftscu010_pinmux_ops,
	.owner = THIS_MODULE,
};

int ftscu010_pinctrl_probe(struct platform_device *pdev,
			   const struct ftscu010_pinctrl_soc_data *data)
{
	struct ftscu010_pmx *pmx;
	struct resource *res;
	
	pmx = devm_kzalloc(&pdev->dev, sizeof(*pmx), GFP_KERNEL);
	if (!pmx) {
		dev_err(&pdev->dev, "Could not alloc ftscu010_pmx\n");
		return -ENOMEM;
	}

	pmx->dev = &pdev->dev;
	pmx->soc = data;

	ftscu010_pinctrl_desc.name = dev_name(&pdev->dev);
	ftscu010_pinctrl_desc.pins = pmx->soc->pins;
	ftscu010_pinctrl_desc.npins = pmx->soc->npins;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0); 
	if (!res)
		return -ENOENT;
	
	pmx->base = devm_ioremap_resource(&pdev->dev, res);
	if (!pmx->base)
		return -ENOMEM;

	pmx->pctl = pinctrl_register(&ftscu010_pinctrl_desc, &pdev->dev, pmx);
	if (!pmx->pctl) {
		dev_err(&pdev->dev, "Could not register FTSCU010 pinmux driver\n");
		return -EINVAL;
	}

	platform_set_drvdata(pdev, pmx);
	dev_info(&pdev->dev, "Initialized FTSCU010 pinmux driver\n");

	return 0;
}
EXPORT_SYMBOL_GPL(ftscu010_pinctrl_probe);

int ftscu010_pinctrl_remove(struct platform_device *pdev)
{
	struct ftscu010_pmx *pmx = platform_get_drvdata(pdev);

	pinctrl_unregister(pmx->pctl);
	return 0;
}
EXPORT_SYMBOL_GPL(ftscu010_pinctrl_remove);