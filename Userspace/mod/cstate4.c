/*
 * cstate.c - Kernel Module that read the c state
 */

#include <linux/module.h>
#include <linux/kernel.h>
//#include <stdlib.h>		// calloc

//#define INFILE "/tmp/cstate_module"

static long msr;

	long* a;
	long* d;
	int n=1;

//static long rdmsr_v(long msr_id);
//static long cpuid1(long fctNumCode);
static inline void cpuid2(int code, long* a, long* d);

int init_module(void)
{
	printk(KERN_INFO "Cstate Module ready to operate.\n");
	//msr = rdmsr_v(0xC0010073);
	//msr = rdmsr_v(0);
	//a = (long*)kmalloc(n, sizeof(int));
	//d = (long*)kmalloc(n, sizeof(int));
	printk(KERN_INFO "\n\n#################################\n");
	printk(KERN_INFO "#### Calling cpuid2#\n");
	//cpuid2(0x00000000, a, d);
// nochmal kompilieren und ausprobieren

	//printk(KERN_INFO "CPUID ret-Value %ld\n", msr);
	printk(KERN_INFO "pointer-adr, a:\t\t %p\n", a);
	printk(KERN_INFO "pointer-adr, d:\t\t %p\n", d);
	//printk(KERN_INFO "pointer-InhaltsValue, *a:\t %ld\n", *a);
	//printk(KERN_INFO "pointer-InhaltsValue, *d:\t %ld\n", *d);
	
	if (msr == 0x68747541) {
		printk(KERN_INFO "CPUID ret-Value entspricht dem richtigen Wert\n");
	}
	
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
