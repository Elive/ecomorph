#!/bin/sh

# Compiz Manager wrapper script
#
# Copyright (c) 2007 Kristian Lyngstøl <kristian@bohemians.org>
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
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#
#
# Contributions by: Treviño (3v1n0) <trevi55@gmail.com>, Ubuntu Packages
#
# Much of this code is based on Beryl code, also licensed under the GPL.
# This script will detect what options we need to pass to compiz to get it
# started, and start a default plugin and possibly window decorator.
#


COMPIZ_NAME="ecomorph" # Final name for compiz (compiz.real)
# For Xgl LD_PRELOAD
#LIBGL_NVIDIA="/usr/lib/nvidia/libGL.so.1.2.xlibmesa"
#LIBGL_FGLRX="/usr/lib/fglrx/libGL.so.1.2.xlibmesa"

# Minimum amount of memory (in kilo bytes) that nVidia cards need
# to be allowed to start
# Set to 262144 to require 256MB
NVIDIA_MEMORY="65536" # 64MB
NVIDIA_SETTINGS="nvidia-settings" # Assume it's in the path by default

# For detecting what driver is in use, the + is for one or more /'s
#XORG_DRIVER_PATH="/usr/lib/xorg/modules/drivers/+"

# Driver whitelist
WHITELIST="nvidia intel ati radeon i810 fglrx"

# blacklist based on the pci ids
# See http://wiki.compiz-fusion.org/Hardware/Blacklist for details
#T="   1002:5954 1002:5854 1002:5955" # ati rs480
#T="$T 1002:4153" # ATI Rv350
#T="$T 8086:2982 8086:2992 8086:29a2 8086:2a02 8086:2a12"  # intel 965
#T="$T 8086:2e02 " # Intel Eaglelake
T="$T 8086:3577 8086:2562 " # Intel 830MG, 845G (LP: #259385)
BLACKLIST_PCIIDS="$T"
unset T

#to override ecomorph [module] selection of COMPIZ_PLUGINS
#set COMPIZ_PLUGINS_OVERRIDE=yes

COMPIZ_OPTIONS=""
COMPIZ_PLUGINS="ini inotify"
COMPIZ_PLUGINS_OVERRIDE="no"
ENV=""

# No indirect by default
INDIRECT="no"

# Default X.org log if xset q doesn't reveal it
XORG_DEFAULT_LOG="/var/log/Xorg.0.log"

# Set to yes to enable verbose
VERBOSE="yes"

# Echos the arguments if verbose
verbose()
{
	if [ "x$VERBOSE" = "xyes" ]; then
		printf "$*" 1>&2
	fi
}

# Check if the nVidia card has enough video ram to make sense
check_nvidia_memory()
{
    if [ ! -x "$NVIDIA_SETTINGS" ]; then
	return 0
    fi

	MEM=$(${NVIDIA_SETTINGS} -q VideoRam | egrep Attribute\ \'VideoRam\'\ .*: | cut -d: -f3 | sed 's/[^0-9]//g')
	if [ $MEM -lt $NVIDIA_MEMORY ]; then
		verbose "Less than ${NVIDIA_MEMORY}kb of memory and nVidia";
		return 1;
	fi
	return 0;
}

# Check for existence if NV-GLX
check_nvidia()
{
	if [ ! -z $NVIDIA_INTERNAL_TEST ]; then
		return $NVIDIA_INTERNAL_TEST;
	fi
	verbose "Checking for nVidia: "
	if xdpyinfo | grep -q NV-GLX ; then
		verbose "present. \n"
		NVIDIA_INTERNAL_TEST=0
		return 0;
	else
		verbose "not present. \n"
		NVIDIA_INTERNAL_TEST=1
		return 1;
	fi
}

# check driver whitelist
# running_under_whitelisted_driver()
# {
# 	LOG=$(xset q|grep "Log file"|awk '{print $3}')
# 	if [ "$LOG" = "" ]; then
# 	    verbose "xset q doesn't reveal the location of the log file. Using fallback $XORG_DEFAULT_LOG \n"
# 	    LOG=$XORG_DEFAULT_LOG;
# 	fi
# 	if [ -z "$LOG" ];then
# 		verbose "AIEEEEH, no Log file found \n"
# 		verbose "$(xset q) \n"
# 	return 0
# 	fi
# 	for DRV in ${WHITELIST}; do
# 		if egrep -q "Loading ${XORG_DRIVER_PATH}${DRV}_drv\.so" $LOG &&
# 		   ! egrep -q "Unloading ${XORG_DRIVER_PATH}${DRV}_drv\.so" $LOG;
# 		then
# 			return 0
# 		fi
# 	done
# 	verbose "No whitelisted driver found\n"
# 	return 1
# }

# check pciid blacklist
# have_blacklisted_pciid()
# {
# 	OUTPUT=$(lspci -n)
# 	for ID in ${BLACKLIST_PCIIDS}; do
# 		if echo "$OUTPUT" | egrep -q "$ID"; then
# 			verbose "Blacklisted PCIID '$ID' found \n"
# 			return 0
# 		fi
# 	done
# 	OUTPUT=$(lspci -vn | grep -i VGA)
# 	verbose "Detected PCI ID for VGA: $OUTPUT\n"
# 	return 1
# }

build_env()
{
	if check_nvidia; then
		ENV="__GL_YIELD=NOTHING "
	fi
	if [ "$INDIRECT" = "yes" ]; then
		ENV="$ENV LIBGL_ALWAYS_INDIRECT=1 "
	fi

	ENV="$ENV FROM_WRAPPER=yes"

	if [ -n "$ENV" ]; then
		export $ENV
	fi
}

build_args()
{
	if [ "x$INDIRECT" = "xyes" ]; then
		COMPIZ_OPTIONS="$COMPIZ_OPTIONS --indirect-rendering "
	fi
	if [ ! -z "$DESKTOP_AUTOSTART_ID" ]; then
		COMPIZ_OPTIONS="$COMPIZ_OPTIONS --sm-client-id $DESKTOP_AUTOSTART_ID"
	fi
	if check_nvidia; then
		if [ "x$INDIRECT" != "xyes" ]; then
			COMPIZ_OPTIONS="$COMPIZ_OPTIONS --loose-binding"
		fi
	fi
}

####################
# Execution begins here.

if [ "x$LIBGL_ALWAYS_INDIRECT" = "x1" ]; then
	INDIRECT="yes";
fi

if check_nvidia && ! check_nvidia_memory; then
    exit 1;
fi


# get environment
build_env
build_args


# these messages are strictly required, do not remove or change them
printf "ECOMORPH_OPTIONS: $COMPIZ_OPTIONS [n]"
printf "ECOMORPH_PLUGINS: $COMPIZ_PLUGINS [n]"
printf "ECOMORPH_PLUGINS_OVERRIDE:$COMPIZ_PLUGINS_OVERRIDE \n"


if [ "x$CM_DRY" = "xyes" ]; then
	verbose "Dry run finished: everything should work with regards to Compiz and 3D.\n"
	verbose "Execute: ${COMPIZ_NAME} $COMPIZ_OPTIONS "$@" $COMPIZ_PLUGINS \n"
	exit 0;
fi

# that line is for run ecomorph, but we don't use it anymore
#${COMPIZ_NAME} $COMPIZ_OPTIONS "$@" $COMPIZ_PLUGINS

