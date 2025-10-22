#!/bin/sh

## This script install fltube at TinyCoreLinux based systems from the .tar.gz generated with "make PREFIX=build tcz_package" command.
if [ ! -f "fltube.tcz" -o ! -f "fltube.tcz.dep" ]; then
    echo "Required files for install are not available: fltube.tcz and fltube.tcz.dep. Download .tar.gz again. Aborting installation..." 1>&2
    exit 1
fi

md5sum -c -s fltube.tcz.md5.txt
[ ! "$?" -eq 0  ] && { echo "MD5 sum for fltube.tcz doesn't match..." 1>&2; exit 1; }
root_partition=$(grep "/mnt" /proc/mounts | cut -d " " -f 1)
partition_name=$(basename "$root_partition")
IS_ONBOOT=`cat /mnt/"$partition_name"/tce/onboot.lst | grep "fltube.tcz"`
[ -z "$IS_ONBOOT" ] && echo "fltube.tcz" >> /mnt/"$partition_name"/tce/onboot.lst
rm -f /mnt/"$partition_name"/tce/optional/fltube.tcz*
cp fltube.tcz* /mnt/"$partition_name"/tce/optional/
echo "Installing FLTube and required dependencies. This can take a while, please wait..."
tce-load -i fltube.tcz      #Dependencies are installed from .dep file...
echo "FLTube was installed succesfully."
exit 0
