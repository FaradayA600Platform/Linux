#make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j2
../uboot/tools/mkimage -A arm64 -O linux -C none -T kernel -a 0x83080000 -e 0x83080000 -n 'Linux-5.10.136' -d arch/arm64/boot/Image arch/arm64/boot/uImage
#aarch64-linux-gnu-objdump -S -d -g -t vmlinux > kernel.asm
