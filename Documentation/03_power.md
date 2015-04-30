
# Power saving subsystem

*Written by Lukas Braband & Marat Timergaliev*

## What was done by the energy subgroup during the project
 
Floor... In one of the first meetings it was pointed out that the project task
requires some kind of "energy saving idle process". This "energy saving idle
process" would run for the amount of time where the current hyperperiod is
already served completely. So during this timespan no resources (here CPU),
agreed on in an SLA, are needed and therefore should be switched off “as
deeply as possible”.

## Power management. CPU sleeping states access

Power management implies dynamic frequency or voltage scaling on the certain
devices based on performance needs. In the scope of power management in the
project, the target goal was saving energy within the usage of CPU during the
system idle time.

The finding of an appropriate way of “switching off” the CPU(s) turned out to
be a more difficult and time consuming task than initially expected. Several
ideas and approaches had come up. In this part of the documentation the
investigated approaches are depicted.

### Sleeping states (C-states)

In order to save power, CPU has been enabled to support low-power mode states,
or in other words c-states (sleeping state).

The basic idea of these modes is to cut the clock signal and power from idle
units inside the CPU. The more units you stop (by cutting the clock), reduce
the voltage or even completely shut down, more energy you save, but more time
is required for the CPU to “wake up” and be again 100% operational.

The CPU power states C0–C3 are defined as follows:

- **C0** is the operating state.

- **C1** (often known as Halt) is a state where the processor is not
executing instructions, but can return to an executing state essentially
instantaneously. All ACPI-conformant processors must support this power
state. Some processors, such as the Pentium 4, also support an `Enhanced
C1 state` (`C1E` or Enhanced Halt State) for lower power consumption. 

- **C2** (often known as Stop-Clock) is a state where the processor
maintains all software-visible state, but may take longer to wake up.
This processor state is optional.

- **C3** (often known as Sleep) is a state where the processor does not
need to keep its cache coherent, but maintains other state. Some
processors have variations on the C3 state (Deep Sleep, Deeper Sleep,
etc.) that differ in how long it takes to wake the processor. This
processor state is optional.

Additional states are defined by manufacturers for some processors. For
example, Intel's Haswell platform has states up to C10, where it
distinguishes core states and package states.
    
C-states are dynamically requested by software and are exposed through ACPI
objects. C-states can be requested on a per-core basis. Software requests a
C-state change in one of two ways, either by executing the HLT instruction or,
for revision E, by reading from an IO specific address `CstateAddr`.

Since the access to c-states required installed ACPI module, here is the
required configuration for linux in order to enable ACPI.

```{style="simple"}
CONFIG_PM=y
CONFIG_ACPI=y
CONFIG_ACPI_AC=y
CONFIG_ACPI_BATTERY=y
CONFIG_ACPI_BUTTON=y
CONFIG_ACPI_FAN=y
CONFIG_ACPI_PROCESSOR=y
CONFIG_ACPI_THERMAL=y
CONFIG_ACPI_BLACKLIRG_YEAR=0
CONFIG_ACPI_EC=y
CONFIG_ACPI_POWER=y
CONFIG_ACPI_SYSTEM=y
```

We had  written the basic class in order to read the `msr` register, specific
for the certain processor. D18f4x118 recommended `msr` register to read, in
order to send the CPU core to a c-state.

The result was not satisfactory, since the system's ACPI was still not enabled
due to the  absence of ACPI component on the motherboard.

We applied different techniques to resolve the absence of necessary component.
Various reasons may exist of failing this task which are related to the
different aspects such as lack of knowledge and proper documentation, CPU or
motherboard related problems (may not support it), absence of necessary
drivers for current CPU and etc.

Some work, that has been done in order to investigate possible solution to
enable ACPI and CPU idle states was as follows:

- Update BIOS. It had to have a special configuration field to enable or disable ACPI, which was absent.
- Installation of `powertop` didn't show that the system has any cpu idle state. 
- Research for possible reason of idle states absence
- CPU idle was not installed along with ACPI
- CPU idle driver missing. Attempt to find a proper driver for our processor failed. 
- Research why ACPI doesn't work properly. Can be that in new kernel version acpi may require special actions, since it should work from `/proc/acpi` directory, which is deprecated in our kernel version, in favor of `/sys` directory. CPU idle configuration failed.

HLT execution cannot be verified since cpu idle states monitoring does not
work.

### Core disabling

The first traditional approach of saving energy did not approve its
efficiency, due of some reasons mentioned above, we decided to apply another
way to manage the power consumption by the CPU.

An easy way, relatively from the first sight, to save energy is disabling
cores.  This technique can be applied on the unstressed systems, because of
low demand of CPU, since the enabling a core takes a relatively long time
(from 15ms to 45ms).

The observations from testing core disabling is provided in the table below.
The benchmark duration - 10 minutes, and the load in each experiments - 100%.


**Table. Benchmark on core disabling. (Duration for all experiments: 10 min).**

-------------------------------------------------------------------------------
Experiments    Cores  Frequency                  Energy     Response
                                              consumption    
------------  ------- ---------------------  -------------  ------------------
1 (Baseline)   4      3 GHz                    77194 Ws     0   
 
2              3      3 Ghz                    71693 Ws     15-45ms (unstable)
 
3              4      3 cores on 3GHz          73150 Ws     5 μs
 
                      1\ core\ on\ 800MHz
 
4              4      4\ cores\ on\ 800MHz     37966 Ws     5 μs
-------------------------------------------------------------------------------
  

The discovery, from the conducted experiments, shows the minor difference in
the indicators of energy saving, which plays lower priority role, rather than
responsiveness of dynamic frequency scaling compared to core disabling. (See
Table - Benchmark on core disabling).

As a result of the experiments, collaborative discussion and defining the
priority, we decided to stick to dynamic frequency scaling.

## Frequency Scaling (P-states)

P-states are operational performance states (states in which the processor is
executing instructions, that is, running software) characterized by a unique
frequency of operation for a CPU core. The P-state control interface supports
dynamic P-state changes in up to 16 P-states called P-states 0 through 15 or
P0 though P15. P0 is the highest power, highest performance P-state; each
ascending P-state number represents a lower-power, lower-performance state.

In our project we use two levels of frequencies (800MHz and 3 GHz), which is
justified by basic need and the easiness of measurements for energy saving.
The other possible frequencies can be considered for the future development.

800 MHz frequency used during the idle time of system in order to save energy.

3 GHz frequency used for the purpose of high performance demand.

The switching to lower frequency happens, when the function of energy saving
is called before starting the idle task, and lasts the duration of a given
time. When the time is over, the function of setting back the high frequency
(3 GHz) is called.

Because scaling the frequency down to 800Mhz has almost the same effect (\ref{nergy})
on energy consumption as disabling the core with the
hlt command and the transition delay is strictly defined and very short (<=5µs
vs ~40ms), we decided to concentrate on getting to work the frequency scaling.
The progress in time contributed to that decision.

We discovered three kinds of ways on how the CPUs frequency can be
manipulated. These are explained as follows:

### Switching frequency via shell

Starting with the topic, the following
linux-command enables us to set the frequency of `cpu0` to 800Mhz:

```{style="simple"}
cpupower -c 0 frequency-set -f 800000
```

### Switching frequency from C in user-space

By looking how the upper command realizes this functionality we found the
function

```{style="simple"}
sysfs_set_frequency(unsigned int cpu, unsigned long target_frequency);
```

In the kernel-sourcecode in file `tools/power/cpupower/lib/sysfs.h` to change the frequency.

<!-- TODO -->
This function simply writes a value into `/sys/devices/system/cpu/cpu3/online` which
then causes the frequency to change to the specified value.

Maximum transition frequency latency is <=5µs, which fits our use case very
well.

### Switching frequency from C in kernel-space

Unfortunately the upper function could not be called from kernel-space
(which is what we need when developing a new scheduler).

We ran several tests on getting to work the scaling of frequencies from the kernel-space. 

One big step that had to be done was to fully separate the two developing
issues into the `scheduling` development branch and the `power_only`
development  branch. One clean Kernel had to be found where the frequency
tests could be ran and continued to develope on. After Stephan Bauroth
had adapted the build-system, it was possible to directly run the current
developments of the `power_only`-branch from the git-repository on the
deneb-server without merging it with the master branch (the current
changes on the OSPJ-Scheduler itself).

By doing this, the bug sources for each development part could be embanked.
Before that there were some OSPJ-Kernel-versions which ran very unstable
(random freezes of the server within minutes or hours) - and nobody knew
whose code was responsible for the unwanted behaviour.

After looking at the [cpufreq-driver code](http://lxr.free-electrons.com/source/drivers/cpufreq/cpufreq.c)
the plan was to copy the current `cpufreq_policy` to two new ones (`policy_min` and `policy_max`).
Under [several other fields](http://lxr.free-electrons.com/source/include/linux/cpufreq.h?v=3.10#L91)
the cpufreq_policy-struct contains the integer variables `min`, `max` and `cur`
which contain the minimum-, maximum and current frequency of that policy.

The new policies should look like this:

**policy_min:**

 - min=800Mhz
 - max=800Mhz
 - cur=800Mhz

and

**policy_max:**

 - min=3000Mhz
 - max=3000Mhz
 - cur=3000Mhz
