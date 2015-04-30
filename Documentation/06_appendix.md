
# Appendices

## Appendix A. Interacting with QEMU via QMP


```{style="code" caption="Gemfile" .ruby}
source "https://rubygems.org" 

gem 'qemu', '~> 0.4'
```

```{style="code" caption="main.rb" .ruby}
#!/usr/bin/env ruby

require 'qemu'
include QEMU

d = Daemon.new

puts 'Daemon constructed'
```

## Appendix B. Untracked files

```{style="code" caption="/etc/default/grub" .bash}
# Copyright 1999-2013 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: /var/cvsroot/gentoo-x86/sys-boot/grub/files/grub.default-2,v 1.4 2013/09/21 18:10:55 floppym Exp $
#
# To populate all changes in this file you need to regenerate your
# grub configuration file afterwards:
#     'grub2-mkconfig -o /boot/grub/grub.cfg'
#
# See the grub info page for documentation on possible variables and
# their associated values. 

GRUB_DISTRIBUTOR="Gentoo"

GRUB_DEFAULT=saved
GRUB_HIDDEN_TIMEOUT=0
GRUB_HIDDEN_TIMEOUT_QUIET=true
GRUB_TIMEOUT=5

# Append parameters to the linux kernel command line
# GRUB_CMDLINE_LINUX=""

# Append parameters to the linux kernel command line for non-recovery entries
#GRUB_CMDLINE_LINUX_DEFAULT="oops=panic panic=-1 CONFIG_DEBUG_KERNEL CONFIG_DEBUG_STACKOVERFLOW CONFIG_DEBUG_STACK_USAGE"
GRUB_CMDLINE_LINUX_DEFAULT="oops=panic CONFIG_DEBUG_KERNEL CONFIG_DEBUG_STACKOVERFLOW CONFIG_DEBUG_STACK_USAGE console=tty0 console=ttyS0,115200n8 maxcpus=1"

# Uncomment to disable graphical terminal (grub-pc only)
#GRUB_TERMINAL=console
GRUB_TERMINAL_INPUT="console serial"
GRUB_TERMINAL_OUTPUT="console serial"
GRUB_SERIAL_COMMAND="serial --speed=115200 --unit=0 --word=8 --parity=no --stop=1"


# The resolution used on graphical terminal.
# Note that you can use only modes which your graphic card supports via VBE.
# You can see them in real GRUB with the command `vbeinfo'.
GRUB_GFXMODE=1280x1280x16

#GRUB_GFXPAYLOAD_LINUX=keep

# Path to theme spec txt file.
# The starfield is by default provided with use truetype.
# NOTE: when enabling custom theme, ensure you have required font/etc.
#GRUB_THEME="/boot/grub/themes/starfield/theme.txt"

# Background image used on graphical terminal.
# Can be in various bitmap formats.
#GRUB_BACKGROUND="/boot/grub/mybackground.png"

# Uncomment if you don't want GRUB to pass "root=UUID=xxx" parameter to kernel
#GRUB_DISABLE_LINUX_UUID=true

# Uncomment to disable generation of recovery mode menu entries
GRUB_DISABLE_RECOVERY=true
```


```{style="code" caption="/var/OSP/deploy/script/build.sh" .bash}
#!/bin/bash
FOLDER=/var/OSP/deploy
#FOLDER=/var/OSP
LOG=$FOLDER/log/$(date +%y%m%d-%H%M).log

LOCKFILE=$FOLDER/build.lock

rmlock() {
    lockfile-remove -l $LOCKFILE
}

lockfile-create -r 0 -l $LOCKFILE || { echo "Lock is taken, skipping the build" ; exit 99;}

cd $FOLDER/operating-system-project
#cd $FOLDER/gitRepo
branch=${1-deploy}

git fetch --all > "$LOG" 2>&1 || { echo 'git fetch failed' ; rmlock; exit 1; }
git checkout $branch --force > "$LOG" 2>&1 || { echo 'git checkout failed' ; rmlock; exit 1; }
PRE=$(uname -r)
PRE=${PRE: -40}
PREV=${PRE:0:8}
git pull origin $branch >> "$LOG" 2>&1 
NEXT=$(git rev-parse HEAD)
NEXT_CMP=${NEXT:0:8}
if [ "$PREV" == "$NEXT_CMP" ]
then
 echo "kernel already up-to-date (rev $PREV)" >> "$LOG" ; rmlock; exit 2;
fi

INSTALLED=$(ls /boot/vmlinuz* | grep -c "$NEXT")

if [ "$INSTALLED" -gt 0 ]
then
   echo -e "kernel $NEXT was already installed (and $NEXT_CMP not equal to $PREV)\nhttps://gitlab.tubit.tu-berlin.de/mirko/operating-system-project/commit/$NEXT_CMP\nhttps://gitlab.tubit.tu-berlin.de/mirko/operating-system-project/commit/$PREV" | tee -a  "$LOG" ; rmlock; exit 3;
fi


cd Kernel
make LOCALVERSION="_$(date +%y%m%d%H%M)_$NEXT" ARCH=x86_64 -j4 >> "$LOG" 2>&1 || { echo 'kernel compilation failed' | tee -a "$LOG"; rmlock; exit 4; }
echo "Kernel was built on $(date)" | tee -a "$LOG"

sudo make -j4 ARCH=x86_64 headers_install >> "$LOG" 2>&1 || { echo 'header installation failed' | tee -a "$LOG"; rmlock; exit 5; }
sudo make -j4 ARCH=x86_64 modules_install >> "$LOG" 2>&1 || { echo 'module installation failed' | tee -a "$LOG"; rmlock; exit 6; }
sudo make -j4 ARCH=x86_64 install >> "$LOG" 2>&1 || { echo 'kernel installation failed' | tee -a "$LOG"; rmlock; exit 7; }

sudo grub2-mkconfig -o /boot/grub/grub.cfg >> "$LOG" 2>&1

cd $FOLDER/operating-system-project/Userspace/ospj-setsched
if [ -e Makefile ]; then
    make >> "$LOG" 2>&1 || { echo 'compiling ospj-setsched failed' | tee -a "$LOG"; rmlock; exit 8; }
    sudo make install >> "$LOG" 2>&1 || { echo 'installing ospj-setsched failed' | tee -a "$LOG"; rmlock; exit 9; }
fi

echo "New kernel $NEXT was successfully installed on $(date)" | tee -a "$LOG"

rmlock

exit 0;
```

```{style="code" caption="/usr/bin/build-kernel" .bash}
#!/bin/bash

mail_fleep() {
echo -e "$2"
echo -e "$2" | mail -s "$1" conv.36nrjbz27sfu41@fleep.io
}

branch=${1-deploy}

mail_fleep "Manual kernel build started" "Installing a fresh kernel from '$branch' branch"

build_output=$(sudo -u gitlab /var/OSP/deploy/script/build.sh $branch)
build_status=$?

mail_fleep "Manual kernel build finished" "The build has finished with status '$build_status':\n$build_output"
```


```{style="code" caption="/usr/bin/rm-kernel" .bash}
#!/bin/bash

mail_fleep() {
echo -e "$2"
echo -e "$2" | mail -s "$1" conv.36nrjbz27sfu41@fleep.io
}

sudo rm -i /boot/*$1*

mail_fleep "Kernel removed" "Kernel build ${1:0:8} was removed from the server"
```

```{style="simple" caption="energy-experiments.txt"}
redefinition of the experiment by mohammad/group:
4 cores @3Ghz vs
3 cores @3Ghz, 1 core @800Mhz vs
3 cores @3Ghz, 1 core disabled

####################################################

Results:
4 cores @3Ghz: 79684 Ws
result is an average value. Executed experiment 3 times (79986 Ws, 79567 Ws, 79500 Ws)
web-interface showed active power of: 133W

3 cores @3Ghz, 1 core @800Mhz: 73150 Ws 
executed: 2x, (73149 Ws, 73150 Ws)
web-interface showed active power of: 121-122W

3 cores @3Ghz, 1 core disabled: 71693 Ws
executed: 2x, (71690 Ws, 71695 Ws)
web-interface showed active power of: 119-120W

(done in Task 2117)



############## older Task: 2049 ####################
Measurement results:
all 4 cores 800Mhz, 4*100% cpu-load:
energy-consumption-counter:
before: 283095371 Ws
after: 283133337 Ws
diff: 37966 Ws
benchmark-duration: 10min

all 4 cores 3Ghz, 4*100% cpu-load:
before: 283158544 Ws
after: 283235738 Ws
diff: 77194 Ws
benchmark-duration: 10min
```
\label{nergy}
