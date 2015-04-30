#!/bin/bash

mkdir -p initramfs

echo 'Recompiling busybox'
cd busybox-1.23.0
make -j4
echo 'Installing Busybox'
make install 1>/dev/null
sudo cp -aR _install/* ../initramfs/
cd ../../Kernel

echo 'Rebuilding kernel'
make ARCH=x86_64 -j2
make ARCH=x86_64 -j2 INSTALL_MOD_PATH=../QEMU/initramfs modules modules_install 1>/dev/null
make ARCH=x86_64 -j2 INSTALL_HDR_PATH=../QEMU/initramfs headers_install 1>/dev/null
echo 'kernel rebuilt, headers and modules installed'

cd ../QEMU
cp ../Userspace/sched_test/demo/demo initramfs/usr/bin/sched_test
cp ../Userspace/fibo/fibo initramfs/usr/bin/fibo

cd initramfs
chmod 755 usr/bin/sched_test
chmod 755 usr/bin/fibo

cp ../init_template init
chmod 755 init

echo 'Compressing initramfs'
find . -print0 | cpio --null -o --format=newc > ../my-initramfs.cpio

