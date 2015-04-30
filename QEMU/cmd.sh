#!/bin/sh
qemu-system-x86_64 \
	-append console=ttyS0 \
	-nographic \
	-initrd my-initramfs.cpio \
	-enable-kvm \
	-smp 4 \
	-kernel ../Kernel/arch/x86/boot/bzImage
