#!/bin/sh

## This script install fltube at TinyCoreLinux based systems from the .tar.gz generated with "make PREFIX=build tcz_package" command.
if [ ! -f "fltube.tcz" -o ! -f "fltube.tcz.dep" ]; then
    echo "Required files for install are not available: fltube.tcz and fltube.tcz.dep. Download .tar.gz again. Aborting installation..." 1>&2
    exit 1
fi

root_partition=$(grep "/mnt" /proc/mounts | cut -d " " -f 1)
partition_name=$(basename "$root_partition")

### Workaround until Python3.10 or superior are available at TinyCore repository..
echo "Python 3.10 or superior are required. Want to uninstall any older version of Python and install a new one? [y/n]"
read decision
if [ $decision == 'Y' -o $decision == 'y' ]; then
    #Remove any version available to install on TinyCore repository
    is_python_3_6_installed=`cat /mnt/"$partition_name"/tce/onboot.lst | grep "python3.6.tcz"`
    is_python_3_9_installed=`cat /mnt/"$partition_name"/tce/onboot.lst | grep "python3.9.tcz"`
    [ ! -z "$is_python_3_6_installed" ] && echo "Removing Python 3.6..." && sed -i '/^python3.6.tcz$/d' /mnt/"$partition_name"/tce/onboot.lst && rm -f /mnt/"$partition_name"/tce/optional/python3.6.tcz*
    [ ! -z "$is_python_3_9_installed" ] && echo "Removing Python 3.9..." && sed -i '/^python3.9.tcz$/d' /mnt/"$partition_name"/tce/onboot.lst && rm -f /mnt/"$partition_name"/tce/optional/python3.9.tcz*
    is_python_3_11_installed=`cat /mnt/"$partition_name"/tce/onboot.lst | grep "python3.11.tcz"`
    if [ -z "$is_python_3_11_installed" ]; then
        echo "Installing Python 3.11..." &&
        wget -q -O /tmp/python3.11.tcz https://gitlab.com/-/project/74160365/uploads/f286ad3d76aabc21492b31853c76bd0c/python3.11.tcz
        mv /tmp/python3.11.tcz /mnt/"$partition_name"/tce/optional/
        echo "python3.11.tcz" >> /mnt/"$partition_name"/tce/onboot.lst
        tce-load -i python3.11.tcz
    else
        echo "Python 3.11 is already installed on your system."
    fi
else
    echo "Aborting installation because Python 3.10 or superior is required..."
    exit 2
fi

echo "Proceding to FLTube installation..."
md5sum -c -s fltube.tcz.md5.txt
[ ! "$?" -eq 0  ] && { echo "MD5 sum for fltube.tcz doesn't match..." 1>&2; exit 1; }
IS_ONBOOT=`cat /mnt/"$partition_name"/tce/onboot.lst | grep "fltube.tcz"`
[ -z "$IS_ONBOOT" ] && echo "fltube.tcz" >> /mnt/"$partition_name"/tce/onboot.lst
rm -f /mnt/"$partition_name"/tce/optional/fltube.tcz*
mv fltube.tcz* /mnt/"$partition_name"/tce/optional/
echo "Installing FLTube and required dependencies. This can take a while, please wait..."
tce-load -i fltube.tcz      #Dependencies are installed from .dep file...
echo "FLTube was installed succesfully."
exit 0
