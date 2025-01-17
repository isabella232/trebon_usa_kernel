/* linux/arch/arm/mach-s5pv210/mach-aries.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/serial_core.h>
#include <linux/gpio.h>
#include <linux/gpio_event.h>
#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/mfd/max8998.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/usb/ch9.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <linux/clk.h>
#include <linux/usb/ch9.h>
#include <linux/input/cypress-touchkey.h>
#include <linux/input.h>
#include <linux/irq.h>
#include <linux/skbuff.h>
#include <linux/input/k3g.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/setup.h>
#include <asm/mach-types.h>

#include <mach/irqs.h>
#include <mach/map.h>
#include <mach/regs-clock.h>
#include <mach/regs-mem.h>
#include <mach/regs-gpio.h>
#include <mach/gpio.h>
#include <mach/gpio-trebon.h>
#include <mach/gpio-trebon-settings.h>
#include <mach/adc.h>
#include <mach/param.h>
#include <mach/system.h>
#include <mach/sec_switch.h>

#include <linux/usb/gadget.h>
#include <linux/usb/android_composite.h>
#include <linux/fsa9480.h>
#include <linux/vibrator-pwm.h>
#include <linux/pn544.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/wlan_plat.h>
#include <linux/mfd/wm8994/wm8994_pdata.h>

#ifdef CONFIG_ANDROID_PMEM
#include <linux/android_pmem.h>
#include <plat/media.h>
#include <mach/media.h>
#endif

#ifdef CONFIG_S5PV210_POWER_DOMAIN
#include <mach/power-domain.h>
#endif

#ifdef CONFIG_VIDEO_ISX005
#include <media/isx005_platform.h>
#endif

#include <media/common_camera_platform.h>
#include <plat/regs-serial.h>
#include <plat/s5pv210.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <plat/fb.h>
#include <plat/mfc.h>
#include <plat/iic.h>
#include <plat/pm.h>

#include <plat/sdhci.h>
#include <plat/fimc.h>
#include <plat/regs-fimc.h>
#include <plat/jpeg.h>
#include <plat/clock.h>
#include <plat/regs-otg.h>
#include <linux/gp2ap020.h>
#include <linux/k3dh.h>
#include <../../../drivers/video/samsung/s3cfb.h>
#include <linux/sec_jack.h>
#include <linux/input/mxt224.h>
#include <linux/max17040_battery.h>
#include <linux/mfd/max8998.h>
#include <linux/switch.h>
#include <linux/i2c/adp5587.h>
#ifdef CONFIG_CHARGER_FAN5403
#include <linux/fan5403_charger.h>
#endif

#if defined(CONFIG_DVFS_LIMIT)
#include <mach/cpu-freq-v210.h>
#endif

#ifdef CONFIG_KERNEL_DEBUG_SEC
#include <linux/kernel_sec_common.h>
#endif
#if defined (CONFIG_SAMSUNG_PHONE_SVNET)
#include <linux/modemctl.h>
#include <linux/onedram.h>
#endif
#include "chief.h"

#ifdef CONFIG_CHARGER_SMB328A
#include <linux/smb328a_charger.h>
#endif
#define RECOVERY_SYSFS

struct class *sec_class;
EXPORT_SYMBOL(sec_class);

struct device *switch_dev;
EXPORT_SYMBOL(switch_dev);

struct device *gps_dev = NULL;
EXPORT_SYMBOL(gps_dev);

unsigned int HWREV =0;
EXPORT_SYMBOL(HWREV);

void (*sec_set_param_value)(int idx, void *value);
EXPORT_SYMBOL(sec_set_param_value);

void (*sec_get_param_value)(int idx, void *value);
EXPORT_SYMBOL(sec_get_param_value);

#ifdef RECOVERY_SYSFS
static int __init recovery_class_init(void);
#endif

#if defined(CONFIG_FB_S3C_LM347DF01)
extern unsigned int lcd_Idtype;
#define LM347DF01_LCD_ID 0x6b8840
#endif

#define KERNEL_REBOOT_MASK      0xFFFFFFFF
#define REBOOT_MODE_FAST_BOOT		7

#define PREALLOC_WLAN_SEC_NUM		4
#define PREALLOC_WLAN_BUF_NUM		160
#define PREALLOC_WLAN_SECTION_HEADER	24

#define WLAN_SECTION_SIZE_0	(PREALLOC_WLAN_BUF_NUM * 128)
#define WLAN_SECTION_SIZE_1	(PREALLOC_WLAN_BUF_NUM * 128)
#define WLAN_SECTION_SIZE_2	(PREALLOC_WLAN_BUF_NUM * 512)
#define WLAN_SECTION_SIZE_3	(PREALLOC_WLAN_BUF_NUM * 1024)
#define IRQ_TOUCH_INT (IRQ_EINT_GROUP18_BASE+5) /* J0_5 */

#define WLAN_SKB_BUF_NUM	17

static struct sk_buff *wlan_static_skb[WLAN_SKB_BUF_NUM];

struct wifi_mem_prealloc {
	void *mem_ptr;
	unsigned long size;
};

static int aries_notifier_call(struct notifier_block *this,
					unsigned long code, void *_cmd)
{
	int mode = REBOOT_MODE_NONE;

	if ((code == SYS_RESTART) && _cmd) {
		if (!strcmp((char *)_cmd, "arm11_fota"))
			mode = REBOOT_MODE_ARM11_FOTA;
		else if (!strcmp((char *)_cmd, "arm9_fota"))
			mode = REBOOT_MODE_ARM9_FOTA;
		else if (!strcmp((char *)_cmd, "recovery")) {
			mode = REBOOT_MODE_RECOVERY;
#ifdef CONFIG_KERNEL_DEBUG_SEC
			kernel_sec_set_upload_cause(BLK_UART_MSG_FOR_FACTRST_2ND_ACK);
#endif
		}
		else if (!strcmp((char *)_cmd, "bootloader"))
			mode = REBOOT_MODE_FAST_BOOT;
		else if (!strcmp((char *)_cmd, "download"))
			mode = REBOOT_MODE_DOWNLOAD;
#ifdef CONFIG_KERNEL_DEBUG_SEC
		else if (!strcmp((char *)_cmd, "factory_reboot")) {
			mode = REBOOT_MODE_NONE;
			kernel_sec_set_upload_cause(BLK_UART_MSG_FOR_FACTRST_2ND_ACK);
		}
#endif
		else
			mode = REBOOT_MODE_NONE;
	}
	if (code != SYS_POWER_OFF) {
		if (sec_set_param_value) {
			sec_set_param_value(__REBOOT_MODE, &mode);
		}
	}

	return NOTIFY_DONE;
}

static struct notifier_block aries_reboot_notifier = {
	.notifier_call = aries_notifier_call,
};

static ssize_t hwrev_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%x\n", HWREV);
}

static DEVICE_ATTR(hwrev, S_IRUGO, hwrev_show, NULL);

static void gps_gpio_init(void)
{
	struct device *gps_dev;

	gps_dev = device_create(sec_class, NULL, 0, NULL, "gps");
	if (IS_ERR(gps_dev)) {
		pr_err("Failed to create device(gps)!\n");
		goto err;
	}
	if (device_create_file(gps_dev, &dev_attr_hwrev) < 0)
		pr_err("Failed to create device file(%s)!\n",
		       dev_attr_hwrev.attr.name);

	gpio_request(GPIO_GPS_nRST, "GPS_nRST");
	s3c_gpio_setpull(GPIO_GPS_nRST, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_GPS_nRST, S3C_GPIO_OUTPUT);
	gpio_direction_output(GPIO_GPS_nRST, 1);

	gpio_request(GPIO_GPS_PWR_EN, "GPS_PWR_EN");
	s3c_gpio_setpull(GPIO_GPS_PWR_EN, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_GPS_PWR_EN, S3C_GPIO_OUTPUT);
	gpio_direction_output(GPIO_GPS_PWR_EN, 0);

	s3c_gpio_setpull(GPIO_GPS_RXD, S3C_GPIO_PULL_UP);
	gpio_export(GPIO_GPS_nRST, 1);
	gpio_export(GPIO_GPS_PWR_EN, 1);

	gpio_export_link(gps_dev, "GPS_nRST", GPIO_GPS_nRST);
	gpio_export_link(gps_dev, "GPS_PWR_EN", GPIO_GPS_PWR_EN);


 err:
	return;
}

static void aries_switch_init(void)
{
	sec_class = class_create(THIS_MODULE, "sec");

	if (IS_ERR(sec_class))
		pr_err("Failed to create class(sec)!\n");

	switch_dev = device_create(sec_class, NULL, 0, NULL, "switch");

	if (IS_ERR(switch_dev))
		pr_err("Failed to create device(switch)!\n");
};

/* Following are default values for UCON, ULCON and UFCON UART registers */
#define S5PV210_UCON_DEFAULT	(S3C2410_UCON_TXILEVEL |	\
				 S3C2410_UCON_RXILEVEL |	\
				 S3C2410_UCON_TXIRQMODE |	\
				 S3C2410_UCON_RXIRQMODE |	\
				 S3C2410_UCON_RXFIFO_TOI |	\
				 S3C2443_UCON_RXERR_IRQEN)

#define S5PV210_ULCON_DEFAULT	S3C2410_LCON_CS8

#define S5PV210_UFCON_DEFAULT	(S3C2410_UFCON_FIFOMODE |	\
				 S5PV210_UFCON_TXTRIG4 |	\
				 S5PV210_UFCON_RXTRIG4)

#define S5PV210_UFCON_GPS	(S3C2410_UFCON_FIFOMODE |	\
				 S5PV210_UFCON_TXTRIG4 |	\
				 S5PV210_UFCON_RXTRIG1)

static struct s3c2410_uartcfg aries_uartcfgs[] __initdata = {
	{
		.hwport		= 0,
		.flags		= 0,
		.ucon		= S5PV210_UCON_DEFAULT,
		.ulcon		= S5PV210_ULCON_DEFAULT,
		.ufcon		= S5PV210_UFCON_DEFAULT,
#ifdef CONFIG_MACH_HERRING
		.wake_peer	= aries_bt_uart_wake_peer,
#endif
	},
	{
		.hwport		= 1,
		.flags		= 0,
		.ucon		= S5PV210_UCON_DEFAULT,
		.ulcon		= S5PV210_ULCON_DEFAULT,
		.ufcon		= S5PV210_UFCON_DEFAULT,
	},
#ifndef CONFIG_FIQ_DEBUGGER
	{
		.hwport		= 2,
		.flags		= 0,
		.ucon		= S5PV210_UCON_DEFAULT,
		.ulcon		= S5PV210_ULCON_DEFAULT,
		.ufcon		= S5PV210_UFCON_DEFAULT,
	},
#endif
	{
		.hwport		= 3,
		.flags		= 0,
		.ucon		= S5PV210_UCON_DEFAULT,
		.ulcon		= S5PV210_ULCON_DEFAULT,
		.ufcon		= S5PV210_UFCON_DEFAULT,
	},
};

#if defined(CONFIG_TOUCHSCREEN_MELFAS)
static struct platform_device s3c_device_melfas = {
        .name = "melfas-ts",
        .id = -1,
};
#elif defined(CONFIG_TOUCHSCREEN_QT602240)
static struct platform_device s3c_device_qtts = {
        .name = "qt602240-ts",
        .id = -1,
};
#endif

#ifdef CONFIG_KEYBOARD_ADP5587
//******************************adding support for adp5587 and keypad*****************************************************
static unsigned int keypad_keycode[] = {
	158, 217, 2, 3, 4, 5, 6, 7, // 1 ~  8
	102, 139, 106, 108, 105, 51, 57, 16, // 9 ~ 16
	150, 155, 301, 56, 21, 22, 23, 24, //17 ~ 24
	25, 26, 27, 28, 29, 30, 31, 32, //25 ~ 32
	33, 34, 35, 36, 37, 38, 39, 40, //33 ~ 40
	28, 103, 50, 49, 48, 47, 46, 45, //41 ~ 48
	44, 42, 14, 38, 37, 36, 35, 34, //49 ~ 56
	33, 32, 31, 30, 25, 24, 23, 22, //57 ~ 64
	21, 20, 19, 18, 17, 16, 11, 10, //65 ~ 72
	9, 8, 7, 6, 5, 4, 3, 2, //73 ~ 80
};

static struct adp5587_kpad_platform_data adp5587_kpd_platform_data = {
	.rows = 0xFF,         /* 11111111 */
	.cols = 0xFF,         /* 11111111 */
	.en_keylock = 0x0,    /* 01000000 */
	.unlock_key1 = 0x0,   /* 00100000 */
	.unlock_key2 = 0x0,   /* 00010000 */
	.keymap = &keypad_keycode[0],
	.keymapsize = ARRAY_SIZE(keypad_keycode),
	.repeat = 0x0,
};

//*********************************************************************************************************************************
#endif

static struct s3cfb_lcd s6d05a3 = {
	.width = 320,
	.height = 480,
	.p_width = 52,
	.p_height = 74,
	.bpp = 32,
	.freq = 80,

	.timing = {
		.h_fp =  12,
		.h_bp =  40,
		.h_sw = 16,
		.v_fp = 8,
		.v_bp = 7,
		.v_fpe = 1,
		.v_bpe = 1,
		.v_sw = 4,
	},

	.polarity = {
		.rise_vclk = 1,
		.inv_hsync = 0,
		.inv_vsync = 0,
		.inv_vden = 0,
	},
};

#ifdef CONFIG_FB_S3C_LM347DF01
static struct s3cfb_lcd lm347df01 = {
	.width = 320,
	.height = 480,
	.p_width = 52,
	.p_height = 74,
	.bpp = 32,
	.freq = 80,

	.timing = {
		.h_fp = 12,
		.h_bp = 40,
		.h_sw = 16,
		.v_fp = 8,
		.v_fpe = 1,
		.v_bp = 7,
		.v_bpe = 1,
		.v_sw = 4,
	},

	.polarity = {
		.rise_vclk = 1,
		.inv_hsync = 0,
		.inv_vsync = 0,
		.inv_vden = 0,
	},
};

#endif

static struct s3cfb_lcd s6e63m0 = {
	.width = 480,
	.height = 800,
	.p_width = 52,
	.p_height = 86,
	.bpp = 24,
	.freq = 60,

	.timing = {
		.h_fp = 16,
		.h_bp = 16,
		.h_sw = 2,
		.v_fp = 28,
		.v_fpe = 1,
		.v_bp = 1,
		.v_bpe = 1,
		.v_sw = 2,
	},
	.polarity = {
		.rise_vclk = 1,
		.inv_hsync = 1,
		.inv_vsync = 1,
		.inv_vden = 1,
	},
};





#define  S5PV210_VIDEO_SAMSUNG_MEMSIZE_FIMC0 (5600 * SZ_1K)
#define  S5PV210_VIDEO_SAMSUNG_MEMSIZE_FIMC1 (9900 * SZ_1K)
#define  S5PV210_VIDEO_SAMSUNG_MEMSIZE_FIMC2 (5600 * SZ_1K)
#define  S5PV210_VIDEO_SAMSUNG_MEMSIZE_MFC0 (13312 * SZ_1K) // 13MB	(24576 * SZ_1K)
#define  S5PV210_VIDEO_SAMSUNG_MEMSIZE_MFC1 (21504 * SZ_1K) // 21MB	(22528 * SZ_1K)
#define  S5PV210_VIDEO_SAMSUNG_MEMSIZE_FIMD (3072 * SZ_1K)
#define  S5PV210_VIDEO_SAMSUNG_MEMSIZE_JPEG (5120 * SZ_1K)
#define  S5PV210_VIDEO_SAMSUNG_MEMSIZE_PMEM (0 * SZ_1K) //5550->0
#define  S5PV210_VIDEO_SAMSUNG_MEMSIZE_GPU1 (3300 * SZ_1K)
#define  S5PV210_VIDEO_SAMSUNG_MEMSIZE_ADSP (1500 * SZ_1K)
#define  S5PV210_VIDEO_SAMSUNG_MEMSIZE_TEXTSTREAM  (3000 * SZ_1K)

static struct s5p_media_device aries_media_devs[] = {
		[0] = {
		.id = S5P_MDEV_FIMD,
		.name = "fimd",
		.bank = 1,
		.memsize = S5PV210_VIDEO_SAMSUNG_MEMSIZE_FIMD,

		.paddr = 0x4FC00000,

	},
	[1] = {
		.id = S5P_MDEV_MFC,
		.name = "mfc",
		.bank = 0,
		.memsize = S5PV210_VIDEO_SAMSUNG_MEMSIZE_MFC0,
		.paddr = 0,
	},
	[2] = {
		.id = S5P_MDEV_MFC,
		.name = "mfc",
		.bank = 1,
		.memsize = S5PV210_VIDEO_SAMSUNG_MEMSIZE_MFC1,
		.paddr = 0,
	},
	[3] = {
		.id = S5P_MDEV_FIMC0,
		.name = "fimc0",
		.bank = 1,
		.memsize = S5PV210_VIDEO_SAMSUNG_MEMSIZE_FIMC0,
		.paddr = 0,
	},
	[4] = {
		.id = S5P_MDEV_FIMC1,
		.name = "fimc1",
		.bank = 1,
		.memsize = S5PV210_VIDEO_SAMSUNG_MEMSIZE_FIMC1,
		.paddr = 0,
	},
	[5] = {
		.id = S5P_MDEV_FIMC2,
		.name = "fimc2",
		.bank = 1,
		.memsize = S5PV210_VIDEO_SAMSUNG_MEMSIZE_FIMC2,
		.paddr = 0,
	},
	[6] = {
		.id = S5P_MDEV_JPEG,
		.name = "jpeg",
		.bank = 0,
		.memsize = S5PV210_VIDEO_SAMSUNG_MEMSIZE_JPEG,
		.paddr = 0,
	},
	[7] = {
		.id = S5P_MDEV_PMEM,
		.name = "pmem",
		.bank = 0,
		.memsize = S5PV210_VIDEO_SAMSUNG_MEMSIZE_PMEM,
		.paddr = 0,
	},
	[8] = {
		.id = S5P_MDEV_PMEM_GPU1,
		.name = "pmem_gpu1",
		.bank = 0,
		.memsize = S5PV210_VIDEO_SAMSUNG_MEMSIZE_GPU1,
		.paddr = 0,
	},
	[9] = {
		.id = S5P_MDEV_PMEM_ADSP,
		.name = "pmem_adsp",
		.bank = 0,
		.memsize = S5PV210_VIDEO_SAMSUNG_MEMSIZE_ADSP,
		.paddr = 0,
	},
	[10] = {
		.id = S5P_MDEV_TEXSTREAM,
		.name = "s3c_bc",
                .bank = 1,
		.memsize = S5PV210_VIDEO_SAMSUNG_MEMSIZE_TEXTSTREAM,
		.paddr = 0,
	},
};

static struct regulator_consumer_supply ldo3_consumer[] = {
	REGULATOR_SUPPLY("usb_io", NULL),
};

static struct regulator_consumer_supply ldo5_consumer[] = {
	REGULATOR_SUPPLY("vtf", NULL),
};

static struct regulator_consumer_supply ldo6_consumer[] = {
	REGULATOR_SUPPLY("vbt_wl", NULL),
};

static struct regulator_consumer_supply ldo7_consumer[] = {
	REGULATOR_SUPPLY("vlcd", NULL),
};

static struct regulator_consumer_supply ldo8_consumer[] = {
	REGULATOR_SUPPLY("usb_core", NULL),
};

static struct regulator_consumer_supply ldo11_consumer[] = {
	REGULATOR_SUPPLY("cam_main_af", NULL),
};

static struct regulator_consumer_supply ldo12_consumer[] = {
	REGULATOR_SUPPLY("vibrator", NULL),
};

static struct regulator_consumer_supply ldo13_consumer[] = {
	REGULATOR_SUPPLY("cam_main_vdd_a", NULL),
};

static struct regulator_consumer_supply ldo14_consumer[] = {
	REGULATOR_SUPPLY("cam_main_vdd_io", NULL),
};

static struct regulator_consumer_supply ldo15_consumer[] = {
	REGULATOR_SUPPLY("cam_main_vdd_d", NULL),
};

static struct regulator_consumer_supply ldo16_consumer[] = {
	REGULATOR_SUPPLY("earmic", NULL),
};

static struct regulator_consumer_supply ldo17_consumer[] = {
	REGULATOR_SUPPLY("vcc_lcd", NULL),
};

static struct regulator_consumer_supply buck1_consumer[] = {
	REGULATOR_SUPPLY("vddarm", NULL),
};

static struct regulator_consumer_supply buck2_consumer[] = {
	REGULATOR_SUPPLY("vddint", NULL),
};

static struct regulator_consumer_supply buck4_consumer[] = {
	REGULATOR_SUPPLY("cam_front_vdd_d",NULL ),
};
static struct regulator_init_data aries_ldo2_data = {
	.constraints	= {
		.name		= "VALIVE_1.2V",
		.min_uV		= 1200000,
		.max_uV		= 1200000,
		.apply_uV	= 1,
		.always_on	= 1,
		.state_mem	= {
			.enabled = 1,
		},
	},
};

static struct regulator_init_data aries_ldo3_data = {
	.constraints	= {
		.name		= "VUSB_1.1V",
		.min_uV		= 1100000,
		.max_uV		= 1100000,
		.apply_uV	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo3_consumer),
	.consumer_supplies	= ldo3_consumer,
};

static struct regulator_init_data aries_ldo4_data = {
	.constraints	= {
		.name		= "VADC_3.3V",
		.min_uV		= 3300000,
		.max_uV		= 3300000,
		.apply_uV	= 1,
		.always_on	= 0,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled = 1,
		},
	},
};

static struct regulator_init_data aries_ldo5_data = {
	.constraints	= {
		.name		= "VTF_2.8V",
		.min_uV		= 2800000,
		.max_uV		= 2800000,
		.apply_uV	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo5_consumer),
	.consumer_supplies	= ldo5_consumer,
};

static struct regulator_init_data aries_ldo6_data = {
	.constraints	= {
		.name		= "VBT_WL_2.8V",
		.min_uV		= 2800000,
		.max_uV		= 2800000,
                .always_on      = 0,
		.apply_uV	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo6_consumer),
	.consumer_supplies	= ldo6_consumer,
};

static struct regulator_init_data aries_ldo7_data = {
	.constraints	= {
		.name		= "VLCD_1.8V",
		.min_uV		= 1800000,
		.max_uV		= 1800000,
		.apply_uV	= 1,
		.always_on	= 0,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo7_consumer),
	.consumer_supplies	= ldo7_consumer,
};

static struct regulator_init_data aries_ldo8_data = {
	.constraints	= {
		.name		= "VUSB_3.3V",
		.min_uV		= 3300000,
		.max_uV		= 3300000,
		.apply_uV	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo8_consumer),
	.consumer_supplies	= ldo8_consumer,
};

static struct regulator_init_data aries_ldo9_data = {
	.constraints	= {
		.name		= "VCC_2.8V_PDA",
		.min_uV		= 2800000,
		.max_uV		= 2800000,
		.apply_uV	= 1,
		.always_on	= 1,
	},
};

static struct regulator_init_data aries_ldo10_data = {
	.constraints	= {
		.name		= "VPLL_1.2V",
		.min_uV		= 1200000,
		.max_uV		= 1200000,
		.apply_uV	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled = 1,
		},
	},
};

static struct regulator_init_data aries_ldo11_data = {
	.constraints	= {
		.name		= "CAM_MAIN_AF_3.0V",
		.min_uV		= 3000000,
		.max_uV		= 3000000,
		.apply_uV	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo11_consumer),
	.consumer_supplies	= ldo11_consumer,
};

static struct regulator_init_data aries_ldo12_data = {
	.constraints	= {
		.name		= "VCC_3.0V_MOTOR",
		.min_uV		= 3000000,
		.max_uV		= 3000000,
		.apply_uV	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo12_consumer),
	.consumer_supplies	= ldo12_consumer,
};

static struct regulator_init_data aries_ldo13_data = {
	.constraints	= {
		.name		= "CAM_MAIN_VDDA_2.8V",
		.min_uV		= 2800000,
		.max_uV		= 2800000,
		.apply_uV	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo13_consumer),
	.consumer_supplies	= ldo13_consumer,
};

static struct regulator_init_data aries_ldo14_data = {
	.constraints	= {
		.name		= "CAM_MAIN_VDDIO_2.8V",
		.min_uV		= 2800000,
		.max_uV		= 2800000,
		.apply_uV	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo14_consumer),
	.consumer_supplies	= ldo14_consumer,
};

static struct regulator_init_data aries_ldo15_data = {
	.constraints	= {
		.name		= "CAM_MAIN_VDDD_1.2V",
		.min_uV		= 1200000,
		.max_uV		= 1200000,
		.apply_uV	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo15_consumer),
	.consumer_supplies	= ldo15_consumer,
};

static struct regulator_init_data aries_ldo16_data = {
	.constraints	= {
		.name		= "EARMIC_1.8V",
		.min_uV		= 1800000,
		.max_uV		= 1800000,
		.apply_uV	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo16_consumer),
	.consumer_supplies	= ldo16_consumer,
};

static struct regulator_init_data aries_ldo17_data = {
	.constraints	= {
		.name		= "VCC_3.0V_LCD",
		.min_uV		= 3000000,
		.max_uV		= 3000000,  // TODO:RECHECK!!!
		.apply_uV	= 1,
		.always_on	= 0,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo17_consumer),
	.consumer_supplies	= ldo17_consumer,
};

static struct regulator_init_data aries_buck1_data = {
	.constraints	= {
		.name		= "VDD_ARM",
		.min_uV		= 750000,
		.max_uV		= 1500000,
		.apply_uV	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE |
				  REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.uV	= 1250000,
			.mode	= REGULATOR_MODE_NORMAL,
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(buck1_consumer),
	.consumer_supplies	= buck1_consumer,
};

static struct regulator_init_data aries_buck2_data = {
	.constraints	= {
		.name		= "VDD_INT",
		.min_uV		= 750000,
		.max_uV		= 1500000,
		.apply_uV	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE |
				  REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.uV	= 1100000,
			.mode	= REGULATOR_MODE_NORMAL,
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(buck2_consumer),
	.consumer_supplies	= buck2_consumer,
};

static struct regulator_init_data aries_buck3_data = {
	.constraints	= {
		.name		= "VCC_1.8V",
		.min_uV		= 1800000,
		.max_uV		= 1800000,
		.apply_uV	= 1,
		.always_on	= 1,
	},
};

static struct regulator_init_data aries_buck4_data = {
        .constraints    = {
                .name           = "CAM_FRONT_VDDD_1.8V",
#if (CONFIG_SEC_HW_VER == 3)
                .min_uV         = 1500000,
                .max_uV         = 1500000,
#else
				.min_uV         = 1800000,
                .max_uV         = 1800000,
#endif
                .apply_uV       = 1,
                .valid_ops_mask = REGULATOR_CHANGE_STATUS,
                .state_mem      = {
                        .disabled = 1,
                },
        },
        .num_consumer_supplies  = ARRAY_SIZE(buck4_consumer),
        .consumer_supplies      = buck4_consumer,
};

static struct max8998_regulator_data aries_regulators[] = {
	{ MAX8998_LDO2,  &aries_ldo2_data },
	{ MAX8998_LDO3,  &aries_ldo3_data },
	{ MAX8998_LDO4,  &aries_ldo4_data },
	{ MAX8998_LDO5,  &aries_ldo5_data },
	{ MAX8998_LDO6,  &aries_ldo6_data },
	{ MAX8998_LDO7,  &aries_ldo7_data },
	{ MAX8998_LDO8,  &aries_ldo8_data },
	{ MAX8998_LDO9,  &aries_ldo9_data },
	{ MAX8998_LDO10, &aries_ldo10_data },
	{ MAX8998_LDO11, &aries_ldo11_data },
	{ MAX8998_LDO12, &aries_ldo12_data },
	{ MAX8998_LDO13, &aries_ldo13_data },
	{ MAX8998_LDO14, &aries_ldo14_data },
	{ MAX8998_LDO15, &aries_ldo15_data },
	{ MAX8998_LDO16, &aries_ldo16_data },
	{ MAX8998_LDO17, &aries_ldo17_data },
	{ MAX8998_BUCK1, &aries_buck1_data },
	{ MAX8998_BUCK2, &aries_buck2_data },
	{ MAX8998_BUCK3, &aries_buck3_data },
	{ MAX8998_BUCK4, &aries_buck4_data },
};

static struct max8998_adc_table_data temper_table[] =  {
	 {109, -70},
	 {111, -60},
	 {112, -50},
	 {113, -40},
	 {114, -30},
	 {116, -20},
	 {117, -10},
	 {118, 0},
	 {120, 10},
	 {121, 20},
	 {122, 30},
	 {124, 40},
	 {126, 50},
	 {127, 60},
	 {129, 70},
	 {130, 80},
	 {132, 90},
	 {133, 100},
	 {135, 110},
	 {136, 120},
	 {138, 130},
	 {140, 140},
	 {142, 150},
	 {144, 160},
	 {146, 170},
	 {149, 180},
	 {152, 190},
	 {154, 200},
	 {157, 210},
	 {159, 220},
	 {161, 230},
	 {163, 240},
	 {165, 250},
	 {167, 260},
	 {169, 270},
	 {170, 280},
	 {172, 290},
	 {173, 300},
	 {174, 310},
	 {175, 320},
	 {177, 330},
	 {178, 340},
	 {180, 350},
	 {181, 360},
	 {183, 370},
	 {184, 380},
	 {185, 390},
	 {187, 400},
	 {188, 410},
	 {189, 420},
	 {191, 430},
	 {192, 440},
	 {193, 450},
	 {194, 460},
	 {196, 470},
	 {197, 480},
	 {199, 490},
	 {200, 500},
	 {202, 510},
	 {203, 520},
	 {205, 530},
	 {206, 540},
	 {207, 550},
	 {208, 560},
	 {210, 570},
	 {211, 580},
	 {213, 590},
	 {214, 600},
	 {215, 610},
	 {216, 620},
	 {217, 630},
	 {219, 640},
	 {220, 650},
};
struct max8998_charger_callbacks *charger_callbacks;
static enum cable_type_t set_cable_status;
static enum acc_type_t set_acc_status;

static void max8998_charger_register_callbacks(
		struct max8998_charger_callbacks *ptr)
{
	charger_callbacks = ptr;
	if ((set_acc_status != 0) && charger_callbacks && charger_callbacks->set_acc_type)
		charger_callbacks->set_acc_type(charger_callbacks, set_acc_status);
	/* if there was a cable status change before the charger was
	ready, send this now */
	if ((set_cable_status != 0) && charger_callbacks && charger_callbacks->set_cable)
		charger_callbacks->set_cable(charger_callbacks, set_cable_status);
}

#ifdef CONFIG_CHARGER_FAN5403
/* function pointers are initialized in "fan5403_probe" */
static struct fan5403_platform_data fan5403_pdata = {
	.start_charging = NULL,
	.stop_charging = NULL,
	.get_vbus_status = NULL,
	.get_charging_status = NULL,
	.set_host_status = NULL,
};
#endif

#ifdef CONFIG_CHARGER_SMB328A
static void smb328a_tx_charge_done(void)
{
        if (charger_callbacks && charger_callbacks->charge_done)
                charger_callbacks->charge_done(charger_callbacks);
}

#ifdef CONFIG_MACH_TREBON
static void smb328a_bat_missing_detection(void)
{
        if (charger_callbacks && charger_callbacks->bat_missing)
                charger_callbacks->bat_missing(charger_callbacks);
}
#endif

/* function pointers are initialized in "smb328a_probe" */
static struct smb328a_platform_data smb328a_pdata = {
        .start_charging = NULL,
        .stop_charging = NULL,
        .get_vbus_status = NULL,
        .tx_charge_done = smb328a_tx_charge_done,
 #ifdef CONFIG_MACH_TREBON
        .bat_missing    = smb328a_bat_missing_detection,
 #endif
};
#endif
static struct max8998_charger_data aries_charger = {
	.register_callbacks	= &max8998_charger_register_callbacks,
	.adc_table		= temper_table,
	.adc_array_size		= ARRAY_SIZE(temper_table),
#ifdef CONFIG_CHARGER_SMB328A
        .charger_ic             = &smb328a_pdata,
#elif CONFIG_CHARGER_FAN5403
	.charger_ic		= &fan5403_pdata,
#endif
	/* all temperature data not yet confirmed (H/W co-work needed) */
	.temp_high_event_threshold	= 217,
	.temp_high_threshold	= 217,
	.temp_high_recovery	= 189,
	.temp_low_recovery	= 115,
	.temp_low_threshold	= 108,
	.temp_high_threshold_lpm	= 193,
	.temp_high_recovery_lpm		= 186,
	.temp_low_recovery_lpm		= 115,
	.temp_low_threshold_lpm		= 108,

	.adc_ch_temperature	= 1,
#ifdef CONFIG_CHARGER_SMB328A
	.adc_ch_current	= 5,
#else
	.adc_ch_current	= 2,
#endif
	.adc_ch_v_f = 4,
	.v_f_max = 2500,
	.v_f_min = 0,
	.termination_curr_adc = -1,	// not used
};

static struct max8998_platform_data max8998_pdata = {
	.num_regulators		= ARRAY_SIZE(aries_regulators),
	.regulators		= aries_regulators,
	.charger		= &aries_charger,
	.buck1_set1		= GPIO_BUCK_1_EN_A,
	.buck1_set2		= GPIO_BUCK_1_EN_B,
	.buck2_set3		= GPIO_BUCK_2_EN,

	.buck1_voltage_set	= { 1275000, 1200000, 1050000, 950000 },
	.buck2_voltage_set	= { 1100000, 1000000 },
};

struct platform_device sec_device_dpram = {
	.name	= "dpram-device",
	.id	= -1,
};

#if defined(CONFIG_FB_S3C_S6D05A3) || defined(CONFIG_FB_S3C_LM347DF01)
#define DISPLAY_CS      GPIO_DISPLAY_CS
#define DISPLAY_CLK     GPIO_DISPLAY_CLK
#define DISPLAY_SI      GPIO_DISPLAY_SI
#define DISPLAY_SO      GPIO_DUMMY


static void s6d05a3_cfg_gpio(struct platform_device *pdev)
{
    int i;

    printk(KERN_ERR "s6d05a3_cfg_gpio");

    for (i = 0; i < 8; i++) {
        s3c_gpio_cfgpin(S5PV210_GPF0(i), S3C_GPIO_SFN(2));
        s3c_gpio_setpull(S5PV210_GPF0(i), S3C_GPIO_PULL_NONE);
    }

    for (i = 0; i < 8; i++) {
        s3c_gpio_cfgpin(S5PV210_GPF1(i), S3C_GPIO_SFN(2));
        s3c_gpio_setpull(S5PV210_GPF1(i), S3C_GPIO_PULL_NONE);
    }

    for (i = 0; i < 8; i++) {
        s3c_gpio_cfgpin(S5PV210_GPF2(i), S3C_GPIO_SFN(2));
        s3c_gpio_setpull(S5PV210_GPF2(i), S3C_GPIO_PULL_NONE);
    }

    for (i = 0; i < 4; i++) {
        s3c_gpio_cfgpin(S5PV210_GPF3(i), S3C_GPIO_SFN(2));
        s3c_gpio_setpull(S5PV210_GPF3(i), S3C_GPIO_PULL_NONE);
    }

	/* mDNIe SEL: why we shall write 0x2 ? */
#ifdef CONFIG_FB_S3C_MDNIE
		writel(0x1, S5P_MDNIE_SEL);
#else
		writel(0x2, S5P_MDNIE_SEL);
#endif

    /* DISPLAY_CS */
    s3c_gpio_cfgpin(DISPLAY_CS, S3C_GPIO_OUTPUT);
    /* DISPLAY_CLK */
    s3c_gpio_cfgpin(DISPLAY_CLK, S3C_GPIO_OUTPUT);
    /* DISPLAY_SI */
    s3c_gpio_cfgpin(DISPLAY_SI, S3C_GPIO_OUTPUT);
    /* LCD_SO */
    //s3c_gpio_cfgpin(DISPLAY_SO, S3C_GPIO_INPUT);

    /* DISPLAY_CS */
    s3c_gpio_setpull(DISPLAY_CS, S3C_GPIO_PULL_NONE);
    /* DISPLAY_CLK */
    s3c_gpio_setpull(DISPLAY_CLK, S3C_GPIO_PULL_NONE);
    /* DISPLAY_SI */
    s3c_gpio_setpull(DISPLAY_SI, S3C_GPIO_PULL_NONE);
    /* LCD_SO */
    //s3c_gpio_setpull(DISPLAY_SO, S3C_GPIO_PULL_UP);
}

static int s6d05a3_reset_lcd(struct platform_device *pdev)
{
    printk(KERN_ERR "s6d05a3_reset_lcd");
    s3c_gpio_cfgpin(GPIO_MLCD_RST, S3C_GPIO_SFN(1));
    //msleep(10);
    if(lcd_Idtype == LM347DF01_LCD_ID)	{
	gpio_set_value(GPIO_MLCD_RST, 1);
    	msleep(80);
    	gpio_set_value(GPIO_MLCD_RST, 0);
    	msleep(170); // more than 10msec
    	gpio_set_value(GPIO_MLCD_RST, 1);
    	msleep(10);	
	return 0;
     }
    gpio_set_value(GPIO_MLCD_RST, 1);
    msleep(12);
    gpio_set_value(GPIO_MLCD_RST, 0);
    msleep(12); // more than 10msec
    gpio_set_value(GPIO_MLCD_RST, 1);
    msleep(50); // wait more than 30 ms after releasing the system reset( > 10msec) and input RGB interface signal (RGB pixel datga, sync signals,dot clock...) ( >= 20msec)
    return 0;
}

void lcd_cfg_gpio_early_suspend(void)
{
		int i;

		for (i = 0; i < 8; i++) {
			s3c_gpio_cfgpin(S5PV210_GPF0(i), S3C_GPIO_OUTPUT);
			gpio_set_value(S5PV210_GPF0(i), 0);
			}

		for (i = 0; i < 8; i++) {
			s3c_gpio_cfgpin(S5PV210_GPF1(i), S3C_GPIO_OUTPUT);
			gpio_set_value(S5PV210_GPF1(i), 0);
			}

		for (i = 0; i < 8; i++) {
			s3c_gpio_cfgpin(S5PV210_GPF2(i), S3C_GPIO_OUTPUT);
			gpio_set_value(S5PV210_GPF2(i), 0);
			}

		for (i = 0; i < 4; i++) {
			s3c_gpio_cfgpin(S5PV210_GPF3(i), S3C_GPIO_OUTPUT);
			gpio_set_value(S5PV210_GPF3(i), 0);
			}

			gpio_set_value(GPIO_MLCD_RST, 0);

			gpio_set_value(GPIO_DISPLAY_CS, 0);
			gpio_set_value(GPIO_DISPLAY_CLK, 0);
			gpio_set_value(GPIO_DISPLAY_SI, 0);
			s3c_gpio_setpull(GPIO_OLED_DET, S3C_GPIO_PULL_DOWN);
}

EXPORT_SYMBOL(lcd_cfg_gpio_early_suspend);

static struct s3c_platform_fb s6d05a3_data __initdata = {
    .hw_ver         = 0x62,     // FORTE_FROYO
    .clk_name       = "sclk_fimd",
    .nr_wins        = 5,
    .default_win    = CONFIG_FB_S3C_DEFAULT_WINDOW,
    .swap           = FB_SWAP_HWORD | FB_SWAP_WORD,

    .lcd = &s6d05a3,
    .cfg_gpio       = s6d05a3_cfg_gpio,
    .backlight_on   = NULL,
    .reset_lcd      = s6d05a3_reset_lcd,
};
#ifdef CONFIG_FB_S3C_LM347DF01
static struct s3c_platform_fb lm347df01_data __initdata = {
    .hw_ver         = 0x62,     // FORTE_FROYO
    .clk_name       = "sclk_fimd",
    .nr_wins        = 5,
    .default_win    = CONFIG_FB_S3C_DEFAULT_WINDOW,
    .swap           = FB_SWAP_HWORD | FB_SWAP_WORD,

    .lcd = &lm347df01,
  //  .cfg_gpio       = s6d16a0x_cfg_gpio,
    .cfg_gpio       = s6d05a3_cfg_gpio,
    .backlight_on   = NULL,
    .reset_lcd      = s6d05a3_reset_lcd,
};
#endif


#define LCD_BUS_NUM     3

static struct spi_board_info spi_board_info[] __initdata = {
    {
        .modalias           = "s6d05a3",
        .platform_data      = NULL,
        .max_speed_hz       = 1200000,
        .bus_num            = LCD_BUS_NUM,
        .chip_select        = 0,
        .mode               = SPI_MODE_3,
        .controller_data    = (void *)DISPLAY_CS,
    },
};

static struct spi_gpio_platform_data s6d05a3_spi_gpio_data = {
    .sck            = DISPLAY_CLK,
    .mosi           = DISPLAY_SI,
    .miso           = -1, //DISPLAY_SO
    .num_chipselect = 1,    // 2
};

static struct platform_device s3c_device_spi_gpio = {
    .name   = "spi_gpio",
    .id     = LCD_BUS_NUM,
    .dev    = {
        .parent         = &s3c_device_fb.dev,
        .platform_data  = &s6d05a3_spi_gpio_data,
    },
};

#endif

static void panel_cfg_gpio(struct platform_device *pdev)
{
	int i;

	for (i = 0; i < 8; i++)
		s3c_gpio_cfgpin(S5PV210_GPF0(i), S3C_GPIO_SFN(2));

	for (i = 0; i < 8; i++)
		s3c_gpio_cfgpin(S5PV210_GPF1(i), S3C_GPIO_SFN(2));

	for (i = 0; i < 8; i++)
		s3c_gpio_cfgpin(S5PV210_GPF2(i), S3C_GPIO_SFN(2));

	for (i = 0; i < 4; i++)
		s3c_gpio_cfgpin(S5PV210_GPF3(i), S3C_GPIO_SFN(2));

	/* mDNIe SEL: why we shall write 0x2 ? */
#ifdef CONFIG_FB_S3C_MDNIE
	writel(0x1, S5P_MDNIE_SEL);
#else
	writel(0x2, S5P_MDNIE_SEL);
#endif

	s3c_gpio_setpull(GPIO_OLED_DET, S3C_GPIO_PULL_NONE);
}


void lcd_cfg_gpio_late_resume(void)
{

}
EXPORT_SYMBOL(lcd_cfg_gpio_late_resume);

static int panel_reset_lcd(struct platform_device *pdev)
{
	int err;

	err = gpio_request(GPIO_MLCD_RST, "MLCD_RST");
	if (err) {
		printk(KERN_ERR "failed to request MP0(5) for "
				"lcd reset control\n");
		return err;
	}

	gpio_direction_output(GPIO_MLCD_RST, 1);
	msleep(10);

	gpio_set_value(GPIO_MLCD_RST, 0);
	msleep(10);

	gpio_set_value(GPIO_MLCD_RST, 1);
	msleep(10);

	gpio_free(GPIO_MLCD_RST);

	return 0;
}

static int panel_backlight_on(struct platform_device *pdev)
{
	return 0;
}

static struct s3c_platform_fb tl2796_data __initdata = {
	.hw_ver		= 0x62,
	.clk_name	= "sclk_fimd",
	.nr_wins	= 5,
	.default_win	= CONFIG_FB_S3C_DEFAULT_WINDOW,
	.swap		= FB_SWAP_HWORD | FB_SWAP_WORD,

	.lcd = &s6e63m0,
	.cfg_gpio	= panel_cfg_gpio,
	.backlight_on	= panel_backlight_on,
	.reset_lcd	= panel_reset_lcd,
};

#define LCD_BUS_NUM	3
#if 1    // heatup - chief
#define DISPLAY_CS      GPIO_DISPLAY_CS
#define DISPLAY_CLK     GPIO_DISPLAY_CLK
#define DISPLAY_SI      GPIO_DISPLAY_SI
#else
#define DISPLAY_CS      S5PV210_MP01(1)
#define SUB_DISPLAY_CS  S5PV210_MP01(2)
#define DISPLAY_CLK     S5PV210_MP04(1)
#define DISPLAY_SI      S5PV210_MP04(3)
#endif
/*
static struct spi_board_info spi_board_info[] __initdata = {
	{
		.modalias	= "tl2796",
		.platform_data	= &aries_panel_data,
		.max_speed_hz	= 1200000,
		.bus_num	= LCD_BUS_NUM,
		.chip_select	= 0,
		.mode		= SPI_MODE_3,
		.controller_data = (void *)DISPLAY_CS,
	},
};
*/
static struct spi_gpio_platform_data tl2796_spi_gpio_data = {
	.sck	= DISPLAY_CLK,
	.mosi	= DISPLAY_SI,
	.miso	= -1,
	.num_chipselect = 2,
};
/*
static struct platform_device s3c_device_spi_gpio = {
	.name	= "spi_gpio",
	.id	= LCD_BUS_NUM,
	.dev	= {
		.parent		= &s3c_device_fb.dev,
		.platform_data	= &tl2796_spi_gpio_data,
	},
};
*/
#ifdef CONFIG_KEYBOARD_ADP5587
static  struct  i2c_gpio_platform_data  i2c15_platdata = {
	.sda_pin = GPIO_SDA,
	.scl_pin = GPIO_SCL,
	.udelay = 0,    /* 250KHz */
	.sda_is_open_drain = 0,
	.scl_is_open_drain = 0,
	.scl_is_output_only = 0,
};

static struct platform_device s3c_device_i2c15 = {
	.name = "i2c-gpio",
	.id = 15,
	.dev.platform_data = &i2c15_platdata,
};
#endif
struct vibrator_platform_data vibrator_data = {

       .timer_id = 1,
       .vib_enable_gpio = GPIO_VIBTONE_EN1,
};

static struct platform_device s3c_device_vibrator = {
       .name = "cm4040_cs",
       .id   = -1,
        .dev  = {
                 .platform_data = &vibrator_data,
               },
};
static struct i2c_gpio_platform_data i2c4_platdata = {
	.sda_pin		= GPIO_AP_SDA_18V,
	.scl_pin		= GPIO_AP_SCL_18V,
	.udelay			= 2, /* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device s3c_device_i2c4 = {
	.name			= "i2c-gpio",
	.id			= 4,
	.dev.platform_data	= &i2c4_platdata,
};

static struct i2c_gpio_platform_data i2c5_platdata = {
	.sda_pin		= GPIO_AP_SDA_28V,
	.scl_pin		= GPIO_AP_SCL_28V,
	.udelay			= 2, /* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device s3c_device_i2c5 = {
	.name			= "i2c-gpio",
	.id			= 5,
	.dev.platform_data	= &i2c5_platdata,
};

static struct i2c_gpio_platform_data i2c6_platdata = {
	.sda_pin		= GPIO_AP_PMIC_SDA,
	.scl_pin		= GPIO_AP_PMIC_SCL,
	.udelay 		= 2, /* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device s3c_device_i2c6 = {
	.name			= "i2c-gpio",
	.id			= 6,
	.dev.platform_data	= &i2c6_platdata,
};

#if 0 // CHIEF
static struct i2c_gpio_platform_data i2c7_platdata = {
	.sda_pin		= GPIO_USB_SDA_28V,
	.scl_pin		= GPIO_USB_SCL_28V,
	.udelay 		= 2, /* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device s3c_device_i2c7 = {
	.name			= "i2c-gpio",
	.id			= 7,
	.dev.platform_data	= &i2c7_platdata,
};
#endif

#ifdef CONFIG_SAMSUNG_FM_SI4709
// For FM radio
static struct i2c_gpio_platform_data i2c8_platdata = {
	.sda_pin		= GPIO_FM_SDA_28V,
	.scl_pin		= GPIO_FM_SCL_28V,
	.udelay 		= 2, /* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device s3c_device_i2c8 = {
	.name			= "i2c-gpio",
	.id			= 8,
	.dev.platform_data	= &i2c8_platdata,
};
#endif

static struct i2c_gpio_platform_data i2c9_platdata = {
	.sda_pin		= FUEL_SDA_18V,
	.scl_pin		= FUEL_SCL_18V,
	.udelay 		= 2, /* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device s3c_device_i2c9 = {
	.name			= "i2c-gpio",
	.id			= 9,
	.dev.platform_data	= &i2c9_platdata,
};


static struct i2c_gpio_platform_data i2c10_platdata = {
	.sda_pin		= _3_TOUCH_SDA_28V,
	.scl_pin		= _3_TOUCH_SCL_28V,
	.udelay 		= 0, /* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device s3c_device_i2c10 = {
	.name			= "i2c-gpio",
	.id			= 10,
	.dev.platform_data	= &i2c10_platdata,
};

static struct i2c_gpio_platform_data i2c11_platdata = {
	.sda_pin		= GPIO_ALS_SDA_28V,
	.scl_pin		= GPIO_ALS_SCL_28V,
	.udelay 		= 2, /* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device s3c_device_i2c11 = {
	.name			= "i2c-gpio",
	.id			= 11,
	.dev.platform_data	= &i2c11_platdata,
};


static struct i2c_gpio_platform_data i2c13_platdata = {
        .sda_pin                = GPIO_BL_SDA_28V,
        .scl_pin                = GPIO_BL_SCL_28V,
        .udelay                 = 0,    /* 250KHz */
        .sda_is_open_drain      = 0,
        .scl_is_open_drain      = 0,
        .scl_is_output_only     = 0,
};

static struct platform_device s3c_device_i2c13 = {
	.name				= "i2c-gpio",
	.id					= 13,
	.dev.platform_data		= &i2c13_platdata,
};


#ifdef CONFIG_PN544
static struct i2c_gpio_platform_data i2c14_platdata = {
	.sda_pin		= NFC_SDA_18V,
	.scl_pin		= NFC_SCL_18V,
	.udelay			= 2,
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device s3c_device_i2c14 = {
	.name			= "i2c-gpio",
	.id			= 14,
	.dev.platform_data	= &i2c14_platdata,
};
#endif

#ifdef CONFIG_CHARGER_FAN5403
static struct i2c_gpio_platform_data i2c16_platdata = {
	.sda_pin		= GPIO_SC_SDA_28V,
	.scl_pin		= GPIO_SC_SCL_28V,
	.udelay 		= 2, /* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device s3c_device_i2c16 = {
	.name			= "i2c-gpio",
	.id			= 16,
	.dev.platform_data	= &i2c16_platdata,
};
#endif

#ifdef CONFIG_VIDEO_ISX005
static struct i2c_gpio_platform_data i2c17_platdata = {
	.sda_pin		= GPIO_CAM_SDA_29V, //GPIO_CAM_SDA_18V,
	.scl_pin		= GPIO_CAM_SCL_29V,//GPIO_CAM_SCL_18V,
	.udelay			= 0,
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device s3c_device_i2c17 = {
	.name			= "i2c-gpio",
	.id			= 17,
	.dev.platform_data	= &i2c17_platdata,
};
#endif


#ifdef CONFIG_KEYPAD_CYPRESS_TOUCH
static void touch_keypad_gpio_init(void)
{
	int ret = 0;

	s3c_gpio_cfgpin(S5PV210_GPJ0(6), GPIO_TOUCH_INT_AF);

	ret = gpio_request(TOUCHKEY_PWR_EN, "TOUCH_EN");
	if (ret)
		printk(KERN_ERR "Failed to request gpio touch_en.\n");
}

static void touch_keypad_onoff(int onoff)
{
	gpio_direction_output(S5PV210_GPJ2(6), onoff);
	printk("[sehun] touch_keypad_onoff=%d\n",onoff);

	if (onoff == TOUCHKEY_OFF)
		msleep(30);
	else
		msleep(25);
}

static const int touch_keypad_code[] = {
	KEY_MENU,
	KEY_HOME,
	KEY_SEARCH,
	KEY_BACK,
};

static struct touchkey_platform_data touchkey_data = {
	.keycode_cnt = ARRAY_SIZE(touch_keypad_code),
	.keycode = touch_keypad_code,
	.touchkey_onoff = touch_keypad_onoff,
};
#endif

struct gpio_event_direct_entry aries_keypad_key_map[] = {
	{
		.gpio	= S5PV210_GPH2(6),
		.code	= KEY_POWER,
	},
};
static struct gpio_event_input_info aries_keypad_key_info = {
	.info.func = gpio_event_input_func,
	.info.no_suspend = true,
	.debounce_time.tv.nsec = 5 * NSEC_PER_MSEC,
	.type = EV_KEY,
	.keymap = aries_keypad_key_map,
	.keymap_size = ARRAY_SIZE(aries_keypad_key_map)
};

static struct gpio_event_info *aries_input_info[] = {
	&aries_keypad_key_info.info,
};


static struct gpio_event_platform_data aries_input_data = {
	.names = {
		"sec_key",
		NULL,
	},
	.info = aries_input_info,
	.info_count = ARRAY_SIZE(aries_input_info),
};

static struct platform_device aries_input_device = {
	.name = GPIO_EVENT_DEV_NAME,
	.id = 0,
	.dev = {
		.platform_data = &aries_input_data,
	},
};
#ifdef CONFIG_S5P_ADC
static struct s3c_adc_mach_info s3c_adc_platform __initdata = {
	/* s5pc110 support 12-bit resolution */
	.delay		= 10000,
	.presc		= 65,
	.resolution	= 12,
};
#endif

/* There is a only common mic bias gpio in aries H/W */
static DEFINE_SPINLOCK(mic_bias_lock);
static bool wm8994_mic_bias;
struct regulator *ear_micbias_regulator ; /* LDO16 */

static int init_ear_mic_bias(void)
{
        if (IS_ERR_OR_NULL(ear_micbias_regulator)) {
                printk("Init ear_micbias_regulator\n");

                ear_micbias_regulator = regulator_get(NULL, "earmic");
                if (IS_ERR_OR_NULL(ear_micbias_regulator)) {
                        pr_err("Failed to get ear_micbias_regulator\n");
                        return -EINVAL;
                }
        }

        return 0;
}

static void set_shared_mic_bias(void)
{
        gpio_set_value(GPIO_MICBIAS_EN, wm8994_mic_bias);
}

static void wm8994_set_mic_bias(bool on)
{
	unsigned long flags;

	printk("main_micbias(%d)\n", on);

	spin_lock_irqsave(&mic_bias_lock, flags);
	wm8994_mic_bias = on;
	set_shared_mic_bias();
	spin_unlock_irqrestore(&mic_bias_lock, flags);
}

static void sec_jack_set_micbias_state(bool on)
{
        static bool prev_state = false;


        if (init_ear_mic_bias()) {
                pr_err("Failed to init earmic regulator\n");
                return;
        }

        if (prev_state != on) {
                printk("ear_micbias(%d)\n", on);

                if (on) {
                        if (regulator_enable(ear_micbias_regulator))
                                pr_err("Failed to enable ear_micbias_regulator\n");
                } else {
                        if (regulator_force_disable(ear_micbias_regulator))
                                pr_err("Failed to disable ear_micbias_regulator\n");
                }

                prev_state = on;
        }
}

static struct wm8994_platform_data wm8994_pdata = {
	.ldo = GPIO_CODEC_LDO_EN,
	.set_mic_bias = wm8994_set_mic_bias,
};



/*
 * Guide for Camera Configuration
*/
static struct regulator *cam_main_vdd_a;
static struct regulator *cam_main_vdd_d;
static struct regulator *cam_main_vdd_io;
static struct regulator *cam_main_af;
static struct regulator *cam_front_vdd_d;

// cmk 2010.10.15 setup
#ifdef CONFIG_VIDEO_S5K5CCGX
/****************************************************************************
 * s5k5ccgx_ldo_en()
 *
 * @param     en             LDO enable
 * @return    void
 * @remark    Function     Set_MAX8998_PM_OUTPUT_Voltage, Set_MAX8998_PM_REG,
 *
 * Oct 15, 2010  initial revision
 *****************************************************************************/
static void s5k5ccgx_ldo_en(bool en)
{
	int err = 0;

	if( IS_ERR_OR_NULL(cam_main_vdd_a) ||
        IS_ERR_OR_NULL(cam_main_vdd_d) ||
        IS_ERR_OR_NULL(cam_main_vdd_io) ||
        IS_ERR_OR_NULL(cam_main_af) ||
        IS_ERR_OR_NULL(cam_front_vdd_d))
    {
        pr_err("Camera regulators were not initialized\n");
		return;
	}

	if(!en)
	{
		goto CAM_POWER_OFF;
    }

    if(HWREV >= 0x02)
    {
        // Enable BUCK4 to turn MAIN_VDD_REG & VT_VDD_REG on.
    	err = regulator_enable(cam_front_vdd_d);
    	if(err)
    	{
    		pr_err("Failed to enable regulator cam_front_vdd_d\n");
    		goto CAM_POWER_OFF;
    	}
    	udelay(5);

    	/* VDD_REG : CAM_3M_1.2V */
        gpio_set_value(GPIO_CAM_VDD_REG_EN, 1);
    	mdelay(1);

    	/* VDDA : CAM_A_2.8V (LDO13) */
    	err = regulator_enable(cam_main_vdd_a);
    	if(err)
    	{
    		pr_err("Failed to enable regulator cam_main_vdd_a\n");
    		goto CAM_POWER_OFF;
    	}
    	udelay(500);

    	/* VT VDD_REG : CAM_1.5V */
        gpio_set_value(GPIO_CAM_VT_VDD_REG_EN, 1);
    	udelay(5);

    	/* VDDIO : CAM_IO_2.8V (LDO14) */
    	err = regulator_enable(cam_main_vdd_io);
    	if(err)
    	{
    		pr_err("Failed to enable regulator cam_main_vdd_io\n");
    		goto CAM_POWER_OFF;
    	}
    	udelay(5);

    	/* VAF : CAM_AF_2.8V or 3.0V (LDO11) */
    	err = regulator_enable(cam_main_af);
    	if(err)
    	{
    		pr_err("Failed to enable regulator cam_main_af\n");
    		goto CAM_POWER_OFF;
    	}
    }
    else
    {
    	/* VDD_REG : CAM_3M_1.2V (LDO15) */
    	err = regulator_enable(cam_main_vdd_d);
    	if(err)
    	{
    		pr_err("Failed to enable regulator cam_main_vdd_d\n");
    		goto CAM_POWER_OFF;
    	}
    	mdelay(1);

    	/* VDDA : CAM_A_2.8V (LDO13) */
    	err = regulator_enable(cam_main_vdd_a);
    	if(err)
    	{
    		pr_err("Failed to enable regulator cam_main_vdd_a\n");
    		goto CAM_POWER_OFF;
    	}
    	udelay(5);

    	/* VT VDD_REG : CAM_1.8V (BUCK4) */
    	err = regulator_enable(cam_front_vdd_d);
    	if(err)
    	{
    		pr_err("Failed to enable regulator cam_front_vdd_d\n");
    		goto CAM_POWER_OFF;
    	}
    	udelay(5);

    	/* VDDIO : CAM_IO_2.8V (LDO14) */
    	err = regulator_enable(cam_main_vdd_io);
    	if(err)
    	{
    		pr_err("Failed to enable regulator cam_main_vdd_io\n");
    		goto CAM_POWER_OFF;
    	}
    	udelay(5);

    	/* VAF : CAM_AF_2.8V or 3.0V (LDO11) */
    	err = regulator_enable(cam_main_af);
    	if(err)
    	{
    		pr_err("Failed to enable regulator cam_main_af\n");
    		goto CAM_POWER_OFF;
    	}
	}
    return;

CAM_POWER_OFF:

    if(HWREV >= 0x02)
    {
    	/* VAF : CAM_AF_2.8V or 3.0V (LDO11) */
    	err = regulator_disable(cam_main_af);
    	if(err)
    	{
    		pr_err("Failed to disable regulator cam_main_af\n");
    	}
        udelay(10);

    	/* VDDIO : CAM_IO_2.8V (LDO14) */
    	err = regulator_disable(cam_main_vdd_io);
    	if(err)
    	{
    		pr_err("Failed to disable regulator cam_main_vdd_io\n");
    	}
        udelay(5);

    	/* VT VDD_REG : CAM_1.5V */
        gpio_set_value(GPIO_CAM_VT_VDD_REG_EN, 0);
        udelay(5);

    	/* VDDA : CAM_A_2.8V (LDO13) */
    	err = regulator_disable(cam_main_vdd_a);
    	if(err)
    	{
    		pr_err("Failed to disable regulator cam_main_vdd_a\n");
    	}
        udelay(5);

    	/* VDD_REG : CAM_3M_1.2V */
    	gpio_set_value(GPIO_CAM_VDD_REG_EN, 0);
        udelay(5);

        // Disable BUCK4
    	err = regulator_disable(cam_front_vdd_d);
    	if(err)
    	{
    		pr_err("Failed to disable regulator cam_front_vdd_d\n");
    	}
    }
    else
    {
    	/* VAF : CAM_AF_2.8V or 3.0V (LDO11) */
    	err = regulator_disable(cam_main_af);
    	if(err)
    	{
    		pr_err("Failed to disable regulator cam_main_af\n");
    	}
        udelay(10);

    	/* VDDIO : CAM_IO_2.8V (LDO14) */
    	err = regulator_disable(cam_main_vdd_io);
    	if(err)
    	{
    		pr_err("Failed to disable regulator cam_main_vdd_io\n");
    	}
        udelay(5);

    	/* VT VDD_REG : CAM_1.8V (BUCK4) */
    	err = regulator_disable(cam_front_vdd_d);
    	if(err)
    	{
    		pr_err("Failed to disable regulator cam_front_vdd_d\n");
    	}
        udelay(5);

    	/* VDDA : CAM_A_2.8V (LDO13) */
    	err = regulator_disable(cam_main_vdd_a);
    	if(err)
    	{
    		pr_err("Failed to disable regulator cam_main_vdd_a\n");
    	}
        udelay(5);

    	/* VDD_REG : CAM_3M_1.2V (LDO15) */
    	err = regulator_disable(cam_main_vdd_d);
    	if(err)
    	{
    		pr_err("Failed to disable regulator cam_main_vdd_d\n");
    	}
	}
}

/****************************************************************************
 * s5k5ccgx_power_on_sequence()
 *
 * @param     void
 * @return    0
 * @remark    Function
 *
 * Oct 15, 2010  initial revision
 *****************************************************************************/
static int s5k5ccgx_power_on_sequence(void)
{
	int err;

	printk("camera_power_on\n");

	/* CAM_MEGA_EN - GPJ0(6) => GPA0(6)*/
	err = gpio_request(GPIO_CAM_MEGA_EN, "GPA0");
	if(err)
	{
		printk(KERN_ERR "failed to request GPIO for camera standby\n");
		return err;
	}

	/* CAM_MEGA_nRST - GPJ1(5) => GPE1(4) */
	err = gpio_request(GPIO_CAM_MEGA_nRST, "GPE1");
	if(err)
	{
		printk(KERN_ERR "failed to request GPE1(4) for camera control\n");

		gpio_free(GPIO_CAM_MEGA_EN);
		return err;
	}

	/* VT_CAM_nRST */
	err = gpio_request(GPIO_VT_CAM_RST, "GPH3(1)");
	if(err)
	{
		printk(KERN_ERR "failed to request GPE1(4) for camera control\n");

		gpio_free(GPIO_CAM_MEGA_EN);
		gpio_free(GPIO_CAM_MEGA_nRST);
		return err;
	}

	/* VT_CAM_STANDBY */
	err = gpio_request(GPIO_VT_CAM_EN, "GPH3(2)");
	if(err)
	{
		printk(KERN_ERR "failed to request GPE1(4) for camera control\n");

		gpio_free(GPIO_CAM_MEGA_EN);
		gpio_free(GPIO_CAM_MEGA_nRST);
		gpio_free(GPIO_VT_CAM_RST);
		return err;
	}

    if(HWREV >= 0x02)
    {
    	/* 3M CORE : VDD_REG 1.2V EN */
    	err = gpio_request(GPIO_CAM_VDD_REG_EN, "GPJ0(6)");
    	if(err)
    	{
    		printk(KERN_ERR "failed to request GPJ0(6) for camera control\n");

    		gpio_free(GPIO_CAM_MEGA_EN);
    		gpio_free(GPIO_CAM_MEGA_nRST);
    		gpio_free(GPIO_VT_CAM_RST);
    		gpio_free(GPIO_VT_CAM_EN);
    		return err;
    	}

    	/* VT CORE : VDD_REG 1.5V EN */
    	err = gpio_request(GPIO_CAM_VT_VDD_REG_EN, "GPJ1(4)");
    	if(err)
    	{
    		printk(KERN_ERR "failed to request GPJ1(4) for camera control\n");

    		gpio_free(GPIO_CAM_MEGA_EN);
    		gpio_free(GPIO_CAM_MEGA_nRST);
    		gpio_free(GPIO_VT_CAM_RST);
    		gpio_free(GPIO_VT_CAM_EN);
    		gpio_free(GPIO_CAM_VDD_REG_EN);
    		return err;
    	}
    }

	gpio_direction_output(GPIO_CAM_MEGA_EN, 0);
	gpio_direction_output(GPIO_CAM_MEGA_nRST, 0);
	gpio_direction_output(GPIO_VT_CAM_RST, 0);
	gpio_direction_output(GPIO_VT_CAM_EN, 0);

    if(HWREV >= 0x02)
    {
    	// MAIN CAM : VDD REG Disable
    	gpio_direction_output(GPIO_CAM_VDD_REG_EN, 0);
    	// VT CAM : VDD REG Disable
    	gpio_direction_output(GPIO_CAM_VT_VDD_REG_EN, 0);
	}

    // MAIN CAM : standby low
    gpio_set_value(GPIO_CAM_MEGA_EN, 0);
    // MAIN CAM : reset low
	gpio_set_value(GPIO_CAM_MEGA_nRST, 0);
    // VT CAM : standby low
    gpio_set_value(GPIO_VT_CAM_EN, 0);
    // VT CAM : reset low
	gpio_set_value(GPIO_VT_CAM_RST, 0);
	msleep(1);

    // Turn LDOs on
	s5k5ccgx_ldo_en(true);
    udelay(100);

    // Sub Cam goes to HI-Z state =====
	// VT CAM : standby high
	gpio_set_value(GPIO_VT_CAM_EN, 1);
	udelay(100);

    // Enable MCLK
	s3c_gpio_cfgpin(GPIO_CAM_MCLK, S3C_GPIO_SFN(0x02));
	msleep(9);

    // VT CAM : reset high
	gpio_set_value(GPIO_VT_CAM_RST, 1);
	msleep(1);

	// VT CAM : standby low
	gpio_set_value(GPIO_VT_CAM_EN, 0);
	udelay(50);
    // ================================

    // MAIN CAM : standby high
    gpio_set_value(GPIO_CAM_MEGA_EN, 1);
	udelay(50);

    // MAIN CAM : reset high
	gpio_set_value(GPIO_CAM_MEGA_nRST, 1);
	msleep(50);

	gpio_free(GPIO_CAM_MEGA_EN);
	gpio_free(GPIO_CAM_MEGA_nRST);
	gpio_free(GPIO_VT_CAM_RST);
	gpio_free(GPIO_VT_CAM_EN);

    if(HWREV >= 0x02)
    {
        gpio_free(GPIO_CAM_VDD_REG_EN);
        gpio_free(GPIO_CAM_VT_VDD_REG_EN);
    }

	return 0;
}

/****************************************************************************
 * s5k5ccgx_power_off_sequence()
 *
 * @param     void
 * @return    err
 * @remark    Function
 *
 * Oct 15, 2010  initial revision
 *****************************************************************************/
static int s5k5ccgx_power_off_sequence(void)
{
	int err;

	printk("camera_power_off\n");

	/* CAM_MEGA_EN - GPJ0(6) => GPA0(6)*/
	err = gpio_request(GPIO_CAM_MEGA_EN, "GPA0");
	if(err)
	{
		printk(KERN_ERR "failed to request GPIO for camera standby\n");
		return err;
	}

	/* CAM_MEGA_nRST - GPJ1(5) => GPE1(4) */
	err = gpio_request(GPIO_CAM_MEGA_nRST, "GPE1");
	if(err)
	{
		printk(KERN_ERR "failed to request GPE1(4) for camera control\n");

		gpio_free(GPIO_CAM_MEGA_EN);
		return err;
	}

	/* VT_CAM_nRST */
	err = gpio_request(GPIO_VT_CAM_RST, "GPH3(1)");
	if(err)
	{
		printk(KERN_ERR "failed to request GPE1(4) for camera control\n");

		gpio_free(GPIO_CAM_MEGA_EN);
		gpio_free(GPIO_CAM_MEGA_nRST);
		return err;
	}

	/* VT_CAM_STANDBY */
	err = gpio_request(GPIO_VT_CAM_EN, "GPH3(2)");
	if(err)
	{
		printk(KERN_ERR "failed to request GPE1(4) for camera control\n");

		gpio_free(GPIO_CAM_MEGA_EN);
		gpio_free(GPIO_CAM_MEGA_nRST);
		gpio_free(GPIO_VT_CAM_RST);
		return err;
	}

    if(HWREV >= 0x02)
    {
    	/* 3M CORE : VDD_REG 1.2V EN */
    	err = gpio_request(GPIO_CAM_VDD_REG_EN, "GPJ0(6)");
    	if(err)
    	{
    		printk(KERN_ERR "failed to request GPJ0(6) for camera control\n");

    		gpio_free(GPIO_CAM_MEGA_EN);
    		gpio_free(GPIO_CAM_MEGA_nRST);
    		gpio_free(GPIO_VT_CAM_RST);
    		gpio_free(GPIO_VT_CAM_EN);
    		return err;
    	}

    	/* VT CORE : VDD_REG 1.5V EN */
    	err = gpio_request(GPIO_CAM_VT_VDD_REG_EN, "GPJ1(4)");
    	if(err)
    	{
    		printk(KERN_ERR "failed to request GPJ1(4) for camera control\n");

    		gpio_free(GPIO_CAM_MEGA_EN);
    		gpio_free(GPIO_CAM_MEGA_nRST);
    		gpio_free(GPIO_VT_CAM_RST);
    		gpio_free(GPIO_VT_CAM_EN);
    		gpio_free(GPIO_CAM_VDD_REG_EN);
    		return err;
    	}
	}

    // VT_CAM : standby low
	gpio_set_value(GPIO_VT_CAM_EN, 0);
	udelay(5);

    // VT_CAM : reset low
	gpio_set_value(GPIO_VT_CAM_RST, 0);
	msleep(2);

    // MAIN CAM : reset low
	gpio_set_value(GPIO_CAM_MEGA_nRST, 0);
	udelay(100);

	// Disable MCLK
	s3c_gpio_cfgpin(GPIO_CAM_MCLK, 0);
	udelay(5);

    // MAIN CAM : standby low
	gpio_set_value(GPIO_CAM_MEGA_EN, 0);
	udelay(5);

    // Turn LDOs off
	s5k5ccgx_ldo_en(false);
	msleep(1);

	gpio_free(GPIO_CAM_MEGA_EN);
	gpio_free(GPIO_CAM_MEGA_nRST);
	gpio_free(GPIO_VT_CAM_RST);
	gpio_free(GPIO_VT_CAM_EN);

    if(HWREV >= 0x02)
    {
        gpio_free(GPIO_CAM_VDD_REG_EN);
        gpio_free(GPIO_CAM_VT_VDD_REG_EN);
    }
	return 0;
}

static int s5k5ccgx_regulator_init(void)
{
    /* BUCK4 */
	if(IS_ERR_OR_NULL(cam_front_vdd_d))
	{
		cam_front_vdd_d = regulator_get(NULL, "cam_front_vdd_d");
		if(IS_ERR_OR_NULL(cam_front_vdd_d))
		{
			pr_err("failed to get cam_front_vdd_d regulator");
			return -EINVAL;
		}
	}

    /* LDO11 */
	if(IS_ERR_OR_NULL(cam_main_af))
	{
		cam_main_af = regulator_get(NULL, "cam_main_af");
		if(IS_ERR_OR_NULL(cam_main_af))
		{
			pr_err("failed to get cam_main_af regulator");
			return -EINVAL;
		}
	}

    /* LDO13 */
	if(IS_ERR_OR_NULL(cam_main_vdd_a))
	{
		cam_main_vdd_a = regulator_get(NULL, "cam_main_vdd_a");
		if(IS_ERR_OR_NULL(cam_main_vdd_a))
		{
			pr_err("failed to get cam_main_vdd_a regulator");
			return -EINVAL;
		}
	}

    /* LDO14 */
	if(IS_ERR_OR_NULL(cam_main_vdd_io))
	{
		cam_main_vdd_io = regulator_get(NULL, "cam_main_vdd_io");
		if(IS_ERR_OR_NULL(cam_main_vdd_io))
		{
			pr_err("failed to get cam_main_vdd_io regulator");
			return -EINVAL;
		}
	}

    /* LDO15 */
	if(IS_ERR_OR_NULL(cam_main_vdd_d))
	{
		cam_main_vdd_d = regulator_get(NULL, "cam_main_vdd_d");
		if(IS_ERR_OR_NULL(cam_main_vdd_d))
		{
			pr_err("failed to get cam_main_vdd_d regulator");
			return -EINVAL;
		}
	}
	return 0;
}

/****************************************************************************
 * s5k5ccgx_power_en()
 *
 * @param     on/off
 * @return    err
 * @remark    Function
 *
 * Oct 15, 2010  initial revision
 *****************************************************************************/
static int s5k5ccgx_power_en(int onoff)
{
	if(s5k5ccgx_regulator_init() < 0)
	{
		pr_err("Failed to initialize camera regulators\n");
		return -EINVAL;
	}

	if(onoff)
	{
		s5k5ccgx_power_on_sequence();
	}
	else
	{
		s5k5ccgx_power_off_sequence();
	}
	return 0;
}

static struct camera_platform_data s5k5ccgx_platform_data = {
	.default_width  = 1024,
	.default_height = 768,
	.pixelformat    = V4L2_PIX_FMT_UYVY,
	.mclk_freq      = 24000000,
	.is_mipi        = 0,
};

static struct i2c_board_info s5k5ccgx_i2c_board_info =
{
    I2C_BOARD_INFO("S5K5CCGX", (0x78 >> 1)),
    .platform_data = &s5k5ccgx_platform_data,
};

static struct s3c_platform_camera s5k5ccgx_platform_camera =
{
	.id             = CAMERA_PAR_A,
	.type           = CAM_TYPE_ITU,
	.fmt            = ITU_601_YCBCR422_8BIT,
	.order422       = CAM_ORDER422_8BIT_CBYCRY,
	.i2c_busnum     = 0,
	.info           = &s5k5ccgx_i2c_board_info,
	.pixelformat    = V4L2_PIX_FMT_UYVY,
	.srclk_name     = "xusbxti",
	.clk_name       = "sclk_cam", // "sclk_cam0",
	.clk_rate       = 24000000,
	.line_length    = 1536,
	.width          = 1024,
	.height         = 768,
	.window =
	{
		.left 	    = 0,
		.top        = 0,
		.width      = 1024,
		.height     = 768,
	},

	/* Polarity */
	.inv_pclk       = 0,
	.inv_vsync      = 1,
	.inv_href       = 0,
	.inv_hsync      = 0,
	.initialized    = 0,
	.cam_power      = s5k5ccgx_power_en,
};
#endif // CONFIG_VIDEO_S5K5CCGX

#ifdef CONFIG_VIDEO_SR030PC30
/****************************************************************************
 * sr030pc30_ldo_en()
 *
 * @param     en             LDO enable
 * @return    void
 * @remark    Function     Set_MAX8998_PM_OUTPUT_Voltage, Set_MAX8998_PM_REG,
 *
 *****************************************************************************/
static void sr030pc30_ldo_en(bool en)
{
	int err = 0;

	if( IS_ERR_OR_NULL(cam_main_vdd_a) ||
        IS_ERR_OR_NULL(cam_main_vdd_d) ||
        IS_ERR_OR_NULL(cam_main_vdd_io) ||
        IS_ERR_OR_NULL(cam_front_vdd_d))
    {
        pr_err("Camera regulators were not initialized\n");
		return;
	}

	if(!en)
	{
		goto CAM_POWER_OFF;
    }

        // Enable BUCK4 to turn MAIN_VDD_REG & VT_VDD_REG on.
    	err = regulator_enable(cam_front_vdd_d);
    	if(err)
    	{
    		pr_err("Failed to enable regulator cam_front_vdd_d\n");
    		goto CAM_POWER_OFF;
    	}
    	udelay(5);

    	/* VDD_REG : CAM_3M_1.2V */
        gpio_set_value(GPIO_CAM_VDD_REG_EN, 1);
    	mdelay(1);

    	/* VDDA : CAM_A_2.8V (LDO13) */
    	err = regulator_enable(cam_main_vdd_a);
    	if(err)
    	{
    		pr_err("Failed to enable regulator cam_main_vdd_a\n");
    		goto CAM_POWER_OFF;
    	}
    	udelay(500);

    	/* VT VDD_REG : CAM_1.5V */
        gpio_set_value(GPIO_CAM_VT_VDD_REG_EN, 1);
    	udelay(5);

    	/* VDDIO : CAM_IO_2.8V (LDO14) */
    	err = regulator_enable(cam_main_vdd_io);
    	if(err)
    	{
    		pr_err("Failed to enable regulator cam_main_vdd_io\n");
    		goto CAM_POWER_OFF;
    	}

    return;

CAM_POWER_OFF:

    	/* VDDIO : CAM_IO_2.8V (LDO14) */
    	err = regulator_disable(cam_main_vdd_io);
    	if(err)
    	{
    		pr_err("Failed to disable regulator cam_main_vdd_io\n");
    	}
        udelay(5);

    	/* VT VDD_REG : CAM_1.5V */
        gpio_set_value(GPIO_CAM_VT_VDD_REG_EN, 0);
        udelay(5);

    	/* VDDA : CAM_A_2.8V (LDO13) */
    	err = regulator_disable(cam_main_vdd_a);
    	if(err)
    	{
    		pr_err("Failed to disable regulator cam_main_vdd_a\n");
    	}
        udelay(5);

    	/* VDD_REG : CAM_3M_1.2V */
    	gpio_set_value(GPIO_CAM_VDD_REG_EN, 0);
        udelay(5);

        // Disable BUCK4
    	err = regulator_disable(cam_front_vdd_d);
    	if(err)
    	{
    		pr_err("Failed to disable regulator cam_front_vdd_d\n");
    	}
    }

/****************************************************************************
 * sr030pc30_power_on_sequence()
 *
 * @param     void
 * @return    0
 * @remark    Function
 *
 *****************************************************************************/
static int sr030pc30_power_on_sequence(void)
{
	int err;

	printk("camera_power_on\n");

	/* CAM_MEGA_EN - GPJ0(6) => GPA0(6)*/
	err = gpio_request(GPIO_CAM_MEGA_EN, "GPA0");
	if(err)
	{
		printk(KERN_ERR "failed to request GPIO for camera standby\n");
		return err;
	}

	/* CAM_MEGA_nRST - GPJ1(5) => GPE1(4) */
	err = gpio_request(GPIO_CAM_MEGA_nRST, "GPE1");
	if(err)
	{
		printk(KERN_ERR "failed to request GPE1(4) for camera control\n");

		gpio_free(GPIO_CAM_MEGA_EN);
		return err;
	}

	/* VT_CAM_nRST */
	err = gpio_request(GPIO_VT_CAM_RST, "GPH3(1)");
	if(err)
	{
		printk(KERN_ERR "failed to request GPE1(4) for camera control\n");

		gpio_free(GPIO_CAM_MEGA_EN);
		gpio_free(GPIO_CAM_MEGA_nRST);
		return err;
	}

	/* VT_CAM_STANDBY */
	err = gpio_request(GPIO_VT_CAM_EN, "GPH3(2)");
	if(err)
	{
		printk(KERN_ERR "failed to request GPE1(4) for camera control\n");

		gpio_free(GPIO_CAM_MEGA_EN);
		gpio_free(GPIO_CAM_MEGA_nRST);
		gpio_free(GPIO_VT_CAM_RST);
		return err;
	}

    	/* 3M CORE : VDD_REG 1.2V EN */
    	err = gpio_request(GPIO_CAM_VDD_REG_EN, "GPJ0(6)");
    	if(err)
    	{
    		printk(KERN_ERR "failed to request GPJ0(6) for camera control\n");

    		gpio_free(GPIO_CAM_MEGA_EN);
    		gpio_free(GPIO_CAM_MEGA_nRST);
    		gpio_free(GPIO_VT_CAM_RST);
    		gpio_free(GPIO_VT_CAM_EN);
    		return err;
    	}

    	/* VT CORE : VDD_REG 1.5V EN */
    	err = gpio_request(GPIO_CAM_VT_VDD_REG_EN, "GPJ1(4)");
    	if(err)
    	{
    		printk(KERN_ERR "failed to request GPJ1(4) for camera control\n");

    		gpio_free(GPIO_CAM_MEGA_EN);
    		gpio_free(GPIO_CAM_MEGA_nRST);
    		gpio_free(GPIO_VT_CAM_RST);
    		gpio_free(GPIO_VT_CAM_EN);
    		gpio_free(GPIO_CAM_VDD_REG_EN);
    		return err;
    	}

	gpio_direction_output(GPIO_CAM_MEGA_EN, 0);
	gpio_direction_output(GPIO_CAM_MEGA_nRST, 0);
	gpio_direction_output(GPIO_VT_CAM_RST, 0);
	gpio_direction_output(GPIO_VT_CAM_EN, 0);

    	// MAIN CAM : VDD REG Disable
    	gpio_direction_output(GPIO_CAM_VDD_REG_EN, 0);
    	// VT CAM : VDD REG Disable
    	gpio_direction_output(GPIO_CAM_VT_VDD_REG_EN, 0);

    // MAIN CAM : standby low
    gpio_set_value(GPIO_CAM_MEGA_EN, 0);
    // MAIN CAM : reset low
	gpio_set_value(GPIO_CAM_MEGA_nRST, 0);
    // VT CAM : standby low
    gpio_set_value(GPIO_VT_CAM_EN, 0);
    // VT CAM : reset low
	gpio_set_value(GPIO_VT_CAM_RST, 0);
	msleep(1);

    // Turn LDOs on
	sr030pc30_ldo_en(true);
    udelay(100);

	// VT CAM : standby high
	gpio_set_value(GPIO_VT_CAM_EN, 1);
	udelay(100);

    // Enable MCLK
	s3c_gpio_cfgpin(GPIO_CAM_MCLK, S3C_GPIO_SFN(0x02));
	udelay(15);

	// MAIN CAM : standby high
	gpio_set_value(GPIO_CAM_MEGA_EN, 1);
	msleep(6);
	
    	// MAIN CAM : reset high
	gpio_set_value(GPIO_CAM_MEGA_nRST, 1);
	msleep(8);

	// MAIN CAM : standby low
	gpio_set_value(GPIO_CAM_MEGA_EN, 0);
	udelay(15);

    // VT CAM : reset high
	gpio_set_value(GPIO_VT_CAM_RST, 1);
	msleep(50);

	gpio_free(GPIO_CAM_MEGA_EN);
	gpio_free(GPIO_CAM_MEGA_nRST);
	gpio_free(GPIO_VT_CAM_RST);
	gpio_free(GPIO_VT_CAM_EN);

        gpio_free(GPIO_CAM_VDD_REG_EN);
        gpio_free(GPIO_CAM_VT_VDD_REG_EN);

	return 0;
}

/****************************************************************************
 * sr030pc30_power_off_sequence()
 *
 * @param     void
 * @return    err
 * @remark    Function
 *
 *****************************************************************************/
static int sr030pc30_power_off_sequence(void)
{
	int err;

	printk("camera_power_off\n");

	/* CAM_MEGA_EN - GPJ0(6) => GPA0(6)*/
	err = gpio_request(GPIO_CAM_MEGA_EN, "GPA0");
	if(err)
	{
		printk(KERN_ERR "failed to request GPIO for camera standby\n");
		return err;
	}

	/* CAM_MEGA_nRST - GPJ1(5) => GPE1(4) */
	err = gpio_request(GPIO_CAM_MEGA_nRST, "GPE1");
	if(err)
	{
		printk(KERN_ERR "failed to request GPE1(4) for camera control\n");

		gpio_free(GPIO_CAM_MEGA_EN);
		return err;
	}

	/* VT_CAM_nRST */
	err = gpio_request(GPIO_VT_CAM_RST, "GPH3(1)");
	if(err)
	{
		printk(KERN_ERR "failed to request GPE1(4) for camera control\n");

		gpio_free(GPIO_CAM_MEGA_EN);
		gpio_free(GPIO_CAM_MEGA_nRST);
		return err;
	}

	/* VT_CAM_STANDBY */
	err = gpio_request(GPIO_VT_CAM_EN, "GPH3(2)");
	if(err)
	{
		printk(KERN_ERR "failed to request GPE1(4) for camera control\n");

		gpio_free(GPIO_CAM_MEGA_EN);
		gpio_free(GPIO_CAM_MEGA_nRST);
		gpio_free(GPIO_VT_CAM_RST);
		return err;
	}

    	/* 3M CORE : VDD_REG 1.2V EN */
    	err = gpio_request(GPIO_CAM_VDD_REG_EN, "GPJ0(6)");
    	if(err)
    	{
    		printk(KERN_ERR "failed to request GPJ0(6) for camera control\n");

    		gpio_free(GPIO_CAM_MEGA_EN);
    		gpio_free(GPIO_CAM_MEGA_nRST);
    		gpio_free(GPIO_VT_CAM_RST);
    		gpio_free(GPIO_VT_CAM_EN);
    		return err;
    	}

    	/* VT CORE : VDD_REG 1.5V EN */
    	err = gpio_request(GPIO_CAM_VT_VDD_REG_EN, "GPJ1(4)");
    	if(err)
    	{
    		printk(KERN_ERR "failed to request GPJ1(4) for camera control\n");

    		gpio_free(GPIO_CAM_MEGA_EN);
    		gpio_free(GPIO_CAM_MEGA_nRST);
    		gpio_free(GPIO_VT_CAM_RST);
    		gpio_free(GPIO_VT_CAM_EN);
    		gpio_free(GPIO_CAM_VDD_REG_EN);
    		return err;
    	}

    // VT CAM : reset low
	gpio_set_value(GPIO_VT_CAM_RST, 0);
	udelay(5);

    	// MAIN CAM : reset low
	gpio_set_value(GPIO_CAM_MEGA_nRST, 0);
	udelay(100);

	// Disable MCLK
	s3c_gpio_cfgpin(GPIO_CAM_MCLK, 0);
	udelay(5);

    // VT CAM : standby low
	gpio_set_value(GPIO_VT_CAM_EN, 0);
	udelay(5);

    // Turn LDOs off
	sr030pc30_ldo_en(false);
	msleep(1);

	gpio_free(GPIO_CAM_MEGA_EN);
	gpio_free(GPIO_CAM_MEGA_nRST);
	gpio_free(GPIO_VT_CAM_RST);
	gpio_free(GPIO_VT_CAM_EN);

        gpio_free(GPIO_CAM_VDD_REG_EN);
        gpio_free(GPIO_CAM_VT_VDD_REG_EN);

	return 0;
}

/****************************************************************************
 * sr031pc30_regulator_init()
 *
 * @param     void
 * @return    err
 * @remark    Function
 *
 *****************************************************************************/
static int sr030pc30_regulator_init(void)
{
    /* BUCK4 */
	if(IS_ERR_OR_NULL(cam_front_vdd_d))
	{
		cam_front_vdd_d = regulator_get(NULL, "cam_front_vdd_d");
		if(IS_ERR_OR_NULL(cam_front_vdd_d))
		{
			pr_err("failed to get cam_front_vdd_d regulator");
			return -EINVAL;
		}
	}

    /* LDO11 */
	if(IS_ERR_OR_NULL(cam_main_af))
	{
		cam_main_af = regulator_get(NULL, "cam_main_af");
		if(IS_ERR_OR_NULL(cam_main_af))
		{
			pr_err("failed to get cam_main_af regulator");
			return -EINVAL;
		}
	}

    /* LDO13 */
	if(IS_ERR_OR_NULL(cam_main_vdd_a))
	{
		cam_main_vdd_a = regulator_get(NULL, "cam_main_vdd_a");
		if(IS_ERR_OR_NULL(cam_main_vdd_a))
		{
			pr_err("failed to get cam_main_vdd_a regulator");
			return -EINVAL;
		}
	}

    /* LDO14 */
	if(IS_ERR_OR_NULL(cam_main_vdd_io))
	{
		cam_main_vdd_io = regulator_get(NULL, "cam_main_vdd_io");
		if(IS_ERR_OR_NULL(cam_main_vdd_io))
		{
			pr_err("failed to get cam_main_vdd_io regulator");
			return -EINVAL;
		}
	}

    /* LDO15 */
	if(IS_ERR_OR_NULL(cam_main_vdd_d))
	{
		cam_main_vdd_d = regulator_get(NULL, "cam_main_vdd_d");
		if(IS_ERR_OR_NULL(cam_main_vdd_d))
		{
			pr_err("failed to get cam_main_vdd_d regulator");
			return -EINVAL;
		}
	}
	return 0;
}

/****************************************************************************
 * sr030pc30_power_en()
 *
 * @param     on/off
 * @return    err
 * @remark    Function
 *
 *****************************************************************************/
static int sr030pc30_power_en(int onoff)
{
	if(sr030pc30_regulator_init() < 0)
	{
		pr_err("Failed to initialize camera regulators\n");
		return -EINVAL;
	}

	if(onoff)
	{
		sr030pc30_power_on_sequence();
	}
	else
	{
		sr030pc30_power_off_sequence();
	}
	return 0;
}

static struct camera_platform_data sr030pc30_platform_data = {
	.default_width = 640,
	.default_height = 480,
	.pixelformat    = V4L2_PIX_FMT_UYVY,
	.mclk_freq      = 24000000,
	.is_mipi        = 0,
	.power_en = sr030pc30_power_en,
};

static struct i2c_board_info sr030pc30_i2c_info = {
	I2C_BOARD_INFO("SR030PC30", 0x60>>1),
	.platform_data = &sr030pc30_platform_data,
};

static struct s3c_platform_camera sr030pc30_platform_camera = {
	.id             = CAMERA_PAR_A,
	.type           = CAM_TYPE_ITU,
	.fmt            = ITU_601_YCBCR422_8BIT,
	.order422       = CAM_ORDER422_8BIT_CBYCRY,
	.i2c_busnum     = 0,
	.info		= &sr030pc30_i2c_info,
	.pixelformat    = V4L2_PIX_FMT_UYVY,
	.srclk_name     = "xusbxti",
	.clk_name       = "sclk_cam",
	.clk_rate       = 24000000,
	.line_length	= 480,
	.width          = 640,
	.height         = 480,
	.window		= {
		.left 	    = 0,
		.top        = 0,
		.width      = 640,
		.height     = 480,
	},

	/* Polarity */
	.inv_pclk	= 0,
	.inv_vsync      = 1,
	.inv_href       = 0,
	.inv_hsync      = 0,
	.initialized    = 0,
	.cam_power	= sr030pc30_power_en,
};
#endif

/* Interface setting */
static struct s3c_platform_fimc fimc_plat_lsi = {
	.srclk_name	= "mout_mpll",
	.clk_name	= "sclk_fimc",
	.lclk_name	= "sclk_fimc_lclk",
	.clk_rate	= 166750000,
	.default_cam	= CAMERA_PAR_A,
	.camera		= {
#ifdef CONFIG_VIDEO_S5K5CCGX
		&s5k5ccgx_platform_camera,
#endif
#ifdef CONFIG_VIDEO_SR030PC30
		&sr030pc30_platform_camera,
#endif
	},
	.hw_ver		= 0x43,
};

#ifdef CONFIG_VIDEO_JPEG_V2
static struct s3c_platform_jpeg jpeg_plat __initdata = {
	.max_main_width	    = 1280,
	.max_main_height	= 960,
	.max_thumb_width	= 320,
	.max_thumb_height	= 240,
};
#endif

/* I2C0 */
static struct i2c_board_info i2c_devs0[] __initdata = {
};

static struct i2c_board_info i2c_devs4[] __initdata = {
	{
		I2C_BOARD_INFO("wm8994", (0x34>>1)),
		.platform_data = &wm8994_pdata,
	},
};

/* I2C1 */
static struct i2c_board_info i2c_devs1[] __initdata = {
};


/* I2C2 */
#if defined (CONFIG_TOUCHSCREEN_ZINITIX)
static struct i2c_board_info i2c_devs2[] __initdata = {
	{
		I2C_BOARD_INFO("zinitix_isp", 0x50),
	},
	{
        I2C_BOARD_INFO("zinitix_touch", 0x20),
	},
};
#endif


#ifdef CONFIG_KEYBOARD_ADP5587
static struct i2c_board_info i2c_devs15[] __initdata = {
	{
		I2C_BOARD_INFO("adp5587", (0x68 >>  1)),
		.platform_data = &adp5587_kpd_platform_data,
		.irq = IRQ_EINT(24),
	},
};

static void __init adp5587_gpio_init(void)
{
	s3c_gpio_cfgpin(GPIO_INT, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(GPIO_INT, S3C_GPIO_PULL_NONE);
	set_irq_type(IRQ_EINT(24), IRQ_TYPE_EDGE_FALLING);
}
#endif

static void __init qt_touch_init(void)
{
        int gpio, irq;

#if defined(CONFIG_TOUCHSCREEN_MELFAS)
    gpio = GPIO_TOUCH_EN; // forte_temp  S5PV210_GPG3(6);                     /* XMMC3DATA_3 */
    gpio_request(gpio, "TOUCH_EN");
    s3c_gpio_cfgpin(gpio, S3C_GPIO_OUTPUT);
    gpio_direction_output(gpio, 1);
    gpio_free(gpio);

    gpio = GPIO_TOUCH_INT; // forte_temp   S5PV210_GPJ0(5);                             /* XMSMADDR_5 */
    gpio_request(gpio, "TOUCH_INT");
    s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(0xf));
    s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP);
    irq = gpio_to_irq(gpio);
    gpio_free(gpio);

    i2c_devs2[0].irq = irq;//PREVENT fixed
#elif defined(CONFIG_TOUCHSCREEN_QT602240)
        gpio = GPIO_TOUCH_EN;
        gpio_request(gpio, "TOUCH_EN");
        s3c_gpio_cfgpin(gpio, S3C_GPIO_OUTPUT);
        gpio_direction_output(gpio, 0);
        gpio_free(gpio);

        gpio = GPIO_TOUCH_INT;
        gpio_request(gpio, "TOUCH_INT");
        s3c_gpio_cfgpin(gpio, S3C_GPIO_INPUT);
        s3c_gpio_setpull(gpio, S3C_GPIO_PULL_DOWN);
        irq = gpio_to_irq(gpio);
        gpio_free(gpio);

        i2c_devs2[1].irq = irq;
#endif
}
#ifdef CONFIG_TOUCHSCREEN_MXT224
static void mxt224_power_on(void)
{
	gpio_direction_output(GPIO_TOUCH_EN, 1);

	mdelay(40);
}

static void mxt224_power_off(void)
{
	gpio_direction_output(GPIO_TOUCH_EN, 0);
}

#define MXT224_MAX_MT_FINGERS 5

static u8 t7_config[] = {GEN_POWERCONFIG_T7,
				64, 255, 50};
static u8 t8_config[] = {GEN_ACQUISITIONCONFIG_T8,
				7, 0, 5, 0, 0, 0, 9, 35};
static u8 t9_config[] = {TOUCH_MULTITOUCHSCREEN_T9,
				139, 0, 0, 19, 11, 0, 32, 25, 2, 1, 25, 3, 1,
				46, MXT224_MAX_MT_FINGERS, 5, 14, 10, 255, 3,
				255, 3, 18, 18, 10, 10, 141, 65, 143, 110, 18};
static u8 t18_config[] = {SPT_COMCONFIG_T18,
				0, 1};
static u8 t20_config[] = {PROCI_GRIPFACESUPPRESSION_T20,
				7, 0, 0, 0, 0, 0, 0, 80, 40, 4, 35, 10};
static u8 t22_config[] = {PROCG_NOISESUPPRESSION_T22,
				5, 0, 0, 0, 0, 0, 0, 3, 30, 0, 0, 29, 34, 39,
				49, 58, 3};
static u8 t28_config[] = {SPT_CTECONFIG_T28,
				1, 0, 3, 16, 63, 60};
static u8 end_config[] = {RESERVED_T255};

static const u8 *mxt224_config[] = {
	t7_config,
	t8_config,
	t9_config,
	t18_config,
	t20_config,
	t22_config,
	t28_config,
	end_config,
};

static struct mxt224_platform_data mxt224_data = {
	.max_finger_touches = MXT224_MAX_MT_FINGERS,
	.gpio_read_done = GPIO_TOUCH_INT,
	.config = mxt224_config,
	.min_x = 0,
	.max_x = 1023,
	.min_y = 0,
	.max_y = 1023,
	.min_z = 0,
	.max_z = 255,
	.min_w = 0,
	.max_w = 30,
	.power_on = mxt224_power_on,
	.power_off = mxt224_power_off,
};

/* I2C2 */
static struct i2c_board_info i2c_devs2[] __initdata = {
	{
		I2C_BOARD_INFO(MXT224_DEV_NAME, 0x4a),
		.platform_data = &mxt224_data,
		.irq = IRQ_EINT_GROUP(18, 5),
	},
};
#endif

/* I2C10 */
#ifdef CONFIG_KEYPAD_CYPRESS_TOUCH
static struct i2c_board_info i2c_devs10[] __initdata = {
	{
		I2C_BOARD_INFO(CYPRESS_TOUCHKEY_DEV_NAME, 0x20),
		.platform_data = &touchkey_data,
		.irq = (IRQ_EINT_GROUP18_BASE + 6),
	},
};
#endif


static struct k3g_platform_data k3g_pdata = {
	.axis_map_x = 1,
	.axis_map_y = 1,
	.axis_map_z = 1,
	.negate_x = 0,
	.negate_y = 0,
	.negate_z = 0,
};

#ifdef CONFIG_SAMSUNG_FM_SI4709
static struct i2c_board_info i2c_devs8[] __initdata = {
	{
		I2C_BOARD_INFO("Si4709", 0x20 >> 1),
		.irq = (IRQ_EINT_GROUP20_BASE + 4), /* J2_4 */
	},
};
#endif

static int fsa9480_init_flag = 0;
static bool mtp_off_status;

#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
extern u16 askonstatus;
void fsa9480_usb_cb(bool attached)
#else
static void fsa9480_usb_cb(bool attached)
#endif
{
	struct usb_gadget *gadget = platform_get_drvdata(&s3c_device_usbgadget);

#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
	if ((gadget) && (askonstatus != 0xabcd)) {
#else
	if (gadget) {
#endif
		if (attached)
			usb_gadget_vbus_connect(gadget);
		else
			usb_gadget_vbus_disconnect(gadget);
	}

	mtp_off_status = false;
	set_cable_status = attached ? CABLE_TYPE_USB : CABLE_TYPE_NONE;
	if (charger_callbacks && charger_callbacks->set_cable)
		charger_callbacks->set_cable(charger_callbacks, set_cable_status);
}

static void fsa9480_usb_charger_cb(bool attached)
{
	set_acc_status = attached ? ACC_TYPE_USB : ACC_TYPE_NONE;
	if (charger_callbacks && charger_callbacks->set_acc_type)
		charger_callbacks->set_acc_type(charger_callbacks, set_acc_status);

	set_cable_status = attached ? CABLE_TYPE_USB : CABLE_TYPE_NONE;
	if (charger_callbacks && charger_callbacks->set_cable)
		charger_callbacks->set_cable(charger_callbacks, set_cable_status);
}

static void fsa9480_charger_cb(bool attached)
{
	set_acc_status = attached ? ACC_TYPE_CHARGER : ACC_TYPE_NONE;
	if (charger_callbacks && charger_callbacks->set_acc_type)
		charger_callbacks->set_acc_type(charger_callbacks, set_acc_status);

	set_cable_status = attached ? CABLE_TYPE_AC : CABLE_TYPE_NONE;
	if (charger_callbacks && charger_callbacks->set_cable)
		charger_callbacks->set_cable(charger_callbacks, set_cable_status);
}

static void fsa9480_jig_cb(bool attached)
{
	set_acc_status = attached ? ACC_TYPE_JIG : ACC_TYPE_NONE;
	if (charger_callbacks && charger_callbacks->set_acc_type)
		charger_callbacks->set_acc_type(charger_callbacks, set_acc_status);
}

static struct switch_dev switch_dock = {
	.name = "dock",
};

static void fsa9480_deskdock_cb(bool attached)
{
	struct usb_gadget *gadget = platform_get_drvdata(&s3c_device_usbgadget);

	if (attached)
		switch_set_state(&switch_dock, 1);
	else
		switch_set_state(&switch_dock, 0);

	if (gadget) {
		if (attached)
			usb_gadget_vbus_connect(gadget);
		else
			usb_gadget_vbus_disconnect(gadget);
	}

	mtp_off_status = false;

	set_acc_status = attached ? ACC_TYPE_DESK_DOCK : ACC_TYPE_NONE;
	if (charger_callbacks && charger_callbacks->set_acc_type)
		charger_callbacks->set_acc_type(charger_callbacks, set_acc_status);
}

static void fsa9480_cardock_cb(bool attached)
{
	set_acc_status = attached ? ACC_TYPE_CAR_DOCK : ACC_TYPE_NONE;
	if (charger_callbacks && charger_callbacks->set_acc_type)
		charger_callbacks->set_acc_type(charger_callbacks, set_acc_status);

	if (attached)
		switch_set_state(&switch_dock, 2);
	else
		switch_set_state(&switch_dock, 0);
}

static void fsa9480_reset_cb(void)
{
	int ret;

	/* for CarDock, DeskDock */
	ret = switch_dev_register(&switch_dock);
	if (ret < 0)
		pr_err("Failed to register dock switch. %d\n", ret);
}

static void fsa9480_set_init_flag(void)
{
	fsa9480_init_flag = 1;
}

static struct fsa9480_platform_data fsa9480_pdata = {
	.usb_cb = fsa9480_usb_cb,
	.usb_charger_cb = fsa9480_usb_charger_cb,
	.charger_cb = fsa9480_charger_cb,
	.deskdock_cb = fsa9480_deskdock_cb,
	.cardock_cb = fsa9480_cardock_cb,
	.jig_cb = fsa9480_jig_cb,
	.reset_cb = fsa9480_reset_cb,
	.set_init_flag = fsa9480_set_init_flag,
};

static struct k3dh_platform_data k3dh_data = {
	.gpio_acc_int = GPIO_ACC_INT,
};

// fsa9480 {{
static struct i2c_board_info i2c_devs5[] __initdata = {
   { /* Accelermeter */
      I2C_BOARD_INFO("k3dh", 0x19),
      .platform_data = &k3dh_data,
   },
   { /* fsa9480 */
      I2C_BOARD_INFO("fsa9480", 0x4A >> 1),
      .platform_data = &fsa9480_pdata,
      .irq = IRQ_EINT(23),
   },
   { /* magmetic */
      I2C_BOARD_INFO("yas529", 0x2E),
   },
   {    /* Gyro Sensor K3G */
      I2C_BOARD_INFO("k3g", 0x69),
      .platform_data = &k3g_pdata,
      .irq = -1, // TODO:RECHECK!!! IRQ_EINT(29), /* DRDY = 29 / INT1 = IRQ_EINT5 */
   }
};

static struct i2c_board_info i2c_devs6[] __initdata = {
#ifdef CONFIG_REGULATOR_MAX8998
	{
		/* The address is 0xCC used since SRAD = 0 */
		I2C_BOARD_INFO("max8998", (0xCC >> 1)),
		.platform_data	= &max8998_pdata,
		.irq		= IRQ_EINT7,
	}, {
		I2C_BOARD_INFO("rtc_max8998", (0x0D >> 1)),
	},
#endif
};

#ifdef CONFIG_CHARGER_FAN5403
static struct i2c_board_info i2c_devs16[] __initdata = {
	{
		I2C_BOARD_INFO("fan5403", 0x6B),
		.platform_data	= &fan5403_pdata,
	},
};
#endif


static struct i2c_board_info i2c_devs13[] __initdata = {
	{
		I2C_BOARD_INFO("A1026_driver", (0x3E)),
	},
};


#ifdef CONFIG_PN544
static struct pn544_i2c_platform_data pn544_pdata = {
	.irq_gpio = NFC_IRQ,
	.ven_gpio = NFC_EN,
	.firm_gpio = NFC_FIRM,
};

static struct i2c_board_info i2c_devs14[] __initdata = {
	{
		I2C_BOARD_INFO("pn544", 0x2b),
		.irq = IRQ_EINT(12),
		.platform_data = &pn544_pdata,
	},
};
#endif

static int max17040_power_supply_register(struct device *parent,
	struct power_supply *psy)
{
	aries_charger.psy_fuelgauge = psy;
	return 0;
}

static void max17040_power_supply_unregister(struct power_supply *psy)
{
	aries_charger.psy_fuelgauge = NULL;
}

#ifdef CONFIG_BATTERY_MAX17043
static void __init max17043_gpio_init(void)
{
	s3c_gpio_cfgpin(GPIO_FUEL_INT, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(GPIO_FUEL_INT, S3C_GPIO_PULL_NONE);
}
#endif

static struct max17040_platform_data max17040_pdata = {
	.power_supply_register = max17040_power_supply_register,
	.power_supply_unregister = max17040_power_supply_unregister,
#ifdef CONFIG_BATTERY_MAX17043
	.rcomp_value = 0xB01F,	// ATHD 1%
#else
	.rcomp_value = 0xC000,
#endif
	.psoc_full = 9380,
	.psoc_empty = 50,
};

static struct i2c_board_info i2c_devs9[] __initdata = {
	{
		I2C_BOARD_INFO("max17040", (0x6D >> 1)),
		.platform_data = &max17040_pdata,
#ifdef CONFIG_BATTERY_MAX17043
		.irq = IRQ_EINT9,
#endif
	},
#ifdef CONFIG_CHARGER_SMB328A
        {
                I2C_BOARD_INFO("smb328a", (0x69 >> 1)),
                .platform_data  = &smb328a_pdata,
 		  .irq		= -1,	/*The Value is determined at run time depending on the HWREV in smb328a_gpio_init*/
        },
#endif
};

#ifdef CONFIG_CHARGER_SMB328A
static void __init smb328a_gpio_init(void)
{
	if(HWREV >= 0x02){
		printk("GPIO_TA_nCHG_REV02 is 0x%x\n", HWREV);
        	s3c_gpio_cfgpin(GPIO_TA_nCHG_REV02, S3C_GPIO_SFN(0xf));
        	s3c_gpio_setpull(GPIO_TA_nCHG_REV02, S3C_GPIO_PULL_NONE);
		 i2c_devs9[1].irq		= IRQ_EINT13;
	}
	else{
		printk("GPIO_TA_nCHG_REV01 is 0x%x\n", HWREV);
		s3c_gpio_cfgpin(GPIO_TA_nCHG_REV01, S3C_GPIO_SFN(0xf));
        	s3c_gpio_setpull(GPIO_TA_nCHG_REV01, S3C_GPIO_PULL_NONE);
		 i2c_devs9[1].irq		= IRQ_EINT_GROUP(18, 3);
	}
}
#endif


static void gp2a_gpio_init(void)
{
	int ret = gpio_request(GPIO_PS_ON, "gp2a_power_supply_on");
	if (ret)
		printk(KERN_ERR "Failed to request gpio gp2a power supply.\n");
}

static int gp2a_power(bool on)
{

	struct regulator *regulator, *regulator1;
	/* this controls the power supply rail to the gp2a IC */
	gpio_direction_output(GPIO_PS_ON, on);
    //if( HWREV < 9 ) /* no leda old version devices*/
     //   return;

	regulator = regulator_get(NULL, "leda");
	regulator1 = regulator_get(NULL, "vsensor");

	if (IS_ERR(regulator) || IS_ERR(regulator1)) {
		printk(KERN_ERR "keypad_ledregulator_get fail\n");
		return;
    }

	if (on)
	{
		regulator_enable(regulator);
		regulator_enable(regulator1);
	}
    else {
		if (regulator_is_enabled(regulator))
			regulator_force_disable(regulator);
		if (regulator_is_enabled(regulator1))
			regulator_force_disable(regulator1);
    }

	regulator_put(regulator);
	regulator_put(regulator1);
	return 0;
}

static int gp2a_light_adc_value(void)
{
	return s3c_adc_get_adc_data(6);
}

static struct gp2a_platform_data gp2a_pdata = {
	.power_on = gp2a_power,
	.p_out = GPIO_PS_VOUT,
	//.light_adc_value = gp2a_light_adc_value
	.adapt_num = 11,
};

static struct i2c_board_info i2c_devs11[] __initdata = {
	{
		I2C_BOARD_INFO("gp2a", (0x72 >> 1)),
		.platform_data = &gp2a_pdata,
	},
};

//static struct i2c_board_info i2c_devs12[] __initdata = {
//	{
//		I2C_BOARD_INFO("yas529", 0x2e),
//	},
//};

static struct resource ram_console_resource[] = {
	{
		.flags = IORESOURCE_MEM,
	}
};

static struct gp2a_platform_data opt_gp2a_data = {
	.power_on = gp2a_power,
	.p_out = GPIO_PS_VOUT,
	.addr = 0x72>>1,
	.version = 0,
	.adapt_num = 11,
};

static struct platform_device opt_gp2a = {
	.name = "gp2a-opt",
	.id = -1,
	.dev        = {
		.platform_data  = &opt_gp2a_data,
	},
};

static struct platform_device ram_console_device = {
	.name = "ram_console",
	.id = -1,
	.num_resources = ARRAY_SIZE(ram_console_resource),
	.resource = ram_console_resource,
};

#ifdef CONFIG_ANDROID_PMEM
static struct android_pmem_platform_data pmem_pdata = {
	.name = "pmem",
	.no_allocator = 1,
	.cached = 1,
	.start = 0,
	.size = 0,
};

static struct android_pmem_platform_data pmem_gpu1_pdata = {
	.name = "pmem_gpu1",
	.no_allocator = 1,
	.cached = 1,
	.buffered = 1,
	.start = 0,
	.size = 0,
};

static struct android_pmem_platform_data pmem_adsp_pdata = {
	.name = "pmem_adsp",
	.no_allocator = 1,
	.cached = 1,
	.buffered = 1,
	.start = 0,
	.size = 0,
};

static struct platform_device pmem_device = {
	.name = "android_pmem",
	.id = 0,
	.dev = { .platform_data = &pmem_pdata },
};

static struct platform_device pmem_gpu1_device = {
	.name = "android_pmem",
	.id = 1,
	.dev = { .platform_data = &pmem_gpu1_pdata },
};

static struct platform_device pmem_adsp_device = {
	.name = "android_pmem",
	.id = 2,
	.dev = { .platform_data = &pmem_adsp_pdata },
};

static void __init android_pmem_set_platdata(void)
{
	pmem_pdata.start = (u32)s5p_get_media_memory_bank(S5P_MDEV_PMEM, 0);
	pmem_pdata.size = (u32)s5p_get_media_memsize_bank(S5P_MDEV_PMEM, 0);

	pmem_gpu1_pdata.start =
		(u32)s5p_get_media_memory_bank(S5P_MDEV_PMEM_GPU1, 0);
	pmem_gpu1_pdata.size =
		(u32)s5p_get_media_memsize_bank(S5P_MDEV_PMEM_GPU1, 0);

	pmem_adsp_pdata.start =
		(u32)s5p_get_media_memory_bank(S5P_MDEV_PMEM_ADSP, 0);
	pmem_adsp_pdata.size =
		(u32)s5p_get_media_memsize_bank(S5P_MDEV_PMEM_ADSP, 0);
}
#endif

struct platform_device sec_device_battery = {
	.name	= "sec-battery",
	.id	= -1,
};

static int sec_switch_get_cable_status(void)
{
	return mtp_off_status ? CABLE_TYPE_NONE : set_cable_status;
}

static int sec_switch_get_phy_init_status(void)
{
	return fsa9480_init_flag;
}

static void sec_switch_set_vbus_status(u8 mode)
{
	if (mode == USB_VBUS_ALL_OFF)
		mtp_off_status = true;

	if (charger_callbacks && charger_callbacks->set_esafe)
		charger_callbacks->set_esafe(charger_callbacks, mode);
}

static void sec_switch_set_usb_gadget_vbus(bool en)
{
	struct usb_gadget *gadget = platform_get_drvdata(&s3c_device_usbgadget);

	if (gadget) {
		if (en)
			usb_gadget_vbus_connect(gadget);
		else
			usb_gadget_vbus_disconnect(gadget);
	}
}

static struct sec_switch_platform_data sec_switch_pdata = {
	.set_vbus_status = sec_switch_set_vbus_status,
	.set_usb_gadget_vbus = sec_switch_set_usb_gadget_vbus,
	.get_cable_status = sec_switch_get_cable_status,
	.get_phy_init_status = sec_switch_get_phy_init_status,
};

struct platform_device sec_device_switch = {
	.name	= "sec_switch",
	.id	= 1,
	.dev	= {
		.platform_data	= &sec_switch_pdata,
	}
};

static struct platform_device sec_device_rfkill = {
	.name	= "bt_rfkill",
	.id	= -1,
};

static struct platform_device sec_device_btsleep = {
	.name	= "bt_sleep",
	.id	= -1,
};

static struct sec_jack_zone sec_jack_zones[] = {
	{
		/* adc == 0, unstable zone, default to 3pole if it stays
		 * in this range for 300ms (30ms delays, 10 samples)
		 */
		.adc_high = 0,
		.delay_ms = 30,
		.check_count = 10,
		.jack_type = SEC_HEADSET_3POLE,
	},
	{
		/* 0 < adc <= 750, unstable zone, default to 3pole if it stays
		 * in this range for 300ms (30ms delays, 10 samples)
		 */
		.adc_high = 750,
		.delay_ms = 30,
		.check_count = 10,
		.jack_type = SEC_HEADSET_3POLE,
	},
	{
		/* 750 < adc <= 1201, 4 pole zone, default to 4pole if it
		 * stays in this range for 200ms (20ms delays, 10 samples)
		 */
		.adc_high = 1201,
		.delay_ms = 20,
		.check_count = 10,
		.jack_type = SEC_HEADSET_4POLE,
	},
	{
		/* 1201 < adc <= 3700, 4 pole zone, default to 4pole if it
		 * stays in this range for 300ms (30ms delays, 10 samples)
		 */
		.adc_high = 3700,
		.delay_ms = 30,
		.check_count = 10,
		.jack_type = SEC_HEADSET_4POLE,
	},
	{
		/* adc > 3700, unstable zone, default to 3pole if it stays
		 * in this range for two seconds (100ms delays, 50 samples)
		 */
		.adc_high = 0x7fffffff,
		.delay_ms = 100,
		.check_count = 50,
		.jack_type = SEC_HEADSET_3POLE,
	},
};

/* Only support one button of earjack in S1_EUR HW.
 * If your HW supports 3-buttons earjack made by Samsung and HTC,
 * add some zones here.
 */
static struct sec_jack_buttons_zone sec_jack_buttons_zones[] = {
	{
		/* 0 <= adc <= 94, stable zone */
		.code		= KEY_MEDIA,
		.adc_low	= 0,
		.adc_high	= 94,
	},
	{
		/* 95 <= adc <= 244, stable zone */
		.code		= KEY_VOLUMEUP,
		.adc_low	= 95,
		.adc_high	= 244,
	},
	{
		/* 245 <= adc <= 520, stable zone */
		.code		= KEY_VOLUMEDOWN,
		.adc_low	= 245,
		.adc_high	= 520,
	},
};

static int sec_jack_get_adc_value(void)
{
	return s3c_adc_get_adc_data(3);
}

struct sec_jack_platform_data sec_jack_pdata = {
	.set_micbias_state = sec_jack_set_micbias_state,
	.get_adc_value = sec_jack_get_adc_value,
	.zones = sec_jack_zones,
	.num_zones = ARRAY_SIZE(sec_jack_zones),
	.buttons_zones = sec_jack_buttons_zones,
	.num_buttons_zones = ARRAY_SIZE(sec_jack_buttons_zones),
	.det_gpio = GPIO_DET_35,
	.send_end_gpio = GPIO_EAR_SEND_END,
};

static struct platform_device sec_device_jack = {
	.name			= "sec_jack",
	.id			= 1, /* will be used also for gpio_event id */
	.dev.platform_data	= &sec_jack_pdata,
};

#define S5PV210_PS_HOLD_CONTROL_REG (S3C_VA_SYS+0xE81C)
static void aries_power_off(void)
{
	int err;
	int mode = REBOOT_MODE_NONE;
	char reset_mode = 'r';
	int phone_wait_cnt = 0;

	/* prevent phone reset when AP off */
	gpio_set_value(GPIO_PHONE_ON, 0);

	err = gpio_request(GPIO_PHONE_ACTIVE, "GPIO_PHONE_ACTIVE");
	/* will never be freed */
	WARN(err, "failed to request GPIO_PHONE_ACTIVE");
	gpio_direction_input(GPIO_nPOWER);

	gpio_direction_input(GPIO_PHONE_ACTIVE);


	/* confirm phone off */
	while (1) {
		if (gpio_get_value(GPIO_PHONE_ACTIVE)) {
			if (phone_wait_cnt > 10) {
				printk(KERN_EMERG
				       "%s: Try to Turn Phone Off by CP_RST\n",
				       __func__);
				gpio_set_value(GPIO_CP_RST, 0);
//				gpio_set_value(GPIO_VIA_PS_HOLD_OFF, 1);
			}
			if (phone_wait_cnt > 12) {
				printk(KERN_EMERG "%s: PHONE OFF Failed\n",
						__func__);
				break;
			}
			phone_wait_cnt++;
			msleep(1000);
		} else {
			printk(KERN_EMERG "%s: PHONE OFF Success\n", __func__);
			break;
		}
	}

	/* set VIA, LTE off again */
//	gpio_set_value(GPIO_CP_RST, 0);
//	gpio_set_value(GPIO_VIA_PS_HOLD_OFF, 1);

	/* Change this API call just before power-off to take the dump. */
//	kernel_sec_clear_upload_magic_number();

	/* clear Silent reset check flag */
//	writel(readl(S5P_INFORM6) & 0xFFFF00FF, S5P_INFORM6);

	while (1) {
		/* Check reboot charging */
		if (charger_callbacks &&
		    charger_callbacks->get_vdcin &&
		    charger_callbacks->get_vdcin(charger_callbacks)) {
			/* watchdog reset */
			pr_info("%s: charger connected, rebooting\n", __func__);
			mode = REBOOT_MODE_CHARGING;
			if (sec_set_param_value)
				sec_set_param_value(__REBOOT_MODE, &mode);
			kernel_sec_clear_upload_magic_number();
			kernel_sec_hw_reset(1);
			arch_reset('r', NULL);
			pr_crit("%s: waiting for reset!\n", __func__);
			while (1);
		}
		kernel_sec_clear_upload_magic_number();

		/* wait for power button release */
		if (gpio_get_value(GPIO_nPOWER)) {
			pr_info("%s: set PS_HOLD low\n", __func__);

			/* PS_HOLD high  PS_HOLD_CONTROL, R/W, 0xE010_E81C */
			writel(readl(S5PV210_PS_HOLD_CONTROL_REG) & 0xFFFFFEFF,
			       S5PV210_PS_HOLD_CONTROL_REG);

			pr_crit("%s: should not reach here!\n", __func__);
		}

		/* if power button is not released, wait and check TA again */
		pr_info("%s: PowerButton is not released.\n", __func__);
		mdelay(1000);
	}
}


#if defined(CONFIG_SAMSUNG_PHONE_SVNET)

/* onedram */
static void onedram_cfg_gpio(void);

static struct onedram_platform_data onedram_data = {
	.cfg_gpio = onedram_cfg_gpio,
};

static struct resource onedram_res[] = {
	[0] = {
		.start = (S5PV210_PA_SDRAM + 0x05000000),
		.end   = (S5PV210_PA_SDRAM + 0x05000000 + SZ_16M - 1),
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_EINT11,
		.end   = IRQ_EINT11,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device onedram = {
	.name          = "onedram",
	.id            = -1,
	.num_resources = ARRAY_SIZE(onedram_res),
	.resource      = onedram_res,
	.dev = {
		.platform_data = &onedram_data,
	},
};

static void onedram_cfg_gpio(void)
{
	s3c_gpio_cfgpin(GPIO_nINT_ONEDRAM_AP, S3C_GPIO_SFN(GPIO_nINT_ONEDRAM_AP_AF));
	s3c_gpio_setpull(GPIO_nINT_ONEDRAM_AP, S3C_GPIO_PULL_UP);
	set_irq_type(GPIO_nINT_ONEDRAM_AP, IRQ_TYPE_LEVEL_LOW);
}


/* Modem control */
static void modemctl_cfg_gpio(void);

static struct modemctl_platform_data mdmctl_data = {
	.name = "lte",
	.gpio_phone_on     = S5PV210_GPH1(6),
	.gpio_phone_active = S5PV210_GPH1(0),
	.gpio_pda_active   = S5PV210_GPF3(5),
	.gpio_cp_reset     = S5PV210_GPH1(4),
	.cfg_gpio          = modemctl_cfg_gpio,
};

static struct resource mdmctl_res[] = {
	[0] = {
		.start = IRQ_EINT8,
		.end   = IRQ_EINT8,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device modemctl = {
	.name          = "modemctl",
	.id            = -1,
	.num_resources = ARRAY_SIZE(mdmctl_res),
	.resource      = mdmctl_res,
	.dev = {
		.platform_data = &mdmctl_data,
	},
};

static void modemctl_cfg_gpio(void)
{
	int err = 0;

	unsigned gpio_phone_on     = mdmctl_data.gpio_phone_on;
	unsigned gpio_phone_active = mdmctl_data.gpio_phone_active;
	unsigned gpio_cp_rst       = mdmctl_data.gpio_cp_reset;
	unsigned gpio_pda_active   = mdmctl_data.gpio_pda_active;
	unsigned gpio_sim_ndetect  = mdmctl_data.gpio_sim_ndetect;

	err = gpio_request(gpio_phone_on, "PHONE_ON");
	if (err)
	{
		printk("fail to request gpio %s\n","PHONE_ON");
	}
	else
	{
		gpio_direction_output(gpio_phone_on, GPIO_LEVEL_LOW);
		s3c_gpio_setpull(gpio_phone_on, S3C_GPIO_PULL_NONE);
	}

	err = gpio_request(gpio_cp_rst, "CP_RST");
	if (err)
	{
		printk("fail to request gpio %s\n","CP_RST");
	}
	else
	{
		gpio_direction_output(gpio_cp_rst, GPIO_LEVEL_LOW);
		s3c_gpio_setpull(gpio_cp_rst, S3C_GPIO_PULL_UP);
	}

	err = gpio_request(gpio_pda_active, "PDA_ACTIVE");
	if (err)
	{
		printk("fail to request gpio %s\n","PDA_ACTIVE");
	}
	else
	{
		gpio_direction_output(gpio_pda_active, GPIO_LEVEL_HIGH);
		s3c_gpio_setpull(gpio_pda_active, S3C_GPIO_PULL_NONE);
	}

	s3c_gpio_cfgpin(gpio_phone_active, S3C_GPIO_INPUT);
	s3c_gpio_setpull(gpio_phone_active, S3C_GPIO_PULL_NONE);
	set_irq_type(gpio_phone_active, IRQ_TYPE_EDGE_BOTH);

	s3c_gpio_cfgpin(gpio_sim_ndetect, S3C_GPIO_SFN(0xF));
	s3c_gpio_setpull(gpio_sim_ndetect, S3C_GPIO_PULL_NONE);
	set_irq_type(gpio_sim_ndetect, IRQ_TYPE_EDGE_BOTH);
}
#endif  /*CONFIG_SAMSUNG_PHONE_SVNET || CONFIG_SAMSUNG_PHONE_SVNET_MODULE*/
static void config_gpio_table(int array_size, unsigned int (*gpio_table)[4])
{
	u32 i, gpio;

	for (i = 0; i < array_size; i++) {
		gpio = gpio_table[i][0];
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(gpio_table[i][1]));
		if (gpio_table[i][2] != S3C_GPIO_SETPIN_NONE)
			gpio_set_value(gpio, gpio_table[i][2]);
		s3c_gpio_setpull(gpio, gpio_table[i][3]);
	}
}

static void config_sleep_gpio_table(int array_size, unsigned int (*gpio_table)[3])
{
	u32 i, gpio;

	for (i = 0; i < array_size; i++) {
		gpio = gpio_table[i][0];
		s3c_gpio_slp_cfgpin(gpio, gpio_table[i][1]);
		s3c_gpio_slp_setpull_updown(gpio, gpio_table[i][2]);
	}
}

static void config_init_gpio(void)
{
	config_gpio_table(ARRAY_SIZE(initial_gpio_table), initial_gpio_table);
}

extern int curr_uart_switch_mode;

void config_sleep_gpio(void)
{
	config_gpio_table(ARRAY_SIZE(sleep_alive_gpio_table), sleep_alive_gpio_table);
	config_sleep_gpio_table(ARRAY_SIZE(sleep_gpio_table), sleep_gpio_table);

	if (gpio_get_value(GPIO_PS_ON)) {
		s3c_gpio_slp_setpull_updown(GPIO_ALS_SDA_28V, S3C_GPIO_PULL_NONE);
		s3c_gpio_slp_setpull_updown(GPIO_ALS_SCL_28V, S3C_GPIO_PULL_NONE);
	} 

	if(curr_uart_switch_mode == 0) {
		s3c_gpio_slp_cfgpin(GPIO_UART_SEL, S3C_GPIO_SLP_OUT0);
		s3c_gpio_slp_setpull_updown(GPIO_UART_SEL, S3C_GPIO_PULL_NONE);
		s3c_gpio_slp_cfgpin(GPIO_UART_SEL1,S3C_GPIO_SLP_OUT0);
		s3c_gpio_slp_setpull_updown(GPIO_UART_SEL1, S3C_GPIO_PULL_NONE);
	}
	else if(curr_uart_switch_mode == 1) {
		s3c_gpio_slp_cfgpin(GPIO_UART_SEL, S3C_GPIO_SLP_OUT1);
		s3c_gpio_slp_setpull_updown(GPIO_UART_SEL, S3C_GPIO_PULL_NONE);
	}
	else if(curr_uart_switch_mode == 2) {
		s3c_gpio_slp_cfgpin(GPIO_UART_SEL, S3C_GPIO_SLP_OUT0);
		s3c_gpio_slp_setpull_updown(GPIO_UART_SEL, S3C_GPIO_PULL_NONE);
		s3c_gpio_slp_cfgpin(GPIO_UART_SEL1,S3C_GPIO_SLP_OUT1);
		s3c_gpio_slp_setpull_updown(GPIO_UART_SEL1 , S3C_GPIO_PULL_NONE);
	}

	printk(KERN_DEBUG "SLPGPIO : BT(%d) WLAN(%d) BT+WIFI(%d)\n",
		gpio_get_value(GPIO_BT_nRST), gpio_get_value(GPIO_WLAN_nRST), gpio_get_value(GPIO_WLAN_BT_EN));
	printk(KERN_DEBUG "SLPGPIO : CODEC_LDO_EN(%d) MICBIAS_EN(%d)\n",
		gpio_get_value(GPIO_CODEC_LDO_EN), gpio_get_value(GPIO_MICBIAS_EN));
	printk(KERN_DEBUG "SLPGPIO : PS_ON(%d) FM_RST(%d) UART_SEL(%d)\n",
		gpio_get_value(GPIO_PS_ON), gpio_get_value(GPIO_FM_RST), gpio_get_value(GPIO_UART_SEL));
}
EXPORT_SYMBOL(config_sleep_gpio);

static unsigned int wlan_on_gpio_table[][4] = {
	      {GPIO_WLAN_EN , GPIO_WLAN_EN_AF, GPIO_LEVEL_HIGH, S3C_GPIO_PULL_NONE},
        {GPIO_WLAN_HOST_WAKE, GPIO_WLAN_HOST_WAKE_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN},
};
static unsigned int wlan_off_gpio_table[][4] = {
		    {GPIO_WLAN_EN , GPIO_WLAN_EN_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
        {GPIO_WLAN_HOST_WAKE, 0 , GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN},
 };
static unsigned int wlan_sdio_on_table[][4] = {
	{GPIO_WLAN_SDIO_CLK, GPIO_WLAN_SDIO_CLK_AF, GPIO_LEVEL_NONE,
		S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_CMD, GPIO_WLAN_SDIO_CMD_AF, GPIO_LEVEL_NONE,
		S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D0, GPIO_WLAN_SDIO_D0_AF, GPIO_LEVEL_NONE,
		S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D1, GPIO_WLAN_SDIO_D1_AF, GPIO_LEVEL_NONE,
		S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D2, GPIO_WLAN_SDIO_D2_AF, GPIO_LEVEL_NONE,
		S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D3, GPIO_WLAN_SDIO_D3_AF, GPIO_LEVEL_NONE,
		S3C_GPIO_PULL_NONE},
};

static unsigned int wlan_sdio_off_table[][4] = {
        {GPIO_WLAN_SDIO_CLK, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
        {GPIO_WLAN_SDIO_CMD, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
        {GPIO_WLAN_SDIO_D0, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
        {GPIO_WLAN_SDIO_D1, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
        {GPIO_WLAN_SDIO_D2, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
        {GPIO_WLAN_SDIO_D3, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
};


static int wlan_power_en(int onoff)
{
	if (onoff) {
		s3c_gpio_cfgpin(GPIO_WLAN_HOST_WAKE,
				S3C_GPIO_SFN(GPIO_WLAN_HOST_WAKE_AF));
		s3c_gpio_setpull(GPIO_WLAN_HOST_WAKE, S3C_GPIO_PULL_DOWN);
		s3c_gpio_cfgpin(GPIO_WLAN_nRST,
				S3C_GPIO_SFN(GPIO_WLAN_nRST_AF));
		s3c_gpio_setpull(GPIO_WLAN_nRST, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_WLAN_nRST, GPIO_LEVEL_HIGH);
		s3c_gpio_slp_cfgpin(GPIO_WLAN_nRST, S3C_GPIO_SLP_OUT1);
		s3c_gpio_slp_setpull_updown(GPIO_WLAN_nRST, S3C_GPIO_PULL_NONE);

		s3c_gpio_cfgpin(GPIO_WLAN_BT_EN, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_WLAN_BT_EN, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_WLAN_BT_EN, GPIO_LEVEL_HIGH);
		s3c_gpio_slp_cfgpin(GPIO_WLAN_BT_EN, S3C_GPIO_SLP_OUT1);
		s3c_gpio_slp_setpull_updown(GPIO_WLAN_BT_EN,
					S3C_GPIO_PULL_NONE);

		msleep(80);
	} else {
		gpio_set_value(GPIO_WLAN_nRST, GPIO_LEVEL_LOW);
		s3c_gpio_slp_cfgpin(GPIO_WLAN_nRST, S3C_GPIO_SLP_OUT0);
		s3c_gpio_slp_setpull_updown(GPIO_WLAN_nRST, S3C_GPIO_PULL_NONE);

		if (gpio_get_value(GPIO_BT_nRST) == 0) {
			gpio_set_value(GPIO_WLAN_BT_EN, GPIO_LEVEL_LOW);
			s3c_gpio_slp_cfgpin(GPIO_WLAN_BT_EN, S3C_GPIO_SLP_OUT0);
			s3c_gpio_slp_setpull_updown(GPIO_WLAN_BT_EN,
						S3C_GPIO_PULL_NONE);
		}
	}
	return 0;
}

static int wlan_reset_en(int onoff)
{
	gpio_set_value(GPIO_WLAN_nRST,
			onoff ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW);
	return 0;
}

static int wlan_carddetect_en(int onoff)
{
	u32 i;
	u32 sdio;

	if (onoff) {
		for (i = 0; i < ARRAY_SIZE(wlan_sdio_on_table); i++) {
			sdio = wlan_sdio_on_table[i][0];
			s3c_gpio_cfgpin(sdio,
					S3C_GPIO_SFN(wlan_sdio_on_table[i][1]));
			s3c_gpio_setpull(sdio, wlan_sdio_on_table[i][3]);
			if (wlan_sdio_on_table[i][2] != GPIO_LEVEL_NONE)
				gpio_set_value(sdio, wlan_sdio_on_table[i][2]);
		}
	} else {
		for (i = 0; i < ARRAY_SIZE(wlan_sdio_off_table); i++) {
			sdio = wlan_sdio_off_table[i][0];
			s3c_gpio_cfgpin(sdio,
				S3C_GPIO_SFN(wlan_sdio_off_table[i][1]));
			s3c_gpio_setpull(sdio, wlan_sdio_off_table[i][3]);
			if (wlan_sdio_off_table[i][2] != GPIO_LEVEL_NONE)
				gpio_set_value(sdio, wlan_sdio_off_table[i][2]);
		}
	}
	udelay(5);

	sdhci_s3c_force_presence_change(&s3c_device_hsmmc1);
	return 0;
}

static struct resource wifi_resources[] = {
	[0] = {
		.name	= "bcm4329_wlan_irq",
      .start   = IRQ_EINT(3),    // chief - WLAN_HOST_WAKE
      .end  = IRQ_EINT(3),
		.flags	= IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL,
	},
};


void s3c_config_gpio_alive_table(int array_size, unsigned int (*gpio_table)[4])
{
	u32 i, gpio;
	printk("gpio_table = [%d] \r\n" , array_size);
	for (i = 0; i < array_size; i++) {
		gpio = gpio_table[i][0];
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(gpio_table[i][1]));
		s3c_gpio_setpull(gpio, gpio_table[i][3]);
		if (gpio_table[i][2] != GPIO_LEVEL_NONE)
		    gpio_set_value(gpio, gpio_table[i][2]);
	}
}


void wlan_setup_power(int on, int flag)
	{
		printk("------------------------------------------------");
		printk("------------------------------------------------");
		printk("------------------------------------------------");
	
		printk(/*KERN_INFO*/ "%s %s", __func__, on ? "on" : "down");
		if (flag != 1) {
			printk(/*KERN_DEBUG*/ "(on=%d, flag=%d)\n", on, flag);

			if (on)
				gpio_set_value(GPIO_WLAN_EN, GPIO_LEVEL_HIGH);
			else
				gpio_set_value(GPIO_WLAN_EN, GPIO_LEVEL_LOW);


		return;
		}
		printk(/*KERN_INFO*/ " --enter\n");
	
		if (on) {

			s3c_config_gpio_alive_table(ARRAY_SIZE(wlan_on_gpio_table), wlan_on_gpio_table);

			s3c_config_gpio_alive_table(ARRAY_SIZE(wlan_sdio_on_table), wlan_sdio_on_table);
	
			udelay(200);
			

			gpio_set_value(GPIO_WLAN_EN, GPIO_LEVEL_HIGH);

			//s3c_gpio_slp_cfgpin(GPIO_WLAN_EN, S3C_GPIO_SLP_OUT1);
	
			printk(KERN_DEBUG "WLAN: GPIO_WLAN_EN = %d \n", gpio_get_value(GPIO_WLAN_EN));
		}
		else {

				gpio_set_value(GPIO_WLAN_EN, GPIO_LEVEL_LOW);

			//s3c_gpio_slp_cfgpin(GPIO_WLAN_EN, S3C_GPIO_SLP_OUT0);

				s3c_config_gpio_alive_table(ARRAY_SIZE(wlan_off_gpio_table), wlan_off_gpio_table);

	
			printk(KERN_DEBUG "WLAN: GPIO_WLAN_EN = %d \n", gpio_get_value(GPIO_WLAN_EN));
	
			s3c_config_gpio_alive_table(ARRAY_SIZE(wlan_sdio_off_table), wlan_sdio_off_table);
		}

		/* mmc_rescan*/
		sdhci_s3c_force_presence_change(&s3c_device_hsmmc1);

	
	}

EXPORT_SYMBOL(wlan_setup_power);

static struct wifi_mem_prealloc wifi_mem_array[PREALLOC_WLAN_SEC_NUM] = {
	{NULL, (WLAN_SECTION_SIZE_0 + PREALLOC_WLAN_SECTION_HEADER)},
	{NULL, (WLAN_SECTION_SIZE_1 + PREALLOC_WLAN_SECTION_HEADER)},
	{NULL, (WLAN_SECTION_SIZE_2 + PREALLOC_WLAN_SECTION_HEADER)},
	{NULL, (WLAN_SECTION_SIZE_3 + PREALLOC_WLAN_SECTION_HEADER)}
};

void *wlan_mem_prealloc(int section, unsigned long size)
{
	if (section == PREALLOC_WLAN_SEC_NUM)
		return wlan_static_skb;

	if ((section < 0) || (section > PREALLOC_WLAN_SEC_NUM))
		return NULL;

	if (wifi_mem_array[section].size < size)
		return NULL;

	return wifi_mem_array[section].mem_ptr;
}
EXPORT_SYMBOL(wlan_mem_prealloc);

#define DHD_SKB_HDRSIZE 		336
#define DHD_SKB_1PAGE_BUFSIZE	((PAGE_SIZE*1)-DHD_SKB_HDRSIZE)
#define DHD_SKB_2PAGE_BUFSIZE	((PAGE_SIZE*2)-DHD_SKB_HDRSIZE)
#define DHD_SKB_4PAGE_BUFSIZE	((PAGE_SIZE*4)-DHD_SKB_HDRSIZE)
int __init aries_init_wifi_mem(void)
{
	int i;
	int j;

	for (i = 0; i < 8; i++) {
		wlan_static_skb[i] = dev_alloc_skb(DHD_SKB_1PAGE_BUFSIZE);
		if (!wlan_static_skb[i])
			goto err_skb_alloc;
	}

	for (; i < 16; i++) {
		wlan_static_skb[i] = dev_alloc_skb(DHD_SKB_2PAGE_BUFSIZE);
		if (!wlan_static_skb[i])
			goto err_skb_alloc;
	}

	wlan_static_skb[i] = dev_alloc_skb(DHD_SKB_4PAGE_BUFSIZE);
	if (!wlan_static_skb[i])
		goto err_skb_alloc;

	for (i = 0 ; i < PREALLOC_WLAN_SEC_NUM ; i++) {
		wifi_mem_array[i].mem_ptr =
				kmalloc(wifi_mem_array[i].size, GFP_KERNEL);

		if (!wifi_mem_array[i].mem_ptr)
			goto err_mem_alloc;
	}
	return 0;

 err_mem_alloc:
	pr_err("Failed to mem_alloc for WLAN\n");
	for (j = 0 ; j < i ; j++)
		kfree(wifi_mem_array[j].mem_ptr);

	i = WLAN_SKB_BUF_NUM;

 err_skb_alloc:
	pr_err("Failed to skb_alloc for WLAN\n");
	for (j = 0 ; j < i ; j++)
		dev_kfree_skb(wlan_static_skb[j]);

	return -ENOMEM;
}
static struct wifi_platform_data wifi_pdata = {
	.set_power		= wlan_power_en,
	.set_reset		= wlan_reset_en,
	.set_carddetect		= wlan_carddetect_en,
	.mem_prealloc		= wlan_mem_prealloc,
};

static struct platform_device sec_device_wifi = {
	.name			= "bcm4329_wlan",
	.id			= 1,
	.num_resources		= ARRAY_SIZE(wifi_resources),
	.resource		= wifi_resources,
	.dev			= {
		.platform_data = &wifi_pdata,
	},
};

static struct platform_device watchdog_device = {
	.name = "watchdog",
	.id = -1,
};

/* UMS */
static struct usb_mass_storage_lun_data luns_data[] = {
	[0] = {
		.filename	= NULL,
		.ro 		= 0,
		.removable	= 1,
		.cdrom 		= 0,
	}
};

static struct usb_mass_storage_platform_data ums_pdata = {
	.vendor			= "SAMSUNG",
	.product		= "UMS",
	.release		= 1,
	.nluns			= 1,
	.luns			= luns_data,
};

struct platform_device chief_usb_mass_storage = {
	.name	= "usb_mass_storage",
	.id	= -1,
	.dev	= {
		.platform_data = &ums_pdata,
	},
};

static struct platform_device *aries_devices[] __initdata = {
	&watchdog_device,
#ifdef CONFIG_FIQ_DEBUGGER
	&s5pv210_device_fiqdbg_uart2,
#endif
	&s5pc110_device_onenand,
#ifdef CONFIG_RTC_DRV_S3C
	&s5p_device_rtc,
#endif
	&s3c_device_keypad,
	&aries_input_device,

	&s5pv210_device_iis0,
	&s3c_device_wdt,

#ifdef CONFIG_FB_S3C
	&s3c_device_fb,
#endif

#ifdef CONFIG_VIDEO_MFC50
	&s3c_device_mfc,
#endif
#ifdef	CONFIG_S5P_ADC
	&s3c_device_adc,
#endif
#ifdef CONFIG_VIDEO_FIMC
	&s3c_device_fimc0,
	&s3c_device_fimc1,
	&s3c_device_fimc2,
#endif

#ifdef CONFIG_VIDEO_JPEG_V2
	&s3c_device_jpeg,
#endif

	&s3c_device_g3d,
	&s3c_device_lcd,

#if defined(CONFIG_FB_S3C_TL2796) || defined(CONFIG_FB_S3C_S6D16A0X) || defined (CONFIG_FB_S3C_S6D05A3)
	&s3c_device_spi_gpio,
#endif
	&sec_device_jack,
	&s3c_device_vibrator,
	&s3c_device_i2c0,
#if defined(CONFIG_S3C_DEV_I2C1)
	&s3c_device_i2c1,
#endif

#if defined(CONFIG_S3C_DEV_I2C2)
	&s3c_device_i2c2,
#endif
	&s3c_device_i2c4,
	&s3c_device_i2c5,  /* accel sensor,  fsa9480, magmetic*/
	&s3c_device_i2c6,
//	&s3c_device_i2c7,
//#ifdef CONFIG_SAMSUNG_FM_SI4709
//	&s3c_device_i2c8,  /* FM radio */
//#endif
	&s3c_device_i2c9,  /* max1704x:fuel_guage */
	&s3c_device_i2c11, /* optical sensor */
//	&s3c_device_i2c12, /* magnetic sensor */
//#ifdef CONFIG_SND_SOC_A1026
//	&s3c_device_i2c13, /* A1026 : noise suppressor */
//#endif
	&s3c_device_i2c13, /* backlight bd60910 */
//#ifdef CONFIG_PN544
//	&s3c_device_i2c14, /* nfc sensor */
//#endif
//#ifdef CONFIG_VIDEO_ISX005
//	&s3c_device_i2c17, /* ISX005 : Rear Camera */
//#endif
#ifdef CONFIG_USB_GADGET
	&s3c_device_usbgadget,
#endif
#ifdef CONFIG_USB_ANDROID
	&s3c_device_android_usb,
#ifdef CONFIG_USB_ANDROID_MASS_STORAGE
	&chief_usb_mass_storage,
#endif
    &sec_device_dpram,
#ifdef CONFIG_USB_ANDROID_RNDIS
	&s3c_device_rndis,
#endif
#endif

#ifdef CONFIG_S3C_DEV_HSMMC
	&s3c_device_hsmmc0,
#endif
#ifdef CONFIG_S3C_DEV_HSMMC1
	&s3c_device_hsmmc1,
#endif
#ifdef CONFIG_S3C_DEV_HSMMC2
	&s3c_device_hsmmc2,
#endif
#ifdef CONFIG_S3C_DEV_HSMMC3
	&s3c_device_hsmmc3,
#endif
	&sec_device_battery,
#ifdef CONFIG_KEYPAD_CYPRESS_TOUCH
	&s3c_device_i2c10,
#endif
	&sec_device_switch,  // samsung switch driver

#ifdef CONFIG_S5PV210_POWER_DOMAIN
	&s5pv210_pd_audio,
	&s5pv210_pd_cam,
	&s5pv210_pd_tv,
	&s5pv210_pd_lcd,
	&s5pv210_pd_g3d,
	&s5pv210_pd_mfc,
#endif

#ifdef CONFIG_ANDROID_PMEM
	&pmem_device,
	&pmem_gpu1_device,
	&pmem_adsp_device,
#endif

#ifdef CONFIG_HAVE_PWM
	&s3c_device_timer[0],
	&s3c_device_timer[1],
	&s3c_device_timer[2],
	&s3c_device_timer[3],
#endif
	&sec_device_rfkill,
	&sec_device_btsleep,
	&ram_console_device,

	&opt_gp2a,

//	&s5p_device_ace,
#if defined(CONFIG_TOUCHSCREEN_MELFAS)
	&s3c_device_melfas,
#endif
#ifdef CONFIG_SND_S5P_RP
	&s5p_device_rp,
#endif
	&sec_device_wifi,
};


static void __init aries_map_io(void)
{
	s5p_init_io(NULL, 0, S5P_VA_CHIPID);
	s3c24xx_init_clocks(24000000);
	s5pv210_gpiolib_init();
	s3c24xx_init_uarts(aries_uartcfgs, ARRAY_SIZE(aries_uartcfgs));
	s5p_reserve_bootmem(aries_media_devs, ARRAY_SIZE(aries_media_devs));
#ifdef CONFIG_MTD_ONENAND
	s5pc110_device_onenand.name = "s5pc110-onenand";
#endif
}

unsigned int pm_debug_scratchpad;

static unsigned int ram_console_start;
static unsigned int ram_console_size;

static void __init aries_fixup(struct machine_desc *desc,
		struct tag *tags, char **cmdline,
		struct meminfo *mi)
{
	mi->bank[0].start = 0x30000000;
	mi->bank[0].size = 80 * SZ_1M;
	mi->bank[0].node = 0;

	mi->bank[1].start = 0x40000000;
	mi->bank[1].size = 256 * SZ_1M;
	mi->bank[1].node = 1;

	mi->bank[2].start = 0x50000000;
	/* 1M for ram_console buffer */
	mi->bank[2].size = 127 * SZ_1M;
	mi->bank[2].node = 2;
	mi->nr_banks = 3;

	ram_console_start = mi->bank[2].start + mi->bank[2].size;
	ram_console_size = SZ_1M - SZ_4K;

	pm_debug_scratchpad = ram_console_start + ram_console_size;
}

/* this function are used to detect s5pc110 chip version temporally */
int s5pc110_version ;

void _hw_version_check(void)
{
	void __iomem *phy_address ;
	int temp;

	phy_address = ioremap(0x40, 1);

	temp = __raw_readl(phy_address);

	if (temp == 0xE59F010C)
		s5pc110_version = 0;
	else
		s5pc110_version = 1;

	printk(KERN_INFO "S5PC110 Hardware version : EVT%d\n",
				s5pc110_version);

	iounmap(phy_address);
}

/*
 * Temporally used
 * return value 0 -> EVT 0
 * value 1 -> evt 1
 */

int hw_version_check(void)
{
	return s5pc110_version ;
}
EXPORT_SYMBOL(hw_version_check);

static void __init fsa9480_gpio_init(void)
{
	s3c_gpio_cfgpin(GPIO_USB_SEL, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_USB_SEL, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_UART_SEL, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_UART_SEL, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_UART_SEL1, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_UART_SEL1, S3C_GPIO_PULL_NONE);

	s3c_gpio_cfgpin(GPIO_JACK_nINT, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(GPIO_JACK_nINT, S3C_GPIO_PULL_NONE);
}

static void __init max8998_gpio_init(void)
{
	s3c_gpio_cfgpin(GPIO_AP_PMIC_IRQ, S3C_GPIO_SFN(GPIO_AP_PMIC_IRQ_AF));
	s3c_gpio_setpull(GPIO_AP_PMIC_IRQ, S3C_GPIO_PULL_NONE);
}

static void __init setup_ram_console_mem(void)
{
	ram_console_resource[0].start = ram_console_start;
	ram_console_resource[0].end = ram_console_start + ram_console_size - 1;
}

static void __init sound_init(void)
{
	u32 reg;

	reg = __raw_readl(S5P_OTHERS);
	reg &= ~(0x3 << 8);
	reg |= 3 << 8;
	__raw_writel(reg, S5P_OTHERS);

	reg = __raw_readl(S5P_CLK_OUT);
	reg &= ~(0x1f << 12);
	reg |= 19 << 12;
	__raw_writel(reg, S5P_CLK_OUT);

	reg = __raw_readl(S5P_CLK_OUT);
	reg &= ~0x1;
	reg |= 0x1;
	__raw_writel(reg, S5P_CLK_OUT);

	gpio_request(GPIO_MICBIAS_EN, "micbias_enable");
    s3c_gpio_cfgpin(GPIO_MICBIAS_EN, S3C_GPIO_OUTPUT);
}

static void __init onenand_init(void)
{
	struct clk *clk = clk_get(NULL, "onenand");
	BUG_ON(!clk);
	clk_enable(clk);
}

static void __init aries_machine_init(void)
{
	setup_ram_console_mem();
	s3c_usb_set_serial();
	platform_add_devices(aries_devices, ARRAY_SIZE(aries_devices));

	/* Find out S5PC110 chip version */
	_hw_version_check();

	pm_power_off = aries_power_off ;

	HWREV = system_rev;
	printk("HWREV is 0x%x\n", HWREV);
	/*initialise the gpio's*/
	config_init_gpio();

#ifdef CONFIG_ANDROID_PMEM
	android_pmem_set_platdata();
#endif

	/* i2c */
	s3c_i2c0_set_platdata(NULL);
#ifdef CONFIG_S3C_DEV_I2C1
	s3c_i2c1_set_platdata(NULL);
#endif

#ifdef CONFIG_S3C_DEV_I2C2
	s3c_i2c2_set_platdata(NULL);
#endif
	/* H/W I2C lines */
	if (system_rev >= 0x05) {
		/* gyro sensor */
		i2c_register_board_info(0, i2c_devs0, ARRAY_SIZE(i2c_devs0));
		/* magnetic and accel sensor */
		i2c_register_board_info(1, i2c_devs1, ARRAY_SIZE(i2c_devs1));
	}
        qt_touch_init();
	i2c_register_board_info(2, i2c_devs2, ARRAY_SIZE(i2c_devs2));

	/* wm8994 codec */
	sound_init();
	i2c_register_board_info(4, i2c_devs4, ARRAY_SIZE(i2c_devs4));
	/* accel sensor,  fsa9480, magmetic*/
	fsa9480_gpio_init(); // fsa9480
	i2c_register_board_info(5, i2c_devs5, ARRAY_SIZE(i2c_devs5));

	max8998_gpio_init();
	i2c_register_board_info(6, i2c_devs6, ARRAY_SIZE(i2c_devs6));

#ifdef CONFIG_CHARGER_SMB328A
        smb328a_gpio_init();
#endif
	i2c_register_board_info(9, i2c_devs9, ARRAY_SIZE(i2c_devs9));
	/* optical sensor */
	gp2a_gpio_init();
	i2c_register_board_info(11, i2c_devs11, ARRAY_SIZE(i2c_devs11));
	/* magnetic sensor */
//	i2c_register_board_info(12, i2c_devs12, ARRAY_SIZE(i2c_devs12));
	/* bd60910 */
//	i2c_register_board_info(13, i2c_devs13, ARRAY_SIZE(i2c_devs13));
#ifdef CONFIG_PN544
	/* nfc sensor */
	i2c_register_board_info(14, i2c_devs14, ARRAY_SIZE(i2c_devs14));
#endif

#if defined(CONFIG_FB_S3C_TL2796)
	spi_register_board_info(spi_board_info, ARRAY_SIZE(spi_board_info));
	s3cfb_set_platdata(&tl2796_data);
#elif defined(CONFIG_FB_S3C_LM347DF01) ||defined(CONFIG_FB_S3C_S6D05A3)
    if(lcd_Idtype == LM347DF01_LCD_ID) {
	printk("**** LM347DF01_LCD Is Loaded *********\n");
	spi_register_board_info(spi_board_info, ARRAY_SIZE(spi_board_info));
	s3cfb_set_platdata(&lm347df01_data);
	}
     else
	{
    spi_register_board_info(spi_board_info, ARRAY_SIZE(spi_board_info));
    s3cfb_set_platdata(&s6d05a3_data);
	}
#endif

#if defined(CONFIG_S5P_ADC)
	s3c_adc_set_platdata(&s3c_adc_platform);
#endif

#if defined(CONFIG_PM)
	s3c_pm_init();
#endif

#ifdef CONFIG_VIDEO_FIMC
	/* fimc */
	s3c_fimc0_set_platdata(&fimc_plat_lsi);
	s3c_fimc1_set_platdata(&fimc_plat_lsi);
	s3c_fimc2_set_platdata(&fimc_plat_lsi);
#endif

#ifdef CONFIG_VIDEO_JPEG_V2
	s3c_jpeg_set_platdata(&jpeg_plat);
#endif

#ifdef CONFIG_VIDEO_MFC50
	/* mfc */
	s3c_mfc_set_platdata(NULL);
#endif

#ifdef CONFIG_S3C_DEV_HSMMC
	s5pv210_default_sdhci0();
#endif
#ifdef CONFIG_S3C_DEV_HSMMC1
	s5pv210_default_sdhci1();
#endif
#ifdef CONFIG_S3C_DEV_HSMMC2
	s5pv210_default_sdhci2();
#endif
#ifdef CONFIG_S3C_DEV_HSMMC3
	s5pv210_default_sdhci3();
#endif
#ifdef CONFIG_S5PV210_SETUP_SDHCI
	s3c_sdhci_set_platdata();
#endif

#if defined(CONFIG_SAMSUNG_PHONE_SVNET)
	platform_device_register(&modemctl);
	platform_device_register(&onedram);
#endif
//	regulator_has_full_constraints();

	register_reboot_notifier(&aries_reboot_notifier);

	aries_switch_init();

	gps_gpio_init();

	aries_init_wifi_mem();

	onenand_init();

	#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
/* soonyong.cho : This is for setting unique serial number */
	s3c_usb_set_serial();
	#endif

	if (gpio_is_valid(GPIO_MSENSE_nRST)) {
		if (gpio_request(GPIO_MSENSE_nRST, "GPB"))
			printk(KERN_ERR "Failed to request GPIO_MSENSE_nRST!\n");
		gpio_direction_output(GPIO_MSENSE_nRST, 1);
	}
	gpio_free(GPIO_MSENSE_nRST);
#ifdef RECOVERY_SYSFS
        recovery_class_init();
#endif

}

#ifdef CONFIG_USB_SUPPORT
/* Initializes OTG Phy. */
void otg_phy_init(void)
{

    printk("%s----------------->\n",__func__);
 
	/* USB PHY0 Enable */
	writel(readl(S5P_USB_PHY_CONTROL) | (0x1<<0),
			S5P_USB_PHY_CONTROL);
	writel((readl(S3C_USBOTG_PHYPWR) & ~(0x3<<3) & ~(0x1<<0)) | (0x1<<5),
			S3C_USBOTG_PHYPWR);
	writel((readl(S3C_USBOTG_PHYCLK) & ~(0x5<<2)) | (0x3<<0),
			S3C_USBOTG_PHYCLK);
	writel((readl(S3C_USBOTG_RSTCON) & ~(0x3<<1)) | (0x1<<0),
			S3C_USBOTG_RSTCON);
	msleep(1);
	writel(readl(S3C_USBOTG_RSTCON) & ~(0x7<<0),
			S3C_USBOTG_RSTCON);
	msleep(1);

	/* rising/falling time */
	writel(readl(S3C_USBOTG_PHYTUNE) | (0x1<<20),
			S3C_USBOTG_PHYTUNE);

	/* set DC level as 6 (6%) */
	writel((readl(S3C_USBOTG_PHYTUNE) & ~(0x380f))|(0x1<<13)|(0x0<<12)|(0x1<<11) | (0x1<<3) | (0x1<<2) | (0x1<< 1) | (0x1<<0),
			S3C_USBOTG_PHYTUNE);//Modified from DC level 6% to 15% to provide max driving voltage.

    printk("USB:S3C_USBOTG_PHYTUNE=0x%x\n",readl(S3C_USBOTG_PHYTUNE));
    
}
EXPORT_SYMBOL(otg_phy_init);

/* USB Control request data struct must be located here for DMA transfer */
struct usb_ctrlrequest usb_ctrl __attribute__((aligned(64)));

/* OTG PHY Power Off */
void otg_phy_off(void)
{
	writel(readl(S3C_USBOTG_PHYPWR) | (0x3<<3),
			S3C_USBOTG_PHYPWR);
	writel(readl(S5P_USB_PHY_CONTROL) & ~(1<<0),
			S5P_USB_PHY_CONTROL);
}
EXPORT_SYMBOL(otg_phy_off);

void usb_host_phy_init(void)
{
	struct clk *otg_clk;

	otg_clk = clk_get(NULL, "otg");
	clk_enable(otg_clk);

	if (readl(S5P_USB_PHY_CONTROL) & (0x1<<1))
		return;

	__raw_writel(__raw_readl(S5P_USB_PHY_CONTROL) | (0x1<<1),
			S5P_USB_PHY_CONTROL);
	__raw_writel((__raw_readl(S3C_USBOTG_PHYPWR)
			& ~(0x1<<7) & ~(0x1<<6)) | (0x1<<8) | (0x1<<5),
			S3C_USBOTG_PHYPWR);
	__raw_writel((__raw_readl(S3C_USBOTG_PHYCLK) & ~(0x1<<7)) | (0x3<<0),
			S3C_USBOTG_PHYCLK);
	__raw_writel((__raw_readl(S3C_USBOTG_RSTCON)) | (0x1<<4) | (0x1<<3),
			S3C_USBOTG_RSTCON);
	__raw_writel(__raw_readl(S3C_USBOTG_RSTCON) & ~(0x1<<4) & ~(0x1<<3),
			S3C_USBOTG_RSTCON);
}
EXPORT_SYMBOL(usb_host_phy_init);

void usb_host_phy_off(void)
{
	__raw_writel(__raw_readl(S3C_USBOTG_PHYPWR) | (0x1<<7)|(0x1<<6),
			S3C_USBOTG_PHYPWR);
	__raw_writel(__raw_readl(S5P_USB_PHY_CONTROL) & ~(1<<1),
			S5P_USB_PHY_CONTROL);
}
EXPORT_SYMBOL(usb_host_phy_off);
#endif

#if defined(CONFIG_KEYPAD_S3C)
void s3c_setup_keypad_cfg_gpio(int rows, int columns)
{
	unsigned int gpio;
	unsigned int end;

	/* set KBR */
	end = S5PV210_GPH3(rows);
	for (gpio = S5PV210_GPH3(0); gpio < end; gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	}

	/* set KBC */
    /* KEYPAD SCAN/COLUMN */
    columns -= 2 ;  /* except scan[6~7] */
	end = S5PV210_GPH2(columns);
	for (gpio = S5PV210_GPH2(0); gpio < end; gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	}
    s3c_gpio_cfgpin(GPIO_KEYSCAN6, S3C_GPIO_OUTPUT) ;
	s3c_gpio_setpull(GPIO_KEYSCAN6, S3C_GPIO_PULL_NONE);
    s3c_gpio_cfgpin(GPIO_KEYSCAN7, S3C_GPIO_OUTPUT) ;
	s3c_gpio_setpull(GPIO_KEYSCAN7, S3C_GPIO_PULL_NONE);
}
EXPORT_SYMBOL(s3c_setup_keypad_cfg_gpio);
#endif
void keypad_led_control(bool onOff)
{
	struct regulator *regulator;

	regulator = regulator_get(NULL, "keyled");
	if (IS_ERR(regulator)) {
		printk(KERN_ERR "keypad_ledregulator_get fail\n");
		return;
    }

	if (onOff)
		regulator_enable(regulator);
    else {
		if (regulator_is_enabled(regulator))
			regulator_force_disable(regulator);
    }

	regulator_put(regulator);
}
EXPORT_SYMBOL(keypad_led_control);

void motor_control(bool onOff)
{
	struct regulator *regulator;

	regulator = regulator_get(NULL, "vibrator");
	if (IS_ERR(regulator)) {
		printk(KERN_ERR "vibrator_regulator_get fail\n");
		return;
    }

	if (onOff)
		regulator_enable(regulator);
    else {
		if (regulator_is_enabled(regulator))
			regulator_force_disable(regulator);
    }

	regulator_put(regulator);
}
EXPORT_SYMBOL(motor_control);

MACHINE_START(SMDKC110, "SMDKC110")
	/* Maintainer: Kukjin Kim <kgene.kim@samsung.com> */
	.phys_io	= S3C_PA_UART & 0xfff00000,
	.io_pg_offst	= (((u32)S3C_VA_UART) >> 18) & 0xfffc,
	.boot_params	= S5P_PA_SDRAM + 0x100,
	.fixup		= aries_fixup,
	.init_irq	= s5pv210_init_irq,
	.map_io		= aries_map_io,
	.init_machine	= aries_machine_init,
#if	defined(CONFIG_S5P_HIGH_RES_TIMERS)
	.timer		= &s5p_systimer,
#else
	.timer		= &s3c24xx_timer,
#endif
MACHINE_END

MACHINE_START(CHIEF, "SMDKC110")
	.phys_io	= S3C_PA_UART & 0xfff00000,
	.io_pg_offst	= (((u32)S3C_VA_UART) >> 18) & 0xfffc,
	.boot_params	= S5P_PA_SDRAM + 0x100,
	.fixup		= aries_fixup,
	.init_irq	= s5pv210_init_irq,
	.map_io		= aries_map_io,
	.init_machine	= aries_machine_init,
	.timer		= &s5p_systimer,
MACHINE_END

void s3c_setup_uart_cfg_gpio(unsigned char port)
{
	switch (port) {
	case 0:
		s3c_gpio_cfgpin(GPIO_BT_RXD, S3C_GPIO_SFN(GPIO_BT_RXD_AF));
		s3c_gpio_setpull(GPIO_BT_RXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_BT_TXD, S3C_GPIO_SFN(GPIO_BT_TXD_AF));
		s3c_gpio_setpull(GPIO_BT_TXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_BT_CTS, S3C_GPIO_SFN(GPIO_BT_CTS_AF));
		s3c_gpio_setpull(GPIO_BT_CTS, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_BT_RTS, S3C_GPIO_SFN(GPIO_BT_RTS_AF));
		s3c_gpio_setpull(GPIO_BT_RTS, S3C_GPIO_PULL_NONE);
		s3c_gpio_slp_cfgpin(GPIO_BT_RXD, S3C_GPIO_SLP_PREV);
		s3c_gpio_slp_setpull_updown(GPIO_BT_RXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_slp_cfgpin(GPIO_BT_TXD, S3C_GPIO_SLP_PREV);
		s3c_gpio_slp_setpull_updown(GPIO_BT_TXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_slp_cfgpin(GPIO_BT_CTS, S3C_GPIO_SLP_PREV);
		s3c_gpio_slp_setpull_updown(GPIO_BT_CTS, S3C_GPIO_PULL_NONE);
		s3c_gpio_slp_cfgpin(GPIO_BT_RTS, S3C_GPIO_SLP_PREV);
		s3c_gpio_slp_setpull_updown(GPIO_BT_RTS, S3C_GPIO_PULL_NONE);
		break;
	case 1:
		s3c_gpio_cfgpin(GPIO_GPS_RXD, S3C_GPIO_SFN(GPIO_GPS_RXD_AF));
		s3c_gpio_setpull(GPIO_GPS_RXD, S3C_GPIO_PULL_UP);
		s3c_gpio_cfgpin(GPIO_GPS_TXD, S3C_GPIO_SFN(GPIO_GPS_TXD_AF));
		s3c_gpio_setpull(GPIO_GPS_TXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_GPS_CTS, S3C_GPIO_SFN(GPIO_GPS_CTS_AF));
		s3c_gpio_setpull(GPIO_GPS_CTS, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_GPS_RTS, S3C_GPIO_SFN(GPIO_GPS_RTS_AF));
		s3c_gpio_setpull(GPIO_GPS_RTS, S3C_GPIO_PULL_NONE);
		break;
	case 2:
		s3c_gpio_cfgpin(GPIO_AP_RXD, S3C_GPIO_SFN(GPIO_AP_RXD_AF));
		s3c_gpio_setpull(GPIO_AP_RXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_AP_TXD, S3C_GPIO_SFN(GPIO_AP_TXD_AF));
		s3c_gpio_setpull(GPIO_AP_TXD, S3C_GPIO_PULL_NONE);
		break;
	case 3:
		s3c_gpio_cfgpin(GPIO_FLM_RXD, S3C_GPIO_SFN(GPIO_FLM_RXD_AF));
		s3c_gpio_setpull(GPIO_FLM_RXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_FLM_TXD, S3C_GPIO_SFN(GPIO_FLM_TXD_AF));
		s3c_gpio_setpull(GPIO_FLM_TXD, S3C_GPIO_PULL_NONE);
		break;
	default:
		break;
	}
}
EXPORT_SYMBOL(s3c_setup_uart_cfg_gpio);


#ifdef RECOVERY_SYSFS

struct class *recovery_class=NULL;
static atomic_t device_count;
static struct device *recovery_dev=NULL;
static char* recovery_name ="recovery";
static int recovery_index=0;

static ssize_t wipedata_read(struct device *dev, struct device_attribute *attr, char *buf)
{
        char cmdline[COMMAND_LINE_SIZE];
        char* pCmd = NULL;

    printk(KERN_INFO "%s\n", __func__);

    if(sec_get_param_value){
        sec_get_param_value(__CMDLINE, cmdline);

        printk("CMDLINE : %s\n",cmdline);
        //This is set from boot
        pCmd = strstr(cmdline, "wipedata");
        if( pCmd ){
            *pCmd = 0x00;
                 sec_set_param_value(__CMDLINE, cmdline);
                 return sprintf(buf, "%s", "wipedata");
        }

    }

        return sprintf(buf, "%s", "null");
}

static ssize_t wipedata_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
        char cmdline[COMMAND_LINE_SIZE];
        char* pCmd = NULL;
        int mode;

    printk(KERN_INFO "%s\n", __func__);

    if(strstr(buf, "null"))
        {
if(sec_get_param_value){
            sec_get_param_value(__CMDLINE, cmdline);

            printk("CMDLINE : %s\n",cmdline);

            //This is set from boot
            pCmd = strstr(cmdline, "wipedata");
            if( pCmd ){
                *pCmd = 0x00;
                    sec_set_param_value(__CMDLINE, cmdline);
            }
        }
        }
    else if(strstr(buf, "wipedata"))
        {
        if(sec_get_param_value){
            sec_get_param_value(__CMDLINE, cmdline);

            printk("CMDLINE : %s\n",cmdline);

            //to reboot as recovery mode to format partitions.
            strcat(cmdline, "wipedata");
                sec_set_param_value(__CMDLINE, cmdline);

                mode = REBOOT_MODE_RECOVERY;
            sec_set_param_value(__REBOOT_MODE, &mode);

        }
        }


        return size;
}

static ssize_t name_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    printk(KERN_INFO "%s\n", __func__);

        return sprintf(buf, "%s\n", recovery_name);
}

static DEVICE_ATTR(wipedata, S_IRUGO | S_IWUSR, wipedata_read, wipedata_write);
static DEVICE_ATTR(name, S_IRUGO | S_IWUSR, name_show, NULL);
static int create_recovery_class(void)
{
    printk(KERN_INFO "%s\n", __func__);

        if (!recovery_class) {
                recovery_class = class_create(THIS_MODULE, "recovery");
                if (IS_ERR(recovery_class))
                        return PTR_ERR(recovery_class);
                atomic_set(&device_count, 0);
        }

        return 0;
}

int recovery_dev_register(void)
{
        int ret;

    printk(KERN_INFO "%s\n", __func__);

        if (!recovery_class) {
                ret = create_recovery_class();
                if (ret < 0)
                        return ret;
        }

        recovery_index = atomic_inc_return(&device_count);
        recovery_dev = device_create(recovery_class, NULL,
                MKDEV(0, recovery_index), NULL, recovery_name);
        if (IS_ERR(recovery_dev))
                return PTR_ERR(recovery_dev);

        ret = device_create_file(recovery_dev, &dev_attr_wipedata);
        if (ret < 0)
                goto err_create_file_1;
        ret = device_create_file(recovery_dev, &dev_attr_name);
        if (ret < 0)
                goto err_create_file_2;

        return 0;

err_create_file_2:
device_remove_file(recovery_dev, &dev_attr_wipedata);
err_create_file_1:
        device_destroy(recovery_class, MKDEV(0, recovery_index));
        printk(KERN_ERR "recovery: Failed to register driver %s\n", recovery_name);

        return ret;
}
EXPORT_SYMBOL_GPL(recovery_dev_register);

void recovery_dev_unregister(struct device *dev)
{
    printk(KERN_INFO "%s\n", __func__);

        device_remove_file(dev, &dev_attr_name);
        device_remove_file(dev, &dev_attr_wipedata);
        device_destroy(recovery_class, MKDEV(0, recovery_index));
}
EXPORT_SYMBOL_GPL(recovery_dev_unregister);

static int __init recovery_class_init(void)
{
        return recovery_dev_register();
}

static void __exit recovery_class_exit(void)
{
    printk(KERN_INFO "%s\n", __func__);

        class_destroy(recovery_class);
}

#endif


