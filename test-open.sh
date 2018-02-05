#! /bin/sh -eux

trap "rm -f openedfiles.tmp" EXIT 
cat /tmp/open_watch_fifo > openedfiles.tmp &  
LD_PRELOAD=$PWD/openWatch.so ./openTest f1.tmp 
grep f1.tmp openedfiles.tmp
echo OK
