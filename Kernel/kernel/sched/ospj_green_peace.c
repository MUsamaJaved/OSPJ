#include <linux/errno.h>
#include <linux/kthread.h>

#include <linux/cpufreq.h>

void ospj_reduce_frequency(int cpu){
	//sysfs_set_frequency(cpu, 800000);
}

void ospj_increase_frequency(int cpu){
	//sysfs_set_frequency(cpu, 3000000);
}

void ospj_green_peace(int cpu){
	//int error = -99;
	printk("[OSPJ] Low freq \n");
	ospj_reduce_frequency(cpu);
	printk("[OSPJ] Start sleeping \n");
	//usleep(10000000);
	printk("[OSPJ] frequency change\n");
	ospj_increase_frequency(cpu);
	printk("[OSPJ] Frequency set back\n");
}

void thread_func(void *arg)
{
	//int i;
	int loc_id = * (int *) arg;
	ospj_green_peace(loc_id);	
}

void try_get_driver(void) {
	const char* driver_name;
	printk("[OSPJ] retrieving cpufreq_get_current_driver - HUHU\n");
	driver_name = cpufreq_get_current_driver();
	printk("[OSPJ] current driver: %s\n", driver_name);
}

void try_get_policy(void) {
	struct cpufreq_policy policy;
	int retval;
	printk("[OSPJ] retrieving cpufreq_get_policy\n");
	retval = cpufreq_get_policy(&policy, 0);
	if (retval == 0) {
		printk("[OSPJ] freq: mix %d max %d\n", policy.min, policy.max);
	}
	printk("[OSPJ] result: %d\n", retval);
}

void ospj_energy_save_mode(int cpu){
	struct cpufreq_policy policy;
	int retval;
	retval = cpufreq_get_policy(&policy, 0);
	if (retval == 0) {
		printk("[OSPJ] freq: mix %d max %d\n", policy.min, policy.max);
	}
	printk("[OSPJ] result of cpufreq_get_policy(): %d\n", retval);
	try_get_driver();
}
