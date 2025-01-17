/* aries-watchdog.c
 *
 * Copyright (C) 2010 Google, Inc.
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

#include <plat/regs-watchdog.h>
#include <mach/map.h>

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/cpufreq.h>
#include <linux/err.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/io.h>

#include <linux/kernel_sec_common.h>


#if defined(CONFIG_AEGIS_USCC)
#define PET_BY_DIRECT_TIMER
#else
#define PET_BY_WORKQUEUE
#endif

/* enable */
static unsigned watchdog_enable = 1; /* on by default */
//module_param_named(enable, watchdog_enable, uint, 0644);

/* reset timeout in PCLK/256/128 (~2048:1s) */
static unsigned watchdog_reset = (30 * 2048);

/* pet timeout in jiffies */
#if defined(PET_BY_DIRECT_TIMER)
static unsigned watchdog_pet = (5 * HZ);
#elif defined(PET_BY_WORKQUEUE)
static unsigned watchdog_pet = (10 * HZ);
#endif
static unsigned long long watchdog_iTime=0;

#if defined(PET_BY_WORKQUEUE)
static struct workqueue_struct *watchdog_wq;
static void watchdog_workfunc(struct work_struct *work);
static DECLARE_DELAYED_WORK(watchdog_work, watchdog_workfunc);
#elif defined(PET_BY_DIRECT_TIMER)
static struct timer_list pet_watchdog_timer;
static void pet_watchdog_timer_fn(unsigned long data);
#endif

#if defined(PET_BY_WORKQUEUE)
static void watchdog_workfunc(struct work_struct *work)
{
	writel(watchdog_reset, S3C2410_WTCNT);
	watchdog_iTime = cpu_clock(raw_smp_processor_id());
	queue_delayed_work(watchdog_wq, &watchdog_work, watchdog_pet);
}
#elif defined(PET_BY_DIRECT_TIMER)
static void pet_watchdog_timer_fn(unsigned long data)
{
	pr_info("%s kicking...%x\n", __func__, readl(S3C2410_WTCNT));
	writel(watchdog_reset, S3C2410_WTCNT);
	pet_watchdog_timer.expires += watchdog_pet;
	add_timer_on(&pet_watchdog_timer, 0);
}
#endif

static void watchdog_start(void)
{
	unsigned int val;

	if (!watchdog_enable)
		return;

	/* set to PCLK / 256 / 128 */
	val = S3C2410_WTCON_DIV128;
	val |= S3C2410_WTCON_PRESCALE(255);
	writel(val, S3C2410_WTCON);

	/* program initial count */
	writel(watchdog_reset, S3C2410_WTCNT);
	writel(watchdog_reset, S3C2410_WTDAT);

	/* start timer */
	val |= S3C2410_WTCON_RSTEN | S3C2410_WTCON_ENABLE;
	writel(val, S3C2410_WTCON);
	watchdog_iTime = cpu_clock(raw_smp_processor_id());

	/* make sure we're ready to pet the dog */
#if defined(PET_BY_WORKQUEUE)
	queue_delayed_work(watchdog_wq, &watchdog_work, watchdog_pet);
#elif defined(PET_BY_DIRECT_TIMER)
	pet_watchdog_timer.expires = jiffies + watchdog_pet;
	add_timer_on(&pet_watchdog_timer, 0);
#endif
}

static void watchdog_stop(void)
{
	writel(0, S3C2410_WTCON);
#if defined(PET_BY_WORKQUEUE)
	/* do nothing? */
#elif defined(PET_BY_DIRECT_TIMER)
	del_timer(&pet_watchdog_timer);
#endif
}

static int watchdog_probe(struct platform_device *pdev)
{
	if (!watchdog_enable)
		return 0;

#if defined(PET_BY_WORKQUEUE)
	watchdog_wq = create_rt_workqueue("pet_watchdog");
	watchdog_start();
#elif defined(PET_BY_DIRECT_TIMER)
	init_timer(&pet_watchdog_timer);
	pet_watchdog_timer.function = pet_watchdog_timer_fn;
	watchdog_start();
#endif

	return 0;
}

static int watchdog_suspend(struct device *dev)
{
	watchdog_stop();
	return 0;
}

static int watchdog_resume(struct device *dev)
{
	watchdog_start();
	return 0;
}

static struct dev_pm_ops watchdog_pm_ops = {
	.suspend_noirq =	watchdog_suspend,
	.resume_noirq =		watchdog_resume,
};	

static struct platform_driver watchdog_driver = {
	.probe =	watchdog_probe,
	.driver = {
		.owner =	THIS_MODULE,
		.name =		"watchdog",
		.pm =		&watchdog_pm_ops,
	},
};
	
static int __init watchdog_init(void)
{
#if defined(CONFIG_TIKAL_USCC) 
	if (KERNEL_SEC_DEBUG_LEVEL_LOW == kernel_sec_get_debug_level())
		watchdog_enable = 1;
	else
		watchdog_enable = 0;
#endif
	if (!watchdog_enable) {
		pr_err("%s: disabled\n", __func__);
		return 0;
	} else
		pr_err("%s: enabled\n", __func__);

	return platform_driver_register(&watchdog_driver);
}

module_init(watchdog_init);
