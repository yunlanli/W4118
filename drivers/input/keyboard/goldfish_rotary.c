// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/types.h>
#include <linux/limits.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/acpi.h>

#ifndef CHAR_BIT
#define CHAR_BIT 8
#endif

enum {
	REG_READ        = 0x00,
	REG_SET_PAGE    = 0x00,
	REG_LEN         = 0x04,
	REG_DATA        = 0x08,

	PAGE_NAME       = 0x00000,
	PAGE_EVBITS     = 0x10000,
	PAGE_ABSDATA    = 0x20000 | EV_ABS,

	IRQ_MAGIC       = 987642334
};

struct event_dev {
	int magic;
	int irq;
	struct input_dev *input;
	void __iomem *addr;
	char name[0];
};

static irqreturn_t rotary_interrupt_impl(struct event_dev *edev)
{
	unsigned int type, code, value;

	type = readl(edev->addr + REG_READ);
	code = readl(edev->addr + REG_READ);
	value = readl(edev->addr + REG_READ);

	input_event(edev->input, type, code, value);
	return IRQ_HANDLED;
}

static irqreturn_t rotary_interrupt(int irq, void *dev_id)
{
	struct event_dev *edev = dev_id;

	if (edev->magic != IRQ_MAGIC)
		return IRQ_NONE;

	return rotary_interrupt_impl(edev);
}

static void rotary_import_bits(struct event_dev *edev,
			       unsigned long bits[],
			       unsigned int type, size_t count)
{
	void __iomem *addr = edev->addr;
	int i, j;
	size_t size;
	uint8_t val;

	writel(PAGE_EVBITS | type, addr + REG_SET_PAGE);

	size = readl(addr + REG_LEN) * CHAR_BIT;
	if (size < count)
		count = size;

	addr += REG_DATA;
	for (i = 0; i < count; i += CHAR_BIT) {
		val = readb(addr++);
		for (j = 0; j < CHAR_BIT; j++)
			if (val & 1 << j)
				set_bit(i + j, bits);
	}
}

static void rotary_import_abs_params(struct event_dev *edev)
{
	struct input_dev *input_dev = edev->input;
	void __iomem *addr = edev->addr;
	u32 val[4];
	int count;
	int i, j;

	writel(PAGE_ABSDATA, addr + REG_SET_PAGE);

	count = readl(addr + REG_LEN) / sizeof(val);
	if (count > ABS_MAX)
		count = ABS_MAX;

	for (i = 0; i < count; i++) {
		if (!test_bit(i, input_dev->absbit))
			continue;

		for (j = 0; j < ARRAY_SIZE(val); j++) {
			int offset = (i * ARRAY_SIZE(val) + j) * sizeof(u32);
			val[j] = readl(edev->addr + REG_DATA + offset);
		}

		input_set_abs_params(input_dev, i,
				     val[0], val[1], val[2], val[3]);
	}
}

static int rotary_probe(struct platform_device *pdev)
{
	struct input_dev *input_dev;
	struct event_dev *edev;
	struct resource *res;
	unsigned int keymapnamelen;
	void __iomem *addr;
	int irq;
	int i;
	int error;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return -EINVAL;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -EINVAL;

	addr = devm_ioremap(&pdev->dev, res->start, PAGE_SIZE);
	if (!addr)
		return -ENOMEM;

	writel(PAGE_NAME, addr + REG_SET_PAGE);
	keymapnamelen = readl(addr + REG_LEN);

	edev = devm_kzalloc(&pdev->dev,
			    sizeof(struct event_dev) + keymapnamelen + 1,
			    GFP_KERNEL);
	if (!edev)
		return -ENOMEM;

	input_dev = devm_input_allocate_device(&pdev->dev);
	if (!input_dev)
		return -ENOMEM;

	edev->magic = IRQ_MAGIC;
	edev->input = input_dev;
	edev->addr = addr;
	edev->irq = irq;

	for (i = 0; i < keymapnamelen; i++)
		edev->name[i] = readb(edev->addr + REG_DATA + i);

	pr_debug("%s: keymap=%s\n", __func__, edev->name);

	input_dev->name = edev->name;
	input_dev->id.bustype = BUS_HOST;
	rotary_import_bits(edev, input_dev->evbit, EV_SYN, EV_MAX);
	rotary_import_bits(edev, input_dev->relbit, EV_REL, REL_MAX);
	rotary_import_bits(edev, input_dev->absbit, EV_ABS, ABS_MAX);

	rotary_import_abs_params(edev);

	error = devm_request_irq(&pdev->dev, edev->irq, rotary_interrupt,
				 IRQF_SHARED, "goldfish-rotary", edev);
	if (error)
		return error;

	error = input_register_device(input_dev);
	if (error)
		return error;

	return 0;
}

static const struct of_device_id goldfish_rotary_of_match[] = {
	{ .compatible = "generic,goldfish-rotary", },
	{},
};
MODULE_DEVICE_TABLE(of, goldfish_rotary_of_match);

static const struct acpi_device_id goldfish_rotary_acpi_match[] = {
	{ "GFSH0008", 0 },
	{ },
};
MODULE_DEVICE_TABLE(acpi, goldfish_rotary_acpi_match);

static struct platform_driver rotary_driver = {
	.probe	= rotary_probe,
	.driver	= {
		.name	= "goldfish_rotary",
		.of_match_table = goldfish_rotary_of_match,
		.acpi_match_table = ACPI_PTR(goldfish_rotary_acpi_match),
	},
};

module_platform_driver(rotary_driver);

MODULE_AUTHOR("Nimrod Gileadi");
MODULE_DESCRIPTION("Goldfish Rotary Encoder Device");
MODULE_LICENSE("GPL v2");
