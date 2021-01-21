# This is proof of concept to run bare metal app on nanopim3, raspberry pi

Prerequisites:
1. Install multipass to run ubuntu server for crosscompile https://multipass.run/
2. Run build server based on ubuntu image:\
multipass launch bionic -c 2 -m 2G -d 10G -n ubuntu-vm

Build:
./make {target}

Run on NanoPI M3 (usb boot)
1. reboot Nano PI
2. ./run

Run on RPI
1. copy files from boot/rpi1 and /build/{board}/kernel.img to SD card
2. reboot RPI
