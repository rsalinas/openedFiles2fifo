#! /bin/sh -eux

trap "rm -f f1.tmp f2.tmp" EXIT
rm -vf f1.tmp f2.tmp
LD_PRELOAD=$PWD/openWatch.so ./openTest f1.tmp
./openTest f2.tmp
stat --format=%a  f1.tmp f2.tmp | uniq | wc -l |grep -q 1
echo OK
