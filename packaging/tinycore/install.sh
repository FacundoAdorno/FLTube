#!/bin/sh

## This script installs fltube at TinyCoreLinux based systems from the .tar.gz generated with "make PREFIX=build tcz_package" command.

### EXIT STATUS CODES
EXITCODE_SUCCESS=0
EXITCODE_CANNOT_INSTALL=3
EXITCODE_REQUIREMENTS_NOT_MET=4
EXITCODE_FILES_INCONSISTENCY=5

if [ ! -z "$1" ]; then
    cat <<"EOF"
[-USAGE-] This script installs FLTube at TinyCoreLinux based systems from the .tar.gz generated with "make PREFIX=build tcz_package" command.
To execute it just run './install.sh'. No parameters are available.
- EXIT STATUS CODES are:
    * EXITCODE_SUCCESS=0                    #All finished OK and without errors.
    * EXITCODE_CANNOT_INSTALL=3             #Cannot finish installation for some generic error.
    * EXITCODE_REQUIREMENTS_NOT_MET=4       #Some requirements must be met before proceed to installation.
    * EXITCODE_FILES_INCONSISTENCY=5        #Some installation files are inconsistents.
EOF
    exit 2;
fi

# First check if is a TinyCoreLinux mounted as a LiveCD or is installed on a disk.
grep -E '/mnt/sr[[:digit:]]' /proc/mounts >& /dev/null && echo "This script cannot install FLTube when running a LiveCD. Please install the distribution on a disk partition before install FLTube. Aborting..." && exit $EXITCODE_REQUIREMENTS_NOT_MET

if [ ! -f "fltube.tcz" -o ! -f "fltube.tcz.dep" ]; then
    echo "Required files for install are not available: fltube.tcz and fltube.tcz.dep. Download .tar.gz again. Aborting installation..." 1>&2
    exit $EXITCODE_FILES_INCONSISTENCY
fi

root_partition=$(grep "/mnt" /proc/mounts | cut -d " " -f 1)
partition_name=$(basename "$root_partition")

# If SSL certificates are not installed, no HTTPS download can be made...
is_cacertificates_installed=`cat /mnt/"$partition_name"/tce/onboot.lst | grep "ca-certificates.tcz"`
if [ -z "$is_cacertificates_installed" ]; then
    echo "Installing SSL certificates before proceding..."
    tce-load -wi ca-certificates.tcz
    [ $? -ne 0 ] && echo "Cannot install SSL certificates. Aborting..." && exit $EXITCODE_REQUIREMENTS_NOT_MET
fi

### Workaround until Python3.10 or superior are available at TinyCore repository..
echo "[QUESTION!] Python 3.10 or superior are required. Want to uninstall any older version of Python and install a new one? [y/n]"
read decision
if [ $decision == 'Y' -o $decision == 'y' ]; then
    PYTHON311_TCZ_URL=https://gitlab.com/-/project/74160365/uploads/f286ad3d76aabc21492b31853c76bd0c/python3.11.tcz
    #Remove any version available to install on TinyCore repository
    is_python_3_6_installed=`cat /mnt/"$partition_name"/tce/onboot.lst | grep "python3.6.tcz"`
    is_python_3_9_installed=`cat /mnt/"$partition_name"/tce/onboot.lst | grep "python3.9.tcz"`
    [ ! -z "$is_python_3_6_installed" ] && echo "Removing Python 3.6..." && sed -i '/^python3.6.tcz$/d' /mnt/"$partition_name"/tce/onboot.lst && rm -f /mnt/"$partition_name"/tce/optional/python3.6.tcz*
    [ ! -z "$is_python_3_9_installed" ] && echo "Removing Python 3.9..." && sed -i '/^python3.9.tcz$/d' /mnt/"$partition_name"/tce/onboot.lst && rm -f /mnt/"$partition_name"/tce/optional/python3.9.tcz*
    is_python_3_11_installed=`cat /mnt/"$partition_name"/tce/onboot.lst | grep "python3.11.tcz"`
    if [ -z "$is_python_3_11_installed" ]; then
        echo "Installing Python 3.11..." &&
        wget -q -O /tmp/python3.11.tcz $PYTHON311_TCZ_URL
        mv /tmp/python3.11.tcz /mnt/"$partition_name"/tce/optional/
        echo "python3.11.tcz" >> /mnt/"$partition_name"/tce/onboot.lst
        tce-load -i python3.11.tcz
        [ $? -ne 0 ] && echo "Python 3.11 cannot be installed for some reason. Aborting operation..." && exit $EXITCODE_REQUIREMENTS_NOT_MET
    else
        echo "Python 3.11 is already installed on your system."
    fi
else
    echo "Aborting installation because Python 3.10 or superior is required..."
    exit $EXITCODE_REQUIREMENTS_NOT_MET
fi

echo "Proceding to FLTube installation..."
md5sum -c -s fltube.tcz.md5.txt
[ ! "$?" -eq 0  ] && { echo "MD5 sum for fltube.tcz doesn't match..." 1>&2; exit $EXITCODE_FILES_INCONSISTENCY; }
IS_ONBOOT=`cat /mnt/"$partition_name"/tce/onboot.lst | grep "fltube.tcz"`
[ -z "$IS_ONBOOT" ] && echo "fltube.tcz" >> /mnt/"$partition_name"/tce/onboot.lst
rm -f /mnt/"$partition_name"/tce/optional/fltube.tcz*
mv fltube.tcz* /mnt/"$partition_name"/tce/optional/
echo "Installing FLTube and required dependencies. This can take a while, please wait..."
tce-load -wi fltube.tcz      #Dependencies should be installed from .dep file, but could fail...
if [ $? -ne 0 ]; then
    echo "Trying to install dependencies manually, because dependencies were not installed automatically by tce-load..."
    while IFS= read -r dep; do
        tce-load -wi $dep
        if [ $? -ne 0 ]; then
            echo "FLTube failed its installation for some reason. Save the ouput logs and inform to developers..."
            exit $EXITCODE_CANNOT_INSTALL
        fi
    done < /mnt/"$partition_name"/tce/optional/fltube.tcz.dep
    echo "Loading again FLTube to system..."
    tce-load -i fltube.tcz
fi

echo "Cleaning up some files..."
rm -f install.sh
echo "FLTube was installed succesfully!"
exit $EXITCODE_SUCCESS
