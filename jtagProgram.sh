#!/bin/sh
#
# Copyright (C) 2006 Sandro Amato
#  
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#  
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#  
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
#
#
# Notes:
# Add the following to /etc/sudoers (using as root the visudo command)
# ALL  ALL=NOPASSWD: /bin/mknod /dev/parport0 c 99 0, /usr/local/jtag/XC3Sprog/detectchain, /usr/local/jtag/XC3Sprog/xc3sprog * *
#

JTAG_DIR="/home/dim/designs/XC3Sprog"
WORK_DIR=`pwd`

if [ "${1}X" == X ] ; then
   echo "No bit-stream file selected"
   echo "usage: ${0} <bit-stream>"
   exit
fi

# if not exist the bit-stream exit
[ -e ${1} ] || { echo "File not found: ${1}"; exit; }

# set the absolute path for the bit-stream file
BITSTREAM_FILE=${1}
{ echo ${1} | grep -q "^\/"; } || { BITSTREAM_FILE=${WORK_DIR}/${1}; }


# if not exist the jtag program exit
[ -e ${JTAG_DIR}/detectchain ] || { echo "Not found: ${JTAG_DIR}/detectchain"; exit; }


# if not exist create the raw parport device
[ -e /dev/parport0 ] || sudo mknod /dev/parport0 c 99 0


# build the args for zenity scanning the jtag chain
cd ${JTAG_DIR}
Z_ARGS=`sudo ${JTAG_DIR}/detectchain | sed -e ' s/[\t]/ /g ' | awk ' { print "FALSE "$3" "$7 } ' | while read LINE 
do
  echo ${LINE}
done`

DEV_NUM=`zenity  --list --radiolist --column "Program" --column "Num." --column "Device" ${Z_ARGS}`

if [ "${DEV_NUM}X" == X ] ; then
   echo "No device selected"
   cd ${WORK_DIR}
   exit
fi
echo "Programming device #${DEV_NUM} ..."

sudo ./xc3sprog -v ${BITSTREAM_FILE} ${DEV_NUM} | zenity --progress --pulsate --auto-close
cd ${WORK_DIR}
zenity --info --text="Device Programmed"
