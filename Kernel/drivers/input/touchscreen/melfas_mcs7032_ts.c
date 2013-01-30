/* drivers/input/touchscreen/melfas_ts_i2c_tsi.c
 *
 * Copyright (C) 2007 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/module.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/hrtimer.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/earlysuspend.h>
#include <mach/gpio.h>
#include <linux/jiffies.h>

#include <asm/io.h>
#include <linux/irq.h>
#include <mach/regs-gpio.h>
#include <plat/gpio-cfg.h>
#include <mach/hardware.h>
#include <asm-generic/gpio.h>
//#include <linux/dprintk.h>
#include "melfas_download.h"

#if CONFIG_MACH_CHIEF
#include <mach/gpio-chief.h>
#include <mach/gpio-chief-settings.h>
#else
#include <mach/forte/gpio-aries.h>
#endif

#include <linux/slab.h>
#include <linux/wakelock.h> 


#define CONFIG_TOUCHSCREEN_MELFAS_FIRMWARE_UPDATE

#define INPUT_INFO_REG 0x10
#define IRQ_TOUCH_INT   (IRQ_EINT_GROUP6_BASE+3)//MSM_GPIO_TO_INT(GPIO_TOUCH_INT)

#define FINGER_NUM	      5 //for multi touch
#define CONFIG_CPU_FREQ
#undef CONFIG_MOUSE_OPTJOY

#ifdef CONFIG_CPU_FREQ
//#include <plat/s3c64xx-dvfs.h>
#include <mach/cpu-freq-v210.h>

#endif 
#ifdef TSP_TEST_MODE
//static uint16_t tsp_test_reference[TS_MELFAS_SENSING_CHANNEL_NUM];
static uint16_t tsp_test_reference[TS_MELFAS_EXCITING_CHANNEL_NUM][TS_MELFAS_SENSING_CHANNEL_NUM];

static uint16_t tsp_test_inspection[TS_MELFAS_EXCITING_CHANNEL_NUM][TS_MELFAS_SENSING_CHANNEL_NUM];
static uint16_t tsp_raw_count[6];
uint8_t refer_y_channel_num = 1;
uint8_t inspec_y_channel_num = 1;
uint8_t refer_test_cnt = 0;
uint8_t inspec_test_cnt = 0;
static int status_ref = 0;
static int status_insp = 0;

#define MELFAS_ENTER_TEST_MODE(_mode_)          melfas_test_mode(_mode_)
#define MELFAS_EXIT_TEST_MODE()                 melfas_test_mode(0x00)
int touch_screen_ctrl_testmode(int cmd, touch_screen_testmode_info_t *test_info, int test_info_num);
static int ts_melfas_test_mode(int cmd, touch_screen_testmode_info_t *test_info, int test_info_num);

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

static uint16_t melfas_ts_inspection_spec_table[TS_MELFAS_SENSING_CHANNEL_NUM*TS_MELFAS_EXCITING_CHANNEL_NUM*2] =
{ 
	265, 301, 301, 301, 300, 300, 300, 301, 301, 300, 263, 441, 501, 502, 501, 500, 500, 500 ,501, 502 ,501, 439,
	244, 280, 280, 279, 279, 279, 279, 279, 279, 278, 241, 407, 467, 467, 466, 465, 465, 465, 465, 465, 464, 402,
	218, 254, 253, 253, 253, 253, 253, 253, 253, 253, 216, 363, 423, 422, 422, 422, 422, 422, 422, 422, 421, 360,
	217, 252, 252, 252, 252, 252, 252, 252, 252, 252, 216, 362, 421, 420, 420, 421, 420, 421, 421, 421, 420, 360,
	216, 251, 251, 251, 251, 251, 252, 251, 252, 251, 215, 360, 419, 419, 418, 419, 419, 419, 419, 420, 419, 359,
	216, 250, 250, 250, 251, 251, 251, 251, 251, 251, 215, 359, 417, 417, 417, 418, 418, 418, 418, 418, 418, 358,
	216, 248, 248, 249, 250, 250, 251, 251, 251, 251, 215, 359, 413, 414, 415, 417, 417, 418, 418, 418, 418, 358,
	220, 252, 251, 250, 250, 250, 249, 248, 246, 245, 209, 367, 420, 418, 417, 417, 417, 415, 413, 411, 408, 348,
	210, 247, 247, 248, 248, 248, 248, 247, 247, 246, 211, 351, 412, 412, 413, 414, 414, 413, 412, 411, 410, 351,
	210, 247, 247, 247, 248, 247, 247, 247, 246, 245, 211, 351, 412, 412, 412, 413, 412, 412, 411, 410, 409, 352,
	210, 246, 246, 246, 246, 246, 245, 245, 245, 244, 211, 350, 410, 410, 409, 410, 409, 409, 408, 408, 407, 352,
	210, 245, 245, 245, 245, 245, 245, 244, 244, 243, 211, 349, 409, 409, 408, 408, 408, 408, 407, 406, 405, 352,
	209, 245, 245, 245, 245, 245, 245, 244, 244, 243, 212, 349, 408, 408, 408, 409, 408, 408, 407, 406, 406, 353,
	210, 245, 246, 246, 247, 246, 246, 245, 245, 244, 213, 349, 409, 409, 411, 411, 411, 410, 409, 408, 407, 355,
	225, 261, 261, 261, 261, 258, 259, 259, 258, 258, 227, 375, 435, 435, 435, 435, 430, 432, 431, 430, 429, 379,
};

touch_screen_t touch_screen =
{
    {0},
    1,
    {0}
};

touch_screen_driver_t melfas_test_mode1 =
{
    {
        TS_MELFAS_VENDOR_NAME,
        TS_MELFAS_VENDOR_CHIP_NAME,
        TS_MELFAS_VENDOR_ID,
        0,
        0,
        TS_MELFAS_TESTMODE_MAX_INTENSITY,
        TS_MELFAS_SENSING_CHANNEL_NUM, TS_MELFAS_EXCITING_CHANNEL_NUM,
        2,
        2150, 2450,
        300,400,
        melfas_ts_inspection_spec_table
    },
    0,
    ts_melfas_test_mode
};

#endif
int get_version_failed = 0;

int melfas_ts_probe(void);
void init_hw_setting(void);
int __init melfas_ts_init(void);

static int debug_level = 5; 
#define debugprintk(level,x...)  if(debug_level>=level) printk(x)

extern int mcsdl_download_binary_data(void);//eunsuk test  [int hw_ver -> void]
extern int mms100_ISC_download_binary_data(void);//eunsuk test  [int hw_ver -> voidi]
extern int mms100_ISC_download_binary_file(void);
#ifdef CONFIG_MOUSE_OPTJOY
extern int get_sending_oj_event();
#endif

extern struct class *sec_class;

struct input_info {
	int max_x;
	int max_y;
	int state;
	int x;
	int y;
	int z;
	int width;
	int finger_id; 
};

struct melfas_ts_driver {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct work_struct  work;
	int irq;
	int hw_rev;
	int fw_ver;
	struct input_info info[FINGER_NUM];
	int suspended;
	struct early_suspend	early_suspend;
	struct wake_lock esdwake; 
};
struct melfas_ts_driver *melfas_ts = NULL;
struct i2c_driver melfas_ts_i2c;
struct workqueue_struct *melfas_ts_wq;

//static struct vreg *vreg_touch;
//static struct vreg *vreg_touchio;

#ifdef CONFIG_HAS_EARLYSUSPEND
void melfas_ts_early_suspend(struct early_suspend *h);
void melfas_ts_late_resume(struct early_suspend *h);
#endif	/* CONFIG_HAS_EARLYSUSPEND */
int melfas_ts_resume(void);
int melfas_ts_suspend(pm_message_t mesg);

int melfas_ts_resume_ESD(void);
int melfas_ts_suspend_ESD(pm_message_t mesg);
#define TOUCH_HOME	0//KEY_HOME
#define TOUCH_MENU	0//KEY_MENU
#define TOUCH_BACK	0//KEY_BACK
#define TOUCH_SEARCH  0//KEY_SEARCH

int melfas_ts_tk_keycode[] =
{ TOUCH_HOME, TOUCH_MENU, TOUCH_BACK, TOUCH_SEARCH, };

struct device *ts_dev;

void mcsdl_vdd_on(void)
{ 
  gpio_set_value(GPIO_TOUCH_EN,1);
  mdelay(25); //MUST wait for 25ms after vreg_enable() 
}

void mcsdl_vdd_off(void)
{
  gpio_set_value(GPIO_TOUCH_EN,0);
  mdelay(100); //MUST wait for 100ms before vreg_enable() 
}

static int melfas_i2c_read(struct i2c_client* p_client, u8 reg, u8* data, int len)
{

	struct i2c_msg msg;

	/* set start register for burst read */
	/* send separate i2c msg to give STOP signal after writing. */
	/* Continous start is not allowed for cypress touch sensor. */

	msg.addr = p_client->addr;
	msg.flags = 0;
	msg.len = 1;
	msg.buf = &reg;

	
	printk(KERN_ERR "[ %s ] addr [ %d ]\n", __func__, msg.addr);		// heatup - test - remove

	if (1 != i2c_transfer(p_client->adapter, &msg, 1))
	{
		printk("%s set data pointer fail! reg(%x)\n", __func__, reg);
		return -EIO;
	}

	/* begin to read from the starting address */

	msg.addr = p_client->addr;
	msg.flags = I2C_M_RD;
	msg.len = len;
	msg.buf = data;

	if (1 != i2c_transfer(p_client->adapter, &msg, 1))
	{
		printk("%s fail! reg(%x)\n", __func__, reg);
		return -EIO;
	}
	
	return 0;
}
static int melfas_i2c_write(struct i2c_client* p_client, u8* data, int len)
{
	struct i2c_msg msg;

	msg.addr = p_client->addr;
	msg.flags = 0; /* I2C_M_WR */
	msg.len = len;
	msg.buf = data ;

	if (1 != i2c_transfer(p_client->adapter, &msg, 1))
	{
		printk("%s set data pointer fail!\n", __func__);
		return -EIO;
	}

	return 0;
}


static void melfas_read_version(void)
{
	u8 buf[2] = {0,};
	
	if (0 == melfas_i2c_read(melfas_ts->client, MCSTS_MODULE_VER_REG, buf, 2))
	{
		get_version_failed  = 0;

		melfas_ts->hw_rev = buf[0];
		melfas_ts->fw_ver = buf[1];
		
		printk("%s :HW Ver : 0x%02x, FW Ver : 0x%02x\n", __func__, buf[0], buf[1]);
	}
	else
	{
		melfas_ts->hw_rev = 0;
		melfas_ts->fw_ver = 0;
		
		get_version_failed = 1;	
				
		printk("%s : Can't find HW Ver, FW ver!\n", __func__);
	}
}

static void melfas_read_resolution(void)
{
	
	uint16_t max_x=0, max_y=0;	

	u8 buf[3] = {0,};
	
	if(0 == melfas_i2c_read(melfas_ts->client, MCSTS_RESOL_HIGH_REG , buf, 3)){

		printk("%s :buf[0] : 0x%02x, buf[1] : 0x%02x, buf[2] : 0x%02x\n", __func__,buf[0],buf[1],buf[2]);

		if(buf[0] == 0){
			melfas_ts->info[0].max_x = 320;
			melfas_ts->info[0].max_y = 480;			
			
			printk("%s : Can't find Resolution!\n", __func__);
			}
		
		else{
			max_x = buf[1] | ((uint16_t)(buf[0] & 0x0f) << 8); 
			max_y = buf[2] | (((uint16_t)(buf[0] & 0xf0) >> 4) << 8); 
			melfas_ts->info[0].max_x = max_x;
			melfas_ts->info[0].max_y = max_y;

			printk("%s :max_x: %d, max_y: %d\n", __func__, melfas_ts->info[0].max_x, melfas_ts->info[0].max_y);
			}
		}

	else
	{
		melfas_ts->info[0].max_x = 320;
		melfas_ts->info[0].max_y = 480;
		
		printk("%s : Can't find Resolution!\n", __func__);
	}
}

static void melfas_firmware_download(void)
{

	int ret;

    
       printk("[F/W D/L] Entry mcsdl_download_binary_data\n");
	ret=mcsdl_download_binary_data(); //eunsuk test [melfas_ts->hw_rev -> ()]
    
       
    if(ret > 0)
    {
        if (melfas_ts->hw_rev < 0) 
        {
            printk(KERN_ERR "i2c_transfer failed\n");
        }

        if (melfas_ts->fw_ver < 0) 
        {
            printk(KERN_ERR "i2c_transfer failed\n");
        }


    }
    else 
    {
         printk("[TOUCH] Firmware update failed.. RESET!\n");
         mcsdl_vdd_off();
         mdelay(500);
         mcsdl_vdd_on();
         mdelay(200);
    }

}

static ssize_t registers_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int status, mode_ctl, hw_rev, fw_ver;
	
	status  = i2c_smbus_read_byte_data(melfas_ts->client, MCSTS_STATUS_REG);
	if (status < 0) {
		printk(KERN_ERR "i2c_smbus_read_byte_data failed\n");;
	}
	mode_ctl = i2c_smbus_read_byte_data(melfas_ts->client, MCSTS_MODE_CONTROL_REG);
	if (mode_ctl < 0) {
		printk(KERN_ERR "i2c_smbus_read_byte_data failed\n");;
	}
	hw_rev = i2c_smbus_read_byte_data(melfas_ts->client, MCSTS_MODULE_VER_REG);
	if (hw_rev < 0) {
		printk(KERN_ERR "i2c_smbus_read_byte_data failed\n");;
	}
	fw_ver = i2c_smbus_read_byte_data(melfas_ts->client, MCSTS_FIRMWARE_VER_REG);
	if (fw_ver < 0) {
		printk(KERN_ERR "i2c_smbus_read_byte_data failed\n");;
	}
	
	sprintf(buf, "[TOUCH] Melfas Tsp Register Info.\n");
	sprintf(buf, "%sRegister 0x00 (status)  : 0x%08x\n", buf, status);
	sprintf(buf, "%sRegister 0x01 (mode_ctl): 0x%08x\n", buf, mode_ctl);
	sprintf(buf, "%sRegister 0x30 (hw_rev)  : 0x%08x\n", buf, hw_rev);
	sprintf(buf, "%sRegister 0x31 (fw_ver)  : 0x%08x\n", buf, fw_ver);

	return sprintf(buf, "%s", buf);
}

static ssize_t registers_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	int ret;
	if(strncmp(buf, "RESET", 5) == 0 || strncmp(buf, "reset", 5) == 0) {
		
	    ret = i2c_smbus_write_byte_data(melfas_ts->client, 0x01, 0x01);
		if (ret < 0) {
			printk(KERN_ERR "i2c_smbus_write_byte_data failed\n");
		}
		printk("[TOUCH] software reset.\n");
	}
	return size;
}

static ssize_t gpio_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	sprintf(buf, "[TOUCH] Melfas Tsp Gpio Info.\n");
	sprintf(buf, "%sGPIO TOUCH_INT : %s\n", buf, gpio_get_value(GPIO_TOUCH_INT)? "HIGH":"LOW"); 
	sprintf(buf, "%sGPIO TOUCH_EN : %s\n", buf, gpio_get_value(GPIO_TOUCH_EN)? "HIGH":"LOW"); 
	sprintf(buf, "%sGPIO TOUCH_SCL : %s\n", buf, gpio_get_value(GPIO_TOUCH_I2C_SCL)? "HIGH":"LOW");     
	sprintf(buf, "%sGPIO TOUCH_SDA : %s\n", buf, gpio_get_value(GPIO_TOUCH_I2C_SDA)? "HIGH":"LOW"); 

	return sprintf(buf, "%s", buf);
}

static ssize_t gpio_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	if(strncmp(buf, "ON", 2) == 0 || strncmp(buf, "on", 2) == 0) {
    		mcsdl_vdd_on();
		//gpio_set_value(GPIO_TOUCH_EN, GPIO_LEVEL_HIGH);
		printk("[TOUCH] enable.\n");
		mdelay(200);
	}

	if(strncmp(buf, "OFF", 3) == 0 || strncmp(buf, "off", 3) == 0) {
    		mcsdl_vdd_off();
		printk("[TOUCH] disable.\n");
	}
	
	if(strncmp(buf, "RESET", 5) == 0 || strncmp(buf, "reset", 5) == 0) {
    		mcsdl_vdd_off();
		mdelay(500);
    		mcsdl_vdd_on();
		printk("[TOUCH] reset.\n");
		mdelay(200);
	}

	return size;
}


static ssize_t firmware_show(struct device *dev, struct device_attribute *attr, char *buf)
{	
	//melfas_read_version();

	sprintf(buf, "H/W rev. 0x%x F/W ver. 0x%x\n", melfas_ts->hw_rev, melfas_ts->fw_ver);
	return sprintf(buf, "%s", buf);
}


static ssize_t firmware_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
#ifdef CONFIG_TOUCHSCREEN_MELFAS_FIRMWARE_UPDATE	
	if(strncmp(buf, "UPDATE", 6) == 0 || strncmp(buf, "update", 6) == 0) 
    {
		printk("[TOUCH] Melfas  H/W version: 0x%02x.\n", melfas_ts->hw_rev);
		printk("[TOUCH] Current F/W version: 0x%02x.\n", melfas_ts->fw_ver);
//        if((melfas_ts->fw_ver <= 0x00) && (melfas_ts->hw_rev <= 0x00))
        {
            melfas_firmware_download();
        }
//        else
        {
//			printk("\nFirmware update error :: Check the your devices version.\n");
        }
	}
#endif

	return size;
}



static ssize_t debug_show(struct device *dev, struct device_attribute *attr, char *buf)
{	
	return sprintf(buf, "%d", debug_level);
}

static ssize_t debug_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	if(buf[0]>'0' && buf[0]<='9') {
		debug_level = buf[0] - '0';
	}

	return size;
}
#ifdef TSP_TEST_MODE

void touch_screen_sleep()
{
	melfas_ts_suspend(PMSG_SUSPEND);
}

void touch_screen_wakeup()
{
	gpio_set_value(GPIO_TSP_LDO_ON, 0);
	msleep(5);		
	//melfas_ts_resume(ts->client);
	gpio_set_value(GPIO_TSP_LDO_ON, 1);
	msleep(70); 
	enable_irq(melfas_ts->client->irq);
}

int touch_screen_get_tsp_info(touch_screen_info_t *tsp_info)
{
    int ret = 0;

    /* chipset independent */
    tsp_info->driver = touch_screen.tsp_info.driver;
    tsp_info->reference.bad_point = touch_screen.tsp_info.reference.bad_point;
    tsp_info->reference.table = touch_screen.tsp_info.reference.table;

    /* chipset dependent */
    /* melfas */
    tsp_info->inspection.bad_point = touch_screen.tsp_info.inspection.bad_point;
    tsp_info->inspection.table = touch_screen.tsp_info.inspection.table;
    return ret;
}


int touch_screen_ctrl_testmode(int cmd, touch_screen_testmode_info_t *test_info, int test_info_num)
{
    int ret = 0;
    bool prev_device_state = FALSE;
    touch_screen.driver = &melfas_test_mode1;
    touch_screen.tsp_info.driver = &(touch_screen.driver->ts_info);

    if (touch_screen.device_state == FALSE)
    {
        touch_screen_wakeup();
        touch_screen.device_state = TRUE;
        msleep(100);
        prev_device_state = TRUE;
    }
	if(test_info == NULL)
		return;
    switch (cmd)
    {
    case TOUCH_SCREEN_TESTMODE_SET_REFERENCE_SPEC_LOW:
    {
       // touch_screen.tsp_info.driver->reference_spec_low = test_info->reference;
       touch_screen.tsp_info.driver->reference_spec_low = 2150;
        break;
    }

    case TOUCH_SCREEN_TESTMODE_SET_REFERENCE_SPEC_HIGH:
    {
       //touch_screen.tsp_info.driver->reference_spec_high = test_info->reference;
		touch_screen.tsp_info.driver->reference_spec_high = 2450;
        break;
    }

    case TOUCH_SCREEN_TESTMODE_SET_INSPECTION_SPEC_LOW:
    {
        //touch_screen.tsp_info.driver->inspection_spec_low = test_info->reference;
        touch_screen.tsp_info.driver->inspection_spec_low = 300;
        break;
    }

    case TOUCH_SCREEN_TESTMODE_SET_INSPECTION_SPEC_HIGH:
    {
        //touch_screen.tsp_info.driver->inspection_spec_high = test_info->reference;
        touch_screen.tsp_info.driver->inspection_spec_low = 400;
        break;
    }

    case TOUCH_SCREEN_TESTMODE_RUN_SELF_TEST:
    {
        printk(KERN_DEBUG "START TOUCH_SCREEN_TESTMODE_RUN_SELF_TEST\n") ;
        int i;
        uint16_t reference;
        uint16_t inspection, inspection_spec_low, inspection_spec_high;
        uint16_t* inspection_spec_table = NULL;
        uint16_t reference_table_size, inspection_table_size;
        touch_screen_testmode_info_t* reference_table = NULL;
        touch_screen_testmode_info_t* inspection_table = NULL;

        reference_table_size = inspection_table_size = touch_screen.tsp_info.driver->x_channel_num * touch_screen.tsp_info.driver->y_channel_num;
        /* chipset independent check item */
        /* reference */
        if (touch_screen.tsp_info.reference.table == NULL)
        {
            touch_screen.tsp_info.reference.table = (touch_screen_testmode_info_t*)kzalloc(sizeof(touch_screen_testmode_info_t) * reference_table_size, GFP_KERNEL);
        }
        else
        {
            /* delete previous reference table */
            kzfree(touch_screen.tsp_info.reference.table);
            touch_screen.tsp_info.reference.table = NULL;
            touch_screen.tsp_info.reference.table = (touch_screen_testmode_info_t*)kzalloc(sizeof(touch_screen_testmode_info_t) * reference_table_size, GFP_KERNEL);
        }
        reference_table = touch_screen.tsp_info.reference.table;

        /* init reference info */
        test_info->bad_point = (uint16_t)0xfffe; // good sensor
        memset(reference_table, 0x00, sizeof(touch_screen_testmode_info_t) * reference_table_size);
        if (test_info != NULL)
        {
            touch_screen.driver->test_mode(TOUCH_SCREEN_TESTMODE_GET_REFERENCE, reference_table, reference_table_size);
			printk(KERN_INFO "reference_table_size = %d", reference_table_size);
			printk(KERN_INFO "touch_screen.tsp_info.driver->reference_spec_low = %d , touch_screen.tsp_info.driver->reference_spec_high = %d", touch_screen.tsp_info.driver->reference_spec_low, touch_screen.tsp_info.driver->reference_spec_high);
            for (i = 0; i < reference_table_size; i++)
            {
                reference = ((reference_table + i)->reference >> 8) & 0x00ff;
                reference |= (reference_table + i)->reference << 8;
				printk(KERN_INFO "reference = %d", reference);				
                if (reference < touch_screen.tsp_info.driver->reference_spec_low || reference > touch_screen.tsp_info.driver->reference_spec_high)
                {
                    /* bad sensor */
                    touch_screen.tsp_info.reference.bad_point = test_info->bad_point = (uint16_t)i;
					status_ref = 1;		 //fail			
                }
            }
            if (test_info->bad_point == 0xfffe)
            {
                touch_screen.tsp_info.reference.bad_point = test_info->bad_point;
                /* good sensor, we don't need to save reference table */
                //free( touch_screen.tsp_info.reference.table);
                //touch_screen.tsp_info.reference.table = NULL;
            }
        }
        else
        {
			status_ref = 1;    // fail
			ret = -1;
        }

        /* chipset dependent check item */
        /* melfas : inspection */
        if (touch_screen.tsp_info.driver->ven_id == 0x50 && test_info_num > 1)
        {
            if (touch_screen.tsp_info.inspection.table == NULL)
            {
                touch_screen.tsp_info.inspection.table = (touch_screen_testmode_info_t*)kzalloc(sizeof(touch_screen_testmode_info_t) * inspection_table_size, GFP_KERNEL);
            }
            else
            {
                /* delete previous reference table */
                kzfree(touch_screen.tsp_info.inspection.table);
                touch_screen.tsp_info.inspection.table = NULL;
                touch_screen.tsp_info.inspection.table = (touch_screen_testmode_info_t*)kzalloc(sizeof(touch_screen_testmode_info_t) * inspection_table_size, GFP_KERNEL);
            }

            inspection_table = touch_screen.tsp_info.inspection.table;
            inspection_spec_table =  touch_screen.tsp_info.driver->inspection_spec_table;

            /* init inspection info */
            (test_info + 1)->bad_point = (uint16_t)0xfffe; // good sensor
            memset(inspection_table, 0x00, sizeof(touch_screen_testmode_info_t) * inspection_table_size);

            if (test_info != NULL)
            {
                touch_screen.driver->test_mode(TOUCH_SCREEN_TESTMODE_GET_INSPECTION, inspection_table, inspection_table_size);
                printk(KERN_INFO "inspection_table_size = %d", inspection_table_size);
                for (i = 0; i < inspection_table_size; i++)
                {
                    inspection = ((((inspection_table + i)->inspection) & 0x0f) << 8);
                    inspection += ((((inspection_table + i)->inspection) >> 8) & 0xff);
					
                    inspection_spec_low = *(inspection_spec_table + i * 2);
                    inspection_spec_high = *(inspection_spec_table + i * 2 + 1);
					
					printk("VALUES- >> %3d: %5d [%5d] %5d:\n", i, inspection_spec_low, inspection, inspection_spec_high);					
                    if (inspection < inspection_spec_low  || inspection > inspection_spec_high)
                    {
                        /* bad sensor */
                        touch_screen.tsp_info.inspection.bad_point = (test_info + 1)->bad_point = (uint16_t)i;
						status_insp = 1;       //fail                
                    }
                }

                if ((test_info + 1)->bad_point == 0xfffe)
                {
                    touch_screen.tsp_info.inspection.bad_point = (test_info + 1)->bad_point;

                    /* good sensor, we don't need to save inspection table */
                    //free( touch_screen.tsp_info.inspection.table);
                    //touch_screen.tsp_info.inspection.table = NULL;
                }
            }
            else
            {
            	status_insp = 1;    //fail
                ret = -1;
            }
        }
        else
        {
            if (test_info_num > 1)
            {
                (test_info + 1)->bad_point = 0xfffe;
            }
        }

        break;
    }

    default:
    {
        if (test_info != NULL)
        {
            printk(KERN_DEBUG "DEFAULT\n");
            ret = touch_screen.driver->test_mode(cmd, test_info, test_info_num);
        }
        else
        {
            ret = -1;
        }
        break;
    }

    }

    if (prev_device_state == TRUE)
    {
        touch_screen_sleep();
    }

    return ret;
}


static int ts_melfas_test_mode(int cmd, touch_screen_testmode_info_t *test_info, int test_info_num)
{
    int i, ret = 0;
    uint8_t buf[TS_MELFAS_SENSING_CHANNEL_NUM*2];

    switch (cmd)
    {
    case TOUCH_SCREEN_TESTMODE_ENTER:
    {
        //melfas_test_mode1.ts_mode = TOUCH_SCREEN_TESTMODE;
        break;
    }

    case TOUCH_SCREEN_TESTMODE_EXIT:
    {
        //melfas_test_mode1.ts_state = TOUCH_SCREEN_NORMAL;
        break;
    }

    case TOUCH_SCREEN_TESTMODE_GET_OP_MODE:
    {
        break;
    }

    case TOUCH_SCREEN_TESTMODE_GET_THRESHOLD:
    {
        ret = melfas_i2c_read(melfas_ts->client, TS_MELFAS_TESTMODE_TSP_THRESHOLD_REG, buf, 1);
        test_info->threshold = buf[0];
        break;
    }

    case TOUCH_SCREEN_TESTMODE_GET_DELTA:
    {
        printk(KERN_DEBUG "DELTA\n");
#if 1
        buf[0] = TS_MELFAS_TESTMODE_CTRL_REG;
        buf[1] = 0x01;
        ret = melfas_i2c_write(melfas_ts->client, buf, 2);

        if (ret > 0)
        {
            melfas_test_mode1.ts_mode = TOUCH_SCREEN_TESTMODE;
            mdelay(50);
        }
        if (melfas_test_mode1.ts_mode == TOUCH_SCREEN_TESTMODE &&
            melfas_test_mode1.ts_info.delta_point_num == test_info_num)
        {
            for (i = 0; i < TS_MELFAS_TESTMODE_MAX_INTENSITY; i++)
            {
                ret |= melfas_i2c_read(melfas_ts->client, TS_MELFAS_TESTMODE_1ST_INTENSITY_REG + i, buf, 1);
                test_info[i].delta = buf[0];

                mdelay(20); // min 10ms, typical 50ms
            }
        }
        else
        {
            ret = -3;
        }
        buf[0] = TS_MELFAS_TESTMODE_CTRL_REG;
        buf[1] = 0x00;
        ret = melfas_i2c_write(melfas_ts->client, buf, 2);
        if (ret > 0)
        {
            melfas_test_mode1.ts_mode = TOUCH_SCREEN_NORMAL;
        }
#else
        melfas_test_mode.ts_mode = TOUCH_SCREEN_TESTMODE;

        if (melfas_test_mode.ts_mode == TOUCH_SCREEN_TESTMODE &&
            (melfas_test_mode.ts_info.x_channel_num * melfas_test_mode.ts_info.y_channel_num) == test_info_num)
        {

            buf[0] = TS_MELFAS_TESTMODE_CTRL_REG;
            buf[1] = 0x02;
            ret = melfas_mcs7000_i2c_write(ts->client, buf, 2);
            mdelay(500);
            mutex_lock(&melfas_ts->lock);

            for (i = 0; i < TS_MELFAS_EXCITING_CHANNEL_NUM; i++)
            {
                ret |= melfas_mcs7000_i2c_read(ts->client, TS_MELFAS_TESTMODE_1ST_INTENSITY_REG, buf, TS_MELFAS_SENSING_CHANNEL_NUM);
                memcpy(&(test_info[i*TS_MELFAS_SENSING_CHANNEL_NUM].delta), buf, TS_MELFAS_SENSING_CHANNEL_NUM);

            }

            buf[0] = TS_MELFAS_TESTMODE_CTRL_REG;
            buf[1] = 0x00;
            ret = melfas_mcs7000_i2c_write(ts->client, buf, 2);
            if (ret == 0)
            {
                melfas_test_mode.ts_mode = TOUCH_SCREEN_NORMAL;
                mdelay(50);
            }
        }
        else
        {
            ret = -3;
        }

        mutex_unlock(&melfas_ts->lock);
#endif
        break;
    }

    case TOUCH_SCREEN_TESTMODE_GET_REFERENCE:
    {
        printk(KERN_DEBUG "REFERENCE\n");
        melfas_test_mode1.ts_mode = TOUCH_SCREEN_TESTMODE;

        if (melfas_test_mode1.ts_mode == TOUCH_SCREEN_TESTMODE &&
            (melfas_test_mode1.ts_info.x_channel_num * melfas_test_mode1.ts_info.y_channel_num) == test_info_num)
        {
            buf[0] = TS_MELFAS_TESTMODE_CTRL_REG;
            buf[1] = 0x02;
            ret = melfas_i2c_write(melfas_ts->client, buf, 2);
            mdelay(500);
//            mutex_lock(&melfas_ts->lock);
            for (i = 0; i < TS_MELFAS_EXCITING_CHANNEL_NUM; i++)
            {
                ret |= melfas_i2c_read(melfas_ts->client, TS_MELFAS_TESTMODE_REFERENCE_DATA_START_REG, buf, TS_MELFAS_SENSING_CHANNEL_NUM * 2);
                //printk("REFERENCE RAW DATA : [%2x%2x]\n",buf[1],buf[0]);
                memcpy(&(test_info[i*TS_MELFAS_SENSING_CHANNEL_NUM].reference), buf, TS_MELFAS_SENSING_CHANNEL_NUM*2);

            }
            buf[0] = TS_MELFAS_TESTMODE_CTRL_REG;
            buf[1] = 0x00;
            ret = melfas_i2c_write(melfas_ts->client, buf, 2);
            if (ret > 0)
            {
                melfas_test_mode1.ts_mode = TOUCH_SCREEN_NORMAL;
                mdelay(50);
            }
        }
        else
        {
            ret = -3;
        }

        //mutex_unlock(&melfas_ts->lock);

        break;
    }

    case TOUCH_SCREEN_TESTMODE_GET_INSPECTION:
    {
        int j;

        melfas_test_mode1.ts_mode = TOUCH_SCREEN_TESTMODE;
        printk(KERN_DEBUG "INSPECTION\n");
		
        if (melfas_test_mode1.ts_mode == TOUCH_SCREEN_TESTMODE &&
            (melfas_test_mode1.ts_info.x_channel_num * melfas_test_mode1.ts_info.y_channel_num) == test_info_num)
        {
            buf[0] = TS_MELFAS_TESTMODE_INSPECTION_DATA_CTRL_REG;
            buf[1] = 0x1A;
            buf[2] = 0x0;
            buf[3] = 0x0;
            buf[4] = 0x0;
            buf[5] = 0x01;	// start flag
            ret = melfas_i2c_write(melfas_ts->client, buf, 6);
            mdelay(1000);
            //mutex_lock(&melfas_ts->lock);
			ret |= melfas_i2c_read(melfas_ts->client, TS_MELFAS_TESTMODE_INSPECTION_DATA_READ_REG, buf, 2); // dummy read
            for (j = 0; j < TS_MELFAS_EXCITING_CHANNEL_NUM; j++)
            {
                for (i = 0; i < TS_MELFAS_SENSING_CHANNEL_NUM; i++)
                {
                    buf[0] = TS_MELFAS_TESTMODE_INSPECTION_DATA_CTRL_REG;
                    buf[1] = 0x1A;
                    buf[2] = j;		// exciting ch
                    buf[3] = i;		// sensing ch
                    buf[4] = 0x0;		// reserved
                    buf[5] = 0x02;	// start flag, 2: output inspection data, 3: output low data
                    ret = melfas_i2c_write(melfas_ts->client, buf, 6);
                    ret |= melfas_i2c_read(melfas_ts->client, TS_MELFAS_TESTMODE_INSPECTION_DATA_READ_REG, buf, 2);
                    memcpy(&(test_info[i+(j*TS_MELFAS_SENSING_CHANNEL_NUM)].inspection), buf, 2);
                }
            }

            buf[0] = TS_MELFAS_TESTMODE_CTRL_REG;
            buf[1] = 0x00;
            ret = melfas_i2c_write(melfas_ts->client, buf, 2);
            if (ret > 0)
            {
                melfas_test_mode1.ts_mode = TOUCH_SCREEN_NORMAL;
                mdelay(50);
            }
            /*
            mcsdl_vdd_off();
            mcsdl_vdd_on();
             mdelay(100);

            melfas_test_mode.ts_mode = TOUCH_SCREEN_NORMAL;
            //mdelay(50);*/
        }
        else
        {
            ret = -3;
        }
        //mutex_unlock(&melfas_ts->lock);
		break;
    }

    default:
    {
        ret = -2;
        break;
    }
    }

	return ret;
}


static ssize_t tsp_test_reference_show(struct device *dev, struct device_attribute *attr, char *buf)
{
  
   int j,i,ref_value,k = 0, written_bytes = 0;    
   uint8_t buf1[2],buff[22];

	printk(KERN_DEBUG "Reference START %s\n", __func__) ;
	
	/* disable TSP_IRQ */
	disable_irq(melfas_ts->irq);
	
	for(k=0; k<=14; k++)  //15 columns
	{
		buf1[0] = 0xA1 ;		/* register address */			
		buf1[1] = 0x02 ;	
		buf1[2] = k;
		if (melfas_i2c_write(melfas_ts->client, buf1, 3) != 0)			
		{				
			printk(KERN_ERR "Failed to enter testmode\n") ;	
		}		

		  mdelay(20);


		if(melfas_i2c_read(melfas_ts->client, 0x50, buff, 22)!= 0)
		{
			printk(KERN_ERR "Failed to read(referece data)\n") ;
		}

		  mdelay(1);

		for ( i = 0; i <= 10; i++ )
		{	
			ref_value = (buff[2*i] << 8) | buff[2*i+1] ;
			tsp_test_reference[k][i] = ref_value;
			written_bytes += sprintf(buf+written_bytes, "%5d,", tsp_test_reference[k][i]) ;
		   // printk("k = %d, i = %d, val = %d, ", k,i,tsp_test_reference[k][i]) ;
		}
	}	
	written_bytes += sprintf(buf+written_bytes,"\b");
	printk(KERN_DEBUG "written_bytes  = %d\n", written_bytes) ;
	mcsdl_vdd_off();
	mdelay(50);
	mcsdl_vdd_on();
	mdelay(250);
	printk("[TOUCH] reset.\n");	
	/* enable TSP_IRQ */
	enable_irq(melfas_ts->client->irq);
	return written_bytes;
	
	
}
static ssize_t tsp_test_reference_store(struct device *dev, struct device_attribute *attr, char *buf, size_t size)
{
        
	unsigned int position;
	int ret;
			
	sscanf(buf,"%d\n",&position);
		
	if (position < 0 || position >= 15)
	{
		printk(KERN_DEBUG "Invalid values\n");
		return -EINVAL; 	   
	}
	refer_y_channel_num = (uint8_t)position;
    return size ;
}

static ssize_t tsp_test_inspection_show(struct device *dev, struct device_attribute *attr, char *buf)
{
   int i, j,k,ret,retry_n;
	unsigned char buf1[6],buf2[2];
	int written_bytes=0; 
	k = inspec_y_channel_num-1;
	printk(KERN_DEBUG "Inspection START %s,%d\n", __func__,inspec_y_channel_num) ;
	/* disable TSP_IRQ */	
	disable_irq(melfas_ts->client->irq);

	buf1[0] = 0xA0;
	buf1[1] = 0x1A;
	buf1[2] = 0x0;
	buf1[3] = 0x0;
	buf1[4] = 0x0;
	buf1[5] = 0x01;	// start flag
	ret = melfas_i2c_write(melfas_ts->client, buf1, 6);
	
	for (retry_n = 1000 ; retry_n ; retry_n--)
	{
		if (gpio_get_value(GPIO_TOUCH_INT) == 0)
		{
			//printk(KERN_DEBUG "TSP_INT low...OK1  delay= %d ms \n",1000-retry_n) ;
			break ; /* reference data were prepared */
		}
		msleep(1);
	}
	if (melfas_i2c_read(melfas_ts->client, 0xA1, buf2, 2) != 0)
	{
		printk(KERN_ERR "Failed to read(dummy value)\n") ;
	}
	for ( j = 0; j <= 14; j++ )
	{
		for ( i = 0; i <= 10; i++ )
		{	
			buf1[0] = 0xA0;
			buf1[1] = 0x1A;
			buf1[2] = j;		// exciting ch
			buf1[3] = i;		// sensing ch
			buf1[4] = 0x0;		// reserved
			buf1[5] = 0x02;	// start flag, 2: output inspection data, 3: output low data
			ret = melfas_i2c_write(melfas_ts->client, buf1, 6);

		#if 0
                     for (retry_n = 20 ; retry_n ; retry_n--)
		     {
				if (gpio_get_value(GPIO_TOUCH_INT) == 0)
				{
					//printk(KERN_DEBUG "TSP_INT low...OK2 delay= %d ms\n",20-retry_n) ;
					break ; /* reference data were prepared */
				}
					  udelay(800);
		     } 
		#endif

			if (melfas_i2c_read(melfas_ts->client, 0xA8, buf2, 2) != 0)
			{
				printk(KERN_ERR "Failed to read(inspection value)\n") ;
			}
			tsp_test_inspection[j][i]= ((buf2[0] << 8) | buf2[1]);
			if(inspec_y_channel_num == 140)
			{
				written_bytes += sprintf(buf+written_bytes, "%d,", tsp_test_inspection[j][i]) ;
				//printk("%d, ", tsp_test_inspection[j][i]) ;
			}
			else 
			{
			  written_bytes += sprintf(buf+written_bytes, "%5d,", tsp_test_inspection[j][i]) ;			  
			}
		}
			//printk(" \n") ;
	}
	written_bytes += sprintf(buf+written_bytes,"\b");
	printk(KERN_DEBUG "written_bytes = %d\n", written_bytes) ;
   
	mcsdl_vdd_off();
	msleep(50);
	mcsdl_vdd_on();
	msleep(250);
	printk("[TOUCH] reset.\n");

	/* enable TSP_IRQ */
	enable_irq(melfas_ts->client->irq);
    if(inspec_y_channel_num == 140)
    {
    	printk("[INSPECTION ALL]\n");
    	return written_bytes;
    }
    else
    {
		return written_bytes;
    }

}

static ssize_t tsp_test_inspection_store(struct device *dev, struct device_attribute *attr, char *buf, size_t size)
{
    unsigned int position;
	int ret;
	
	sscanf(buf,"%d\n",&position);
	if(position == 140)
	inspec_y_channel_num = (uint8_t)position;
	else {
	if (position < 0 || position >= 15) {
		printk(KERN_DEBUG "Invalid values\n");
		return -EINVAL; 	   
	}

	inspec_y_channel_num = (uint8_t)position;
	}
	return size;
}

/* Touch Reference ************************************************************/
static ssize_t tsp_channel_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    unsigned char   v,exciting_ch, sensing_ch ;

    printk(KERN_DEBUG "%s\n", __func__) ;
    /* sensing channel */
    if (melfas_i2c_read(melfas_ts->client, 0x3c, &v, 1) != 0)
    {
        printk(KERN_ERR "Failed to read(sensing_ch)\n") ;
    }
    sensing_ch = v & 0x1F ;

    /* exciting channel */
    if (melfas_i2c_read(melfas_ts->client, 0x3d, &v, 1) != 0)
    {
        printk(KERN_ERR "Failed to read(exciting_ch)\n") ;
    }
    exciting_ch = v & 0x1F ;
    printk(KERN_DEBUG "sensing_ch = %d, exciting_ch = %d\n", sensing_ch, exciting_ch) ;
	return sprintf(buf, "%d,%d\n",sensing_ch,exciting_ch);
}
/* Test Mode ******************************************************************/
static int  melfas_test_mode(uint8_t test_mode)
{
    uint8_t     buff[2] ;

    buff[0] = 0xA0 ;        /* register address */
    buff[1] = test_mode ;
    if (melfas_i2c_write(melfas_ts->client, buff, 2) != 0)
    {
        printk(KERN_ERR "can not enter the test(or normal) mode 0x%02x\n", test_mode) ;
        return -1 ;
    }

    return 0 ;
}

static ssize_t tsp_raw_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    unsigned char   buff[2], buf2[6];
	int i;
    printk(KERN_DEBUG "%s\n", __func__) ;
    
    /* disable TSP_IRQ */
    disable_irq(melfas_ts->client->irq);

    /* enter test mode 0x02 */
      
	if (MELFAS_ENTER_TEST_MODE(0x01) < 0)   
    {
         debugprintk(5, "TSP TEST MODE(02) Error!!!\n") ;
         return -1 ;
     }
     mdelay(100);
	 if (melfas_i2c_read(melfas_ts->client, 0x40, buf2, 5) != 0)
	 {
		printk(KERN_ERR "Failed to read(raw count)\n") ;
	 }
	 for(i=0; i<5; i++)
	 {
	 	tsp_raw_count[i] = ((buf2[i] << 8) | buf2[i+1]);
	 }
	 /* enable TSP_IRQ */
	enable_irq(melfas_ts->irq); 
	return sprintf(buf, "%5d %5d %5d %5d %5d \n", 
		tsp_raw_count[0], tsp_raw_count[1], tsp_raw_count[2], tsp_raw_count[3], tsp_raw_count[4]); // TODO:RECHECK with Platform App
}

static ssize_t tsp_raw_store(struct device *dev, struct device_attribute *attr, char *buf, size_t size)
{
	if(strncasecmp(buf, "start", 5) == 0)
	{
        /* disable TSP_IRQ */
        disable_irq(melfas_ts->irq);

        /* enter test mode 0x04 */
        if (MELFAS_ENTER_TEST_MODE(0x20) < 0)   /* Document erratum : not 0xB4 */
        {
            debugprintk(5, "TSP TEST MODE(0x04) Error!!!\n") ;
            return size ;
        }
	}
	else if(strncasecmp(buf, "stop", 4) == 0)
	{
        /* enter normal mode */
        if (MELFAS_EXIT_TEST_MODE() < 0)
        {
            debugprintk(5, "TSP TEST MODE(0x00) Error!!!\n") ;
            return size ;
        }

        mcsdl_vdd_off();
        mdelay(500);
        mcsdl_vdd_on();
        printk("[TOUCH] reset.\n");
        mdelay(200);

        /* enable TSP_IRQ */
        enable_irq(melfas_ts->irq);
	}
    else
    {
        debugprintk(5, "TSP Error Unknwon commad!!!\n") ;
    }

    return size ;
}

static ssize_t tsp_name_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "M e l f a s , M M S 1 0 0\n");
}

static ssize_t tsp_test_show(struct device *dev, struct device_attribute *attr, char *buf)
{

   return sprintf(buf, "%u\n", status_ref);  
}

static ssize_t tsp_test_store(struct device *dev, struct device_attribute *attr, char *buf, size_t size)
{
    touch_screen.driver = &melfas_test_mode1;
    touch_screen.tsp_info.driver = &(touch_screen.driver->ts_info);
	status_ref = 0;
	status_insp = 0;
    if (strncmp(buf, "self", 4) == 0)
    {
        /* disable TSP_IRQ */
        printk(KERN_DEBUG "START %s\n", __func__) ;
        disable_irq(melfas_ts->client->irq);
        touch_screen_info_t tsp_info = {0};
        touch_screen_get_tsp_info(&tsp_info);

        uint16_t reference_table_size;
        touch_screen_testmode_info_t* reference_table = NULL;
        reference_table_size = tsp_info.driver->x_channel_num * tsp_info.driver->y_channel_num;
        reference_table = (touch_screen_testmode_info_t*)kzalloc(sizeof(touch_screen_testmode_info_t) * reference_table_size, GFP_KERNEL);
        touch_screen_ctrl_testmode(TOUCH_SCREEN_TESTMODE_RUN_SELF_TEST, reference_table, reference_table_size);

        mcsdl_vdd_off();
        mdelay(500);
        mcsdl_vdd_on();
        printk("[TOUCH] reset.\n");
        mdelay(200);

        /* enable TSP_IRQ */
        enable_irq(melfas_ts->client->irq);
    }
    else
    {
        printk("TSP Error Unknwon commad!!!\n");
    }

    return size ;
}


static DEVICE_ATTR(tsp_reference, 0664, tsp_test_reference_show, tsp_test_reference_store) ;
static DEVICE_ATTR(tsp_inspection, 0664, tsp_test_inspection_show, tsp_test_inspection_store) ;
static DEVICE_ATTR(tsp_channel, 0664, tsp_channel_show, NULL);
static DEVICE_ATTR(tsp_raw, 0664, tsp_raw_show, tsp_raw_store);
static DEVICE_ATTR(tsp_name, 0664, tsp_name_show, NULL);
static DEVICE_ATTR(tsp_test, 0664, tsp_test_show, tsp_test_store) ;
#endif


static DEVICE_ATTR(gpio, S_IRUGO | S_IWUSR, gpio_show, gpio_store);
static DEVICE_ATTR(registers, S_IRUGO | S_IWUSR, registers_show, registers_store);
static DEVICE_ATTR(firmware, S_IRUGO | S_IWUSR, firmware_show, firmware_store);
static DEVICE_ATTR(debug, 0664/*S_IRUGO | S_IWUGO*/, debug_show, debug_store);


void melfas_ts_work_func(struct work_struct *work)
{
  int ret;
  int ret1;
  int i = 0;
  u8 id = 0;
  uint8_t start_reg;
  uint8_t buf1[6]; // 8-> 6 melfas recommand

  struct i2c_msg msg[2];

  printk(KERN_ERR "==!!== melfas_ts_work_func \n");		// heatup - test
  
  msg[0].addr = melfas_ts->client->addr;
  msg[0].flags = 0; 
  msg[0].len = 1;
  msg[0].buf = &start_reg;
  start_reg = MCSTS_INPUT_INFO_REG;
  msg[1].addr = melfas_ts->client->addr;
  msg[1].flags = I2C_M_RD; 
  msg[1].len = sizeof(buf1);
  msg[1].buf = buf1;
  

  ret  = i2c_transfer(melfas_ts->client->adapter, &msg[0], 1);
  ret1 = i2c_transfer(melfas_ts->client->adapter, &msg[1], 1);
  
  if((ret < 0) ||  (ret1 < 0)) 
  {
  	printk(KERN_ERR "==melfas_ts_work_func: i2c_transfer failed!!== ret:%d ,ret1:%d\n",ret,ret1);
	
  }
  else
  {    
    int x = buf1[2] | ((uint16_t)(buf1[1] & 0x0f) << 8); 
    int y = buf1[3] | (((uint16_t)(buf1[1] & 0xf0) >> 4) << 8); 
    int z = buf1[4];
    int finger = buf1[0] & 0x0f;  //Touch Point ID
	
/*LnT : added for ESD */	
 if(finger == 0x0f) { // ESD Detection
   printk("\n[TSP]************************* MELFAS_ESD Detection ***********************\n");
   wake_lock(&melfas_ts->esdwake);
   melfas_ts_suspend_ESD(PMSG_SUSPEND);
   mcsdl_vdd_off();
   msleep(100);
   mcsdl_vdd_on();
   melfas_ts_resume_ESD();
   wake_unlock(&melfas_ts->esdwake);
  }
/* LnT end */	
	 int touchaction = (int)((buf1[0] >> 4) & 0x3); //Touch action
#ifdef CONFIG_CPU_FREQ
//    set_dvfs_perf_level();
#endif
	finger = finger -1; // melfas touch  started  touch finger id from the  index 1
	id = finger; // android input id : 0~ 

      switch(touchaction) {
        case 0x0: // Non-touched state (Rlease Event)
        
			//melfas_ts->info[id].x = -1;
			//melfas_ts->info[id].y = -1;
			//melfas_ts->info[id].z = -1;
			//melfas_ts->info[id].finger_id = finger; 
			//z = 0;
//			debugprintk(5," TOUCH RELEASE\n");
            if(melfas_ts->info[id].z == -1)
            {
                enable_irq(melfas_ts->irq);
                return;
            }
			melfas_ts->info[id].x = x;
			melfas_ts->info[id].y = y;
			melfas_ts->info[id].z = 0;
			melfas_ts->info[id].finger_id = finger;
			z = 0;
//			s5pc110_unlock_dvfs_high_level(DVFS_LOCK_TOKEN_4);
			
          break;

        case 0x1: //touched state (Press Event)
            if(melfas_ts->info[id].x == x
            && melfas_ts->info[id].y == y
            && melfas_ts->info[id].z == z
            && melfas_ts->info[id].finger_id == finger)
            {
                enable_irq(melfas_ts->irq);
                return;
            }
            
			melfas_ts->info[id].x = x;
			melfas_ts->info[id].y = y;
			melfas_ts->info[id].z = z;
			melfas_ts->info[id].finger_id = finger; 
//			if(melfas_ts->info[id].z == 0) // generated duplicate event 
//				return;
//			s5pc110_lock_dvfs_high_level(DVFS_LOCK_TOKEN_4,0);
//			 debugprintk(5," TOUCH PRESS\n");		 
          break;

        case 0x2: 

          break;

        case 0x3: // Palm Touch
          printk(KERN_DEBUG "[TOUCH] Palm Touch!\n");
          break;

        case 0x7: // Proximity
          printk(KERN_DEBUG "[TOUCH] Proximity!\n");
          break;
      }
	  
      melfas_ts->info[id].state = touchaction;
//		if(touchaction) // press
//			melfas_ts->info[id].z = z;
//		else // release
//			melfas_ts->info[id].z = 0;
//		if(touchaction == 0x1 && (melfas_ts->info[id].z == 0))// duplicate touch work
//			return;
//		if(nPreID >= id || bChangeUpDn)
		if(((touchaction == 1)&&(z!=0))||((touchaction == 0)&&(z==0)))
		{
	//		if(((touchaction == 1)&&(z!=0)))
		//		s5pc110_lock_dvfs_high_level(DVFS_LOCK_TOKEN_4,0);
	//		else
		//		s5pc110_unlock_dvfs_high_level(DVFS_LOCK_TOKEN_4);
			
		  	for ( i= 0; i<FINGER_NUM; ++i ) 
			{
				if(melfas_ts->info[i].z== -1) continue;
				if(melfas_ts->info[i].x > 318) continue;
				input_report_abs(melfas_ts->input_dev, ABS_MT_POSITION_X, melfas_ts->info[i].x);
				input_report_abs(melfas_ts->input_dev, ABS_MT_POSITION_Y, melfas_ts->info[i].y);
				input_report_abs(melfas_ts->input_dev, ABS_MT_TOUCH_MAJOR, melfas_ts->info[i].z ? 40 : 0);		
//				input_report_abs(melfas_ts->input_dev, ABS_MT_TRACKING_ID, melfas_ts->info[i].finger_id);
				input_report_abs(melfas_ts->input_dev, ABS_MT_WIDTH_MAJOR,melfas_ts->info[i].finger_id * 0xff +  5);		
				input_mt_sync(melfas_ts->input_dev);
//				debugprintk(5,"[TOUCH_MT] x1: %4d, y1: %4d, z1: %4d, finger: %4d, i=[%d] \n", x, y,(melfas_ts->info[id].z), finger,i);			
	//			input_sync(melfas_ts->input_dev);
				if(melfas_ts->info[i].z == 0)
					melfas_ts->info[i].z = -1;
			}
    		input_sync(melfas_ts->input_dev);	
	 }
//		PreState = touchaction;
  }

 // if(readl(gpio_pend_mask_mem)&INT_BIT_MASK)
//		 writel(readl(gpio_pend_mask_mem)|INT_BIT_MASK, gpio_pend_mask_mem);
  
//  s3c_gpio_cfgpin(GPIO_TOUCH_INT, S3C_GPIO_SFN(0xf));

  enable_irq(melfas_ts->irq);
}


irqreturn_t melfas_ts_irq_handler(int irq, void *dev_id)
{
	disable_irq_nosync(melfas_ts->irq);
	queue_work(melfas_ts_wq, &melfas_ts->work);
	printk("MELFAS TS IRQ[%d] HANDLER! /n",melfas_ts->irq);
	return IRQ_HANDLED;
}

//#define HW_VERSION  0x10
//#define FW_VERSION  0x04
#define HW_VERSION  0x51
#define FW_VERSION  0x25


int melfas_ts_probe(void)
{
	int ret = 0;
	uint16_t max_x=0, max_y=0;

	printk("\n====================================================");
	printk("\n=======         [TOUCH SCREEN] PROBE       =========");
	printk("\n====================================================\n");

	readl(S5P_VA_GPIO + 0x00c8);
	writel(0xffffffff, S5P_VA_GPIO + 0x00cc);

	if (!i2c_check_functionality(melfas_ts->client->adapter, I2C_FUNC_I2C/*I2C_FUNC_I2C*//*I2C_FUNC_SMBUS_BYTE_DATA*/)) {
		printk(KERN_ERR "melfas_ts_probe: need I2C_FUNC_I2C\n");
		ret = -ENODEV;
		goto err_check_functionality_failed;
	}

	INIT_WORK(&melfas_ts->work, melfas_ts_work_func);
	wake_lock_init(&melfas_ts->esdwake,WAKE_LOCK_SUSPEND,"melfas_ts_esdwake"); 

	melfas_read_version();
	//	if( (melfas_ts->hw_rev == HW_VERSION) && (melfas_ts->fw_ver < FW_VERSION)||get_version_failed)
	//if( ((melfas_ts->hw_rev != HW_VERSION) && (melfas_ts->fw_ver != FW_VERSION))||(get_version_failed))
	if( (melfas_ts->fw_ver != FW_VERSION)||(get_version_failed))
	{ 	 
       	printk(KERN_ERR "melfas_ts_probe: mcsdl_download_binary_data\n");

		ret = mcsdl_download_binary_data();
		melfas_read_version(); 
	}
	else
	{
		MCSDL_VDD_SET_LOW();
		msleep(400);      // Delay for Stable VDD
		MCSDL_VDD_SET_HIGH();
	}	

	if(ret < 0)
	{
		printk(KERN_ERR "SET Download Fail - errro code [%d] \n",ret);
	}


	printk(KERN_INFO "[TOUCH] Melfas  H/W version: 0x%02x.\n", melfas_ts->hw_rev);
	printk(KERN_INFO "[TOUCH] Current F/W version: 0x%02x.\n", melfas_ts->fw_ver);

	if((melfas_ts->fw_ver == 0x00) || (melfas_ts->hw_rev == 0x00))
	{    
		printk("FIRMWARE DOWNLOAD START! \n");
		//	melfas_firmware_download();
	}

	else
	{
		printk("Firmware update fail or upper firmware! \n");
	}


	//	melfas_read_resolution();
	max_x = 320;//melfas_ts->info[0].max_x ;
	max_y = 480;//melfas_ts->info[0].max_y ;
	printk("melfas_ts_probe: max_x: %d, max_y: %d\n", max_x, max_y);

	melfas_ts->input_dev = input_allocate_device();
	if (melfas_ts->input_dev == NULL) {
		ret = -ENOMEM;
		printk(KERN_ERR "melfas_ts_probe: Failed to allocate input device\n");
		goto err_input_dev_alloc_failed;
	}

	melfas_ts->input_dev->name = "melfas_ts_input";
#if 1
	set_bit(EV_SYN, melfas_ts->input_dev->evbit);
	set_bit(EV_KEY, melfas_ts->input_dev->evbit);
	set_bit(BTN_TOUCH, melfas_ts->input_dev->keybit);
	set_bit(EV_ABS, melfas_ts->input_dev->evbit);
#else
	set_bit(EV_SYN, melfas_ts->input_dev->evbit);
	set_bit(EV_KEY, melfas_ts->input_dev->evbit);
	set_bit(TOUCH_HOME, melfas_ts->input_dev->keybit);
	set_bit(TOUCH_MENU, melfas_ts->input_dev->keybit);
	set_bit(TOUCH_BACK, melfas_ts->input_dev->keybit);
	set_bit(TOUCH_SEARCH, melfas_ts->input_dev->keybit);

	melfas_ts->input_dev->keycode = melfas_ts_tk_keycode;	

	set_bit(BTN_TOUCH, melfas_ts->input_dev->keybit);
	set_bit(EV_ABS, melfas_ts->input_dev->evbit);
#endif
#if 1
	input_set_abs_params(melfas_ts->input_dev, ABS_X, 0, 319/*479*/, 0, 0);
	input_set_abs_params(melfas_ts->input_dev, ABS_Y, 0, 479/*799*/, 0, 0);
	input_set_abs_params(melfas_ts->input_dev, ABS_MT_POSITION_X, 0, 319/*479*/, 0, 0);
	input_set_abs_params(melfas_ts->input_dev, ABS_MT_POSITION_Y, 0, 479/*799*/, 0, 0);
	input_set_abs_params(melfas_ts->input_dev, ABS_PRESSURE, 0, 255, 0, 0);
	input_set_abs_params(melfas_ts->input_dev, ABS_TOOL_WIDTH, 0, 15, 0, 0);
	input_set_abs_params(melfas_ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(melfas_ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, 30, 0, 0);
#else
	input_set_abs_params(melfas_ts->input_dev, ABS_MT_TRACKING_ID, 0, 10, 0, 0);
	input_set_abs_params(melfas_ts->input_dev, ABS_MT_POSITION_X, 0, max_x, 0, 0);
	input_set_abs_params(melfas_ts->input_dev, ABS_MT_POSITION_Y, 0, max_y, 0, 0);
	input_set_abs_params(melfas_ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
#endif
	printk("melfas_ts_probe: max_x: %d, max_y: %d\n", max_x, max_y);

	ret = input_register_device(melfas_ts->input_dev);
	if (ret) {
		printk(KERN_ERR "melfas_ts_probe: Unable to register %s input device\n", melfas_ts->input_dev->name);
		goto err_input_register_device_failed;
	}

	melfas_ts->client->irq = IRQ_TOUCH_INT;	// heatup - test
	melfas_ts->irq = melfas_ts->client->irq; //add by KJB  //Sunanda
	// init_hw_setting();
	ret = request_irq(melfas_ts->irq, melfas_ts_irq_handler, IRQF_DISABLED, "melfas_ts irq", 0);
	if(ret == 0) {
		printk(KERN_INFO "melfas_ts_probe: Start touchscreen %s \n", melfas_ts->input_dev->name);
	}
	else {
		printk("request_irq failed\n");
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	melfas_ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	melfas_ts->early_suspend.suspend = melfas_ts_early_suspend;
	melfas_ts->early_suspend.resume = melfas_ts_late_resume;
	register_early_suspend(&melfas_ts->early_suspend);
#endif	/* CONFIG_HAS_EARLYSUSPEND */

	return 0;
	//err_misc_register_device_failed:
err_input_register_device_failed:
	input_free_device(melfas_ts->input_dev);

err_input_dev_alloc_failed:
	//err_detect_failed:
	kfree(melfas_ts);
	//err_alloc_data_failed:
err_check_functionality_failed:
	return ret;

}

int melfas_ts_remove(struct i2c_client *client)
{
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&melfas_ts->early_suspend);
#endif	/* CONFIG_HAS_EARLYSUSPEND */
	free_irq(melfas_ts->irq, 0);
	input_unregister_device(melfas_ts->input_dev);
	return 0;
}

int melfas_ts_gen_touch_up(void)
{
  // report up key if needed
  int i;
  for ( i= 0; i<FINGER_NUM; ++i ){
  	if(melfas_ts->info[i].state == 0x1){ /*down state*/

		melfas_ts->info[i].state = 0x0;
		int finger = melfas_ts->info[i].finger_id;
    	int x = melfas_ts->info[i].x;
    	int y = melfas_ts->info[i].y;
    	int z = melfas_ts->info[i].z;
    	printk("[TOUCH] GENERATE UP KEY x: %4d, y: %4d, z: %4d\n", x, y, z);
	input_report_abs(melfas_ts->input_dev, ABS_MT_TRACKING_ID, finger);

	if (x) 	input_report_abs(melfas_ts->input_dev, ABS_MT_POSITION_X, x);
    	if (y)	input_report_abs(melfas_ts->input_dev, ABS_MT_POSITION_Y, y);

	input_report_abs(melfas_ts->input_dev, ABS_PRESSURE, z);

    	input_sync(melfas_ts->input_dev);
	}
  }    
}


int melfas_ts_suspend(pm_message_t mesg)
{
	int i=0;

	disable_irq(melfas_ts->irq);

	melfas_ts->suspended = true;
	melfas_ts_gen_touch_up();

	i = cancel_work_sync(&melfas_ts->work);
	if(i) {
		enable_irq(melfas_ts->irq);
		printk("===cancel_work_sync = %d\n",i);
	}

	for(i=0; i<FINGER_NUM; i++)
	{
		melfas_ts->info[i].z = -1;
	}

	mcsdl_vdd_off();

	printk(KERN_INFO "%s:!\n", __func__);
	return 0;
}

int melfas_ts_resume(void)
{

  mcsdl_vdd_on();
  msleep(200);//300-> Minimum stable time 200msec for MMS100 series after power on
  melfas_ts->suspended = false;
  enable_irq(melfas_ts->irq);  

  printk(KERN_INFO "%s: !\n", __func__);
  return 0;
}

int melfas_ts_suspend_ESD(pm_message_t mesg)
{
	int i=0;

	melfas_ts->suspended = true;
	melfas_ts_gen_touch_up();
	//  disable_irq(melfas_ts->irq);
	disable_irq(melfas_ts->irq);

	for(i=0; i<FINGER_NUM; i++)
	{
		melfas_ts->info[i].z = -1;
	}

	gpio_set_value(GPIO_TOUCH_EN,0);
	gpio_set_value(GPIO_TOUCH_I2C_SCL, 0);  // TOUCH SCL DIS
	gpio_set_value(GPIO_TOUCH_I2C_SDA, 0);  // TOUCH SDA DIS

	msleep(1);

	printk(KERN_INFO "%s: melfas_ts_suspend!\n", __func__);
	return 0;
}

int melfas_ts_resume_ESD(void)
{

#if 0
	init_hw_setting();
#else
	gpio_set_value(GPIO_TOUCH_I2C_SCL, 1);  // TOUCH SCL EN
	gpio_set_value(GPIO_TOUCH_I2C_SDA, 1);  // TOUCH SDA EN    
	msleep(1);
	gpio_set_value(GPIO_TOUCH_EN,1);
#endif  
	msleep(300);//300-> Minimum stable time 200msec for MMS100 series after power on
	melfas_ts->suspended = false;
#if 1  
	enable_irq(melfas_ts->irq);  
#else
	enable_irq(melfas_ts->irq);  
#endif
	printk(KERN_INFO "%s: melfas_ts_resume!\n", __func__);
	return 0;
}
int tsp_preprocess_suspend(void)
{
#if 0 // blocked for now.. we will gen touch when suspend func is called
  // this function is called before kernel calls suspend functions
  // so we are going suspended if suspended==false
  if(melfas_ts->suspended == false) {  
    // fake as suspended
    melfas_ts->suspended = true;
    
    //generate and report touch event
    melfas_ts_gen_touch_up();
  }
#endif
  return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
void melfas_ts_early_suspend(struct early_suspend *h)
{
	melfas_ts_suspend(PMSG_SUSPEND);
}

void melfas_ts_late_resume(struct early_suspend *h)
{
	melfas_ts_resume();
}
#endif	/* CONFIG_HAS_EARLYSUSPEND */


int melfas_i2c_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
	melfas_ts->client = client;
    printk(" MELFAS I2C PROBE \n");
	i2c_set_clientdata(client, melfas_ts);
	return 0;
}

static int __devexit melfas_i2c_remove(struct i2c_client *client)
{
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&melfas_ts->early_suspend);
#endif  /* CONFIG_HAS_EARLYSUSPEND */
	free_irq(melfas_ts->irq, 0);
	input_unregister_device(melfas_ts->input_dev);
   
	melfas_ts = i2c_get_clientdata(client);
	kfree(melfas_ts);
	return 0;
}

struct i2c_device_id melfas_id[] = {
	{ "melfas_ts_i2c", 0 },
	{ }
};

struct i2c_driver melfas_ts_i2c = {
	.driver = {
		.name	= "melfas_ts_i2c",
		.owner	= THIS_MODULE,
	},
	.probe 		= melfas_i2c_probe,
	.remove		= __devexit_p(melfas_i2c_remove),
	.id_table	= melfas_id,
};


void init_hw_setting(void)
{
	int ret=0;
#if 0
	vreg_touch = vreg_get(NULL, "wlan2"); /* VTOUCH_2.8V */
	vreg_touchio = vreg_get(NULL, "gp13"); /* VTOUCHIO_1.8V */
	
	ret = vreg_enable(vreg_touch);
#endif	
	if (!(gpio_get_value(GPIO_TOUCH_EN))) 
	{
		ret=gpio_direction_output(GPIO_TOUCH_EN,1);
		printk(KERN_ERR "%s: vreg_touch enable failed (%d)\n", __func__, ret);
//		return -EIO;
	}
	else 
	{
		printk(KERN_INFO "%s: vreg_touch enable success!\n", __func__);
	}
#if 0	
	ret = vreg_enable(vreg_touchio);
	
	if (ret) { 
		printk(KERN_ERR "%s: vreg_touchio enable failed (%d)\n", __func__, ret);
		return -EIO;
	}
	else {
		printk(KERN_INFO "%s: vreg_touchio enable success!\n", __func__);
	}
#endif
	mdelay(100);
	
//	s3c_gpio_cfgpin(GPIO_TOUCH_I2C_SCL, S3C_GPIO_OUTPUT); s3c_gpio_setpull(GPIO_TOUCH_I2C_SCL,S3C_GPIO_PULL_UP);
//	s3c_gpio_cfgpin(GPIO_TOUCH_I2C_SDA, S3C_GPIO_OUTPUT); s3c_gpio_setpull(GPIO_TOUCH_I2C_SDA,S3C_GPIO_PULL_UP);
	s3c_gpio_cfgpin(GPIO_TOUCH_INT, S3C_GPIO_SFN(0xf)/*S3C_GPIO_INPUT*/); s3c_gpio_setpull(GPIO_TOUCH_INT,S3C_GPIO_PULL_UP);
	set_irq_type(IRQ_TOUCH_INT, IRQ_TYPE_LEVEL_LOW); //chief.boot.temp changed from edge low to level low VERIFY!!!

	mdelay(10);

}

static struct platform_driver melfas_ts_driver =  {
	.probe	= melfas_ts_probe,
	.remove = melfas_ts_remove,
	.driver = {
		.name = "melfas-ts",
		.owner	= THIS_MODULE,
	},
};


int __init melfas_ts_init(void)
{
	int ret;
	printk("\n====================================================");
	printk("\n=======         [TOUCH SCREEN] INIT        =========");
	printk("\n====================================================\n");

	init_hw_setting();

	ts_dev = device_create(sec_class, NULL, 0, NULL, "ts");
	if (IS_ERR(ts_dev))
		pr_err("Failed to create device(ts)!\n");
	if (device_create_file(ts_dev, &dev_attr_gpio) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_gpio.attr.name);
	if (device_create_file(ts_dev, &dev_attr_registers) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_registers.attr.name);
	if (device_create_file(ts_dev, &dev_attr_firmware) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_firmware.attr.name);
	if (device_create_file(ts_dev, &dev_attr_debug) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_debug.attr.name);
#ifdef TSP_TEST_MODE
	if (device_create_file(ts_dev, &dev_attr_tsp_reference) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_tsp_reference.attr.name);
	if (device_create_file(ts_dev, &dev_attr_tsp_inspection) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_tsp_inspection.attr.name);
	if (device_create_file(ts_dev, &dev_attr_tsp_channel) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_tsp_channel.attr.name);
	if (device_create_file(ts_dev, &dev_attr_tsp_raw) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_tsp_raw.attr.name);	
	if (device_create_file(ts_dev, &dev_attr_tsp_name) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_tsp_name.attr.name);	
	if (device_create_file(ts_dev, &dev_attr_tsp_test) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_tsp_test.attr.name);	
#endif
	melfas_ts = kzalloc(sizeof(struct melfas_ts_driver), GFP_KERNEL);
	if(melfas_ts == NULL) {
		return -ENOMEM;
	}

	ret = i2c_add_driver(&melfas_ts_i2c);
	if(ret) printk("[%s], i2c_add_driver failed...(%d)\n", __func__, ret);

	printk(KERN_ERR "[HEATUP] ret : %d, melfas_ts->client name : %s\n",ret,melfas_ts->client->name);


	if(!melfas_ts->client) {
		printk("###################################################\n");
		printk("##                                               ##\n");
		printk("##    WARNING! TOUCHSCREEN DRIVER CAN'T WORK.    ##\n");
		printk("##    PLEASE CHECK YOUR TOUCHSCREEN CONNECTOR!   ##\n");
		printk("##                                               ##\n");
		printk("###################################################\n");
		i2c_del_driver(&melfas_ts_i2c);
		return 0;
	}
	melfas_ts_wq = create_singlethread_workqueue("melfas_ts_wq");
	if (!melfas_ts_wq)
		return -ENOMEM;

	return platform_driver_register(&melfas_ts_driver);

}

void __exit melfas_ts_exit(void)
{
	i2c_del_driver(&melfas_ts_i2c);
	if (melfas_ts_wq)
		destroy_workqueue(melfas_ts_wq);
}
late_initcall(melfas_ts_init);
//module_init(melfas_ts_init);
module_exit(melfas_ts_exit);

MODULE_DESCRIPTION("Melfas Touchscreen Driver");
MODULE_LICENSE("GPL");
