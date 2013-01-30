/*
 * LM347DF01 HVGA TFT LCD Panel Driver
*/

#include <linux/wait.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/lcd.h>
#include <linux/backlight.h>


#include <plat/gpio-cfg.h>
#include <mach/gpio-chief.h>
#include <mach/gpio-chief-settings.h>


#include "s3cfb.h"


#define LCD_CSX_HIGH	gpio_set_value(GPIO_DISPLAY_CS, GPIO_LEVEL_HIGH);
#define LCD_CSX_LOW	gpio_set_value(GPIO_DISPLAY_CS, GPIO_LEVEL_LOW);

#define LCD_SCL_HIGH	gpio_set_value(GPIO_DISPLAY_CLK, GPIO_LEVEL_HIGH);
#define LCD_SCL_LOW		gpio_set_value(GPIO_DISPLAY_CLK, GPIO_LEVEL_LOW);

#define LCD_SDI_HIGH	gpio_set_value(GPIO_DISPLAY_SI, GPIO_LEVEL_HIGH);
#define LCD_SDI_LOW		gpio_set_value(GPIO_DISPLAY_SI, GPIO_LEVEL_LOW);

#define LCD_SDO_READ	gpio_get_value(GPIO_DISPLAY_SO);


/* Sleep Out Command*/
#define LM347_SLPOUT		0x11

/* Power Setting Sequence*/
#define LM347_PSS1		0xEF
#define LM347_SETPWR	0xB1
#define LM347_SETDISP	0xB2
/* Initiliazing Sequence*/		
#define LM347_SETWAVEFORM	0xB4
#define LM347_SETVCOM		0xB6
#define LM347_SETASG		0xD5
#define LM347_SETMADCTL	0x36
#define LM347_SETPIXELFMT	0x3A
#define LM347_SETLSVREF	0xF2
#define LM347_SETPOL		0xF3
/* Gamma Setting Sequence*/		
#define LM347_GAMMARED	0xE2
#define LM347_GAMMAGREEN	0xE1
#define LM347_GAMMABLUE	0xE0
/* Display On*/
#define LM347_DISPON		0x29
/* Display Off*/
#define LM347_DISPOFF		0x28
/* SLEEP IN */
#define LM347_SLPIN			0x10

#define DEFAULT_USLEEP  10

struct setting_table_lmlcd {
	u8 command;	
	u8 parameters;
	u8 parameter[50];
	s32 wait;
};

#define LM347DF_POWER_ON_SETTINGS	(int)(sizeof(lm347df_power_on_setting_table)/sizeof(struct setting_table_lmlcd))
#define LM347DF_POWER_OFF_SETTINGS	(int)(sizeof(lm347df_power_off_setting_table)/sizeof(struct setting_table_lmlcd))
#define LM347DF_SLEEP_OUT_SETTINGS	(int)(sizeof(lm347df_sleep_out_setting_table)/sizeof(struct setting_table_lmlcd))



static struct setting_table_lmlcd lm347df_display_off_settings_table[] = {
	{ LM347_DISPOFF,   0, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0 },//Display off
};

static struct setting_table_lmlcd lm347df_display_on_settings_table[] = {
	{	LM347_DISPON,  	0, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 50 },//DISPON
};
static struct setting_table_lmlcd lm347df_power_off_setting_table[] = {
	{ LM347_DISPOFF,   0, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0 },//Display off
	{ LM347_SLPIN,     0, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 120 },//Sleep in
};

static struct setting_table_lmlcd lm347df_sleep_out_setting_table[] = {
	{	LM347_SLPOUT,  	0, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 10 },//SLPOUT
};

static struct setting_table_lmlcd lm347df_power_on_setting_table[] = {
		/*  Power Setting Sequence*/
	{	LM347_PSS1,  		2, { 0x74, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },	0 },
	{	LM347_SETPWR,  		9, { 0x01, 0x00, 0x22, 0x11, 0x73, 0x70, 0xEC, 0x15, 0x2C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },	0 },//SETPWR
	{	LM347_SETDISP,  		8, { 0x66, 0x06, 0xAA, 0x88, 0x88, 0x08, 0x08, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },	0 },//SETDISP
		/* Initiliazing Sequence*/		
	{	LM347_SETWAVEFORM,  	5, { 0x10, 0x00, 0x32, 0x32, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },	0 },//DISPWAVEFORM
	{	LM347_SETVCOM,  		8, { 0x74, 0x86, 0x22, 0x00, 0x22, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },	0 },//VCOM
	{	LM347_SETASG,  			3, { 0x02, 0x43, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },	0 },//ASG
	{	LM347_SETMADCTL, 		1, { 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0 },//MADCTL
	{	LM347_SETPIXELFMT, 	  	1, { 0x77, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0 },//PIXELFMT
	//{	LM347_SETLSVREF,  		6, { 0x00, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0 },//POL
	{	LM347_SETPOL, 	  		1, { 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0 },//POL
		/* Gamma Setting Sequence*/		
	{	LM347_GAMMARED, 	  	34, { 0x29, 0x2A, 0x2B, 0x0D, 0x0B, 0x03, 0x2D, 0x34, 0x07, 0x0D, 0x12, 0x18, 0x1A, 0x18, 0x19, 0x13, 0x18,
									 0x0F, 0x12, 0x14,	 0x0A, 0x09, 0x01, 0x1D, 0x2B, 0x07, 0x0C, 0x12, 0x17, 0x1A, 0x18, 0x18, 0x13, 0x18,},	0 },//GAMMARED		
	{	LM347_GAMMAGREEN,  		34, { 0x1D, 0x24, 0x2A, 0x0C, 0x07, 0x03, 0x31, 0x32, 0x08, 0x0F, 0x12, 0x17, 0x1B, 0x18, 0x19, 0x14, 0x19,
									 0x02, 0x0C, 0x12, 0x09, 0x09, 0x00, 0x20, 0x2E, 0x08, 0x0F, 0x12, 0x18, 0x19, 0x18, 0x18, 0x14, 0x1A},	0 },//GAMMAGREEN		
	{	LM347_GAMMABLUE,  		34, { 0x4B, 0x49, 0x47, 0x11, 0x0B, 0x02, 0x3C, 0x36, 0x06, 0x0E, 0x11, 0x15, 0x19, 0x17, 0x18, 0x10, 0x17, 
									 0x33, 0x33, 0x33, 0x11, 0x0C, 0x03, 0x2F, 0x2F, 0x07, 0x0F, 0x11, 0x16, 0x18, 0x17, 0x17, 0x0F, 0x16},	0 },//GAMMABLUE											
		/* Display On*/
	{	LM347_PSS1,	2, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },	0 },//PSS1 			
	{	LM347_DISPON,  	0, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 50 },//DISPON

};

static void setting_table_write(struct setting_table_lmlcd *table)
{
	s32 i, j;

	LCD_CSX_HIGH
	udelay(DEFAULT_USLEEP);
	LCD_SCL_HIGH 
	udelay(DEFAULT_USLEEP);

	/* Write Command */
	LCD_CSX_LOW
	udelay(DEFAULT_USLEEP);
	
	LCD_SCL_LOW 
	udelay(DEFAULT_USLEEP);		
	LCD_SDI_LOW 
	udelay(DEFAULT_USLEEP);
	LCD_SCL_HIGH 
	udelay(DEFAULT_USLEEP); 

   	for (i = 7; i >= 0; i--) { 
		LCD_SCL_LOW
		udelay(DEFAULT_USLEEP);
		if ((table->command >> i) & 0x1)
			LCD_SDI_HIGH
		else
			LCD_SDI_LOW
		udelay(DEFAULT_USLEEP);	
		LCD_SCL_HIGH
		udelay(DEFAULT_USLEEP);	
	}

	LCD_CSX_HIGH
	udelay(DEFAULT_USLEEP);	

	/* Write Parameter */
	if ((table->parameters) > 0) {
		for (j = 0; j < table->parameters; j++) {
			LCD_CSX_LOW 
			udelay(DEFAULT_USLEEP); 	
		
			LCD_SCL_LOW 
			udelay(DEFAULT_USLEEP); 	
			LCD_SDI_HIGH 
			udelay(DEFAULT_USLEEP);
			LCD_SCL_HIGH 
			udelay(DEFAULT_USLEEP); 	

			for (i = 7; i >= 0; i--) { 
				LCD_SCL_LOW
				udelay(DEFAULT_USLEEP);	
				if ((table->parameter[j] >> i) & 0x1)
					LCD_SDI_HIGH
				else
					LCD_SDI_LOW
				udelay(DEFAULT_USLEEP);	
				LCD_SCL_HIGH
				udelay(DEFAULT_USLEEP);					
			}
		
			LCD_CSX_HIGH 
			udelay(DEFAULT_USLEEP); 	
		}
	}
	
	msleep(table->wait);
}

u8 lm347df_readlcdid(void)
{
	s32 i, j;
	u8 readlcdidcmd = 0x04;
	u8 readlcdiddata[3] = {0x0,0x0,0x0};

	LCD_CSX_HIGH
	udelay(DEFAULT_USLEEP);
	LCD_SCL_HIGH 
	udelay(DEFAULT_USLEEP);

	/* Write Command */
	LCD_CSX_LOW
	udelay(DEFAULT_USLEEP);
	
	LCD_SCL_LOW 
	udelay(DEFAULT_USLEEP);		
	LCD_SDI_LOW 
	udelay(DEFAULT_USLEEP);
	LCD_SCL_HIGH 
	udelay(DEFAULT_USLEEP); 

	/*send LCD read id command 0x04*/
   	for (i = 7; i >= 0; i--) { 
		LCD_SCL_LOW
		udelay(DEFAULT_USLEEP);
		if ((readlcdidcmd >> i) & 0x1)
			LCD_SDI_HIGH
		else
			LCD_SDI_LOW
		udelay(DEFAULT_USLEEP);	
		LCD_SCL_HIGH
		udelay(DEFAULT_USLEEP);	
	}

       /* Read LCD ID data 3 bytes*/
	LCD_SCL_LOW				
	udelay(DEFAULT_USLEEP);
	   
   	for (i = 0; i <= 2; i++) { 
	   	for (j = 7; j >= 0; j--) {
			u8 data = 0;
			LCD_SCL_HIGH
			udelay(DEFAULT_USLEEP);
			data = LCD_SDO_READ
			readlcdiddata[i] |= ((data ? 1: 0) << j );
			LCD_SCL_LOW				
			udelay(DEFAULT_USLEEP);
	   	}
		udelay(DEFAULT_USLEEP);	
	}	

	LCD_CSX_HIGH

	msleep(10);

	printk("readlcdiddata[0] = %x, readlcdiddata[1] = %x, readlcdiddata[2] = %x \n",readlcdiddata[0],readlcdiddata[1],readlcdiddata[2] );

	if((readlcdiddata[0] == 0x6B) && (readlcdiddata[1] == 0x88) && (readlcdiddata[2] == 0x40))
		return 1;
	else
		return 0;
}

void lm347df_display_off_sequence(void)
{
 
  struct setting_table_lmlcd *p_display_off_set_tbl = lm347df_display_off_settings_table;

  printk("lm347df_start_dispayoff_sequence \n");
  /* Send display off*/
  setting_table_write(&p_display_off_set_tbl[0]);
}

void lm347df_display_on_sequence(void)
{
 
  struct setting_table_lmlcd *p_display_on_set_tbl = lm347df_display_on_settings_table;

  printk("lm347df_start_dispayon_sequence \n");
  /* Send display on*/
  setting_table_write(&p_display_on_set_tbl[0]);
}

void lm347df_start_poweron_sequence(void)
{
  int i;
  int tblsize;
  struct setting_table_lmlcd *p_power_set_tbl = lm347df_power_on_setting_table;
  struct setting_table_lmlcd *p_sleepout_set_tbl = lm347df_sleep_out_setting_table;
  struct setting_table_lmlcd *p_poweroff_set_tbl = lm347df_power_off_setting_table;  
  u8 bIslm347df = 0;

  printk("lm347df_start_poweron_sequence \n");

  /* Send Sleep Out Command*/
  setting_table_write(&p_sleepout_set_tbl[0]);
  
  //bIslm347df = lm347df_readlcdid();

  tblsize = LM347DF_POWER_ON_SETTINGS;
  
  for (i = 0; i < tblsize; i++)
    setting_table_write(&p_power_set_tbl[i]);

  //msleep(50); // stable time for another command
  
}

void lm347df_start_poweroff_sequence(void)
{
	int i = 0;
	int tblsize;
	struct setting_table_lmlcd *p_poweroff_set_tbl = lm347df_power_off_setting_table;  
	 
	tblsize = LM347DF_POWER_OFF_SETTINGS;
	for (i = 0; i < tblsize; i++)
		setting_table_write(&p_poweroff_set_tbl[i]);
	
}

