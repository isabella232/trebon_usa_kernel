/**
 *	S5PV210 Internal dpram specific settings
 */
#include <linux/module.h>
#include <mach/regs-gpio.h>
#include <mach/regs-mem.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/wakelock.h>
#include <linux/semaphore.h>
#include <linux/delay.h>


/*	S5PV210 Interanl Dpram Special Function Register 	*/
#define IDPRAM_MIFCON_INT2APEN      (1<<2)
#define IDPRAM_MIFCON_INT2MSMEN     (1<<3)
#define IDPRAM_MIFCON_DMATXREQEN_0  (1<<16)
#define IDPRAM_MIFCON_DMATXREQEN_1  (1<<17)
#define IDPRAM_MIFCON_DMARXREQEN_0  (1<<18)
#define IDPRAM_MIFCON_DMARXREQEN_1  (1<<19)
#define IDPRAM_MIFCON_FIXBIT        (1<<20)

#define IDPRAM_MIFPCON_ADM_MODE     (1<<6) /* mux / demux mode  */

#define IDPRAM_DMA_ADR_MASK         0x3FFF
#define IDPRAM_DMA_TX_ADR_0         /* shift 0 */
#define IDPRAM_DMA_TX_ADR_1         /* shift 16  */
#define IDPRAM_DMA_RX_ADR_0         /* shift 0  */
#define IDPRAM_DMA_RX_ADR_1         /* shift 16  */

#define IRQ_DPRAM_AP_NIT_N IRQ_MSM
#define IDPRAM_SFR_PHYSICAL_ADDR 0xED008000 
#define IDPRAM_SFR_SIZE 0x1C

struct idpram_sfr_reg {
	unsigned int2ap;
	unsigned int2msm;
	unsigned mifcon;
	unsigned mifpcon;
	unsigned msmintclr;
	unsigned dma_tx_adr;
	unsigned dma_rx_adr;
};

/*	S5PV210 Internal Dpram GPIO table 	*/
struct idpram_gpio_data {
	unsigned num;
	unsigned cfg;
	unsigned pud;
	unsigned val;
};

static volatile void __iomem *s5pv210_dpram_sfr_va;

#define IDPRAM_ADDRESS_DEMUX 1
static struct idpram_gpio_data idpram_gpio_address[] = {
#ifdef IDPRAM_ADDRESS_DEMUX
	{
		.num = S5PV210_GPJ0(0),	/* MSM_ADDR 0 -12 */
		.cfg = S3C_GPIO_SFN(0x2), 
		.pud = S3C_GPIO_PULL_NONE,
	}, {
		.num = S5PV210_GPJ0(1),////changed from S5PV310_GPE1
		.cfg = S3C_GPIO_SFN(0x2),
		.pud = S3C_GPIO_PULL_NONE,
	}, {
		.num = S5PV210_GPJ0(2),
		.cfg = S3C_GPIO_SFN(0x2),
		.pud = S3C_GPIO_PULL_NONE,
	}, {
		.num = S5PV210_GPJ0(3),
		.cfg = S3C_GPIO_SFN(0x2),
		.pud = S3C_GPIO_PULL_NONE,
	}, {
		.num = S5PV210_GPJ0(4),
		.cfg = S3C_GPIO_SFN(0x2),
		.pud = S3C_GPIO_PULL_NONE,
	}, {
		.num = S5PV210_GPJ0(5),
		.cfg = S3C_GPIO_SFN(0x2),
		.pud = S3C_GPIO_PULL_NONE,
	}, {
		.num = S5PV210_GPJ0(6),
		.cfg = S3C_GPIO_SFN(0x2),
		.pud = S3C_GPIO_PULL_NONE,
	}, {
		.num = S5PV210_GPJ0(7),
		.cfg = S3C_GPIO_SFN(0x2),
		.pud = S3C_GPIO_PULL_NONE,
	}, {
		.num = S5PV210_GPJ1(0),   //changed from S5PV310_GPE2
		.cfg = S3C_GPIO_SFN(0x2),
		.pud = S3C_GPIO_PULL_NONE,
	}, {
		.num = S5PV210_GPJ1(1),
		.cfg = S3C_GPIO_SFN(0x2),
		.pud = S3C_GPIO_PULL_NONE,
	}, {
		.num = S5PV210_GPJ1(2),
		.cfg = S3C_GPIO_SFN(0x2),
		.pud = S3C_GPIO_PULL_NONE,
	}, {
		.num = S5PV210_GPJ1(3),
		.cfg = S3C_GPIO_SFN(0x2),
		.pud = S3C_GPIO_PULL_NONE,
	}, {
		.num = S5PV210_GPJ1(4),
		.cfg = S3C_GPIO_SFN(0x2),
		.pud = S3C_GPIO_PULL_NONE,
	}, {
		.num = S5PV210_GPJ1(5),  
		.cfg = S3C_GPIO_SFN(0x2),
		.pud = S3C_GPIO_PULL_NONE,
	},
#endif
};

static struct idpram_gpio_data idpram_gpio_data[] = {
	{
		.num = S5PV210_GPJ2(0), /* MSM_DATA 0 - 15 */ //changed from S5PV310_GPE3
		.cfg = S3C_GPIO_SFN(0x2),
		.pud = S3C_GPIO_PULL_NONE,
	}, {
		.num = S5PV210_GPJ2(1),
		.cfg = S3C_GPIO_SFN(0x2),
		.pud = S3C_GPIO_PULL_NONE,
	}, {
		.num = S5PV210_GPJ2(2),
		.cfg = S3C_GPIO_SFN(0x2),
		.pud = S3C_GPIO_PULL_NONE,
	}, {
		.num = S5PV210_GPJ2(3),
		.cfg = S3C_GPIO_SFN(0x2),
		.pud = S3C_GPIO_PULL_NONE,
	}, {
		.num = S5PV210_GPJ2(4),
		.cfg = S3C_GPIO_SFN(0x2),
		.pud = S3C_GPIO_PULL_NONE,
	}, {
		.num = S5PV210_GPJ2(5),
		.cfg = S3C_GPIO_SFN(0x2),
		.pud = S3C_GPIO_PULL_NONE,
	}, {
		.num = S5PV210_GPJ2(6),
		.cfg = S3C_GPIO_SFN(0x2),
		.pud = S3C_GPIO_PULL_NONE,
	}, {
		.num = S5PV210_GPJ2(7),
		.cfg = S3C_GPIO_SFN(0x2),
		.pud = S3C_GPIO_PULL_NONE,
	}, {
		.num = S5PV210_GPJ3(0),   //changed from S5PV310_GPE4
		.cfg = S3C_GPIO_SFN(0x2),
		.pud = S3C_GPIO_PULL_NONE,
	}, {
		.num = S5PV210_GPJ3(1),
		.cfg = S3C_GPIO_SFN(0x2),
		.pud = S3C_GPIO_PULL_NONE,
	}, {
		.num = S5PV210_GPJ3(2),
		.cfg = S3C_GPIO_SFN(0x2),
		.pud = S3C_GPIO_PULL_NONE,
	}, {
		.num = S5PV210_GPJ3(3),
		.cfg = S3C_GPIO_SFN(0x2),
		.pud = S3C_GPIO_PULL_NONE,
	}, {
		.num = S5PV210_GPJ3(4),
		.cfg = S3C_GPIO_SFN(0x2),
		.pud = S3C_GPIO_PULL_NONE,
	}, {
		.num = S5PV210_GPJ3(5),
		.cfg = S3C_GPIO_SFN(0x2),
		.pud = S3C_GPIO_PULL_NONE,
	}, {
		.num = S5PV210_GPJ3(6),
		.cfg = S3C_GPIO_SFN(0x2),
		.pud = S3C_GPIO_PULL_NONE,
	}, {
		.num = S5PV210_GPJ3(7),
		.cfg = S3C_GPIO_SFN(0x2),
		.pud = S3C_GPIO_PULL_NONE,
	},
};

static struct idpram_gpio_data idpram_gpio_init_control[] = {
	{
		.num = S5PV210_GPJ4(0), /* MDM_CSn */  //changed from S5PV310_GPE0 & S5PV210_GPJ4(1)
		.cfg = S3C_GPIO_SFN(0x2),
		.pud = S3C_GPIO_PULL_NONE,
	}, {
		.num = S5PV210_GPJ4(1), /* MDM_WEn */ //changed from S5PV310_GPE0 & S5PV210_GPJ4(0)
		.cfg = S3C_GPIO_SFN(0x2),
		.pud = S3C_GPIO_PULL_NONE,
	}, {
		.num = S5PV210_GPJ4(2), /* MDM_Rn */
		.cfg = S3C_GPIO_SFN(0x2),
		.pud = S3C_GPIO_PULL_NONE,
	}, {
		.num = S5PV210_GPJ4(3), /* MDM_IRQn */
		.cfg = S3C_GPIO_SFN(0x2),
		.pud = S3C_GPIO_PULL_NONE,//changed from S3C_GPIO_PULL_UP
	},
#ifndef IDPRAM_ADDRESS_DEMUX
	{
		.num = S5PV210_GPJ4(4), /* MDM_ADVN */   //changed from S5PV310_GPE0   
		.cfg = S3C_GPIO_SFN(0x2),
		.pud = S3C_GPIO_PULL_NONE,
	},
#endif
};

static struct idpram_gpio_data idpram_gpio_deinit_control[] = {
	{
		.num = S5PV210_GPJ4(0), /* MDM_CSn */  //changed from S5PV310_GPE0
		.cfg = S3C_GPIO_SFN(0x2),
		.pud = S3C_GPIO_PULL_NONE,
	}, {
		.num = S5PV210_GPJ4(1), /* MDM_WEn */
		.cfg = S3C_GPIO_SFN(0x2),
		.pud = S3C_GPIO_PULL_NONE,
	}, {
		.num = S5PV210_GPJ4(2), /* MDM_Rn */
		.cfg = S3C_GPIO_SFN(0x2),
		.pud = S3C_GPIO_PULL_NONE,
	}, {
		.num = S5PV210_GPJ4(3), /* MDM_IRQn */
		.cfg = S3C_GPIO_SFN(0x2),
		.pud = S3C_GPIO_PULL_NONE,
	},
#ifndef IDPRAM_ADDRESS_DEMUX
	{
		.num = S5PV210_GPJ4(4), /* MDM_ADVN */ //changed from S5PV310_GPE0
		.cfg = S3C_GPIO_SFN(0x2),
		.pud = S3C_GPIO_PULL_NONE,
	}
#endif
};

static void idpram_gpio_cfg(struct idpram_gpio_data *gpio)
{
	printk(KERN_DEBUG "idpram set gpio num=%d, cfg=%d, pud=%d, val=%d\n",
		gpio->num, gpio->cfg, gpio->pud, gpio->val);

	s3c_gpio_cfgpin(gpio->num, gpio->cfg);
	s3c_gpio_setpull(gpio->num, gpio->pud);
	if (gpio->val)
		gpio_set_value(gpio->num, gpio->val);
}

void idpram_gpio_init(void)
{
	int i;
  
	for (i = 0; i < ARRAY_SIZE(idpram_gpio_address); i++)
		idpram_gpio_cfg(&idpram_gpio_address[i]);

	for (i = 0; i < ARRAY_SIZE(idpram_gpio_data); i++)
		idpram_gpio_cfg(&idpram_gpio_data[i]);

	for (i = 0; i < ARRAY_SIZE(idpram_gpio_init_control); i++)
		idpram_gpio_cfg(&idpram_gpio_init_control[i]);
	
	printk(KERN_DEBUG "idpram gpio init done"); 	
}
EXPORT_SYMBOL(idpram_gpio_init);

int idpram_sfr_init(void)
{ 
	volatile struct idpram_sfr_reg __iomem *sfr;
	struct clk *clk;

	/* enable internal dpram clock */
	clk = clk_get(NULL, "modem");
	if (!clk) {
		printk(KERN_ERR  "idpram failed to get clock %s\n", __func__);
		return -EFAULT;
	}
	clk_enable(clk);
	if (!s5pv210_dpram_sfr_va) {
		s5pv210_dpram_sfr_va = (volatile struct idpram_sfr_reg __iomem *)
		ioremap_nocache(IDPRAM_SFR_PHYSICAL_ADDR, IDPRAM_SFR_SIZE);
		if (!s5pv210_dpram_sfr_va) {
			printk(KERN_ERR "idpram_sfr_base io-remap fail\n");
			/*iounmap(idpram_base);*/
			return -EFAULT;
		}
	}
	sfr = s5pv210_dpram_sfr_va;
	sfr->mifcon = (IDPRAM_MIFCON_FIXBIT | IDPRAM_MIFCON_INT2APEN); 
	
#ifndef IDPRAM_ADDRESS_DEMUX
	sfr->mifpcon = (IDPRAM_MIFPCON_ADM_MODE);
#endif
    	printk(KERN_DEBUG "idpram_sfr_init done");
	return 0;
}
EXPORT_SYMBOL(idpram_sfr_init);


void s5pv210_dpram_int_clear(void)
{   
   	//printk("[MODEM_IF] %s %d \n",__FUNCTION__,__LINE__);
	volatile struct idpram_sfr_reg __iomem *sfr = s5pv210_dpram_sfr_va;
	sfr->msmintclr = 0xFF;
}
EXPORT_SYMBOL(s5pv210_dpram_int_clear);
