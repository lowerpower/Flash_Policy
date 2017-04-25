#!/bin/sh

install -m644 policy.txt /etc/flash/ 
install -m644 pconfig.txt /etc/flash/ 

install -m755 ../src/flash_policyd /usr/local/bin/
install -m755 flash_policyd.redhat /etc/rc.d/init.d/flash_policyd
chkconfig --add flash_policyd

