#!/bin/sh

CONFIG_DIR=/etc/flash/

if [ ! -d $CONFIG_DIR ]; then
      mkdir $CONFIG_DIR 
fi

install -m644 policy.txt $CONFIG_DIR 
install -m644 pconfig.txt $CONFIG_DIR 

install -m755 ../src/flash_policyd /usr/local/bin/
install -m755 flash_policyd.service /lib/systemd/system/
systemctl enable flash_policyd.service

