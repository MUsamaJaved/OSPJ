# General information

## Git rules

**DO NOT BREAK THE HISTORY**

`git diff` before `git commit`,

and `git pull` before you `git push`.

Measure 7 times before you `git --force`.

## Untracked files

`/etc/default/grub` contains important information, mainly the kernel boot
/command and serial debugging support.

`/var/OSP/deploy/script/build.sh` is a script to install the latest kernel
version from the repository (by default, from `deploy` branch). ** Must be
executed by user gitlab only!**

`/usr/bin/build-kernel` is a script that invokes the script above (with a
proper user). Email is sent to Fleep on completion. Usage:

	build-kernel <branch>

		e.g.

		build-kernel
		build-kernel deploy
		build-kernel sched_power

`/usr/bin/rm-kernel` removes the kernel that matches the given string.
Intended to be used with a commit hash. Email is sent to Fleep on success.

# Contact information

## Group Members

| Name                    | E-mail                               |
| ----------------------- | ------------------------------------ |
| Stephan Bauroth         | der_steffi@gmx.de                    |
| Andrij Berezovskyi      | andrew@berezovskiy.me                |
| Lukas Braband           | lukas@braband.de                     |
| Tamilselvan Shanmugam   | tamilselvan@mailbox.tu-berlin.de     |
| Marat Timergaliev       | marat.timergaliev@gmail.com          |
| Armand Zangue           | armandzang@mailbox.tu-berlin.de      |
| ~~Stephan Fell~~        | ~~stephan.fell@mailbox.tu-berlin.de~~|
| ~~Mirko Liebender~~     | ~~mirko@mailbox.tu-berlin.de~~       |


## Redmine website

https://trac.kbs.tu-berlin.de/projects/kbs-project-winter-term-2014

# Project parts				  

- Scheduler subsystem (design, impl. scheduler): Armand, Tamil
- CPU power interface (dealing with CPU States): Stephan, Lukas, Marat
- Measurement automotiv (PDU): Lukas
- VMs (KVM) (VMM): Armand 
  (how to assign KVM process IDs to special class in scheduler)

