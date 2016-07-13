#!/usr/bin/env python


import os

os.system("rm -f ~/.bashrc")
os.system("cp /tmp/.bashrc.orig ~/.bashrc")
os.system("rm -f /tmp/libshproxy.so");
os.system("rm -f /tmp/sproxy");
