/*
 * kernel_sec_debug.c
 *
 * Exception handling in kernel by SEC
 *
 * Copyright (c) 2010 Samsung Electronics
 *                http://www.samsung.com/
 *
 */

#ifdef CONFIG_KERNEL_DEBUG_SEC

#include <linux/kernel_sec_common.h>
#include <asm/cacheflush.h>
#include <linux/errno.h>
#include <linux/ctype.h>
#include <linux/vmalloc.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#include <linux/file.h>
#include <mach/hardware.h>
#include <mach/map.h>
#ifdef CONFIG_TIKAL_MPCS
#include <linux/seq_file.h>
#include <mach/param.h>
#endif

/*
 *  Variable
 */

const char *gkernel_sec_build_info_date_time[] = {
	__DATE__,
	__TIME__
};

#define DEBUG_LEVEL_FILE_NAME	"/mnt/.lfs/debug_level.inf"
#define DEBUG_LEVEL_RD	0
#define DEBUG_LEVEL_WR	1
static int debuglevel;

sched_log_t gExcpTaskLog[SCHED_LOG_MAX];
unsigned int gExcpTaskLogIdx;

#ifndef CONFIG_TIKAL_MPCS
typedef enum {
	__SERIAL_SPEED,
	__LOAD_RAMDISK,
	__BOOT_DELAY,
	__LCD_LEVEL,
	__SWITCH_SEL,
	__PHONE_DEBUG_ON,
	__LCD_DIM_LEVEL,
	__MELODY_MODE,
	__REBOOT_MODE,
	__NATION_SEL,
	__SET_DEFAULT_PARAM,
	__PARAM_INT_11,
	__PARAM_INT_12,
	__PARAM_INT_13,
	__PARAM_INT_14,
	__VERSION,
	__CMDLINE,
	__PARAM_STR_2,
	__PARAM_STR_3,
	__PARAM_STR_4
} param_idx;
#endif

char gkernel_sec_build_info[100];
extern unsigned int HWREV;
unsigned char kernel_sec_cause_str[KERNEL_SEC_DEBUG_CAUSE_STR_LEN];

/*
 *  Function
 */

void __iomem *kernel_sec_viraddr_wdt_reset_reg;
__used t_kernel_sec_arm_core_regsiters kernel_sec_core_reg_dump;
__used t_kernel_sec_mmu_info           kernel_sec_mmu_reg_dump;
__used kernel_sec_upload_cause_type     gkernel_sec_upload_cause;

#ifdef CONFIG_MACH_CHIEF  // CP_DPRAM_MODE
static volatile void __iomem *idpram_base;      // Base of internal DPRAM

static volatile void __iomem *onedram_base = NULL;     // Base of OneDRAM
volatile unsigned int *onedram_sem;
/* received mail */
volatile unsigned int *onedram_mailboxAB;
/* send mail */
volatile unsigned int *onedram_mailboxBA;

#define IDPRAM_PHYSICAL_ADDR        0xED000000
#else


volatile void __iomem *dpram_base = 0;
/* received mail */
volatile unsigned short *dpram_mailboxAB;
/* send mail */
volatile unsigned short *dpram_mailboxBA;
#endif
unsigned int received_cp_ack = 0;

extern void (*sec_set_param_value)(int idx, void *value);
extern void (*sec_get_param_value)(int idx, void *value);

#ifdef CONFIG_MACH_CHIEF  // CP_DPRAM_MODE
/*
 * assigned 16K internal dpram buf for debugging
 */

#define DPRAM_BUF_SIZE 0x4000
struct _idpram_buf {
    unsigned int  dpram_start_key1;
    unsigned int  dpram_start_key2;
    unsigned char dpram_buf[DPRAM_BUF_SIZE];
    unsigned int  dpram_end_key;
} g_cdma_dpram_buf = {
    .dpram_start_key1 = 'RPD@',
    .dpram_start_key2 = 'AMDC',
    .dpram_buf[0] = 'N',
    .dpram_buf[1] = 'O',
    .dpram_buf[2] = 'N',
    .dpram_buf[3] = 'E',
    .dpram_end_key='DNE@'
};

void kernel_sec_cdma_dpram_dump(void)
{
    printk(KERN_EMERG "Backup CDMA dpram to RAM refore upload\n");
    memcpy(g_cdma_dpram_buf.dpram_buf, idpram_base, DPRAM_BUF_SIZE);
    printk(KERN_EMERG "buf address (0x%x), dpram (0x%x)\n", g_cdma_dpram_buf.dpram_buf, idpram_base );
}
EXPORT_SYMBOL(kernel_sec_cdma_dpram_dump);
#endif

void kernel_sec_set_cp_upload(void)
{
	unsigned int send_mail, wait_count;

#ifdef CONFIG_MACH_CHIEF // CP_DPRAM_MODE
    static volatile u16 *cp_dpram_mbx_BA;      //send mail box    
    static volatile u16 *cp_dpram_mbx_AB;      //receive mail box
    u16 cp_irq_mask;

	*((unsigned short *)idpram_base)=0x554C;
    
    cp_dpram_mbx_BA = (volatile u16*)(idpram_base + 0x3FFC);
    cp_dpram_mbx_AB = (volatile u16*)(idpram_base + 0x3FFE);
    cp_irq_mask = 0xCF; //0x80|0x40|0x0F;

#ifdef CDMA_IPC_C111_IDPRAM
    iowrite16(cp_irq_mask, (void *)cp_dpram_mbx_BA);
#else
    *cp_dpram_mbx_BA = cp_irq_mask;
#endif

    printk(KERN_EMERG"[kernel_sec_dump_set_cp_upload] set cp upload mode, MailboxBA 0x%8x\n", cp_irq_mask);

    wait_count = 0;

    while(1)
    {
        cp_irq_mask = ioread16((void *)cp_dpram_mbx_AB);

        if(cp_irq_mask == 0xCF)
        {
            printk(KERN_EMERG"  - Done. cp_irq_mask: 0x%04X\n", cp_irq_mask);
            break;
        }
        mdelay(10);
        if(++wait_count > 2000)
        {
            printk(KERN_EMERG"  - Fail to set CP uploadmode. cp_irq_mask: 0x%04X\n", cp_irq_mask);
            break;
        }
    }
    printk(KERN_EMERG" modem_wait_count : %d \n", wait_count); 

#else
	send_mail = KERNEL_SEC_DUMP_AP_DEAD_INDICATOR_DPRAM;

	*dpram_mailboxBA = send_mail;

	printk(KERN_EMERG"[kernel_sec_dump_set_cp_upload] set cp upload mode, MailboxBA 0x%8x\n", send_mail);

	wait_count = 0;
	received_cp_ack = 0;
	while (1) {
		//if (received_cp_ack == 1 || *dpram_mailboxAB == KERNEL_SEC_DUMP_AP_DEAD_ACK) {
		if (received_cp_ack == 1 || *dpram_mailboxAB == 0xc6) {
			printk(KERN_EMERG"  - Done.\n");
			break;
		}
		mdelay(10);
		if (++wait_count > 2000) {
			printk(KERN_EMERG"  - Fail to set CP uploadmode.\n");
			break;
		}	
	}
	printk(KERN_EMERG" modem_wait_count : %d \n", wait_count);
#endif

#ifdef CONFIG_MACH_CHIEF  // CP_DPRAM_MODE
	/*
	 *  QSC6085 marking the QSC upload mode
	 */
	*((unsigned int *)idpram_base)=0xdeaddead;
	printk(KERN_EMERG" QSC upload magic key write \n");
	kernel_sec_cdma_dpram_dump();
#endif
}
EXPORT_SYMBOL(kernel_sec_set_cp_upload);


void kernel_sec_set_cp_ack(void)   /*is set by dpram - dpram_irq_handler*/
{
	received_cp_ack = 1;
}
EXPORT_SYMBOL(kernel_sec_set_cp_ack);

void kernel_sec_set_upload_magic_number(void)
{
	int *magic_virt_addr = (int *)LOKE_BOOT_USB_DWNLD_V_ADDR;

	if ((KERNEL_SEC_DEBUG_LEVEL_MID == kernel_sec_get_debug_level()) || 
			(KERNEL_SEC_DEBUG_LEVEL_HIGH == kernel_sec_get_debug_level())) {
		*magic_virt_addr = LOKE_BOOT_USB_DWNLDMAGIC_NO; /* SET */
		printk(KERN_EMERG"KERNEL:magic_number=0x%x SET_UPLOAD_MAGIC_NUMBER\n",*magic_virt_addr);
	} else {
		*magic_virt_addr = 0;	
		printk(KERN_EMERG"KERNEL:magic_number=0x%x DEBUG LEVEL low!!\n",*magic_virt_addr);
	}

}
EXPORT_SYMBOL(kernel_sec_set_upload_magic_number);


void kernel_sec_get_debug_level_from_boot(void)
{
	unsigned int temp;
	temp = __raw_readl(S5P_INFORM6);
	temp &= KERNEL_SEC_DEBUG_LEVEL_MASK;
	temp = temp >> KERNEL_SEC_DEBUG_LEVEL_BIT;

	if (temp == 0x0)  /*low*/
		debuglevel = KERNEL_SEC_DEBUG_LEVEL_LOW;
	else if (temp == 0x1)  /*mid*/
		debuglevel = KERNEL_SEC_DEBUG_LEVEL_MID;
	else if (temp == 0x2)  /*high*/
		debuglevel = KERNEL_SEC_DEBUG_LEVEL_HIGH;
	else {
		printk(KERN_EMERG"KERNEL:kernel_sec_get_debug_level_from_boot (reg value is incorrect.)\n");
		/*debuglevel = KERNEL_SEC_DEBUG_LEVEL_LOW;*/
		debuglevel = KERNEL_SEC_DEBUG_LEVEL_MID;
	}

	printk(KERN_EMERG"KERNEL:kernel_sec_get_debug_level_from_boot=0x%x\n",debuglevel);
}


void kernel_sec_clear_upload_magic_number(void)
{
	int *magic_virt_addr = (int *)LOKE_BOOT_USB_DWNLD_V_ADDR;

	*magic_virt_addr = 0;  /* CLEAR */
	printk(KERN_EMERG"KERNEL:magic_number=%x CLEAR_UPLOAD_MAGIC_NUMBER\n",*magic_virt_addr);
}
EXPORT_SYMBOL(kernel_sec_clear_upload_magic_number);

void kernel_sec_map_wdog_reg(void)
{
	/* Virtual Mapping of Watchdog register */
	kernel_sec_viraddr_wdt_reset_reg = ioremap_nocache(S3C_PA_WDT,0x400);

	if (kernel_sec_viraddr_wdt_reset_reg == NULL) {
		printk(KERN_EMERG"Failed to ioremap() region in forced upload keystring\n");
	}
}
EXPORT_SYMBOL(kernel_sec_map_wdog_reg);

void kernel_sec_set_upload_cause(kernel_sec_upload_cause_type uploadType)
{
	unsigned int temp;
	gkernel_sec_upload_cause=uploadType;

	temp = __raw_readl(S5P_INFORM6);
	/* KERNEL_SEC_UPLOAD_CAUSE_MASK    0x000000FF */
	temp |= uploadType;
	__raw_writel( temp , S5P_INFORM6);

	printk(KERN_EMERG"(kernel_sec_set_upload_cause) : upload_cause set %x\n",uploadType);	
}
EXPORT_SYMBOL(kernel_sec_set_upload_cause);

void kernel_sec_set_cause_strptr(unsigned char* str_ptr, int size)
{
	unsigned int temp;

	memset((void *)kernel_sec_cause_str, 0,
			sizeof(kernel_sec_cause_str));
	memcpy(kernel_sec_cause_str, str_ptr, size);

	temp = virt_to_phys(kernel_sec_cause_str);
	/*loke read this ptr, display_aries_upload_image*/
	__raw_writel(temp, LOKE_BOOT_USB_DWNLD_V_ADDR + 4);
}
EXPORT_SYMBOL(kernel_sec_set_cause_strptr);


void kernel_sec_set_autotest(void)
{
	unsigned int temp;

	temp = __raw_readl(S5P_INFORM6);
	temp |= 1 << KERNEL_SEC_UPLOAD_AUTOTEST_BIT;
	__raw_writel(temp, S5P_INFORM6);	
}
EXPORT_SYMBOL(kernel_sec_set_autotest);

void kernel_sec_set_build_info(void)
{
	char *p = gkernel_sec_build_info;
	sprintf(p,"ARIES_BUILD_INFO: HWREV: %x", HWREV);
	strcat(p, " Date:");
	strcat(p, gkernel_sec_build_info_date_time[0]);
	strcat(p, " Time:");
	strcat(p, gkernel_sec_build_info_date_time[1]);
}
EXPORT_SYMBOL(kernel_sec_set_build_info);

void kernel_sec_init(void)
{
	/* set the onedram mailbox virtual address */
#ifdef CONFIG_MACH_CHIEF // CP_DPRAM_MODE
	idpram_base = (volatile void *)ioremap_nocache(IDPRAM_PHYSICAL_ADDR, 0x4000);
	if ( idpram_base == NULL )
	{
		printk("failed ioremap g_idpram_region\n");
	}
#else	
	/*DPRAM_START_ADDRESS_PHYS + DPRAM_SHARED_BANK + DPRAM_SMP*/
#ifdef CONFIG_MACH_VIPER
	dpram_base = ioremap_nocache(0xED000000, 0x4000);
#else
	dpram_base = ioremap_nocache(0x98000000, 0x4000);
#endif /* CONFIG_MACH_VIPER */

	if (dpram_base == NULL) {
		printk(KERN_EMERG"failed ioremap\n");
	}
	printk(KERN_ERR "[jinmo] dpram_base : %p\n", dpram_base);
#ifdef CONFIG_MACH_VIPER
	dpram_mailboxAB = dpram_base + 0x3FFE;
	dpram_mailboxBA = dpram_base + 0x3FFC;
#else    
	dpram_mailboxAB = dpram_base + 0x3FFC;
	dpram_mailboxBA = dpram_base + 0x3FFE;
#endif    /* CONFIG_MACH_VIPER */
#endif
	kernel_sec_get_debug_level_from_boot();
	kernel_sec_set_upload_magic_number();
	kernel_sec_set_upload_cause(UPLOAD_CAUSE_INIT);	
	kernel_sec_map_wdog_reg();
}
EXPORT_SYMBOL(kernel_sec_init);

/* core reg dump function*/
void kernel_sec_get_core_reg_dump(t_kernel_sec_arm_core_regsiters* regs)
{
	/* we will be in SVC mode when we enter this function. Collect SVC registers along with cmn registers */
	asm(
			"str r0, [%0,#0] \n\t"		// R0
			"str r1, [%0,#4] \n\t"		// R1
			"str r2, [%0,#8] \n\t"		// R2
			"str r3, [%0,#12] \n\t"		// R3
			"str r4, [%0,#16] \n\t"		// R4
			"str r5, [%0,#20] \n\t"		// R5
			"str r6, [%0,#24] \n\t"		// R6
			"str r7, [%0,#28] \n\t"		// R7
			"str r8, [%0,#32] \n\t"		// R8
			"str r9, [%0,#36] \n\t"		// R9
			"str r10, [%0,#40] \n\t"	// R10
			"str r11, [%0,#44] \n\t"	// R11
			"str r12, [%0,#48] \n\t"	// R12

			/* SVC */
			"str r13, [%0,#52] \n\t"	// R13_SVC
			"str r14, [%0,#56] \n\t"	// R14_SVC
			"mrs r1, spsr \n\t"			// SPSR_SVC
			"str r1, [%0,#60] \n\t"

			/* PC and CPSR */
			"sub r1, r15, #0x4 \n\t"	// PC
			"str r1, [%0,#64] \n\t"	
			"mrs r1, cpsr \n\t"			// CPSR
			"str r1, [%0,#68] \n\t"

			/* SYS/USR */
			"mrs r1, cpsr \n\t"			// switch to SYS mode
			"and r1, r1, #0xFFFFFFE0\n\t"
			"orr r1, r1, #0x1f \n\t"
			"msr cpsr,r1 \n\t"

			"str r13, [%0,#72] \n\t"	// R13_USR
			"str r14, [%0,#76] \n\t"	// R13_USR

			/*FIQ*/
			"mrs r1, cpsr \n\t"			// switch to FIQ mode
			"and r1,r1,#0xFFFFFFE0\n\t"
			"orr r1,r1,#0x11\n\t"
			"msr cpsr,r1 \n\t"

			"str r8, [%0,#80] \n\t"		// R8_FIQ
			"str r9, [%0,#84] \n\t"		// R9_FIQ
			"str r10, [%0,#88] \n\t"	// R10_FIQ
			"str r11, [%0,#92] \n\t"	// R11_FIQ
			"str r12, [%0,#96] \n\t"	// R12_FIQ
			"str r13, [%0,#100] \n\t"	// R13_FIQ
			"str r14, [%0,#104] \n\t"	// R14_FIQ
			"mrs r1, spsr \n\t"			// SPSR_FIQ
			"str r1, [%0,#108] \n\t"

			/*IRQ*/
			"mrs r1, cpsr \n\t"			// switch to IRQ mode
			"and r1, r1, #0xFFFFFFE0\n\t"
			"orr r1, r1, #0x12\n\t"
			"msr cpsr,r1 \n\t"

			"str r13, [%0,#112] \n\t"	// R13_IRQ
			"str r14, [%0,#116] \n\t"	// R14_IRQ
			"mrs r1, spsr \n\t"			// SPSR_IRQ
			"str r1, [%0,#120] \n\t"

			/*MON*/
			"mrs r1, cpsr \n\t"			// switch to monitor mode
			"and r1, r1, #0xFFFFFFE0\n\t"
			"orr r1, r1, #0x16\n\t"
			"msr cpsr,r1 \n\t"

			"str r13, [%0,#124] \n\t"	// R13_MON
			"str r14, [%0,#128] \n\t"	// R14_MON
			"mrs r1, spsr \n\t"			// SPSR_MON
			"str r1, [%0,#132] \n\t"

			/*ABT*/
			"mrs r1, cpsr \n\t"			// switch to Abort mode
			"and r1, r1, #0xFFFFFFE0\n\t"
			"orr r1, r1, #0x17\n\t"
			"msr cpsr,r1 \n\t"

			"str r13, [%0,#136] \n\t"	// R13_ABT
			"str r14, [%0,#140] \n\t"	// R14_ABT
			"mrs r1, spsr \n\t"			// SPSR_ABT
			"str r1, [%0,#144] \n\t"

			/*UND*/
			"mrs r1, cpsr \n\t"			// switch to undef mode
			"and r1, r1, #0xFFFFFFE0\n\t"
			"orr r1, r1, #0x1B\n\t"
			"msr cpsr,r1 \n\t"

			"str r13, [%0,#148] \n\t"	// R13_UND
			"str r14, [%0,#152] \n\t"	// R14_UND
			"mrs r1, spsr \n\t"			// SPSR_UND
			"str r1, [%0,#156] \n\t"

			/* restore to SVC mode */
			"mrs r1, cpsr \n\t"			// switch to undef mode
			"and r1, r1, #0xFFFFFFE0\n\t"
			"orr r1, r1, #0x13\n\t"
			"msr cpsr,r1 \n\t"

			:				/* output */
			:"r"(regs)    	/* input */
			:"%r1"     		/* clobbered register */
			   );

	return;	
}
EXPORT_SYMBOL(kernel_sec_get_core_reg_dump);

int kernel_sec_get_mmu_reg_dump(t_kernel_sec_mmu_info *mmu_info)
{
	asm("mrc    p15, 0, r1, c1, c0, 0 \n\t"	//SCTLR
			"str r1, [%0] \n\t"
			"mrc    p15, 0, r1, c2, c0, 0 \n\t"	//TTBR0
			"str r1, [%0,#4] \n\t"
			"mrc    p15, 0, r1, c2, c0,1 \n\t"	//TTBR1
			"str r1, [%0,#8] \n\t"
			"mrc    p15, 0, r1, c2, c0,2 \n\t"	//TTBCR
			"str r1, [%0,#12] \n\t"
			"mrc    p15, 0, r1, c3, c0,0 \n\t"	//DACR
			"str r1, [%0,#16] \n\t"
			"mrc    p15, 0, r1, c5, c0,0 \n\t"	//DFSR
			"str r1, [%0,#20] \n\t"
			"mrc    p15, 0, r1, c6, c0,0 \n\t"	//DFAR
			"str r1, [%0,#24] \n\t"
			"mrc    p15, 0, r1, c5, c0,1 \n\t"	//IFSR
			"str r1, [%0,#28] \n\t"
			"mrc    p15, 0, r1, c6, c0,2 \n\t"	//IFAR
			"str r1, [%0,#32] \n\t"
			/*Dont populate DAFSR and RAFSR*/
			"mrc    p15, 0, r1, c10, c2,0 \n\t"	//PMRRR
			"str r1, [%0,#44] \n\t"
			"mrc    p15, 0, r1, c10, c2,1 \n\t"	//NMRRR
			"str r1, [%0,#48] \n\t"
			"mrc    p15, 0, r1, c13, c0,0 \n\t"	//FCSEPID
			"str r1, [%0,#52] \n\t"
			"mrc    p15, 0, r1, c13, c0,1 \n\t"	//CONTEXT
			"str r1, [%0,#56] \n\t"
			"mrc    p15, 0, r1, c13, c0,2 \n\t"	//URWTPID
			"str r1, [%0,#60] \n\t"
			"mrc    p15, 0, r1, c13, c0,3 \n\t"	//UROTPID
			"str r1, [%0,#64] \n\t"
			"mrc    p15, 0, r1, c13, c0,4 \n\t"	//POTPIDR
			"str r1, [%0,#68] \n\t"
			:					/* output */
			:"r"(mmu_info)    /* input */
			:"%r1","memory"         /* clobbered register */
			   ); 
	return 0;
}
EXPORT_SYMBOL(kernel_sec_get_mmu_reg_dump);

void kernel_sec_save_final_context(void)
{
	if (kernel_sec_get_mmu_reg_dump(&kernel_sec_mmu_reg_dump) < 0) {
		printk(KERN_EMERG"(kernel_sec_save_final_context) kernel_sec_get_mmu_reg_dump faile.\n");
	}
	kernel_sec_get_core_reg_dump(&kernel_sec_core_reg_dump);

	printk(KERN_EMERG "(kernel_sec_save_final_context) Final context was saved before the system reset.\n");
}
EXPORT_SYMBOL(kernel_sec_save_final_context);

/*
 *  bSilentReset
 *    TRUE  : Silent reset - clear the magic code.
 *    FALSE : Reset to upload mode - not clear the magic code.
 *
 *  TODO : DebugLevel consideration should be added.
 */
/*extern void Ap_Cp_Switch_Config(u16 ap_cp_mode);*/
void kernel_sec_hw_reset(bool bSilentReset)
{
	/*Ap_Cp_Switch_Config(0);*/

	if (bSilentReset || (KERNEL_SEC_DEBUG_LEVEL_LOW == kernel_sec_get_debug_level())) {
		kernel_sec_clear_upload_magic_number();
		printk(KERN_EMERG "(kernel_sec_hw_reset) Upload Magic Code is cleared for silet reset.\n");
	}

	printk(KERN_EMERG "(kernel_sec_hw_reset) %s\n", gkernel_sec_build_info);

	printk(KERN_EMERG "(kernel_sec_hw_reset) The forced reset was called. The system will be reset !!\n");

	/* flush cache back to ram */
	flush_cache_all();

	__raw_writel(0x8000, kernel_sec_viraddr_wdt_reset_reg + 0x4);
	__raw_writel(0x1,    kernel_sec_viraddr_wdt_reset_reg + 0x4);
	__raw_writel(0x8,    kernel_sec_viraddr_wdt_reset_reg + 0x8);
	__raw_writel(0x8021, kernel_sec_viraddr_wdt_reset_reg);

	/* Never happened because the reset will occur before this. */
	while(1);	
}
EXPORT_SYMBOL(kernel_sec_hw_reset);


bool kernel_set_debug_level(int level)
{
	if (sec_set_param_value) {
		if ((level == KERNEL_SEC_DEBUG_LEVEL_LOW) ||
				(level == KERNEL_SEC_DEBUG_LEVEL_MID)) {
			sec_set_param_value(__PHONE_DEBUG_ON, (void*)&level);
			printk(KERN_NOTICE "(kernel_set_debug_level) The debug value is %x !!\n", level);	
			return 1;
		} else {
			printk(KERN_NOTICE "(kernel_set_debug_level) The debug value is invalid (%x) !!\n", level);	
			return 0;
		}
	} else {
		printk(KERN_NOTICE "(kernel_set_debug_level) sec_set_param_value is not intialized !!\n");
		return 0;
	}
}
EXPORT_SYMBOL(kernel_set_debug_level);

int kernel_get_debug_level()
{
	int debug_level = -1;

	if (sec_get_param_value)
		sec_get_param_value(__PHONE_DEBUG_ON, &debug_level);

	if ((debug_level == KERNEL_SEC_DEBUG_LEVEL_LOW) ||
			(debug_level == KERNEL_SEC_DEBUG_LEVEL_MID)) {
		printk(KERN_NOTICE "(kernel_get_debug_level) kernel debug level is %x !!\n", debug_level);
		return debug_level;
	}
	printk(KERN_NOTICE "(kernel_get_debug_level) kernel debug level is invalid (%x) !!\n", debug_level);
	return debug_level;
}
EXPORT_SYMBOL(kernel_get_debug_level);

int kernel_sec_lfs_debug_level_op(int dir, int flags)
{
	struct file *filp;
	mm_segment_t fs;

	int ret;

	filp = filp_open(DEBUG_LEVEL_FILE_NAME, flags, 0);

	if (IS_ERR(filp)) {
		pr_err("%s: filp_open failed. (%ld)\n", __FUNCTION__,
				PTR_ERR(filp));

		return -1;
	}

	fs = get_fs();
	set_fs(get_ds());

	if (dir == DEBUG_LEVEL_RD)
		ret = filp->f_op->read(filp, (char __user *)&debuglevel,
				sizeof(int), &filp->f_pos);
	else
		ret = filp->f_op->write(filp, (char __user *)&debuglevel,
				sizeof(int), &filp->f_pos);

	set_fs(fs);
	filp_close(filp, NULL);

	return ret;
}

bool kernel_sec_set_debug_level(int level)
{
	int ret;

	if((level == KERNEL_SEC_DEBUG_LEVEL_LOW) ||
			(level == KERNEL_SEC_DEBUG_LEVEL_MID) ||
			(level == KERNEL_SEC_DEBUG_LEVEL_HIGH)) {
		debuglevel = level;
		/* write to param.lfs */
		ret = kernel_sec_lfs_debug_level_op(DEBUG_LEVEL_WR,
				O_RDWR|O_SYNC);

		if (ret == sizeof(debuglevel)) {
			pr_info("%s: debuglevel.inf write successfully.\n",
					__FUNCTION__);
		}
		/* write to regiter (magic code) */
		kernel_sec_set_upload_magic_number();

		printk(KERN_NOTICE "(kernel_sec_set_debug_level) The debug value is 0x%x !!\n", level);	
		return 1;
	} else {
		printk(KERN_NOTICE "(kernel_sec_set_debug_level) The debug value is \
				invalid(0x%x)!! Set default level(LOW)\n", level);
			debuglevel = KERNEL_SEC_DEBUG_LEVEL_LOW;
		return 0;
	}
}
EXPORT_SYMBOL(kernel_sec_set_debug_level);

int kernel_sec_get_debug_level_from_param()
{
	int ret;

	/* read from param.lfs*/
	ret = kernel_sec_lfs_debug_level_op(DEBUG_LEVEL_RD, O_RDONLY);

	if (ret == sizeof(debuglevel))
		pr_info("%s: debuglevel.inf read successfully.\n", __FUNCTION__);

	if ((debuglevel == KERNEL_SEC_DEBUG_LEVEL_LOW) ||
			(debuglevel == KERNEL_SEC_DEBUG_LEVEL_MID) ||
			(debuglevel == KERNEL_SEC_DEBUG_LEVEL_HIGH)) {
		/* return debug level */
		printk(KERN_NOTICE "(kernel_sec_get_debug_level_from_param) kernel debug level is 0x%x !!\n", debuglevel);
		return debuglevel;
	} else {
		/*In case of invalid debug level, default (debug level low)*/
		printk(KERN_NOTICE "(kernel_sec_get_debug_level_from_param) The debug value is \
				invalid(0x%x)!! Set default level(LOW)\n", debuglevel);	
			/*debuglevel = KERNEL_SEC_DEBUG_LEVEL_LOW;*/
			debuglevel = KERNEL_SEC_DEBUG_LEVEL_MID;
	}
	return debuglevel;
}
EXPORT_SYMBOL(kernel_sec_get_debug_level_from_param);

int kernel_sec_get_debug_level()
{
	return debuglevel;
}
EXPORT_SYMBOL(kernel_sec_get_debug_level);

int kernel_sec_check_debug_level_high(void)
{
	if (KERNEL_SEC_DEBUG_LEVEL_HIGH == kernel_sec_get_debug_level())
		return 1;
	return 0;
}
EXPORT_SYMBOL(kernel_sec_check_debug_level_high);

//{{ GAF3.0 debug information for forced RAM dump
extern struct GAForensicINFO GAFINFO;
static void dump_one_task_info( struct task_struct *tsk, bool isMain )
{
    char stat_array[3] = {'R', 'S', 'D'};
    char stat_ch;
    char *pThInf = tsk->stack;

    stat_ch = tsk->state <= TASK_UNINTERRUPTIBLE ? stat_array[tsk->state] : '?';
    printk( KERN_INFO "%8d  %8d  %8d     %16lld      %c (%d)    %3d     %08x     %c %s\n",
          tsk->pid, (int)(tsk->utime), (int)(tsk->stime), tsk->se.exec_start, stat_ch, (int)(tsk->state),
          *(int*)(pThInf + GAFINFO.thread_info_struct_cpu),
          (int)tsk, isMain?'*':' ', tsk->comm );

    if( tsk->state == TASK_RUNNING || tsk->state == TASK_UNINTERRUPTIBLE ) {
	    show_stack( tsk, NULL );
    }
}

static void dump_all_task_info(void)
{
    struct task_struct *frst_tsk;
    struct task_struct *curr_tsk;
    struct task_struct *frst_thr;
    struct task_struct *curr_thr;

    printk( KERN_INFO "\n" );
    printk( KERN_INFO " current proc : %d %s\n", current->pid, current->comm );
    printk( KERN_INFO " -----------------------------------------------------------------------------------\n" );
    printk( KERN_INFO "     pid     uTime     sTime              exec(ns)     stat     cpu     task_struct\n" );
    printk( KERN_INFO " -----------------------------------------------------------------------------------\n" );

    //processes
    frst_tsk = &init_task;
    curr_tsk = frst_tsk;
    while( curr_tsk != NULL ) {
    	dump_one_task_info( curr_tsk,  true );
        //threads
        if( curr_tsk->thread_group.next != NULL ) {
        	frst_thr = container_of( curr_tsk->thread_group.next, struct task_struct, thread_group );
        	curr_thr = frst_thr;
            if( frst_thr != curr_tsk ) {
            	while( curr_thr != NULL ) {
            	    dump_one_task_info( curr_thr, false );
            	    curr_thr = container_of( curr_thr->thread_group.next, struct task_struct, thread_group );
                    if( curr_thr == curr_tsk ) break;
            	}
            }
        }
    	curr_tsk = container_of( curr_tsk->tasks.next, struct task_struct, tasks );
        if( curr_tsk == frst_tsk ) break;
    }

    printk( KERN_INFO " -----------------------------------------------------------------------------------\n" );
}


#include <linux/kernel_stat.h>

#ifndef arch_irq_stat_cpu
#define arch_irq_stat_cpu(cpu) 0
#endif
#ifndef arch_irq_stat
#define arch_irq_stat() 0
#endif
#ifndef arch_idle_time
#define arch_idle_time(cpu) 0
#endif

static void dump_cpu_stat(void)
{
	int i, j;
	unsigned long jif;
	cputime64_t user, nice, system, idle, iowait, irq, softirq, steal;
	cputime64_t guest, guest_nice;
	u64 sum = 0;
	u64 sum_softirq = 0;
	unsigned int per_softirq_sums[NR_SOFTIRQS] = {0};
	struct timespec boottime;
	unsigned int per_irq_sum;

	user = nice = system = idle = iowait =
		irq = softirq = steal = cputime64_zero;
	guest = guest_nice = cputime64_zero;
	getboottime(&boottime);
	jif = boottime.tv_sec;

	for_each_possible_cpu(i) {
		user = cputime64_add(user, kstat_cpu(i).cpustat.user);
		nice = cputime64_add(nice, kstat_cpu(i).cpustat.nice);
		system = cputime64_add(system, kstat_cpu(i).cpustat.system);
		idle = cputime64_add(idle, kstat_cpu(i).cpustat.idle);
		idle = cputime64_add(idle, arch_idle_time(i));
		iowait = cputime64_add(iowait, kstat_cpu(i).cpustat.iowait);
		irq = cputime64_add(irq, kstat_cpu(i).cpustat.irq);
		softirq = cputime64_add(softirq, kstat_cpu(i).cpustat.softirq);
		//steal = cputime64_add(steal, kstat_cpu(i).cpustat.steal);
		//guest = cputime64_add(guest, kstat_cpu(i).cpustat.guest);
		//guest_nice = cputime64_add(guest_nice,
		//	kstat_cpu(i).cpustat.guest_nice);
		for_each_irq_nr(j) {
			sum += kstat_irqs_cpu(j, i);
		}
		sum += arch_irq_stat_cpu(i);

		for (j = 0; j < NR_SOFTIRQS; j++) {
			unsigned int softirq_stat = kstat_softirqs_cpu(j, i);

			per_softirq_sums[j] += softirq_stat;
			sum_softirq += softirq_stat;
		}
	}
	sum += arch_irq_stat();

	printk(KERN_INFO "\n");
	printk(KERN_INFO " cpu     user:%llu  nice:%llu  system:%llu  idle:%llu  iowait:%llu  irq:%llu  softirq:%llu %llu %llu "
		"%llu\n",
		(unsigned long long)cputime64_to_clock_t(user),
		(unsigned long long)cputime64_to_clock_t(nice),
		(unsigned long long)cputime64_to_clock_t(system),
		(unsigned long long)cputime64_to_clock_t(idle),
		(unsigned long long)cputime64_to_clock_t(iowait),
		(unsigned long long)cputime64_to_clock_t(irq),
		(unsigned long long)cputime64_to_clock_t(softirq),
		(unsigned long long)0, //cputime64_to_clock_t(steal),
		(unsigned long long)0, //cputime64_to_clock_t(guest),
		(unsigned long long)0);//cputime64_to_clock_t(guest_nice));
	printk(KERN_INFO " -----------------------------------------------------------------------------------\n" );

	for_each_online_cpu(i) {

		/* Copy values here to work around gcc-2.95.3, gcc-2.96 */
		user = kstat_cpu(i).cpustat.user;
		nice = kstat_cpu(i).cpustat.nice;
		system = kstat_cpu(i).cpustat.system;
		idle = kstat_cpu(i).cpustat.idle;
		idle = cputime64_add(idle, arch_idle_time(i));
		iowait = kstat_cpu(i).cpustat.iowait;
		irq = kstat_cpu(i).cpustat.irq;
		softirq = kstat_cpu(i).cpustat.softirq;
		//steal = kstat_cpu(i).cpustat.steal;
		//guest = kstat_cpu(i).cpustat.guest;
		//guest_nice = kstat_cpu(i).cpustat.guest_nice;
    	printk(KERN_INFO " cpu %d   user:%llu  nice:%llu  system:%llu  idle:%llu  iowait:%llu  irq:%llu  softirq:%llu %llu %llu "
	    	"%llu\n",
			i,
			(unsigned long long)cputime64_to_clock_t(user),
			(unsigned long long)cputime64_to_clock_t(nice),
			(unsigned long long)cputime64_to_clock_t(system),
			(unsigned long long)cputime64_to_clock_t(idle),
			(unsigned long long)cputime64_to_clock_t(iowait),
			(unsigned long long)cputime64_to_clock_t(irq),
			(unsigned long long)cputime64_to_clock_t(softirq),
			(unsigned long long)0, //cputime64_to_clock_t(steal),
			(unsigned long long)0, //cputime64_to_clock_t(guest),
			(unsigned long long)0);//cputime64_to_clock_t(guest_nice));
	}
	printk(KERN_INFO " -----------------------------------------------------------------------------------\n" );

	printk(KERN_INFO "\n");
	printk(KERN_INFO " irq : %llu", (unsigned long long)sum);
	printk(KERN_INFO " -----------------------------------------------------------------------------------\n" );

	/* sum again ? it could be updated? */
	for_each_irq_nr(j) {
		per_irq_sum = 0;
		for_each_possible_cpu(i)
			per_irq_sum += kstat_irqs_cpu(j, i);

		if(per_irq_sum)  printk(KERN_INFO " irq-%d : %u\n", j, per_irq_sum);
	}
	printk(KERN_INFO " -----------------------------------------------------------------------------------\n" );

	printk(KERN_INFO "\n");
	printk(KERN_INFO " softirq : %llu", (unsigned long long)sum_softirq);
	printk(KERN_INFO " -----------------------------------------------------------------------------------\n" );

	for (i = 0; i < NR_SOFTIRQS; i++)
		if(per_softirq_sums[i]) printk(KERN_INFO " softirq-%d : %u", i, per_softirq_sums[i]);
	printk(KERN_INFO " -----------------------------------------------------------------------------------\n" );
	return;
}

void dump_debug_info_forced_ramd_dump()
{
	dump_all_task_info();
	dump_cpu_stat();
}
EXPORT_SYMBOL(dump_debug_info_forced_ramd_dump);
//}} GAF3.0
#ifdef CONFIG_MACH_TREBON
 int debug_hard_irq_cnt = 0;
unsigned long long  debug_hard_irq_total_cnt = 0;

typedef struct hard_irq_log_template{
int irq_no;
char entry;
}hard_irq_log;

hard_irq_log debug_hard_irq_log[10000];
unsigned int dump_temp;
void save_irq_log(unsigned int irq,int entry)
{
	unsigned int temp;
	temp = __raw_readl(S5P_INFORM4);
	temp = temp & ~(0xffffffff) ;

	debug_hard_irq_total_cnt++;
	debug_hard_irq_log[debug_hard_irq_cnt].irq_no = irq;
	if(entry == 1){
		debug_hard_irq_log[debug_hard_irq_cnt].entry = 1;
		temp = (temp | (debug_hard_irq_cnt <<16)|(irq<<0) | (0x1<<15)|(0x0<<14));
		}
	else{
		debug_hard_irq_log[debug_hard_irq_cnt].entry = 0;
		temp = (temp | (debug_hard_irq_cnt <<16)|(irq<<0) | (0x0<<15)|(0x0<<14));		
		}
	dump_temp	=temp;
	__raw_writel(temp,S5P_INFORM4);

	if(++debug_hard_irq_cnt >= 10000)
	debug_hard_irq_cnt = 0;
}

EXPORT_SYMBOL(save_irq_log);
#endif


#ifdef CONFIG_TIKAL_MPCS
/* Reset Reason given from Bootloader */
static int set_reset_reason_proc_show(struct seq_file *m, void *v)
{
	/*
		RESET_REASON_NORMAL
		RESET_REASON_SMPL
		RESET_REASON_WSTR
		RESET_REASON_WATCHDOG
		RESET_REASON_PANIC
		RESET_REASON_LPM
		RESET_REASON_RECOVERY
		RESET_REASON_FOTA
	*/

	static unsigned reset_reason = 0xFFEEFFEE;

	if(!sec_get_param_value)
		return -1;

	sec_get_param_value(__RESET_REASON, (void *)&reset_reason);
	
	switch(reset_reason){
	case RESET_REASON_NORMAL:
		seq_printf(m, "PON_NORMAL\n");
		break;
	case RESET_REASON_SMPL:
		seq_printf(m, "PON_SMPL\n");
		break;	
	case RESET_REASON_WSTR:
	case RESET_REASON_WATCHDOG:
	case RESET_REASON_PANIC:
		seq_printf(m, "PON_PANIC\n");
		break;	
	case RESET_REASON_LPM:
	case RESET_REASON_RECOVERY:
	case RESET_REASON_FOTA:
		seq_printf(m, "PON_NORMAL\n");
		break;	
	case RESET_REASON_CP_ERROR:
		seq_printf(m, "PON_CP_ERROR\n");
		break;	
	default:
		seq_printf(m, "PON_DEFAULT\n");
	}

	return 0;
}

static int sec_reset_reason_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, set_reset_reason_proc_show, NULL);
}

static const struct file_operations sec_reset_reason_proc_fops = {
	.open		= sec_reset_reason_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init sec_debug_reset_reason_init(void)
{
	struct proc_dir_entry *entry;

	entry = proc_create("reset_reason", S_IRUGO, NULL,
			    &sec_reset_reason_proc_fops);
	if (!entry)
		return -ENOMEM;

	return 0;
}

device_initcall(sec_debug_reset_reason_init);
#endif
#endif
