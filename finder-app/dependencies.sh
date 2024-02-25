#!/bin/bash
# Dependency installation script for kernel build.
# Author: Siddhant Jajoo.

sudo apt update
sudo apt install -y libssl-dev
sudo apt install -y u-boot-tools
sudo apt install -y qemu
sudo apt install -y curl
