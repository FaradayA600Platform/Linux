/**
 *  drivers/reset/faraday/reset-a600.c
 *
 *  Faraday A600 Reset Driver
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

#include <linux/io.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/reset-controller.h>

#define to_reset_data(x) container_of(x, struct a600_reset_data, rc_dev)

struct a600_reset_data {
	struct reset_controller_dev rc_dev;
	void __iomem                *base;
};

static int a600_assert(struct reset_controller_dev *rc_dev,
				    unsigned long id)
{
	struct a600_reset_data *data = to_reset_data(rc_dev);
	unsigned int offset = (id >> 8) & 0xffff;
	unsigned int shift = id & 0xff;
	unsigned int val;

	val = readl(data->base + offset);
	val &= ~BIT(shift);
	writel(val, data->base + offset);

	return 0;
}

static int a600_deassert(struct reset_controller_dev *rc_dev,
				    unsigned long id)
{
	struct a600_reset_data *data = to_reset_data(rc_dev);
	unsigned int offset = (id >> 8) & 0xffff;
	unsigned int shift = id & 0xff;
	unsigned int val;

	val = readl(data->base + offset);
	val |= BIT(shift);
	writel(val, data->base + offset);

	return 0;
}

static const struct reset_control_ops a600_reset_ops = {
	.assert = a600_assert,
	.deassert = a600_deassert,
};

static int a600_reset_xlate(struct reset_controller_dev *rcdev,
			      const struct of_phandle_args *reset_spec)
{
	unsigned int offset, bit;

	offset = reset_spec->args[0];
	bit = reset_spec->args[1];

	return (offset << 8) | bit;
}

static int a600_reset_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct resource *res;
	struct a600_reset_data *data;

	data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	data->base = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (IS_ERR(data->base))
		return PTR_ERR(data->base);

	data->rc_dev.owner = THIS_MODULE;
	data->rc_dev.of_node = np;
	data->rc_dev.of_reset_n_cells = 2;
	data->rc_dev.of_xlate = a600_reset_xlate;
	data->rc_dev.ops = &a600_reset_ops;
	data->rc_dev.nr_resets = 32;

	platform_set_drvdata(pdev, data);

	return devm_reset_controller_register(&pdev->dev, &data->rc_dev);
}

static const struct of_device_id a600_reset_match[] = {
	{
		.compatible = "faraday,a600-reset",
	},
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, a600_reset_match);

static struct platform_driver a600_reset_driver = {
	.probe = a600_reset_probe,
	.driver = {
		.name = "reset-a600",
		.of_match_table = a600_reset_match,
	},
};

static int __init a600_reset_init(void)
{
	return platform_driver_register(&a600_reset_driver);
}

postcore_initcall(a600_reset_init);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("B.C. Chen <bcchen@faraday-tech.com>");
MODULE_DESCRIPTION("Faraday A600 reset driver");