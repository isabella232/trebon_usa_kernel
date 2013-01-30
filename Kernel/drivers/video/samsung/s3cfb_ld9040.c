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
#include <linux/ld9040.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-fb.h>
#include <linux/earlysuspend.h>


#define SLEEPMSEC		0x1000
#define ENDDEF			0x2000
#define DEFMASK			0xFF00
#define DIM_BL	20
#define MIN_BL	30
#define MAX_BL	255
#define MAX_GAMMA_VALUE	24	// we have 25 levels. -> 16 levels -> 24 levels
#define ELVSS_GAMMA_VALUE1 4   //100CD
#define ELVSS_GAMMA_VALUE2 10  //160CD
#define ELVSS_GAMMA_VALUE3 14  //200CD
#define CRITICAL_BATTERY_LEVEL 5

#define M2_LCD_ID1      0xA1
#define M2_LCD_ID2      0x02
#define M2_LCD_ID3      0x11

#define SM2_LCD_ID1_A1      0xA1
#define SM2_LCD_ID1_A2      0xA2
#define SM2_LCD_ID2      0x12
#define SM2_LCD_ID3      0x11

typedef enum {
    LCD_PANEL_M2 = 1,
	LCD_PANEL_SM2_A1,
	LCD_PANEL_SM2_A2
}lcd_panel_type;

enum init_check{
	INIT_PRE = 0,
	INIT_PASS
};
extern unsigned int HWREV;

extern struct class *sec_class;

/*********** for debug **********************************************************/
#if 0
#define gprintk(fmt, x... ) printk("%s(%d): " fmt, __FUNCTION__ , __LINE__ , ## x)
#else
#define gprintk(x...) do { } while (0)
#endif
/*******************************************************************************/

#ifdef CONFIG_FB_S3C_MDNIE
extern void init_mdnie_class(void);
#endif

#define ACL_ENABLE

#if defined(CONFIG_TIKAL_USCC) || defined(CONFIG_TIKAL_MPCS)
#define SMART_DIMMING
#endif


#ifdef SMART_DIMMING
#include "ld9042_panel.h"
#include "smart_dimming_ld9042.h"

#define spidelay(nsecs)	do {} while (0)

#define MAX_GAMMA_LEVEL	25
#define GAMMA_PARAM_LEN	21

#define LD9040_ID3		0x11
#define LDI_ID_REG		0xDA
#define LDI_ID_LEN		3
#define LDI_MTP_REG		0xD6
#define LDI_MTP_LEN		18

#define ELVSS_OFFSET_MIN	0x0D
#define ELVSS_OFFSET_1	0x0C
#define ELVSS_OFFSET_2	0x09
#define ELVSS_OFFSET_MAX	0x00
#define ELVSS_LIMIT		0x29
#endif

struct s5p_lcd {
	int ldi_enable;
	int bl;
	int acl_enable;
	int cur_acl;
	int on_19gamma;
	int init_check;
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

struct s5p_lcd *lcd;
static unsigned int lcd_type;
 static int __init check_lcdtype(char *str)
{
        lcd_type = simple_strtol(str, NULL, 16);
        return 1;
}

 //2=A1line , 3=A2line
__setup("s3cfb_ld9040.lcd_type=", check_lcdtype);



static DEFINE_MUTEX(brightness_mutex);
int gamma_value = 0;

static int ld9040_spi_write_driver(struct s5p_lcd *lcd, u16 reg)
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
/* reading with 3-WIRE SPI with GPIO */
static inline void setcs(u8 is_on)
{
	gpio_set_value(GPIO_DISPLAY_CS, is_on);
}

static inline void setsck(u8 is_on)
{
	gpio_set_value(GPIO_DISPLAY_CLK, is_on);
}

static inline void setmosi(u8 is_on)
{
	gpio_set_value(GPIO_DISPLAY_SI, is_on);
}

static inline unsigned int getmiso(void)
{
	return !!gpio_get_value(GPIO_DISPLAY_SI);
}

static inline void setmosi2miso(u8 is_on)
{
	if (is_on)
		s3c_gpio_cfgpin(GPIO_DISPLAY_SI, S3C_GPIO_INPUT);
	else
		s3c_gpio_cfgpin(GPIO_DISPLAY_SI, S3C_GPIO_OUTPUT);
}

struct spi_ops ops = {
	.setcs = setcs,
	.setsck = setsck,
	.setmosi = setmosi,
	.setmosi2miso = setmosi2miso,
	.getmiso = getmiso,
};

static int ld9040_spi_read(struct s5p_lcd *lcd, unsigned int addr,
	unsigned char *buf, unsigned int len, unsigned int dummy_bit)
{
	struct s5p_panel_data *pdata = lcd->data;
	struct spi_ops *ops = pdata->ops;
	unsigned int bits;

	int i;
	int j;

	bits = lcd->g_spi->bits_per_word - 1;

	mutex_lock(&lcd->lock);

	ops->setcs(0);
	spidelay(0);

	for (j = bits; j >= 0; j--) {
		ops->setsck(0);
		spidelay(0);

		ops->setmosi((addr >> j) & 1);
		spidelay(0);

		ops->setsck(1);
		spidelay(0);
	}

	ops->setmosi2miso(1);	/* SDI as input */
	spidelay(0);

	for (j = 0; j < dummy_bit; j++) {
		ops->setsck(0);
		spidelay(0);

		ops->setsck(1);
		spidelay(0);

		ops->getmiso();
		spidelay(0);
	}

	for (i = 0; i < len; i++) {
		for (j = bits - 1; j >= 0; j--) {
			ops->setsck(0);
			spidelay(0);

			ops->setsck(1);
			spidelay(0);

			buf[i] |= (unsigned char)(ops->getmiso() << j);
			spidelay(0);
		}
		/* printk(KERN_INFO "0x%x, %d\n", buf[i], buf[i]); */
	}

	ops->setcs(1);
	spidelay(0);

	ops->setmosi2miso(0);	/* SDI as output */
	spidelay(0);

	mutex_unlock(&lcd->lock);

	return 0;
}

static int ld9040_read_id(struct s5p_lcd *lcd, unsigned int addr)
{
	unsigned char buf[1] = {0};

	ld9040_spi_read(lcd, addr, buf, 1, 8);

	return *buf;
}

static int spi_read_multi_byte(struct s5p_lcd *lcd,
	unsigned int addr, unsigned char *buf, unsigned int len)
{
	if (len == 1)
		ld9040_spi_read(lcd, addr, buf, len, 8);
	else
		ld9040_spi_read(lcd, addr, buf, len, 1);

	return 0;
}

static void ld9042_init_smart_dimming_table_22(struct s5p_lcd *lcd)
{
	unsigned int i, j;
	unsigned char gamma_22[GAMMA_PARAM_LEN] = {0,};

	for (i = 0; i < MAX_GAMMA_LEVEL; i++) {
		calc_gamma_table_22(&lcd->smart, candela_table[i], gamma_22);
		for (j = 0; j < GAMMA_PARAM_LEN; j++)
			ld9042_22gamma_table[i][j+1] = (gamma_22[j] | 0x100);
	}
#if 0
	for (i = 0; i < MAX_GAMMA_LEVEL; i++) {
		for (j = 0; j < GAMMA_PARAM_LEN; j++)
			printk("0x%02x, ", ld9042_22gamma_table[i][j+1] & ~(0x100));
		printk("\n");
	}
#endif
}

static void ld9042_init_smart_dimming_table_19(struct s5p_lcd *lcd)
{
	unsigned int i, j;
	unsigned char gamma_19[GAMMA_PARAM_LEN] = {0,};

	for (i = 0; i < MAX_GAMMA_LEVEL; i++) {
		calc_gamma_table_19(&lcd->smart, candela_table[i], gamma_19);
		for (j = 0; j < GAMMA_PARAM_LEN; j++)
			ld9042_19gamma_table[i][j+1] = (gamma_19[j] | 0x100);
	}
#if 0
	for (i = 0; i < MAX_GAMMA_LEVEL; i++) {
		for (j = 0; j < GAMMA_PARAM_LEN; j++)
			printk("0x%02x, ", ld9042_19gamma_table[i][j+1] & ~(0x100));
		printk("\n");
	}
#endif
}

static void ld9042_init_smart_elvss_table(struct s5p_lcd *lcd)
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
#endif

static void ld9040_panel_send_sequence(struct s5p_lcd *lcd,
	const u16 *seq)
{
	int i = 0;

	const u16 *wbuf;

	mutex_lock(&lcd->lock);

	wbuf = seq;

	while ((wbuf[i] & DEFMASK) != ENDDEF) {
		if ((wbuf[i] & DEFMASK) != SLEEPMSEC) {
			ld9040_spi_write_driver(lcd, wbuf[i]);
			i += 1;
		} else {
			msleep(wbuf[i+1]);
			i += 2;
		}
	}

	mutex_unlock(&lcd->lock);
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

#ifdef ACL_ENABLE
static void update_acl(struct s5p_lcd *lcd)
{
	struct s5p_panel_data *pdata = lcd->data;
	//int gamma_value;
	//gamma_value = get_gamma_value_from_bl(lcd->bl);

	if( lcd_type == LCD_PANEL_SM2_A2 ){
		if (lcd->acl_enable){
			if(lcd->cur_acl == 0)  {
				if (gamma_value ==0 || gamma_value ==1)
					dev_dbg(lcd->dev,"if bl_value is 0 or 1, acl_on skipped\n");
				else{
					ld9040_panel_send_sequence(lcd, pdata->acl_init);
					msleep(20);
				}
			}
 			switch (gamma_value) {
			case 0 ... 1:
				if (lcd->cur_acl != 0) {
					ld9040_panel_send_sequence(lcd, pdata->acl_table_sm2[0]);
					lcd->cur_acl = 0;
					printk(" ACL_cutoff_set Percentage : 0!!\n");
				}
				break;
			case 2 ... 19:
				if (lcd->cur_acl != 40)	{
					ld9040_panel_send_sequence(lcd, pdata->acl_table_sm2[1]);
					lcd->cur_acl = 40;
					printk(" ACL_cutoff_set Percentage : 40!! Gamma_Level : 2 !!\n");
				}
				break;
			default:
				if (lcd->cur_acl != 50)	{
					ld9040_panel_send_sequence(lcd, pdata->acl_table_sm2[6]);
					lcd->cur_acl = 50;
					printk(" ACL_cutoff_set Percentage : 50!!\n");
				}
			}
		}
		else{
			if(lcd->cur_acl != 0){
				ld9040_panel_send_sequence(lcd, pdata->acl_table_sm2[0]);
				lcd->cur_acl = 0;
				printk(" ACL_cutoff_OFF!!\n");
			}
		}
	} else {
		if (lcd->acl_enable) {
			if(lcd->cur_acl == 0)  {
				if (gamma_value ==0 || gamma_value ==1) {
					dev_dbg(lcd->dev,"if bl_value is 0 or 1, acl_on skipped\n");
				} else{
					ld9040_panel_send_sequence(lcd, pdata->acl_init);
					msleep(20);
				}
			}
			switch (gamma_value) {
			case 0 ... 1:
				if (lcd->cur_acl != 0) {
					ld9040_panel_send_sequence(lcd, pdata->acl_table[0]);
					lcd->cur_acl = 0;
					gprintk(" ACL_cutoff_set Percentage : 0!!\n");
				}
				break;
			case 2 ... 12:
				if (lcd->cur_acl != 40)	{
					ld9040_panel_send_sequence(lcd, pdata->acl_table[9]);
					lcd->cur_acl = 40;
					gprintk(" ACL_cutoff_set Percentage : 40!!\n");
				}
				break;
			case 13:
				if (lcd->cur_acl != 43)	{
					ld9040_panel_send_sequence(lcd, pdata->acl_table[16]);
					lcd->cur_acl = 43;
					gprintk(" ACL_cutoff_set Percentage : 43!!\n");
				}
				break;
			case 14:
				if (lcd->cur_acl != 45)	{
					ld9040_panel_send_sequence(lcd, pdata->acl_table[10]);
					lcd->cur_acl = 45;
					gprintk(" ACL_cutoff_set Percentage : 45!!\n");
				}
				break;
			case 15:
				if (lcd->cur_acl != 47)	{
					ld9040_panel_send_sequence(lcd, pdata->acl_table[11]);
					lcd->cur_acl = 47;
					gprintk(" ACL_cutoff_set Percentage : 47!!\n");
				}
				break;
			case 16:
				if (lcd->cur_acl != 48)	{
					ld9040_panel_send_sequence(lcd, pdata->acl_table[12]);
					lcd->cur_acl = 48;
					gprintk(" ACL_cutoff_set Percentage : 48!!\n");
				}
				break;
			default:
				if (lcd->cur_acl != 50)	{
					ld9040_panel_send_sequence(lcd, pdata->acl_table[13]);
					lcd->cur_acl = 50;
					gprintk(" ACL_cutoff_set Percentage : 50!!\n");
				}
			}
		} else	{
			ld9040_panel_send_sequence(lcd, pdata->acl_table[0]);
			lcd->cur_acl  = 0;
			gprintk(" ACL_cutoff_set Percentage : 0!!\n");
		}
	}

}
#endif

static void update_elvss(struct s5p_lcd *lcd, int elvss)
{
	struct s5p_panel_data *pdata = lcd->data;

	if (lcd_type == LCD_PANEL_SM2_A2) {
		if (elvss <= MAX_GAMMA_VALUE) {
			if (elvss <= ELVSS_GAMMA_VALUE1)
				ld9040_panel_send_sequence(lcd, pdata->elvss_table_sm2[0]);
			else if(elvss <= ELVSS_GAMMA_VALUE2)
				ld9040_panel_send_sequence(lcd, pdata->elvss_table_sm2[1]);
			else if(elvss <= ELVSS_GAMMA_VALUE3)
				ld9040_panel_send_sequence(lcd, pdata->elvss_table_sm2[2]);
			else
				ld9040_panel_send_sequence(lcd, pdata->elvss_table_sm2[3]);
		}
	} else if((lcd_type == LCD_PANEL_M2)) {
		if(elvss <= MAX_GAMMA_VALUE) {
			if(elvss <= ELVSS_GAMMA_VALUE1)
				ld9040_panel_send_sequence(lcd, pdata->elvss_table[0]);
			else if(elvss <= ELVSS_GAMMA_VALUE2)
				ld9040_panel_send_sequence(lcd, pdata->elvss_table[1]);
			else if(elvss <= ELVSS_GAMMA_VALUE3)
				ld9040_panel_send_sequence(lcd, pdata->elvss_table[2]);
			else
				ld9040_panel_send_sequence(lcd, pdata->elvss_table[3]);
		}
	}
}






static void update_brightness(struct s5p_lcd *lcd)
{

	struct s5p_panel_data *pdata = lcd->data;
	gamma_value = get_gamma_value_from_bl(lcd->bl);

	pr_info("%s : id=%d, brightness=%d, gammavalue=%d\n", __func__, lcd_type, lcd->bl, gamma_value);

	if (lcd_type == LCD_PANEL_SM2_A2) {
		update_elvss(lcd, gamma_value);
#ifdef ACL_ENABLE
		update_acl(lcd);
#endif
		ld9040_panel_send_sequence(lcd, pdata->gamma22_table_sm2[gamma_value]);
	} else if ((lcd_type == LCD_PANEL_M2)) {
		update_elvss(lcd, gamma_value);
#ifdef ACL_ENABLE
		update_acl(lcd);
#endif
		ld9040_panel_send_sequence(lcd, pdata->gamma22_table_hw8[gamma_value]);
		ld9040_panel_send_sequence(lcd, pdata->gamma_update);

	} else {
#ifdef ACL_ENABLE
		update_acl(lcd);
#endif
		ld9040_panel_send_sequence(lcd, pdata->gamma22_table[gamma_value]);
		ld9040_panel_send_sequence(lcd, pdata->gamma_update);
	}
}

unsigned int request_bl = 255;

static void ld9040_ldi_enable(struct s5p_lcd *lcd)
{
	struct s5p_panel_data *pdata = lcd->data;

	ld9040_panel_send_sequence(lcd, pdata->seq_etc_set);

	if (lcd_type == LCD_PANEL_SM2_A2) {
		ld9040_panel_send_sequence(lcd, pdata->seq_display_set_sm2);
		ld9040_panel_send_sequence(lcd, pdata->seq_panel_set_sm2);
	} else if((lcd_type == LCD_PANEL_M2) ||( lcd_type == LCD_PANEL_SM2_A1)){
		ld9040_panel_send_sequence(lcd, pdata->seq_display_set_hw8);
		ld9040_panel_send_sequence(lcd, pdata->seq_panel_set_hw8);
	}
	ld9040_panel_send_sequence(lcd, pdata->standby_off);
	msleep(160);

	if(lcd->init_check == INIT_PRE){
		lcd->init_check = INIT_PASS;
	}else{
		if (lcd_type == LCD_PANEL_SM2_A2) {
			ld9040_panel_send_sequence(lcd, pdata->elvss_set_sm2);
			ld9040_panel_send_sequence(lcd, pdata->elvss_level4_sm2);
			ld9040_panel_send_sequence(lcd, pdata->elvss_power_set_sm2);
			ld9040_panel_send_sequence(lcd, pdata->gamma210_sm2);
		} else if((lcd_type == LCD_PANEL_M2) ||(lcd_type == LCD_PANEL_SM2_A1)) {
			ld9040_panel_send_sequence(lcd, pdata->elvss_set_sm2);
			ld9040_panel_send_sequence(lcd, pdata->elvss_level4);
			ld9040_panel_send_sequence(lcd, pdata->elvss_power_set_hw8);
			ld9040_panel_send_sequence(lcd, pdata->gamma210_hw8);
		} else {
			ld9040_panel_send_sequence(lcd, pdata->elvss_set);
			ld9040_panel_send_sequence(lcd, pdata->gamma210);
		}

		ld9040_panel_send_sequence(lcd, pdata->display_on);

		update_brightness(lcd);
	}

	if(request_bl != lcd->bl) {
		lcd->bl = request_bl;
		update_brightness(lcd);
		request_bl = 0;
	}

	lcd->ldi_enable = 1;
}

static void ld9040_ldi_disable(struct s5p_lcd *lcd)
{
	struct s5p_panel_data *pdata = lcd->data;

	lcd->ldi_enable = 0;

	ld9040_panel_send_sequence(lcd, pdata->display_off);
	ld9040_panel_send_sequence(lcd, pdata->standby_on);

}


static int s5p_lcd_set_power(struct lcd_device *ld, int power)
{
	struct s5p_lcd *lcd = lcd_get_data(ld);
	struct s5p_panel_data *pdata = lcd->data;

	printk(KERN_DEBUG "s5p_lcd_set_power is called: %d", power);

	if (power)
		ld9040_panel_send_sequence(lcd, pdata->display_on);
	else
		ld9040_panel_send_sequence(lcd, pdata->display_off);

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
		printk("[gamma set] in gammaset_file_cmd_store, input value = %d\n", value);
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
#ifdef ACL_ENABLE
			update_acl(lcd);
#endif
		}
	}

	return size;
}

static DEVICE_ATTR(aclset_file_cmd, 0664, aclset_file_cmd_show, aclset_file_cmd_store);

static ssize_t lcdtype_file_cmd_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "SMD_AMS427GL10\n");
}

static ssize_t lcdtype_file_cmd_store(
        struct device *dev, struct device_attribute *attr,
        const char *buf, size_t size)
{
    return size;
}
static DEVICE_ATTR(lcdtype_file_cmd, 0664, lcdtype_file_cmd_show, lcdtype_file_cmd_store);

static ssize_t lcd_on_off_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	struct s5p_panel_data *pdata = lcd->data;

	if (size < 1)
		return -EINVAL;

	if (strnicmp(buf, "on", 2) == 0 || strnicmp(buf, "1", 1) == 0)
		ld9040_panel_send_sequence(lcd, pdata->display_on);

	else if (strnicmp(buf, "off", 3) == 0 || strnicmp(buf, "0", 1) == 0)
		ld9040_panel_send_sequence(lcd, pdata->display_off);

	return size;
}
static DEVICE_ATTR(lcd_on_off, 0664, NULL, lcd_on_off_store);

unsigned int setted_bl = 0;

static int s5p_bl_update_status(struct backlight_device *bd)
{
	struct s5p_lcd *lcd = bl_get_data(bd);
	int bl = bd->props.brightness;

	pr_debug("\nupdate status brightness %d\n",
				bd->props.brightness);

	if (bl < 0 || bl > 255)
		return -EINVAL;

	//printk("%s request br : %d\n",__func__,bl);

	if (lcd->ldi_enable) {
		lcd->bl = bl;
		pr_debug("\n bl :%d\n", bl);
		update_brightness(lcd);
	}

	else {
		msleep(2);
		request_bl = bl;
	}


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

void ld9040_early_suspend(struct early_suspend *h)
{
	struct s5p_lcd *lcd = container_of(h, struct s5p_lcd,
								early_suspend);

	ld9040_ldi_disable(lcd);

	return ;
}
void ld9040_late_resume(struct early_suspend *h)
{
	struct s5p_lcd *lcd = container_of(h, struct s5p_lcd,
								early_suspend);

	ld9040_ldi_enable(lcd);

	return ;
}

void ld9040_ldi_init(int on_off)
{
	if(on_off)
		ld9040_ldi_enable(lcd);
	else if(!on_off)
		ld9040_ldi_disable(lcd);
	return ;
}

static int __devinit ld9040_probe(struct spi_device *spi)
{
	int ret;
#ifdef SMART_DIMMING
	unsigned int i;
	u8 mtp_data[LDI_MTP_LEN] = {0,};
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
	if (!lcd->data) {
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
	lcd->init_check = INIT_PRE;

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

	printk("lcd_type= %d \n" , lcd_type);
	if(lcd_type == LCD_PANEL_M2)
		lcd_type = LCD_PANEL_SM2_A2;

	ld9040_ldi_enable(lcd);
#ifdef CONFIG_HAS_EARLYSUSPEND
	lcd->early_suspend.suspend = ld9040_early_suspend;
	lcd->early_suspend.resume = ld9040_late_resume;
	lcd->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1;
	register_early_suspend(&lcd->early_suspend);
#endif

#ifdef SMART_DIMMING
	lcd->data->ops = &ops;

	for (i = 0; i < LDI_ID_LEN; i++) {
		lcd->id[i] = ld9040_read_id(lcd, LDI_ID_REG + i);
		lcd->smart.panelid[i] = lcd->id[i];
	}

	if (lcd->id[2] == LD9040_ID3)
		return 0;

	/* prepare initial data to operate smart dimming */

	printk(KERN_INFO "id: %x, %x, %x", lcd->id[0], lcd->id[1], lcd->id[2]);

	init_table_info_22(&lcd->smart);
	init_table_info_19(&lcd->smart);

	spi_read_multi_byte(lcd, LDI_MTP_REG, mtp_data, LDI_MTP_LEN);

	calc_voltage_table(&lcd->smart, mtp_data);

#if 0
	for (i = 0; i < LDI_MTP_LEN; i++)
		printk(KERN_INFO "%d\n", mtp_data[i]);
#endif

	ld9042_init_smart_dimming_table_22(lcd);
	ld9042_init_smart_dimming_table_19(lcd);
	ld9042_init_smart_elvss_table(lcd);

	lcd->data->elvss_table = (const unsigned short **)ELVSS_TABLE;
	lcd->data->gamma19_table = (const unsigned short **)ld9042_19gamma_table;
	lcd->data->gamma22_table = (const unsigned short **)ld9042_22gamma_table;
#endif

	pr_info("ld9040_probe successfully proved\n");

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

static int __devexit ld9040_remove(struct spi_device *spi)
{
	struct s5p_lcd *lcd = spi_get_drvdata(spi);

	unregister_early_suspend(&lcd->early_suspend);

	backlight_device_unregister(lcd->bl_dev);

	ld9040_ldi_disable(lcd);

	kfree(lcd);

	return 0;
}

static struct spi_driver ld9040_driver = {
	.driver = {
		.name	= "ld9040",
		.owner	= THIS_MODULE,
	},
	.probe		= ld9040_probe,
	.remove		= __devexit_p(ld9040_remove),
};

static int __init ld9040_init(void)
{
	return spi_register_driver(&ld9040_driver);
}
static void __exit ld9040_exit(void)
{
	spi_unregister_driver(&ld9040_driver);
}

module_init(ld9040_init);
module_exit(ld9040_exit);


MODULE_AUTHOR("SAMSUNG");
MODULE_DESCRIPTION("ld9040 LCD Driver");
MODULE_LICENSE("GPL");
