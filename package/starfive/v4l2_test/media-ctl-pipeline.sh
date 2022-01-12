#!/bin/sh
# SPDX-License-Identifier: GPL-2.0
#
# Copyright (C) 2021 StarFive Technology Co., Ltd.
#

USAGE="Usage: media-ctl-pipeline -d /dev/media0 -i interface_type -s sensor_type -a {start|stop}"

devname="/dev/media0"

while getopts "d:i:s:a:" arg
do
	case $arg in
		d)
			devname=$OPTARG
			echo "select dev name: $devname"
			;;
		i)
			interface_type=$OPTARG
			echo "select interface_type: $interface_type"
			;;
		s)
			sensor_type=$OPTARG
			echo "select sensor_type: $sensor_type"
			;;
		a)
			action=$OPTARG
			echo "select action: $action"
			;;
		?)
			echo "unknow argument"
			exit 1
			;;
	esac
done

if [ -z $devname]; then
	echo "unknow media device"
	echo $USAGE
	exit 1
fi

if [ -z $interface_type ]; then
	echo "unknow interface type"
	echo $USAGE
	exit 1
fi

if [ -z $sensor_type ]; then
	echo "unknow sensor type"
	echo $USAGE
	exit 1
fi

if [ -z $action ]; then
	echo "unknow action type"
	echo $USAGE
	exit 1
fi

echo "Pipeline -d $devname -i $interface_type -s $sensor_type -a $action"

case $interface_type in
	dvp)
		case $action in
			start)
				# media-ctl -d $devname -vl "'sc2235 1-0030':0 -> 'stf_dvp0':0 [1]"
				# media-ctl -d $devname -vl "'ov5640 1-003c':0 -> 'stf_dvp0':0 [1]"
				case $sensor_type in
					VIN)
						echo "DVP vin enable pipeline:"
						media-ctl -d $devname -vl "'stf_dvp0':1 -> 'stf_vin0_wr':0 [1]"
						# media-ctl -d $devname -vl "'stf_vin0_wr':1 -> 'stf_vin0_wr_video0':0 [1]"
						;;
					ISP0)
						echo "DVP ISP0 enable pipeline:"
						media-ctl -d $devname -vl "'stf_dvp0':1 -> 'stf_isp0':0 [1]"
						media-ctl -d $devname -vl "'stf_isp0':1 -> 'stf_vin0_isp0':0 [1]"
						# media-ctl -d $devname -vl "'stf_vin0_isp0':1 -> 'stf_vin0_isp0_video1':0 [1]"
						;;
					ISP0RAW)
						echo "DVP ISP0RAW enable pipeline:"
						media-ctl -d $devname -vl "'stf_dvp0':1 -> 'stf_isp0':0 [1]"
						media-ctl -d $devname -vl "'stf_isp0':1 -> 'stf_vin0_isp0_raw':0 [1]"
						# media-ctl -d $devname -vl "'stf_vin0_isp0':1 -> 'stf_vin0_isp0_video1':0 [1]"
						;;
					ISP1)
						echo "DVP ISP1 enable pipeline:"
						media-ctl -d $devname -vl "'stf_dvp0':1 -> 'stf_isp1':0 [1]"
						media-ctl -d $devname -vl "'stf_isp1':1 -> 'stf_vin0_isp1':0 [1]"
						# media-ctl -d $devname -vl "'stf_vin0_isp1':1 -> 'stf_vin0_isp1_video2':0 [1]"
						;;
					ISP1RAW)
						echo "DVP ISP1RAW enable pipeline:"
						media-ctl -d $devname -vl "'stf_dvp0':1 -> 'stf_isp1':0 [1]"
						media-ctl -d $devname -vl "'stf_isp1':1 -> 'stf_vin0_isp1_raw':0 [1]"
						# media-ctl -d $devname -vl "'stf_vin0_isp1':1 -> 'stf_vin0_isp1_video2':0 [1]"
						;;
					*)
						echo $USAGE
						exit 1
						;;
				esac
				;;
			stop)
				# media-ctl -d $devname -vl "'sc2235 1-0030':0 -> 'stf_dvp0':0 [0]"
				# media-ctl -d $devname -vl "'ov5640 1-003c':0 -> 'stf_dvp0':0 [0]"
				case $sensor_type in
					VIN)
						echo "DVP vin disable pipeline:"
						media-ctl -d $devname -vl "'stf_dvp0':1 -> 'stf_vin0_wr':0 [0]"
						# media-ctl -d $devname -vl "'stf_vin0_wr':1 -> 'stf_vin0_wr_video0':0 [0]"
						;;
					ISP0)
						echo "DVP ISP0 disable pipeline:"
						media-ctl -d $devname -vl "'stf_dvp0':1 -> 'stf_isp0':0 [0]"
						media-ctl -d $devname -vl "'stf_isp0':1 -> 'stf_vin0_isp0':0 [0]"
						# media-ctl -d $devname -vl "'stf_vin0_isp0':1 -> 'stf_vin0_isp0_video1':0 [0]"
						;;
					ISP0RAW)
						echo "DVP ISP0RAW disable pipeline:"
						media-ctl -d $devname -vl "'stf_dvp0':1 -> 'stf_isp0':0 [0]"
						media-ctl -d $devname -vl "'stf_isp0':1 -> 'stf_vin0_isp0_raw':0 [0]"
						# media-ctl -d $devname -vl "'stf_vin0_isp0':1 -> 'stf_vin0_isp0_video1':0 [0]"
						;;
					ISP1)
						echo "DVP ISP1 disable pipeline:"
						media-ctl -d $devname -vl "'stf_dvp0':1 -> 'stf_isp1':0 [0]"
						media-ctl -d $devname -vl "'stf_isp1':1 -> 'stf_vin0_isp1':0 [0]"
						# media-ctl -d $devname -vl "'stf_vin0_isp1':1 -> 'stf_vin0_isp1_video2':0 [0]"
						;;
					ISP1RAW)
						echo "DVP ISP1RAW disable pipeline:"
						media-ctl -d $devname -vl "'stf_dvp0':1 -> 'stf_isp1':0 [0]"
						media-ctl -d $devname -vl "'stf_isp1':1 -> 'stf_vin0_isp1_raw':0 [0]"
						# media-ctl -d $devname -vl "'stf_vin0_isp1':1 -> 'stf_vin0_isp1_video2':0 [0]"
						;;
					*)
						echo $USAGE
						exit 1
						;;
				esac
				;;
			*)
				echo $USAGE
				exit 1
				;;
		esac
		;;
	csiphy0)
		case $action in
			start)
				# media-ctl -d $devname -vl "'ov4689 0-0036':0 -> 'stf_csiphy0':0 [1]"
				case $sensor_type in
					VIN)
						echo "csiphy0 CSIRX0 vin enable pipeline:"
						;;
					ISP0)
						echo "csiphy0 CSIRX0 ISP0 enable pipeline:"
						media-ctl -d $devname -vl "'stf_csiphy0':1 -> 'stf_csi0':0 [1]"
						media-ctl -d $devname -vl "'stf_csi0':1 -> 'stf_isp0':0 [1]"
						media-ctl -d $devname -vl "'stf_isp0':1 -> 'stf_vin0_isp0':0 [1]"
						# media-ctl -d $devname -vl "'stf_vin0_isp0':1 -> 'stf_vin0_isp0_video1':0 [1]"
						;;
					ISP0RAW)
						echo "csiphy0 CSIRX0 ISP0RAW enable pipeline:"
						media-ctl -d $devname -vl "'stf_csiphy0':1 -> 'stf_csi0':0 [1]"
						media-ctl -d $devname -vl "'stf_csi0':1 -> 'stf_isp0':0 [1]"
						media-ctl -d $devname -vl "'stf_isp0':1 -> 'stf_vin0_isp0_raw':0 [1]"
						# media-ctl -d $devname -vl "'stf_vin0_isp0':1 -> 'stf_vin0_isp0_video1':0 [1]"
						;;
					ISP1)
						echo "csiphy0 CSIRX0 ISP1 enable pipeline:"
						media-ctl -d $devname -vl "'stf_csiphy0':1 -> 'stf_csi0':0 [1]"
						media-ctl -d $devname -vl "'stf_csi0':1 -> 'stf_isp1':0 [1]"
						media-ctl -d $devname -vl "'stf_isp1':1 -> 'stf_vin0_isp1':0 [1]"
						# media-ctl -d $devname -vl "'stf_vin0_isp1':1 -> 'stf_vin0_isp1_video2':0 [1]"
						;;
					ISP1RAW)
						echo "csiphy0 CSIRX0 ISP1RAW enable pipeline:"
						media-ctl -d $devname -vl "'stf_csiphy0':1 -> 'stf_csi0':0 [1]"
						media-ctl -d $devname -vl "'stf_csi0':1 -> 'stf_isp1':0 [1]"
						media-ctl -d $devname -vl "'stf_isp1':1 -> 'stf_vin0_isp1_raw':0 [1]"
						# media-ctl -d $devname -vl "'stf_vin0_isp1':1 -> 'stf_vin0_isp1_video2':0 [1]"
						;;

					*)
						echo $USAGE
						exit 1
						;;
				esac
				;;
			stop)
				# media-ctl -d $devname -vl "'ov4689 0-0036':0 -> 'stf_csiphy0':0 [0]"
				case $sensor_type in
					VIN)
						echo "csiphy0 CSIRX0 vin disable pipeline:"
						;;
					ISP0)
						echo "csiphy0 CSIRX0 ISP0 disable pipeline:"
						media-ctl -d $devname -vl "'stf_csiphy0':1 -> 'stf_csi0':0 [0]"
						media-ctl -d $devname -vl "'stf_csi0':1 -> 'stf_isp0':0 [0]"
						media-ctl -d $devname -vl "'stf_isp0':1 -> 'stf_vin0_isp0':0 [0]"
						# media-ctl -d $devname -vl "'stf_vin0_isp0':1 -> 'stf_vin0_isp0_video1':0 [0]"
						;;
					ISP0RAW)
						echo "csiphy0 CSIRX0 ISP0RAW disable pipeline:"
						media-ctl -d $devname -vl "'stf_csiphy0':1 -> 'stf_csi0':0 [0]"
						media-ctl -d $devname -vl "'stf_csi0':1 -> 'stf_isp0':0 [0]"
						media-ctl -d $devname -vl "'stf_isp0':1 -> 'stf_vin0_isp0_raw':0 [0]"
						# media-ctl -d $devname -vl "'stf_vin0_isp0':1 -> 'stf_vin0_isp0_video1':0 [0]"
						;;
					ISP1)
						echo "csiphy0 CSIRX0 ISP1 disable pipeline:"
						media-ctl -d $devname -vl "'stf_csiphy0':1 -> 'stf_csi0':0 [0]"
						media-ctl -d $devname -vl "'stf_csi0':1 -> 'stf_isp1':0 [0]"
						media-ctl -d $devname -vl "'stf_isp1':1 -> 'stf_vin0_isp1':0 [0]"
						# media-ctl -d $devname -vl "'stf_vin0_isp1':1 -> 'stf_vin0_isp1_video2':0 [0]"
						;;
					ISP1RAW)
						echo "csiphy0 CSIRX0 ISP1RAW disable pipeline:"
						media-ctl -d $devname -vl "'stf_csiphy0':1 -> 'stf_csi0':0 [0]"
						media-ctl -d $devname -vl "'stf_csi0':1 -> 'stf_isp1':0 [0]"
						media-ctl -d $devname -vl "'stf_isp1':1 -> 'stf_vin0_isp1_raw':0 [0]"
						# media-ctl -d $devname -vl "'stf_vin0_isp1':1 -> 'stf_vin0_isp1_video2':0 [0]"
						;;

					*)
						echo $USAGE
						exit 1
						;;
				esac
				;;
			*)
				echo $USAGE
				exit 1
				;;
		esac
		;;
	csiphy1)
		case $action in
			start)
				# media-ctl -d $devname -vl "'ov4689 2-0036':0 -> 'stf_csiphy1':0 [1]"
				case $sensor_type in
					VIN)
						echo "csiphy1 CSIRX0 vin enable pipeline:"
						;;
					ISP0)
						echo "csiphy1 CSIRX1 ISP0 enable pipeline:"
						media-ctl -d $devname -vl "'stf_csiphy1':1 -> 'stf_csi1':0 [1]"
						media-ctl -d $devname -vl "'stf_csi1':1 -> 'stf_isp0':0 [1]"
						media-ctl -d $devname -vl "'stf_isp0':1 -> 'stf_vin0_isp0':0 [1]"
						# media-ctl -d $devname -vl "'stf_vin0_isp0':1 -> 'stf_vin0_isp0_video1':0 [1]"
						;;
					ISP0RAW)
						echo "csiphy1 CSIRX1 ISP0RAW enable pipeline:"
						media-ctl -d $devname -vl "'stf_csiphy1':1 -> 'stf_csi1':0 [1]"
						media-ctl -d $devname -vl "'stf_csi1':1 -> 'stf_isp0':0 [1]"
						media-ctl -d $devname -vl "'stf_isp0':1 -> 'stf_vin0_isp0_raw':0 [1]"
						# media-ctl -d $devname -vl "'stf_vin0_isp0':1 -> 'stf_vin0_isp0_video1':0 [1]"
						;;
					ISP1)
						echo "csiphy1 CSIRX1 ISP1 enable pipeline:"
						media-ctl -d $devname -vl "'stf_csiphy1':1 -> 'stf_csi1':0 [1]"
						media-ctl -d $devname -vl "'stf_csi1':1 -> 'stf_isp1':0 [1]"
						media-ctl -d $devname -vl "'stf_isp1':1 -> 'stf_vin0_isp1':0 [1]"
						# media-ctl -d $devname -vl "'stf_vin0_isp1':1 -> 'stf_vin0_isp1_video2':0 [1]"
						;;
					ISP1RAW)
						echo "csiphy1 CSIRX1 ISP1RAW enable pipeline:"
						media-ctl -d $devname -vl "'stf_csiphy1':1 -> 'stf_csi1':0 [1]"
						media-ctl -d $devname -vl "'stf_csi1':1 -> 'stf_isp1':0 [1]"
						media-ctl -d $devname -vl "'stf_isp1':1 -> 'stf_vin0_isp1_raw':0 [1]"
						# media-ctl -d $devname -vl "'stf_vin0_isp1':1 -> 'stf_vin0_isp1_video2':0 [1]"
						;;

					*)
						echo $USAGE
						exit 1
						;;
				esac
				;;
			stop)
				# media-ctl -d $devname -vl "'ov4689 0-0036':0 -> 'stf_csiphy0':0 [0]"
				case $sensor_type in
					VIN)
						echo "csiphy1 CSIRX0 vin disable pipeline:"
						;;
					ISP0)
						echo "csiphy1 CSIRX1 ISP0 disable pipeline:"
						media-ctl -d $devname -vl "'stf_csiphy1':1 -> 'stf_csi1':0 [0]"
						media-ctl -d $devname -vl "'stf_csi1':1 -> 'stf_isp0':0 [0]"
						media-ctl -d $devname -vl "'stf_isp0':1 -> 'stf_vin0_isp0':0 [0]"
						# media-ctl -d $devname -vl "'stf_vin0_isp0':1 -> 'stf_vin0_isp0_video1':0 [0]"
						;;
					ISP0RAW)
						echo "csiphy1 CSIRX1 ISP0RAW disable pipeline:"
						media-ctl -d $devname -vl "'stf_csiphy1':1 -> 'stf_csi1':0 [0]"
						media-ctl -d $devname -vl "'stf_csi1':1 -> 'stf_isp0':0 [0]"
						media-ctl -d $devname -vl "'stf_isp0':1 -> 'stf_vin0_isp0_raw':0 [0]"
						# media-ctl -d $devname -vl "'stf_vin0_isp0':1 -> 'stf_vin0_isp0_video1':0 [0]"
						;;
					ISP1)
						echo "csiphy1 CSIRX1 ISP1 disable pipeline:"
						media-ctl -d $devname -vl "'stf_csiphy1':1 -> 'stf_csi1':0 [0]"
						media-ctl -d $devname -vl "'stf_csi1':1 -> 'stf_isp1':0 [0]"
						media-ctl -d $devname -vl "'stf_isp1':1 -> 'stf_vin0_isp1':0 [0]"
						# media-ctl -d $devname -vl "'stf_vin0_isp1':1 -> 'stf_vin0_isp1_video2':0 [0]"
						;;
					ISP1RAW)
						echo "csiphy1 CSIRX1 ISP1RAW disable pipeline:"
						media-ctl -d $devname -vl "'stf_csiphy1':1 -> 'stf_csi1':0 [0]"
						media-ctl -d $devname -vl "'stf_csi1':1 -> 'stf_isp1':0 [0]"
						media-ctl -d $devname -vl "'stf_isp1':1 -> 'stf_vin0_isp1_raw':0 [0]"
						# media-ctl -d $devname -vl "'stf_vin0_isp1':1 -> 'stf_vin0_isp1_video2':0 [0]"
						;;
					*)
						echo $USAGE
						exit 1
						;;
				esac
				;;
			*)
				echo $USAGE
				exit 1
				;;
		esac
		;;
	*)
		echo $USAGE
		exit 1
		;;
esac

exit 0;
