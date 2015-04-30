// change one ohter thing ass?
// change some things to test gitlab-repo
/*
 * cstate.c - Kernel Module that read the c state
 */

#include <linux/module.h>
#include <linux/kernel.h>
//#include "/usr/include/stdlib.h"		// calloc
//#include <linux/stdlib.h>		// calloc

//#define INFILE "/tmp/cstate_module"

static long msr;

static	long a;
static	long d;
static	long reg_ebx;
static	long reg_ecx;
/*static*/	long reg_edx = 5;

//static long rdmsr_v(long msr_id);
//static long cpuid1(long fctNumCode);
static inline void cpuid2(int code, long* a, long* d);
void foo(void);

int init_module(void)
{
	printk(KERN_INFO "Cstate Module ready to operate.\n");
	a=0; d=0;
	//msr = rdmsr_v(0xC0010073);
	//msr = rdmsr_v(0);
	//a = (long*)kmalloc(n, sizeof(long));
	//d = (long*)kmalloc(n, sizeof(long));
	printk(KERN_INFO "\n\n#################################\n");
	printk(KERN_INFO "#### Calling cpuid2#\n");
	//cpuid2(0x00000000, &a, &d);
	cpuid2(0x80000000, &a, &d);
	//asm("mov reg_ebx, %%ebx"); // move from cpu-register ebx to c-var reg_ebx
	//asm("mov reg_ecx, %%ecx");
// nochmal kompilieren und ausprobieren

	//printk(KERN_INFO "CPUID ret-Value %ld\n", msr);
	printk(KERN_INFO "pointer-adr, a:\t\t %p\n", &a);
	printk(KERN_INFO "pointer-adr, d:\t\t %p\n", &d);
	printk(KERN_INFO "blabla\n");
	//printk(KERN_INFO "pointer-InhaltsValue, a:\t %x\n", a);
	//printk(KERN_INFO "pointer-InhaltsValue, d:\t %x\n", d);
	printk(KERN_INFO "reg_ebx d:\t\t\t %ld\n", reg_ebx);
	printk(KERN_INFO "reg_ecx d:\t\t\t %ld\n", reg_ecx);
	printk(KERN_INFO "reg_edx d:\t\t\t %ld\n", reg_edx);
	
	if (msr == 0x68747541) {
		printk(KERN_INFO "CPUID ret-Value entspricht dem richtigen Wert\n");
	}
	
	printk(KERN_INFO "will return now\n");
	return 1;
}

void cleanup_module(void)
{
	printk(KERN_INFO "Cstate Module cleanup\n");
}

static inline void cpuid2(int code, long* a, long* d)
{
    asm volatile ( "cpuid" : "=a"(*a), "=d"(*d) : "0"(code) : "ebx", "ecx" );
}

void foo(void) {
	asm("mov reg_edx, %%edx");
}

/*
int a=10, b;
asm ("	movl %%eax, 0h; "	// mov the constant 0x0 to register eax
	:"=r"(b)        	// output
	:"r"(a)         	// input 
	:"%eax"        		// clobbered register 
    );
*/

/*
static long cpuid1(long fctNumCode)
{
	long msr_value=0;
	asm volatile("movl %%eax, 0 " ::: "%eax");
	//see amdvol 3
	asm volatile ( "cpuid" : "=A" (msr_value) : "c" (fctNumCode) );
	return msr_value;
}


static long rdmsr_v(long msr_id)
{
	long long msr_value;
	asm volatile ( "rdmsr" : "=A" (msr_value) : "c" (msr_id) );
	return msr_value;
}
*/

//
//make clean all && insmod ./cstate1.ko
// to see output: dmesg
