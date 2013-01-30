/*
 * s6e63m0 AMOLED Panel Driver for the Samsung Universal board
 *
 * Derived from drivers/video/omap/lcd-apollon.c
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <linux/wait.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/lcd.h>
#include <linux/backlight.h>
#include <linux/tl2796.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-fb.h>
#include <linux/earlysuspend.h>
 
#define SLEEPMSEC		0x1000
#define ENDDEF			0x2000
#define DEFMASK		0xFF00
#define DIM_BL	20
#define MIN_BL	30
#define MAX_BL	255
#define MAX_GAMMA_VALUE	24

extern struct class *sec_class;

/*********** for debug **********************************************************/
#if 1
#define gprintk(fmt, x... ) printk("%s(%d): " fmt, __FUNCTION__ , __LINE__ , ## x)
#else
#define gprintk(x...) do { } while (0)
#endif
/*******************************************************************************/

#ifdef CONFIG_FB_S3C_MDNIE
extern void init_mdnie_class(void);
#endif

#if defined(CONFIG_AEGIS_USCC)
#define SMART_DIMMING
#endif

#ifdef SMART_DIMMING
#include "s6e63m0_panel.h"
#include "smart_dimming_s6e63m0.h"

#define spidelay(nsecs)	do {} while (0)

#define MAX_GAMMA_LEVEL	25
#define GAMMA_PARAM_LEN	21

#define tl2796_ID2		0xa1
#define tl2796_ID2_2		0xa2
#define LDI_ID_REG		0xDA
#define LDI_ID_LEN		3
#define LDI_MTP_REG		0xD3
#define LDI_MTP_LEN		21

#define ELVSS_OFFSET_MIN	0x0D
#define ELVSS_OFFSET_1		0x09
#define ELVSS_OFFSET_2		0x07
#define ELVSS_OFFSET_MAX	0x00
#define ELVSS_LIMIT		0x29
#endif

struct s5p_lcd {
	int ldi_enable;
	int bl;
	int acl_enable;
	int cur_acl;
	int on_19gamma;
	const struct tl2796_gamma_adj_points *gamma_adj_points;
	struct mutex	lock;
	struct device *dev;
	struct spi_device *g_spi;
	struct s5p_panel_data	*data;
	struct backlight_device *bl_dev;
	struct lcd_device *lcd_dev;
	struct class *acl_class;
	struct device *switch_aclset_dev;
	struct class *gammaset_class;
	struct device *switch_gammaset_dev;
	struct device *sec_lcdtype_dev;
	struct early_suspend    early_suspend;

#ifdef SMART_DIMMING
	unsigned char			id[3];
	struct str_smart_dim		smart;
#endif
};

struct s5p_lcd *lcdpower; 

static int s6e63m0_spi_write_driver(struct s5p_lcd *lcd, u16 reg)
{
	u16 buf[1];
	int ret;
	struct spi_message msg;

	struct spi_transfer xfer = {
		.len	= 2,
		.tx_buf	= buf,
	};

	buf[0] = reg;

	spi_message_init(&msg);
	spi_message_add_tail(&xfer, &msg);

	ret = spi_sync(lcd->g_spi, &msg);

	if (ret < 0)
		pr_err("%s error\n", __func__);

	return ret ;
}

#ifdef SMART_DIMMING

static void s6e63m0_init_smart_dimming_table_22(struct s5p_lcd *lcd)
{
	unsigned int i, j;
	unsigned char gamma_22[GAMMA_PARAM_LEN] = {0,};

	for (i = 0; i < MAX_GAMMA_LEVEL; i++) {
		calc_gamma_table_22(&lcd->smart, candela_table[i], gamma_22);
		for (j = 0; j < GAMMA_PARAM_LEN; j++)
			s6e63m0_22gamma_table[i][j+2] = (gamma_22[j] | 0x100); // j+2 : for first value
	}
#if 0
	for (i = 0; i < MAX_GAMMA_LEVEL; i++) {
		for (j = 0; j < GAMMA_PARAM_LEN; j++)
			printk("0x%02x, ", s6e63m0_22gamma_table[i][j+1] & ~(0x100));
		printk("\n");
	}
#endif
}

static void s6e63m0_init_smart_dimming_table_19(struct s5p_lcd *lcd)
{
	unsigned int i, j;
	unsigned char gamma_19[GAMMA_PARAM_LEN] = {0,};

	for (i = 0; i < MAX_GAMMA_LEVEL; i++) {
		calc_gamma_table_19(&lcd->smart, candela_table[i], gamma_19);
		for (j = 0; j < GAMMA_PARAM_LEN; j++)
			s6e63m0_19gamma_table[i][j+2] = (gamma_19[j] | 0x100); // j+2 : for first value
	}
#if 0
	for (i = 0; i < MAX_GAMMA_LEVEL; i++) {
		for (j = 0; j < GAMMA_PARAM_LEN; j++)
			printk("0x%02x, ", s6e63m0_19gamma_table[i][j+1] & ~(0x100));
		printk("\n");
	}
#endif
}

static void s6e63m0_init_smart_elvss_table(struct s5p_lcd *lcd)
{
	unsigned int i, j;
	unsigned char elvss, b2;

	elvss = lcd->id[2] & (~(BIT(6) | BIT(7)));

	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			b2 = elvss + ((ELVSS_TABLE[i][j+1]) & ~(0x100));
			b2 = (b2 > ELVSS_LIMIT) ? ELVSS_LIMIT : b2;
			ELVSS_TABLE[i][j+1] = (b2 | 0x100);
		}
	}
#if 0
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++)
			printk("0x%02x, ", ELVSS_TABLE[i][j+1] & ~(0x100));
		printk("\n");
	}
#endif

}
static void s6e63m0_parallel_read(struct s5p_lcd *lcd, u8 cmd,
				 u8 *data, size_t len)
{
	int i;
	struct s5p_panel_data *pdata = lcd->data;
	int delay = 10;

	gpio_set_value(pdata->gpio_dcx, 0);
	udelay(delay);
	gpio_set_value(pdata->gpio_wrx, 0);
	for (i = 0; i < 8; i++)
		gpio_direction_output(pdata->gpio_db[i], (cmd >> i) & 1);

	udelay(delay);
	gpio_set_value(pdata->gpio_wrx, 1);
	udelay(delay);
	gpio_set_value(pdata->gpio_dcx, 1);
	for (i = 0; i < 8; i++)
		gpio_direction_input(pdata->gpio_db[i]);

	udelay(delay);
	gpio_set_value(pdata->gpio_rdx, 0);
	udelay(delay);
	gpio_set_value(pdata->gpio_rdx, 1);
	udelay(delay);

	while (len--) {
		u8 d = 0;
		gpio_set_value(pdata->gpio_rdx, 0);
		udelay(delay);
		for (i = 0; i < 8; i++)
			d |= gpio_get_value(pdata->gpio_db[i]) << i;
		*data++ = d;

		gpio_set_value(pdata->gpio_rdx, 1);
		udelay(delay);
	}
	gpio_set_value(pdata->gpio_rdx, 1);

}

static int s6e63m0_parallel_setup_gpios(struct s5p_lcd *lcd, bool init)
{
	int ret;
	struct s5p_panel_data *pdata = lcd->data;

	if (!pdata->configure_mtp_gpios)
		return -EINVAL;

	if (init) {
		ret = pdata->configure_mtp_gpios(pdata, true);
		if (ret)
			return ret;

		gpio_set_value(pdata->gpio_csx, 0);
		gpio_set_value(pdata->gpio_rdx, 1);
		gpio_set_value(pdata->gpio_wrx, 1);
		gpio_set_value(pdata->gpio_dcx, 0);
	} else {
		pdata->configure_mtp_gpios(pdata, false);
		gpio_set_value(pdata->gpio_csx, 0);
	}

	return 0;
}

static int s6e63m0_read_id(struct s5p_lcd *lcd, unsigned int addr)
{
	unsigned char buf[1] = {0};

	s6e63m0_parallel_read(lcd, addr, buf, 1);

	return *buf;
}

#endif
static int s6e63m0_panel_send_sequence(struct s5p_lcd *lcd,
	const u16 *wbuf)
{
	int i = 0;
	int ret = 0;

	while ((wbuf[i] & DEFMASK) != ENDDEF) {
		if ((wbuf[i] & DEFMASK) != SLEEPMSEC) {
			s6e63m0_spi_write_driver(lcd, wbuf[i]);
			i += 1;
		} else {
			msleep(wbuf[i+1]);
			i += 2;
		}
	}

	return ret;
}

static int get_gamma_value_from_bl(int bl)
{
	int gamma_value = 0;
	int gamma_val_x10 = 0;

	if (bl >= MIN_BL)		{
		gamma_val_x10 = 10*(MAX_GAMMA_VALUE-1)*bl/(MAX_BL-MIN_BL) + (10 - 10*(MAX_GAMMA_VALUE-1)*(MIN_BL)/(MAX_BL-MIN_BL)) ;
		gamma_value = (gamma_val_x10+5)/10;
	} else {
		gamma_value = 0;
	}

	return gamma_value;
}

#ifdef CONFIG_FB_S3C_TL2796_ACL
static void update_acl(struct s5p_lcd *lcd)
{
	struct s5p_panel_data *pdata = lcd->data;
	int gamma_value;

	gamma_value = get_gamma_value_from_bl(lcd->bl);

	if (lcd->acl_enable) {
		if ((lcd->cur_acl == 0) && (gamma_value != 1)) {
			s6e63m0_panel_send_sequence(lcd, pdata->acl_init);
			msleep(20);
		}

		switch (gamma_value) {
		case 0 ... 1:
			if (lcd->cur_acl != 0) {
				s6e63m0_panel_send_sequence(lcd, pdata->acl_table[0]);
				lcd->cur_acl = 0;
				gprintk(" ACL_cutoff_set Percentage : 0!!\n");
			}
			break;
		case 2:
			if (lcd->cur_acl != 20)	{
				s6e63m0_panel_send_sequence(lcd, pdata->acl_table[3]);
				lcd->cur_acl = 20;
				gprintk(" ACL_cutoff_set Percentage : 20!!\n");
			}
			break;
		case 3:
			if (lcd->cur_acl != 32)	{
				s6e63m0_panel_send_sequence(lcd, pdata->acl_table[6]);
				lcd->cur_acl = 32;
				gprintk(" ACL_cutoff_set Percentage : 32!!\n");
			}
			break;
		case 4 ... 23:
                        if (lcd->cur_acl != 40) {
				s6e63m0_panel_send_sequence(lcd, pdata->acl_table[9]);
				lcd->cur_acl = 40;
				gprintk(" ACL_cutoff_set Percentage : 40!!\n");
			}
			break;
		default:
			if (lcd->cur_acl != 50)	{
				s6e63m0_panel_send_sequence(lcd, pdata->acl_table[13]);
				lcd->cur_acl = 50;
				gprintk(" ACL_cutoff_set Percentage : 50!!\n");
			}
		}
	} else	{
		s6e63m0_panel_send_sequence(lcd, pdata->acl_table[0]);
		lcd->cur_acl  = 0;
		gprintk(" ACL_cutoff_set Percentage : 0!!\n");
	}

}
#endif
static int update_elvss(struct s5p_lcd *lcd, int level)
{	
	int ret = 0;
	struct s5p_panel_data *pdata = lcd->data;
	//printk("[AMOLED] s6e63m0_set_elvss..%d ~.\n",level);
	
	switch (level) 
	{	
		case 0 ... 4: /* 30cd ~ 100cd */		 
			ret = s6e63m0_panel_send_sequence(lcd ,pdata->elvss_table[0]);		
			break;	
		case 5 ... 10: /* 110cd ~ 160cd */		
			ret = s6e63m0_panel_send_sequence(lcd, pdata->elvss_table[1]);		
			break;
		case 11 ... 14: /* 170cd ~ 200cd */		
			ret = s6e63m0_panel_send_sequence(lcd, pdata->elvss_table[2]);		
			break;
		case 15 ... 24: /* 210cd ~ 300cd */		
			ret = s6e63m0_panel_send_sequence(lcd, pdata->elvss_table[3]);		
			break;	
		default:		
			break;
	}
	
	if (ret) {
		return -EIO;
	}
	return ret;

}

static void update_brightness(struct s5p_lcd *lcd)
{
	struct s5p_panel_data *pdata = lcd->data;
	int gamma_value;

	gamma_value = get_gamma_value_from_bl(lcd->bl);
	update_elvss(lcd ,gamma_value);

	printk("bl:%d, gamma:%d,on_19:%d\n", lcd->bl, gamma_value, lcd->on_19gamma);

#ifdef CONFIG_FB_S3C_TL2796_ACL
	update_acl(lcd);
#endif
	if (lcd->on_19gamma)
		s6e63m0_panel_send_sequence(lcd, pdata->gamma19_table[gamma_value]);
	else
		s6e63m0_panel_send_sequence(lcd, pdata->gamma22_table[gamma_value]);

	s6e63m0_panel_send_sequence(lcd, pdata->gamma_update);
}


static void tl2796_ldi_enable(struct s5p_lcd *lcd)
{
	struct s5p_panel_data *pdata = lcd->data;

	mutex_lock(&lcd->lock);

	s6e63m0_panel_send_sequence(lcd, pdata->seq_panel_set);
	s6e63m0_panel_send_sequence(lcd, pdata->seq_display_set);
	s6e63m0_panel_send_sequence(lcd, pdata->gamma180);
	s6e63m0_panel_send_sequence(lcd, pdata->seq_etc_set);

	s6e63m0_panel_send_sequence(lcd, pdata->standby_off);
	s6e63m0_panel_send_sequence(lcd, pdata->display_on);

	update_brightness(lcd);
	
	lcd->ldi_enable = 1;

	mutex_unlock(&lcd->lock);
}

static void tl2796_ldi_disable(struct s5p_lcd *lcd)
{
	struct s5p_panel_data *pdata = lcd->data;

	mutex_lock(&lcd->lock);

	lcd->ldi_enable = 0;
	s6e63m0_panel_send_sequence(lcd, pdata->standby_on);

	mutex_unlock(&lcd->lock);
}


static int s5p_lcd_set_power(struct lcd_device *ld, int power)
{
	struct s5p_lcd *lcd = lcd_get_data(ld);
	struct s5p_panel_data *pdata = lcd->data;

	printk(KERN_DEBUG "s5p_lcd_set_power is called: %d", power);

	if (power)
		s6e63m0_panel_send_sequence(lcd, pdata->display_on);
	else
		s6e63m0_panel_send_sequence(lcd, pdata->display_off);

	return 0;
}

static int s5p_lcd_check_fb(struct lcd_device *lcddev, struct fb_info *fi)
{
	return 0;
}

struct lcd_ops s5p_lcd_ops = {
	.set_power = s5p_lcd_set_power,
	.check_fb = s5p_lcd_check_fb,
};

static ssize_t gammaset_file_cmd_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct s5p_lcd *lcd = dev_get_drvdata(dev);

	gprintk("called %s\n", __func__);

	return sprintf(buf, "%u\n", lcd->bl);
}
static ssize_t gammaset_file_cmd_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct s5p_lcd *lcd = dev_get_drvdata(dev);
	int value;

	sscanf(buf, "%d", &value);

	if ((lcd->ldi_enable) && ((value == 0) || (value == 1))) {
		//printk("[gamma set] in gammaset_file_cmd_store, input value = %d\n", value);
		if (value != lcd->on_19gamma)	{
			lcd->on_19gamma = value;
			update_brightness(lcd);
		}
	}

	return size;
}

static DEVICE_ATTR(gammaset_file_cmd, 0664, gammaset_file_cmd_show, gammaset_file_cmd_store);

static ssize_t aclset_file_cmd_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct s5p_lcd *lcd = dev_get_drvdata(dev);
	gprintk("called %s\n", __func__);

	return sprintf(buf, "%u\n", lcd->acl_enable);
}
static ssize_t aclset_file_cmd_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	struct s5p_lcd *lcd = dev_get_drvdata(dev);
	int value;

	sscanf(buf, "%d", &value);

	if ((lcd->ldi_enable) && ((value == 0) || (value == 1))) {
		printk(KERN_INFO "[acl set] in aclset_file_cmd_store, input value = %d\n", value);
		if (lcd->acl_enable != value) {
			lcd->acl_enable = value;
#ifdef CONFIG_FB_S3C_TL2796_ACL
			update_acl(lcd);
#endif
		}
	}

	return size;
}

static DEVICE_ATTR(aclset_file_cmd, 0664, aclset_file_cmd_show, aclset_file_cmd_store);

static ssize_t lcdtype_file_cmd_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "SMD_AMS397GE03\n");
}

static ssize_t lcdtype_file_cmd_store(
        struct device *dev, struct device_attribute *attr,
        const char *buf, size_t size)
{
    return size;
}
static DEVICE_ATTR(lcdtype_file_cmd, 0664, lcdtype_file_cmd_show, lcdtype_file_cmd_store);

static int s5p_bl_update_status(struct backlight_device *bd)
{
	struct s5p_lcd *lcd = bl_get_data(bd);
	int bl = bd->props.brightness;

	pr_debug("\nupdate status brightness %d\n",
				bd->props.brightness);

	if (bl < 0 || bl > 255)
		return -EINVAL;

	mutex_lock(&lcd->lock);

	lcd->bl = bl;

	if (lcd->ldi_enable) {
		pr_debug("\n bl :%d\n", bl);
		update_brightness(lcd);
	}

	mutex_unlock(&lcd->lock);

	return 0;
}


static int s5p_bl_get_brightness(struct backlight_device *bd)
{
	struct s5p_lcd *lcd = bl_get_data(bd);

	printk(KERN_DEBUG "\n reading brightness\n");

	return lcd->bl;
}

const struct backlight_ops s5p_bl_ops = {
	.update_status = s5p_bl_update_status,
	.get_brightness = s5p_bl_get_brightness,
};

void tl2796_early_suspend(struct early_suspend *h)
{
	struct s5p_lcd *lcd = container_of(h, struct s5p_lcd,
								early_suspend);

	printk("%s\n",__FUNCTION__);
	tl2796_ldi_disable(lcd);

	return ;
}
void tl2796_late_resume(struct early_suspend *h)
{
	struct s5p_lcd *lcd = container_of(h, struct s5p_lcd,
								early_suspend);
	printk("%s\n",__FUNCTION__);
	tl2796_ldi_enable(lcd);

	return ;
}

static ssize_t lcd_on_off_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	struct s5p_panel_data *pdata = lcdpower->data;

	if (size < 1)
		return -EINVAL;

	if (strnicmp(buf, "on", 2) == 0 || strnicmp(buf, "1", 1) == 0)
		s6e63m0_panel_send_sequence(lcdpower, pdata->display_on);

	else if (strnicmp(buf, "off", 3) == 0 || strnicmp(buf, "0", 1) == 0)
		s6e63m0_panel_send_sequence(lcdpower, pdata->display_off);

	return size;
}

static DEVICE_ATTR(lcd_on_off, S_IRUGO | S_IWUSR, NULL, lcd_on_off_store);

static int __devinit tl2796_probe(struct spi_device *spi)
{
	struct s5p_lcd *lcd;
	int ret;
#ifdef SMART_DIMMING
	unsigned int i, j;
	u8 mtp_data[LDI_MTP_LEN] = {0,};

	u16 prepare_mtp_read[] = {
		/* LV2, LV3, MTP lock release code */
		0xf0, 0x15a, 0x15a,
		0xf1, 0x15a, 0x15a,
		0xfc, 0x15a, 0x15a,
		/* MTP cell enable */
		0xd1, 0x180,
		/* MPU  8bit read mode start */
		0xfc, 0x10c,0x100,

		ENDDEF, 0x0000
	};

#endif
	lcd = kzalloc(sizeof(struct s5p_lcd), GFP_KERNEL);
	if (!lcd) {
		pr_err("failed to allocate for lcd\n");
		ret = -ENOMEM;
		goto err_alloc;
	}
	mutex_init(&lcd->lock);

	spi->bits_per_word = 9;
	if (spi_setup(spi)) {
		pr_err("failed to setup spi\n");
		ret = -EINVAL;
		goto err_setup;
	}

	lcd->g_spi = spi;
	lcd->dev = &spi->dev;
	lcd->bl = 255;

	if (!spi->dev.platform_data) {
		dev_err(lcd->dev, "failed to get platform data\n");
		ret = -EINVAL;
		goto err_setup;
	}
	lcd->data = (struct s5p_panel_data *)spi->dev.platform_data;

	if (!lcd->data->gamma19_table || !lcd->data->gamma19_table ||
		!lcd->data->seq_display_set || !lcd->data->seq_etc_set ||
		!lcd->data->display_on || !lcd->data->display_off ||
		!lcd->data->standby_on || !lcd->data->standby_off ||
		!lcd->data->acl_init || !lcd->data->acl_table ||
		!lcd->data->gamma_update) {
		dev_err(lcd->dev, "Invalid platform data\n");
		ret = -EINVAL;
		goto err_setup;
	}

	lcd->bl_dev = backlight_device_register("s5p_bl",
			&spi->dev, lcd, &s5p_bl_ops, NULL);
	if (!lcd->bl_dev) {
		dev_err(lcd->dev, "failed to register backlight\n");
		ret = -EINVAL;
		goto err_setup;
	}

	lcd->bl_dev->props.max_brightness = 255;

	lcd->lcd_dev = lcd_device_register("s5p_lcd",
			&spi->dev, lcd, &s5p_lcd_ops);
	if (!lcd->lcd_dev) {
		dev_err(lcd->dev, "failed to register lcd\n");
		ret = -EINVAL;
		goto err_setup_lcd;
	}

	lcd->gammaset_class = class_create(THIS_MODULE, "gammaset");
	if (IS_ERR(lcd->gammaset_class))
		pr_err("Failed to create class(gammaset_class)!\n");

	lcd->switch_gammaset_dev = device_create(lcd->gammaset_class, &spi->dev, 0, lcd, "switch_gammaset");
	if (!lcd->switch_gammaset_dev) {
		dev_err(lcd->dev, "failed to register switch_gammaset_dev\n");
		ret = -EINVAL;
		goto err_setup_gammaset;
	}

	if (device_create_file(lcd->switch_gammaset_dev, &dev_attr_gammaset_file_cmd) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_gammaset_file_cmd.attr.name);

	lcd->acl_enable = 0;
	lcd->cur_acl = 0;
	lcd->ldi_enable = 1;

	lcd->acl_class = class_create(THIS_MODULE, "aclset");
	if (IS_ERR(lcd->acl_class))
		pr_err("Failed to create class(acl_class)!\n");

	lcd->switch_aclset_dev = device_create(lcd->acl_class, &spi->dev, 0, lcd, "switch_aclset");
	if (IS_ERR(lcd->switch_aclset_dev))
		pr_err("Failed to create device(switch_aclset_dev)!\n");

	if (device_create_file(lcd->switch_aclset_dev, &dev_attr_aclset_file_cmd) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_aclset_file_cmd.attr.name);

	 if (sec_class == NULL)
	 	sec_class = class_create(THIS_MODULE, "sec");
	 if (IS_ERR(sec_class))
                pr_err("Failed to create class(sec)!\n");

	 lcd->sec_lcdtype_dev = device_create(sec_class, NULL, 0, NULL, "sec_lcd");
	 if (IS_ERR(lcd->sec_lcdtype_dev))
	 	pr_err("Failed to create device(ts)!\n");

	 if (device_create_file(lcd->sec_lcdtype_dev, &dev_attr_lcdtype_file_cmd) < 0)
	 	pr_err("Failed to create device file(%s)!\n", dev_attr_lcdtype_file_cmd.attr.name);

	 if (device_create_file(lcd->sec_lcdtype_dev, &dev_attr_lcd_on_off) < 0)
	 	pr_err("Failed to create device file(%s)!\n", dev_attr_lcd_on_off.attr.name);

#ifdef CONFIG_FB_S3C_MDNIE
	init_mdnie_class();  //set mDNIe UI mode, Outdoormode
#endif

	spi_set_drvdata(spi, lcd);

//	tl2796_ldi_enable(lcd);
#ifdef CONFIG_HAS_EARLYSUSPEND
	lcd->early_suspend.suspend = tl2796_early_suspend;
	lcd->early_suspend.resume = tl2796_late_resume;
	lcd->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1;
	register_early_suspend(&lcd->early_suspend);
#endif

#ifdef SMART_DIMMING

	s6e63m0_panel_send_sequence(lcd, prepare_mtp_read);

	if (s6e63m0_parallel_setup_gpios(lcd, true)) {
		pr_err("%s: could not configure gpios\n", __func__);
		return 0;
	}

	for (i = 0; i < LDI_ID_LEN; i++) {
		lcd->id[i] = s6e63m0_read_id(lcd, LDI_ID_REG + i);
		lcd->smart.panelid[i] = lcd->id[i];
	}

	if ((lcd->id[1] == tl2796_ID2) || (lcd->id[1] == tl2796_ID2_2)) {
		s6e63m0_parallel_setup_gpios(lcd, false);
		tl2796_ldi_enable(lcd);
		return 0;
	}

	/* prepare initial data to operate smart dimming */

	printk(KERN_INFO "id: %x, %x, %x", lcd->id[0], lcd->id[1], lcd->id[2]);

	init_table_info_22(&lcd->smart);
	/* disable S/D for Camera Mode(1.9 gamma) */
	/* init_table_info_19(&lcd->smart); */

	s6e63m0_parallel_read(lcd, LDI_MTP_REG, mtp_data, LDI_MTP_LEN);

	s6e63m0_parallel_setup_gpios(lcd, false);

	calc_voltage_table(&lcd->smart, mtp_data);

#if 0
	for (i = 0; i < LDI_MTP_LEN ; i++)
		printk(KERN_INFO "%02x\n", mtp_data[i]);
#endif

	s6e63m0_init_smart_dimming_table_22(lcd);
	/* disable S/D for Camera Mode(1.9 gamma) */
	/* s6e63m0_init_smart_dimming_table_19(lcd); */
	s6e63m0_init_smart_elvss_table(lcd);

	lcd->data->elvss_table = (const unsigned short **)ELVSS_TABLE;
	/* disable S/D for Camera Mode(1.9 gamma) */
	/* lcd->data->gamma19_table = (const unsigned short **)s6e63m0_19gamma_table; */
	lcd->data->gamma22_table = (const unsigned short **)s6e63m0_22gamma_table;


#endif

#if 0
	for (i = 0; i < MAX_GAMMA_LEVEL; i++) {
		for (j = 0; j < GAMMA_PARAM_LEN; j++)
			printk("0x%02x, ", lcd->data->gamma22_table[i][j+1] & ~(0x100));
		printk("\n");
	}
#endif
	tl2796_ldi_enable(lcd);
	pr_info("tl2796_probe successfully proved\n");
	lcdpower = (struct s5p_lcd *) lcd; 
	return 0;
err_setup_gammaset:
	lcd_device_unregister(lcd->lcd_dev);

err_setup_lcd:
	backlight_device_unregister(lcd->bl_dev);

err_setup:
	mutex_destroy(&lcd->lock);
	kfree(lcd);

err_alloc:
	return ret;
}

static int __devexit tl2796_remove(struct spi_device *spi)
{
	struct s5p_lcd *lcd = spi_get_drvdata(spi);

	unregister_early_suspend(&lcd->early_suspend);

	backlight_device_unregister(lcd->bl_dev);

	tl2796_ldi_disable(lcd);

	kfree(lcd);

	return 0;
}

static struct spi_driver tl2796_driver = {
	.driver = {
		.name	= "tl2796",
		.owner	= THIS_MODULE,
	},
	.probe		= tl2796_probe,
	.remove		= __devexit_p(tl2796_remove),
};

static int __init tl2796_init(void)
{
	return spi_register_driver(&tl2796_driver);
}
static void __exit tl2796_exit(void)
{
	spi_unregister_driver(&tl2796_driver);
}

module_init(tl2796_init);
module_exit(tl2796_exit);


MODULE_AUTHOR("SAMSUNG");
MODULE_DESCRIPTION("s6e63m0 LDI driver");
MODULE_LICENSE("GPL");
