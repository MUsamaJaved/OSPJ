/*
 * cstate.c - Kernel Module that read the c state
 */

#include <linux/module.h>
#include <linux/kernel.h>

//#define INFILE "/tmp/cstate_module"

static long msr;

static long rdmsr_v(long msr_id);

int init_module(void)
{
	printk(KERN_INFO "Cstate Module ready to operate.\n");
	//msr = rdmsr_v(0xC0010073);
	msr = rdmsr_v(0);
	printk(KERN_INFO "\n\n#################################\n");
	printk(KERN_INFO "MSR Value %ld\n", msr);
	return 1;
}

void cleanup_module(void)
{
	printk(KERN_INFO "Cstate Module cleanup\n");
}

static long rdmsr_v(long msr_id)
{
	long long msr_value;
	asm volatile ( "rdmsr" : "=A" (msr_value) : "c" (msr_id) );
	return msr_value;
}

