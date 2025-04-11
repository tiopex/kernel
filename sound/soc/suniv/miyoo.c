/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option)any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/regmap.h>
#include <linux/reset.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>

#include <asm/io.h>
#include <asm/gpio.h>
#include <asm/arch-suniv/dma.h>
#include <asm/arch-suniv/cpu.h>
#include <asm/arch-suniv/gpio.h>
#include <asm/arch-suniv/clock.h>
#include <asm/arch-suniv/codec.h>
#include <asm/arch-suniv/common.h>

#define USE_EARPHONE          1		// set to 0 if UART pin becomes inaccessible
#define DMA_IRQ_NUM     	  19

static int miyoo_snd = 1;
int ret;
module_param(miyoo_snd, int, 0444);
MODULE_PARM_DESC(miyoo_snd, "Handheld type");

struct suniv_iomm {
	uint8_t *gpio;
	uint8_t *dma;
};
static struct suniv_iomm iomm={0};

static void suniv_ioremap(void)
{
	iomm.dma = (uint8_t*)ioremap(SUNIV_DMA_BASE, 4096);
	iomm.gpio = (uint8_t*)ioremap(SUNIV_GPIO_BASE, 4096);
}

static void suniv_gpio_init(void)
{
	uint32_t ret;
	switch(miyoo_snd) {
		case 1:
			ret = readl(iomm.gpio + PA_CFG0);
#if defined(USE_EARPHONE)
			ret&= 0xfffff0f0;
#else
			ret &= 0xfffffff0;
#endif
			ret |= 0x00000001;
			writel(ret, iomm.gpio + PA_CFG0);
			suniv_setbits(iomm.gpio + PA_DATA, (1 << 0));
			break;
		case 2:
			ret = readl(iomm.gpio + PD_CFG0);
			ret &= 0xfffffff0;
			ret |= 0x00000001;
			writel(ret, iomm.gpio + PD_CFG0);
			suniv_setbits(iomm.gpio + PD_DATA, (1 << 0));

			ret = readl(iomm.gpio + PD_CFG1);
			ret &= 0xffffff1f;
			ret |= 0x00000010;
			writel(ret, iomm.gpio + PD_CFG1);
			suniv_setbits(iomm.gpio + PD_DATA, (1 << 9));
			break;
	}

}

static irqreturn_t dma_handler(int irq, void *dev_id)
{
	ret = readl(iomm.gpio + PA_DATA);
	if(ret & 4){
		suniv_setbits(iomm.gpio + PA_DATA, (1 << 0));
	}
	else{
		suniv_clrbits(iomm.gpio + PA_DATA, (1 << 0));
	}
	return IRQ_NONE;
}

static int __init enable_speaker_init(void) {
	suniv_ioremap();
	suniv_gpio_init();
	ret = request_irq(DMA_IRQ_NUM, dma_handler, IRQF_SHARED, "miyoo-jack", (void *)dma_handler);
	if (ret) {
		pr_err("Failed to get DMA IRQ\n");
		return ret;
	}
	return 0;
}

static void __exit enable_speaker_exit(void) {
}

module_init(enable_speaker_init);
module_exit(enable_speaker_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("tiopxyz@gmail.com");
MODULE_DESCRIPTION("GPIO Enable speaker driver");
MODULE_VERSION("1.0");