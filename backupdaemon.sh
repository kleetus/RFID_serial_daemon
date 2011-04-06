#!/bin/bash
cd /tmp
filename="daemon.tar.bz2"
tar -cjf $filename ~/daemon &> /dev/null
scp $filename `cat ~/daemon/backuphost` 




