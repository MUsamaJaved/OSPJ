#!/bin/sh
qemu-system-x86_64 \
	-append console=ttyS0 \
	-nographic \
	-m 512M \
	-initrd my-initramfs.cpio \
	-kernel ../Kernel/arch/x86/boot/bzImage
