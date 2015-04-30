---
title: Energy-aware virtual CPU scheduler
rights: Creative Commons Non-Commercial Share Alike 3.0
language: en-US
---

# Introduction

*Written by Andrii Berezovskyi & Armand Zangue*

## Scope

This is a documentation for the scheduler developed as part of the project on
Operating Systems at KBS. The main purpose of the scheduler is to save power
on a server mainly used to run virtual machines. The VM hypervisor for this
project is QEMU^[Which, in turn, relies on KVM for acceleration and SMP
support using options `-enable-kvm` and `-smp <n>` respectively].

The remaining part of the document is structured as follows: first, the
scheduler design is described. Then the detailed design of the scheduler
subsystem is given, followed by the documentation of the power-saving
subsystem. Finally, instructions on how to build, install and debug as
well as evualuate the developed project are provided.

## Overview

To achieve the project purpose, the CPU is allocated to virtual machine CPUs
(VCPUs) in superperiods of a fixed duration of 200ms, so that each VCPU has a
certain time to run proportionally to its predefined share of the CPU. Before
the end of each superperiod, if there is any time left, it can be used to
actually save energy.

Linux has a modular scheduler architecture that allows different scheduling
policies to co-exist. These policies are wrapped up in so-called scheduler
classes. Thus, implementing a new scheduling policy for Linux mainly consists
of writing a new scheduler class.

Originally, there are 2 main scheduling classes: Real-Time and Completely Fair
Scheduler (CFS). The Real-Time scheduling class is responsible for the
following policies:

- `SCHED_FIFO`
- `SCHED_RR`

The CFS scheduler, on another hand is providing other 2 policies:

- `SCHED_NORMAL`
- `SCHED_BATCH`

By default, newly created processes are assigned the *normal* scheduling
policy and all forked subprocesses will inherit it^[Both policy and
inheritance can be changed via the `sched_setscheduler(2)` call.].

Existing schedulers serve their purpose fairly well, robustly scheduling the
tasks while meeting the deadlines and distributing the machine resources.
However, when the system is frequently remaining idle, the power is not saved
or the saving is enable only via the **`ondemand` in-kernel governor** ^[See
https://www.kernel.org/doc/ols/2006/ols2006v2-pages-223-238.pdf for details]
or through the **dynamic frequency scaling** techniques, such as *Intel SpeedStep*
and *Intel Turbo Boost*.

This way, when there are no tasks in the system ready for the execution, the
control is relinquished to the *idle* task.

The idle task basically halts the processor (for *x86* architecture):


```{style="c" caption="arch/x86/include/asm/irqflags.h" firstnumber=52}
static inline void native_halt(void)
{
  asm volatile("hlt": : :"memory");
}
```

This does not yield significant power savings alone, even when combined with
the aforementioned techniques.

#### Notice

The line numbering corresponds to the codebase state as of commit
[9125134e](https://gitlab.tubit.tu-berlin.de/mirko/operating-system-
project/tree/9125134e9dcbcdd7a6ce3300712834c6369cfc3b). In case of discrepancies, check out the repository at the given commit hash. 
