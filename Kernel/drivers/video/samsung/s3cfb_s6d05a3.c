/*
 * s6d0a3 HVGA TFT LCD Panel Driver
*/

#include <linux/wait.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/lcd.h>
#include <linux/backlight.h>
#include <linux/irq.h>
#include <linux/interrupt.h>

#include <plat/gpio-cfg.h>
#include <mach/gpio-chief.h>
#include <mach/gpio-chief-settings.h>


#include "s3cfb.h"

#define FACTORY_CHECK
extern unsigned int HWREV;

#define DIM_BL	20
#define MIN_BL	30
#define MAX_BL	255

#define SLEEPMSEC		0x1000
#define ENDDEF			0x2000
#define DEFMASK		0xFF00
#define DIM_BL	20
#define MIN_BL	30
#define MAX_BL	255
#define MAX_GAMMA_VALUE	24

#define DATA_ONLY 0xFF
#define COMMAND_ONLY	0xFE

#define MAX_GAMMA_VALUE	24	// we have 25 levels. -> 16 levels -> 24 levels
#define CRITICAL_BATTERY_LEVEL 5

#define FBDBG_TRACE()       printk(KERN_ERR "%s\n", __FUNCTION__)

static struct s5p_lcd lcd;

////////////////////////////////////////////////////////////////////////////////

#define DISPLAY_CS	GPIO_DISPLAY_CS
#define DISPLAY_CLK	GPIO_DISPLAY_CLK
#define DISPLAY_SI	GPIO_DISPLAY_SI
////////////////////////////////////////////////////////////////////////////////


static int locked = 0;
static int ldi_enable = 0;


int current_gamma_value = -1;
int spi_ing = 0;
int on_19gamma = 0;

#ifdef CONFIG_FB_S3C_LM347DF01
#define LM347DF01_LCD_ID 0x6b8840
unsigned int lcd_Idtype;
extern void lm347df_start_poweroff_sequence(void);
extern void lm347df_start_poweron_sequence(void);
extern void lm347df_display_on_sequence(void);
extern void lm347df_display_off_sequence(void);

EXPORT_SYMBOL(lcd_Idtype);

 static int __init check_lcdtype(char *str)
{
	printk("**************** str **************=%s\n", str);
	lcd_Idtype = simple_strtol(str, NULL, 16);
	printk("**************** lcd_Idtype **************=%x\n", lcd_Idtype);
	
	return 1;
}

 //2=A1line , 3=A2line
__setup("lcd_Id=", check_lcdtype);

 #endif

#define ENLVL2		0xF0
#define ENMTP			0xF1
#define ENLVL3		0xFC
#define MANPWRSEQ	0xF3
#define PWRCTL		0xF4
#define SLEEPOUT		0x11
#define DISCTL		0xF2
#define SRCCTL		0xF6
#define PANELCTL		0xF8
#define IFCTL			0xF7
#define RGBGAMMA	0xF9
#define PGAMMACTL	0xFA
#define NGAMMACTL	0xFB
#define IPIXELFMT		0x3A
#define COLADDRSET	0x2A
#define PGADDRSET	0x2B
#define ZZINVCTL		0xED
#define WRITECDISP	0x53
#define WRITEDBRIG	0x51
#define DISPLAYON	0x29
#define DISPLAYOFF	0x28


struct setting_table {
	u8 command;	
	u8 parameters;
	u8 parameter[50];
	s32 wait;
};


static struct setting_table power_on_setting_table[] = {
	{	ENLVL2,  		2, { 0x5A, 0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },	0 }, //Enable Level 2
	{	ENMTP,		2, { 0x5A, 0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0 }, //Enable MTP & DSTB
	{	ENLVL3,		2, { 0xA5, 0xA5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0 },	//Enable Level 3
	{	MANPWRSEQ,	7, { 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0 },	//manual power seq
	{	PWRCTL,		15,{ 0x02, 0xB6, 0x52, 0x52, 0x52, 0x52, 0x50, 0x32, 0x13, 0x54, 0x51, 0x11, 0x2A, 0x2A, 0xB3, 0x00, 0x00, 0x00, 0x00 }, 0 }, //power control
	{	SLEEPOUT,	0, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 120}, //sleep out		
	{	DISCTL,		13,{ 0x3C, 0x7E, 0x03, 0x08, 0x08, 0x02, 0x10, 0x00, 0x2F, 0x10, 0xC8, 0x5D, 0x5D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0 },	//display control
	{	SRCCTL,		8,  { 0x29, 0x02, 0x0F, 0x00, 0x14, 0x44, 0x11, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0 },	
	{	PANELCTL,	8, { 0x33, 0x00, 0x19, 0x21, 0x40, 0x00, 0x09, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0 },	
	{	IFCTL,		4, { 0x18, 0x81, 0x10, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0 },
	{	RGBGAMMA,	1, { 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0 }, // Red gamma selection
	{	PGAMMACTL,	14,{ 0x16, 0x2D, 0x09, 0x12, 0x1F,0x00, 0x00, 0x02, 0x34, 0x32,0x2A, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0 }, // Positive gamma control
	{	NGAMMACTL,	14,{ 0x16, 0x2D, 0x09, 0x12, 0x1F,0x00, 0x00, 0x02, 0x34, 0x32,0x2A, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0 }, // Negative gamma control 	
	{	RGBGAMMA,	1, { 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0 }, // Green gamma selection 	
	{	PGAMMACTL,	14,{ 0x04, 0x2C, 0x08, 0x1E, 0x30,0x1D, 0x17, 0x13, 0x24, 0x26,0x20, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ,0x00}, 0 }, // Positive gamma control
	{	NGAMMACTL,	14,{ 0x04, 0x2C, 0x08, 0x1E, 0x30,0x1D, 0x17, 0x17, 0x24, 0x26,0x20, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ,0x00}, 0 }, // Negative gamma control
	{	RGBGAMMA,	1, { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0 }, // Blue gamma selection
	{	PGAMMACTL,	14,{ 0x22, 0x2E, 0x0A, 0x15, 0x20,0x04, 0x07, 0x02, 0x38, 0x33,0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00 }, 0 }, // Positive gamma control	
	{	NGAMMACTL,	14,{ 0x22, 0x2D, 0x0A, 0x15, 0x20,0x04, 0x06, 0x02, 0x38, 0x35,0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0 }, // Negative gamma control		
	{	IPIXELFMT,	1, { 0x77, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0 }, // Interface Pixel Format	
	{	COLADDRSET,	4, { 0x00, 0xF0, 0x01, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0 }, // Column Address Set
	{	PGADDRSET,	4, { 0x00, 0x00, 0x01, 0xDF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0 }, // Page Address Set
	{	ZZINVCTL,		1, { 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0 }, // Zig Zag Inv Control
	{	WRITECDISP,	1, { 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0 }, // Write Control Display
	{	WRITEDBRIG,	1, { 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0 }, // 
	{	DISPLAYON,	0, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 100 }, // Display on
};

#define POWER_OFF_SETTINGS	(int)(sizeof(power_off_setting_table)/sizeof(struct setting_table))
#define POWER_ON_SETTINGS	(int)(sizeof(power_on_setting_table)/sizeof(struct setting_table))

#define LCD_CSX_HIGH	gpio_set_value(GPIO_DISPLAY_CS, GPIO_LEVEL_HIGH);
#define LCD_CSX_LOW	gpio_set_value(GPIO_DISPLAY_CS, GPIO_LEVEL_LOW);

#define LCD_SCL_HIGH	gpio_set_value(GPIO_DISPLAY_CLK, GPIO_LEVEL_HIGH);
#define LCD_SCL_LOW	gpio_set_value(GPIO_DISPLAY_CLK, GPIO_LEVEL_LOW);

#define LCD_SDI_HIGH	gpio_set_value(GPIO_DISPLAY_SI, GPIO_LEVEL_HIGH);
#define LCD_SDI_LOW	gpio_set_value(GPIO_DISPLAY_SI, GPIO_LEVEL_LOW);
#define LCD_SDO_READ	gpio_get_value(GPIO_LCD_SO);
#define usleep	udelay
#define DEFAULT_USLEEP  10

static void setting_table_write(struct setting_table *table)
{
	s32 i, j;

	LCD_CSX_HIGH
	usleep(DEFAULT_USLEEP);
	LCD_SCL_HIGH 
	usleep(DEFAULT_USLEEP);

	/* Write Command */
	LCD_CSX_LOW
	usleep(DEFAULT_USLEEP);
	
	LCD_SCL_LOW 
	usleep(DEFAULT_USLEEP);		
	LCD_SDI_LOW 
	usleep(DEFAULT_USLEEP);
	LCD_SCL_HIGH 
	usleep(DEFAULT_USLEEP); 

   	for (i = 7; i >= 0; i--) { 
		LCD_SCL_LOW
		usleep(DEFAULT_USLEEP);
		if ((table->command >> i) & 0x1)
			LCD_SDI_HIGH
		else
			LCD_SDI_LOW
		usleep(DEFAULT_USLEEP);	
		LCD_SCL_HIGH
		usleep(DEFAULT_USLEEP);	
	}

	LCD_CSX_HIGH
	usleep(DEFAULT_USLEEP);	

	/* Write Parameter */
	if ((table->parameters) > 0) {
		for (j = 0; j < table->parameters; j++) {
			LCD_CSX_LOW 
			usleep(DEFAULT_USLEEP); 	
		
			LCD_SCL_LOW 
			usleep(DEFAULT_USLEEP); 	
			LCD_SDI_HIGH 
			usleep(DEFAULT_USLEEP);
			LCD_SCL_HIGH 
			usleep(DEFAULT_USLEEP); 	

			for (i = 7; i >= 0; i--) { 
				LCD_SCL_LOW
				usleep(DEFAULT_USLEEP);	
				if ((table->parameter[j] >> i) & 0x1)
					LCD_SDI_HIGH
				else
					LCD_SDI_LOW
				usleep(DEFAULT_USLEEP);	
				LCD_SCL_HIGH
				usleep(DEFAULT_USLEEP);					
			}
		
			LCD_CSX_HIGH 
			usleep(DEFAULT_USLEEP); 	
		}
	}
	
	msleep(table->wait);
}

void start_lcd_poweron_sequence1(void)
{
  int i;
  int tblsize;
  struct setting_table *p_power_set_tbl = power_on_setting_table;

  tblsize = POWER_ON_SETTINGS;
  
  msleep(20); // for chief ... Need to delay time more than 20msec for start input lcd power-on sequence
  for (i = 0; i < tblsize; i++)
    setting_table_write(&p_power_set_tbl[i]);

  msleep(1); // stable time for another command
}

#define FEATURE_LCD_ESD_DET

#ifdef FEATURE_LCD_ESD_DET
#define GPIO_ESD_DET	GPIO_DISPLAY_ESD    //S5PV210_GPH3(0)  //
static int used_esd_detet = 1;
static int phone_power_off = 0;
static void s6d05a3_esd_irq_handler_cancel(void);
//static void s6d05a3_esd_timer_handler(unsigned long data);
static irqreturn_t s6d05a3_esd_irq_handler(int irq, void *handle);
//static struct timer_list g_lcd_esd_timer;
//static int esd_repeat_count = 0;
#define ESD_REPEAT_MAX    3
struct delayed_work lcd_esd_work;
void s6d05a3_esd(void);
#endif

//#ifdef CONFIG_FB_S3C_MDNIE
//extern void init_mdnie_class(void);
//#endif

extern struct class *sec_class;

struct s5p_lcd{
	struct spi_device *g_spi;
	struct lcd_device *lcd_dev;
#ifdef FACTORY_CHECK
          struct device *sec_lcdtype_dev;
#endif
};

#ifdef GAMMASET_CONTROL
struct class *gammaset_class;
struct device *switch_gammaset_dev;
#endif


const unsigned short SEQ_STAND_BY_ON[] = {
	0x10, COMMAND_ONLY,
	
	ENDDEF, 0x0000
};

const unsigned short SEQ_DISPLAY_ON[] = {
	0x29, COMMAND_ONLY,
	
	ENDDEF, 0x0000
};

const unsigned short SEQ_DISPLAY_OFF[] = {
	0x28, COMMAND_ONLY,
	
	ENDDEF, 0x0000
};

const unsigned short SEQ_STAND_BY_OFF[] = {
	0x11, COMMAND_ONLY,

	ENDDEF, 0x0000
};

const unsigned short SEQ_LEVEL2_COMMAND_ACCESS[] = {
	0xF0, 0x5A,
	DATA_ONLY, 0x5A,

	ENDDEF, 0x0000
};

const unsigned short SEQ_LEVEL2_COMMAND_ACCESS1[] = {
	0xF1, 0x5A,
	DATA_ONLY, 0x5A,
 
	ENDDEF, 0x0000
};
const unsigned short SEQ_LEVEL3_COMMAND_ACCESS[] = {
	0xFC, 0xA5,
	DATA_ONLY, 0xA5,
 
	ENDDEF, 0x0000
};

const unsigned short SEQ_MANPWRSEQ[] = {
	0xF3, 0x07,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x04,
	
	ENDDEF, 0x0000
};

const unsigned short SEQ_PWRCTL[] = {
	0xF4, 0x02,
	DATA_ONLY, 0xBA,
	DATA_ONLY, 0x5D,
	DATA_ONLY, 0x5D,
	DATA_ONLY, 0x5D,
	DATA_ONLY, 0x5D,
	DATA_ONLY, 0x50,
	DATA_ONLY, 0x32,
	DATA_ONLY, 0x13,
	DATA_ONLY, 0x54,
	DATA_ONLY, 0x51,
	DATA_ONLY, 0x11,
	DATA_ONLY, 0x2A,
	DATA_ONLY, 0x2A,
	DATA_ONLY, 0xB3,
	
	ENDDEF, 0x0000
};
const unsigned short SEQ_SLEEP_OUT[] = {
	0x11, COMMAND_ONLY,
		
	ENDDEF, 0x0000
};

const unsigned short SEQ_DISCTL[] = {
	0xF2, 0x3C,
	DATA_ONLY, 0x7E,
	DATA_ONLY, 0x03,
	DATA_ONLY, 0x08,
	DATA_ONLY, 0x08,
	DATA_ONLY, 0x02,
	DATA_ONLY, 0x10,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x2F,
	DATA_ONLY, 0x10,
	DATA_ONLY, 0xC8,
	DATA_ONLY, 0x5D,
	DATA_ONLY, 0x5D,
		
	ENDDEF, 0x0000
};

const unsigned short SEQ_SRCCTL[] = {
	0xF6, 0x29,
	DATA_ONLY, 0x02,
	DATA_ONLY, 0x0F,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x14,
	DATA_ONLY, 0x44,
	DATA_ONLY, 0x11,
	DATA_ONLY, 0x15,
			
	ENDDEF, 0x0000
};

const unsigned short SEQ_PANELCTL[] = {
	0xF8, 0x55,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x16,
	DATA_ONLY, 0x21,
	DATA_ONLY, 0x40,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x05,
	DATA_ONLY, 0x0A,
			
	ENDDEF, 0x0000
};

const unsigned short SEQ_IFCTL[] = {
	0xF7, 0x18,
	DATA_ONLY, 0x81,
	DATA_ONLY, 0x10,
	DATA_ONLY, 0x02,
			
	ENDDEF, 0x0000
};

const unsigned short SEQ_RED_GAMMA[] = {
	0xF9, 0x24,
	
	ENDDEF, 0x0000
};

const unsigned short RGAMMA_POSITIVE_SEQ[] = {
	0xFA, 0x22,
	DATA_ONLY, 0x2F,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x09,
	DATA_ONLY, 0x0C,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x36,
	DATA_ONLY, 0x2E,
	DATA_ONLY, 0x24,
	DATA_ONLY, 0x10,
	DATA_ONLY, 0x0F,
	DATA_ONLY, 0x00,
	
	ENDDEF, 0x0000
};

const unsigned short RGAMMA_NEGATIVE_SEQ[] = {
	0xFB, 0x22,
	DATA_ONLY, 0x2F,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x09,
	DATA_ONLY, 0x0C,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x36,
	DATA_ONLY, 0x2E,
	DATA_ONLY, 0x24,
	DATA_ONLY, 0x10,
	DATA_ONLY, 0x0F,
	DATA_ONLY, 0x00,
	
	ENDDEF, 0x0000
};

const unsigned short SEQ_GREEN_GAMMA[] = {
	0xF9, 0x22,
	
	ENDDEF, 0x0000
};

const unsigned short GGAMMA_POSITIVE_SEQ[] = {
	0xFA, 0x02,
	DATA_ONLY, 0x20,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x18,
	DATA_ONLY, 0x26,
	DATA_ONLY, 0x14,
	DATA_ONLY, 0x0E,
	DATA_ONLY, 0x0B,
	DATA_ONLY, 0x2F,
	DATA_ONLY, 0x2F,
	DATA_ONLY, 0x33,
	DATA_ONLY, 0x2F,
	DATA_ONLY, 0x23,
	DATA_ONLY, 0x00,
	
	ENDDEF, 0x0000
};

const unsigned short GGAMMA_NEGATIVE_SEQ[] = {
	0xFB, 0x02,
	DATA_ONLY, 0x20,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x18,
	DATA_ONLY, 0x26,
	DATA_ONLY, 0x14,
	DATA_ONLY, 0x0E,
	DATA_ONLY, 0x0B,
	DATA_ONLY, 0x2F,
	DATA_ONLY, 0x2F,
	DATA_ONLY, 0x33,
	DATA_ONLY, 0x2F,
	DATA_ONLY, 0x23,
	DATA_ONLY, 0x00,
	
	ENDDEF, 0x0000
};

const unsigned short SEQ_BLUE_GAMMA[] = {
	0xF9, 0x21,
	
	ENDDEF, 0x0000
};

const unsigned short BGAMMA_POSITIVE_SEQ[] = {
	0xFA, 0x1A,
	DATA_ONLY, 0x24,
	DATA_ONLY, 0x10,
	DATA_ONLY, 0x33,
	DATA_ONLY, 0x30,
	DATA_ONLY, 0x11,
	DATA_ONLY, 0x0A,
	DATA_ONLY, 0x05,
	DATA_ONLY, 0x35,
	DATA_ONLY, 0x33,
	DATA_ONLY, 0x24,
	DATA_ONLY, 0x05,
	DATA_ONLY, 0x03,
	DATA_ONLY, 0x00,
	
	ENDDEF, 0x0000
};

const unsigned short BGAMMA_NEGATIVE_SEQ[] = {
	0xFA, 0x1A,
	DATA_ONLY, 0x24,
	DATA_ONLY, 0x10,
	DATA_ONLY, 0x33,
	DATA_ONLY, 0x30,
	DATA_ONLY, 0x11,
	DATA_ONLY, 0x0A,
	DATA_ONLY, 0x05,
	DATA_ONLY, 0x35,
	DATA_ONLY, 0x33,
	DATA_ONLY, 0x24,
	DATA_ONLY, 0x05,
	DATA_ONLY, 0x03,
	DATA_ONLY, 0x00,
	
	ENDDEF, 0x0000
};

const unsigned short INTERFACE_PIXEL[] = {
	0x3A, 0x77,
	
	ENDDEF, 0x0000
};

const unsigned short COLUMN_ADDRESS[] = {
	0x2A, 0x00,
	DATA_ONLY, 0xF0,
	DATA_ONLY, 0x01,
	DATA_ONLY, 0x3F,
	
	ENDDEF, 0x0000
};

const unsigned short PAGE_ADDRESS[] = {
	0x2B, 0x00,
	DATA_ONLY, 0x00,
	DATA_ONLY, 0x01,
	DATA_ONLY, 0xDF,
	
	ENDDEF, 0x0000
};

const unsigned short ZIGZAG_INV_CTL[] = {
	0xED, 0x08,
	
	ENDDEF, 0x0000
};

const unsigned short WRITE_CTL_DISPLAY[] = {
	0x53, 0x24,
	
	ENDDEF, 0x0000
};

const unsigned short WRITE_DISPLAY_BRI[] = {
	0x51, 0xFF,
	
	ENDDEF, 0x0000
};


static inline void lcd_cs_value(int level)
{
	gpio_set_value(GPIO_DISPLAY_CS, level);
}

static inline void lcd_clk_value(int level)
{
	gpio_set_value(GPIO_DISPLAY_CLK, level);
}

static inline void lcd_si_value(int level)
{
	gpio_set_value(GPIO_DISPLAY_SI, level);
}
static void s6d05a3_c110_spi_write_byte(unsigned char address, unsigned char command)
{
	int     j;
	unsigned short data;

	data = (address << 8) + command;

	lcd_cs_value(1);
	lcd_si_value(1);
	lcd_clk_value(1);
	udelay(1);

	lcd_cs_value(0);
	udelay(1);

	for (j = 8; j >= 0; j--)
	{
		lcd_clk_value(0);

		/* data high or low */
		if ((data >> j) & 0x0001)
			lcd_si_value(1);
		else
			lcd_si_value(0);

		udelay(1);

		lcd_clk_value(1);
		udelay(1);
	}

	lcd_cs_value(1);
	udelay(1);
}

static void s6d05a3_spi_write(unsigned char address, unsigned char command)
{
	if (address != DATA_ONLY)
		s6d05a3_c110_spi_write_byte(0x0, address);

	if (command != COMMAND_ONLY)
		s6d05a3_c110_spi_write_byte(0x1, command);
}

void s6d05a3_panel_send_sequence(const unsigned short *wbuf)
{
	int i = 0;

	while ((wbuf[i] & DEFMASK) != ENDDEF) {
		if ((wbuf[i] & DEFMASK) != SLEEPMSEC)
			s6d05a3_spi_write(wbuf[i], wbuf[i+1]);
		else
			udelay(wbuf[i+1]*1000);
		i += 2;
	}
}

void start_lcd_poweron_sequence(void)
{
  
  printk("start_lcd_poweron_sequence\n");
		
		s6d05a3_panel_send_sequence(SEQ_LEVEL2_COMMAND_ACCESS);
		s6d05a3_panel_send_sequence(SEQ_LEVEL2_COMMAND_ACCESS1);
		s6d05a3_panel_send_sequence(SEQ_LEVEL3_COMMAND_ACCESS);
		s6d05a3_panel_send_sequence(SEQ_MANPWRSEQ);
		s6d05a3_panel_send_sequence(SEQ_PWRCTL);
		s6d05a3_panel_send_sequence(SEQ_SLEEP_OUT);
		mdelay(120);
		s6d05a3_panel_send_sequence(SEQ_DISCTL);
		s6d05a3_panel_send_sequence(SEQ_SRCCTL);
		s6d05a3_panel_send_sequence(SEQ_PANELCTL);
		s6d05a3_panel_send_sequence(SEQ_IFCTL);
		s6d05a3_panel_send_sequence(SEQ_RED_GAMMA);
		s6d05a3_panel_send_sequence(RGAMMA_POSITIVE_SEQ);
		s6d05a3_panel_send_sequence(RGAMMA_NEGATIVE_SEQ);
		s6d05a3_panel_send_sequence(SEQ_GREEN_GAMMA);
		s6d05a3_panel_send_sequence(GGAMMA_POSITIVE_SEQ);
		s6d05a3_panel_send_sequence(GGAMMA_NEGATIVE_SEQ);
		s6d05a3_panel_send_sequence(SEQ_BLUE_GAMMA);
		s6d05a3_panel_send_sequence(BGAMMA_POSITIVE_SEQ);
		s6d05a3_panel_send_sequence(BGAMMA_NEGATIVE_SEQ);
		s6d05a3_panel_send_sequence(INTERFACE_PIXEL);
		s6d05a3_panel_send_sequence(COLUMN_ADDRESS);
		s6d05a3_panel_send_sequence(PAGE_ADDRESS);
		s6d05a3_panel_send_sequence(ZIGZAG_INV_CTL);
		s6d05a3_panel_send_sequence(WRITE_CTL_DISPLAY);
		s6d05a3_panel_send_sequence(WRITE_DISPLAY_BRI);
		s6d05a3_panel_send_sequence(SEQ_DISPLAY_ON);
		mdelay(45);
}

void start_lcd_poweroff_sequence(void)
{
    s6d05a3_panel_send_sequence(SEQ_STAND_BY_ON);
}

int IsLDIEnabled(void)
{
	return ldi_enable;
}
EXPORT_SYMBOL(IsLDIEnabled);

////////////////////////////////////////////////////////////////////////////////
static void SetLDIEnabledFlag(int OnOff)
{
	ldi_enable = OnOff;
	if(ldi_enable)
	printk("******%s********,LCD ON COMPLETED \n", __FUNCTION__);
	else
	printk("******%s********,LCD OFF COMPLETED \n", __FUNCTION__);
}

////////////////////////////////////////////////////////////////////////////////
void s6d05a3_lcd_reset(void)
{
	s3c_gpio_cfgpin(GPIO_MLCD_RST, S3C_GPIO_SFN(1));
	msleep(10);
	if(lcd_Idtype == LM347DF01_LCD_ID){
		gpio_set_value(GPIO_MLCD_RST, 1);
		msleep(80);
		gpio_set_value(GPIO_MLCD_RST, 0);
		msleep(170);
		gpio_set_value(GPIO_MLCD_RST, 1);
		msleep(10);
	}
	else{
		gpio_set_value(GPIO_MLCD_RST, 1);
		msleep(12);
		gpio_set_value(GPIO_MLCD_RST, 0);
		msleep(12); // more than 10msec
		gpio_set_value(GPIO_MLCD_RST, 1);
		msleep(50); // wait more than 30 ms after releasing the system reset( > 10msec) and input RGB interface signal (RGB pixel datga, sync signals,dot clock...) ( >= 20msec)
	}
	
}
void lcd_esd_irq_enable(void) 
{
	if(HWREV>=4) 
		enable_irq(gpio_to_irq(GPIO_ESD_DET));
}

////////////////////////////////////////////////////////////////////////////////
void s6d05a3_ldi_init(void)
{
    FBDBG_TRACE();
	#ifdef CONFIG_FB_S3C_LM347DF01
	if(lcd_Idtype == LM347DF01_LCD_ID)	{
		s3c_gpio_cfgpin(GPIO_DISPLAY_SO, S3C_GPIO_INPUT);
		lm347df_start_poweron_sequence();

		SetLDIEnabledFlag(1);

		return;
	}
#endif

	start_lcd_poweron_sequence1();
    
SetLDIEnabledFlag(1);
}


////////////////////////////////////////////////////////////////////////////////
void s6d05a3_ldi_stand_by(void)
{
    FBDBG_TRACE();
	
	#ifdef CONFIG_FB_S3C_LM347DF01
  	/* Start power off sequence so that other LCD can restart*/
	if(lcd_Idtype == LM347DF01_LCD_ID)	{
		lm347df_start_poweroff_sequence();
		return;
     }

#endif

	start_lcd_poweroff_sequence();
}


////////////////////////////////////////////////////////////////////////////////
void s6d05a3_ldi_disable(void)
{
	int pending_work;
    FBDBG_TRACE();   	                                   
#if defined(FEATURE_LCD_ESD_DET)
	if((HWREV>=4) ) {
	disable_irq_nosync(gpio_to_irq(GPIO_ESD_DET));
	pending_work=cancel_work_sync(&lcd_esd_work);
	if(pending_work){
		enable_irq(gpio_to_irq(GPIO_ESD_DET));
		printk(KERN_ERR "===cancel_work_sync pending_work = %d \n",pending_work);
	}
	printk(KERN_ERR "esd completed");	
	printk(KERN_ERR "suspend completed");
	}
#endif
	s6d05a3_ldi_stand_by() ;

	SetLDIEnabledFlag(0);
	printk( "LCD OFF !!! \n");
}

////////////////////////////////////////////////////////////////////////////////
void s3cfb_set_lcd_info(struct s3cfb_global *ctrl)
{
    FBDBG_TRACE();
#if 0
	s6d16a0x.init_ldi = NULL;
	ctrl->lcd = &s6d16a0x ;
#endif
}

//mkh:lcd operations and functions
////////////////////////////////////////////////////////////////////////////////
static int s5p_lcd_set_power(struct lcd_device *ld, int power)
{
    FBDBG_TRACE() ;
#if 0
	if(power)
	{
        s6d16a0x_write_sequence(s6d16a0x_disp_on_seq) ;
	}
	else
    {
		s6d16a0x_write_sequence(s6d16a0x_disp_off_seq);
	}
#endif
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

// cmk 2011.04.11 fixed camera & video player black screen issue. Add s5p_lcd_check_fb with return 0.
static int s5p_lcd_check_fb(struct lcd_device *lcddev, struct fb_info *fi)
{
	return 0;
}

static struct lcd_ops s5p_lcd_ops = {
	.set_power = s5p_lcd_set_power,
	.check_fb = s5p_lcd_check_fb,
};


////////////////////////////////////////////////////////////////////////////////
#ifdef FACTORY_CHECK
ssize_t lcdtype_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	printk("%s \n", __func__);
#if defined(CONFIG_FB_S3C_LM347DF01) 
	
	if (lcd_Idtype == LM347DF01_LCD_ID) {
		char * name = "SMD_AMS397GE03";
		return sprintf(buf,"%s\n", name);
		}
	else 
#endif
		{
		char * name = "AUO_H365QVN01";
		return sprintf(buf,"%s\n", name);
		}
}


static ssize_t lcd_on_off_store(
                 struct device *dev, struct device_attribute *attr,
                 const char *buf, size_t size)
 {
 
         if (size < 1)
                return -EINVAL;
 
         if (strnicmp(buf, "on", 2) == 0 || strnicmp(buf, "1", 1) == 0)
		lm347df_display_on_sequence();
        else if (strnicmp(buf, "off", 3) == 0 || strnicmp(buf, "0", 1) == 0)
		lm347df_display_off_sequence();

         return size;
 }
 
 static DEVICE_ATTR(lcd_on_off, 0664, NULL, lcd_on_off_store);


ssize_t lcdtype_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	printk(KERN_NOTICE "%s:%s\n", __func__, buf);

	return size;
}
static DEVICE_ATTR(lcdtype,0644, lcdtype_show, lcdtype_store);
#endif

static int __init s6d05a3_probe(struct spi_device *spi)
{
	int ret;

    FBDBG_TRACE() ;
	spi->bits_per_word = 9;
	ret = spi_setup(spi);
	lcd.g_spi = spi;
	lcd.lcd_dev = lcd_device_register("s5p_lcd",&spi->dev,&lcd,&s5p_lcd_ops);
	dev_set_drvdata(&spi->dev,&lcd);

#if defined(FEATURE_LCD_ESD_DET)
	if(HWREV>=4){
	if( used_esd_detet != 0 ) {
	if(gpio_request( GPIO_ESD_DET, "GPIO_GPH22"))
	   printk(KERN_ERR "request fail GPIO_ESD_DET\n");
	gpio_direction_input(GPIO_ESD_DET);
	s3c_gpio_cfgpin(GPIO_ESD_DET, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(GPIO_ESD_DET ,S3C_GPIO_PULL_NONE);
	set_irq_type(gpio_to_irq(GPIO_ESD_DET), IRQ_TYPE_EDGE_RISING);
	
       }
//		init_timer(&g_lcd_esd_timer);
//		g_lcd_esd_timer.function = s6d05a3_esd_timer_handler;
				
		INIT_DELAYED_WORK(&lcd_esd_work, s6d05a3_esd);
		ret = request_irq(gpio_to_irq(GPIO_ESD_DET), s6d05a3_esd_irq_handler, IRQF_TRIGGER_RISING, "LCD_ESD_DET", NULL);
		
		if (ret) {
		printk("%s, request_irq failed %d(ESD_DET), ret= %d\n", __func__, GPIO_ESD_DET, ret);    
		}
	}
#endif

{
	 if (sec_class == NULL)
	 	sec_class = class_create(THIS_MODULE, "sec");
	 if (IS_ERR(sec_class))
                pr_err("Failed to create class(sec)!\n");

	 lcd.sec_lcdtype_dev = device_create(sec_class, NULL, 0, NULL, "sec_lcd");
	 if (IS_ERR(lcd.sec_lcdtype_dev))
	 	pr_err("Failed to create device(ts)!\n");

	  if (device_create_file(lcd.sec_lcdtype_dev, &dev_attr_lcd_on_off) < 0)

	 	pr_err("Failed to create device file()!\n");
#ifdef FACTORY_CHECK
	  if (device_create_file(lcd.sec_lcdtype_dev, &dev_attr_lcdtype) < 0)
	 	pr_err("Failed to create device file()!\n");
#endif
}

    SetLDIEnabledFlag(1);

    //s6d16a0x_ldi_init() ; // TODO:FORTE_TEST
    //if (ret)
    //   printk(KERN_ERR "s6d16a0x not found\n") ;

#ifdef GAMMASET_CONTROL //for 1.9/2.2 gamma control from platform
	gammaset_class = class_create(THIS_MODULE, "gammaset");
	if (IS_ERR(gammaset_class))
		pr_err("Failed to create class(gammaset_class)!\n");

	switch_gammaset_dev = device_create(gammaset_class, NULL, 0, NULL, "switch_gammaset");
	if (IS_ERR(switch_gammaset_dev))
		pr_err("Failed to create device(switch_gammaset_dev)!\n");

	if (device_create_file(switch_gammaset_dev, &dev_attr_gammaset_file_cmd) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_gammaset_file_cmd.attr.name);
#endif

#ifdef ACL_ENABLE //ACL On,Off
	acl_class = class_create(THIS_MODULE, "aclset");
	if (IS_ERR(acl_class))
		pr_err("Failed to create class(acl_class)!\n");

	switch_aclset_dev = device_create(acl_class, NULL, 0, NULL, "switch_aclset");
	if (IS_ERR(switch_aclset_dev))
		pr_err("Failed to create device(switch_aclset_dev)!\n");

	if (device_create_file(switch_aclset_dev, &dev_attr_aclset_file_cmd) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_aclset_file_cmd.attr.name);
#endif

#ifdef CONFIG_FB_S3C_MDNIE
	init_mdnie_class();  //set mDNIe UI mode, Outdoormode
#endif

	if (ret < 0){
		printk(KERN_ERR "%s::%d-> s6d05a3 probe failed Err=%d\n",__func__,__LINE__,ret);
		return 0;
	}
	printk(KERN_INFO "%s::%d->s6d05a3 probed successfuly(ret=%d)\n",__func__,__LINE__, ret);
	return ret;
}

#ifdef CONFIG_PM // add by ksoo (2009.09.07)
int s6d05a3_suspend(struct platform_device *pdev, pm_message_t state)
{
    FBDBG_TRACE() ;
	s6d05a3_ldi_disable();
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
int s6d05a3_resume(struct platform_device *pdev, pm_message_t state)
{
    FBDBG_TRACE() ;

	s6d05a3_ldi_init();

	return 0;
}
#endif

////////////////////////////////////////////////////////////////////////////////
static struct spi_driver s6d05a3_driver = {
	.driver = {
		.name	= "s6d05a3",
		.owner	= THIS_MODULE,
	},
	.probe		= s6d05a3_probe,
	.remove		= __exit_p(s6d16a0x_remove),

	.suspend		= NULL,
	.resume		= NULL,
};
#ifdef FEATURE_LCD_ESD_DET
void s6d05a3_esd(void)
{
	FBDBG_TRACE() ;
		if(  gpio_get_value(GPIO_ESD_DET) == 1 )
		{
			printk(KERN_ERR "%s reset lcd panel\n",__func__);
			s6d05a3_ldi_stand_by();
			s6d05a3_lcd_reset();
			s6d05a3_ldi_init();
		}

//	g_lcd_esd_timer.expires = get_jiffies_64() + msecs_to_jiffies(1000);
//	add_timer(&g_lcd_esd_timer);
	enable_irq(gpio_to_irq(GPIO_ESD_DET));	
}


static irqreturn_t s6d05a3_esd_irq_handler(int irq, void *handle)
{
	FBDBG_TRACE() ;
	disable_irq_nosync(gpio_to_irq(GPIO_ESD_DET));
	if( used_esd_detet) {
		schedule_delayed_work(&lcd_esd_work, 100);
	}
	return IRQ_HANDLED;
}

static void s6d05a3_esd_irq_handler_cancel(void)
{
  cancel_work_sync(&lcd_esd_work);
}
#if 0
static void s6d05a3_esd_timer_handler(unsigned long data)
{
	FBDBG_TRACE() ;
  if(suspend_check==1)
  	return;
  if( (gpio_get_value(GPIO_ESD_DET) == 1))
  {
    disable_irq_nosync(gpio_to_irq(GPIO_ESD_DET));
    schedule_delayed_work(&lcd_esd_work, 100);
  }
  else
  {
    esd_repeat_count += 1;
	printk("esd_repeat_count:%d",esd_repeat_count);
    if( esd_repeat_count > ESD_REPEAT_MAX)
    {
      esd_repeat_count = 0;
    }
    else
    {
      g_lcd_esd_timer.expires = get_jiffies_64() + msecs_to_jiffies(1000);
      add_timer(&g_lcd_esd_timer);
    }
  }
}

#endif

#endif
////////////////////////////////////////////////////////////////////////////////
static int __init s6d05a3_init(void)
{
    FBDBG_TRACE() ;
	return spi_register_driver(&s6d05a3_driver);
}

////////////////////////////////////////////////////////////////////////////////
static void __exit s6d05a3_exit(void)
{
    FBDBG_TRACE() ;
	spi_unregister_driver(&s6d05a3_driver);
}

module_init(s6d05a3_init);
module_exit(s6d05a3_exit);


MODULE_AUTHOR("SAMSUNG");
MODULE_DESCRIPTION("S6D05A3 driver");
MODULE_LICENSE("GPL");

