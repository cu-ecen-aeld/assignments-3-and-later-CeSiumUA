#!/bin/bash
# Dependency installation script for kernel build.
# Author: Siddhant Jajoo.


sudo apt-get install -y libssl-dev
sudo apt-get install -y bc u-boot-tools kmod cpio flex bison psmisc
sudo apt-get install -y qemu
sudo apt-get install -y curl ruby cmake git build-essential bsdmainutils valgrind wget qemu-system-arm apt-utils tzdata dialog sed make binutils bash patch gzip bzip2 perl tar cpio unzip rsync file bc wget python libncurses5-dev git qemu netcat iputils-ping
