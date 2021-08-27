#!/bin/sh
# SPDX-License-Identifier: GPL-2.0
#
# Copyright (C) 2021 StarFive Technology Co., Ltd.
#

USAGE="Usage: media-ctl-pipeline interface_type sensor_type {start|stop}"

echo "Pipeline $1 $2 $3"

case $1 in
	dvp)
		case $3 in
			start)
				# media-ctl -vl "'sc2235 1-0030':0 -> 'stf_dvp0':0 [1]"
				# media-ctl -vl "'ov5640 1-003c':0 -> 'stf_dvp0':0 [1]"
				case $2 in
					VIN)
						echo "DVP vin enable pipeline:"
						media-ctl -vl "'stf_dvp0':1 -> 'stf_vin0_wr':0 [1]"
						# media-ctl -vl "'stf_vin0_wr':1 -> 'stf_vin0_wr_video0':0 [1]"
						;;
					ISP0)
						echo "DVP ISP0 enable pipeline:"
						media-ctl -vl "'stf_dvp0':1 -> 'stf_isp0':0 [1]"
						media-ctl -vl "'stf_isp0':1 -> 'stf_vin0_isp0':0 [1]"
						# media-ctl -vl "'stf_vin0_isp0':1 -> 'stf_vin0_isp0_video1':0 [1]"
						;;
					ISP0RAW)
						echo "DVP ISP0RAW enable pipeline:"
						media-ctl -vl "'stf_dvp0':1 -> 'stf_isp0':0 [1]"
						media-ctl -vl "'stf_isp0':1 -> 'stf_vin0_isp0_raw':0 [1]"
						# media-ctl -vl "'stf_vin0_isp0':1 -> 'stf_vin0_isp0_video1':0 [1]"
						;;
					ISP1)
						echo "DVP ISP1 enable pipeline:"
						media-ctl -vl "'stf_dvp0':1 -> 'stf_isp1':0 [1]"
						media-ctl -vl "'stf_isp1':1 -> 'stf_vin0_isp1':0 [1]"
						# media-ctl -vl "'stf_vin0_isp1':1 -> 'stf_vin0_isp1_video2':0 [1]"
						;;
					ISP1RAW)
						echo "DVP ISP1RAW enable pipeline:"
						media-ctl -vl "'stf_dvp0':1 -> 'stf_isp1':0 [1]"
						media-ctl -vl "'stf_isp1':1 -> 'stf_vin0_isp1_raw':0 [1]"
						# media-ctl -vl "'stf_vin0_isp1':1 -> 'stf_vin0_isp1_video2':0 [1]"
						;;
					*)
						echo $USAGE
						exit 1
						;;
				esac
				;;
			stop)
				# media-ctl -vl "'sc2235 1-0030':0 -> 'stf_dvp0':0 [0]"
				# media-ctl -vl "'ov5640 1-003c':0 -> 'stf_dvp0':0 [0]"
				case $2 in
					VIN)
						echo "DVP vin disable pipeline:"
						media-ctl -vl "'stf_dvp0':1 -> 'stf_vin0_wr':0 [0]"
						# media-ctl -vl "'stf_vin0_wr':1 -> 'stf_vin0_wr_video0':0 [0]"
						;;
					ISP0)
						echo "DVP ISP0 disable pipeline:"
						media-ctl -vl "'stf_dvp0':1 -> 'stf_isp0':0 [0]"
						media-ctl -vl "'stf_isp0':1 -> 'stf_vin0_isp0':0 [0]"
						# media-ctl -vl "'stf_vin0_isp0':1 -> 'stf_vin0_isp0_video1':0 [0]"
						;;
					ISP0RAW)
						echo "DVP ISP0RAW disable pipeline:"
						media-ctl -vl "'stf_dvp0':1 -> 'stf_isp0':0 [0]"
						media-ctl -vl "'stf_isp0':1 -> 'stf_vin0_isp0_raw':0 [0]"
						# media-ctl -vl "'stf_vin0_isp0':1 -> 'stf_vin0_isp0_video1':0 [0]"
						;;
					ISP1)
						echo "DVP ISP1 disable pipeline:"
						media-ctl -vl "'stf_dvp0':1 -> 'stf_isp1':0 [0]"
						media-ctl -vl "'stf_isp1':1 -> 'stf_vin0_isp1':0 [0]"
						# media-ctl -vl "'stf_vin0_isp1':1 -> 'stf_vin0_isp1_video2':0 [0]"
						;;
					ISP1RAW)
						echo "DVP ISP1RAW disable pipeline:"
						media-ctl -vl "'stf_dvp0':1 -> 'stf_isp1':0 [0]"
						media-ctl -vl "'stf_isp1':1 -> 'stf_vin0_isp1_raw':0 [0]"
						# media-ctl -vl "'stf_vin0_isp1':1 -> 'stf_vin0_isp1_video2':0 [0]"
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
		case $3 in
			start)
				# media-ctl -vl "'ov4689 0-0036':0 -> 'stf_csiphy0':0 [1]"
				case $2 in
					VIN)
						echo "csiphy0 CSIRX0 vin enable pipeline:"
						;;
					ISP0)
						echo "csiphy0 CSIRX0 ISP0 enable pipeline:"
						media-ctl -vl "'stf_csiphy0':1 -> 'stf_csi0':0 [1]"
						media-ctl -vl "'stf_csi0':1 -> 'stf_isp0':0 [1]"
						media-ctl -vl "'stf_isp0':1 -> 'stf_vin0_isp0':0 [1]"
						# media-ctl -vl "'stf_vin0_isp0':1 -> 'stf_vin0_isp0_video1':0 [1]"
						;;
					ISP0RAW)
						echo "csiphy0 CSIRX0 ISP0RAW enable pipeline:"
						media-ctl -vl "'stf_csiphy0':1 -> 'stf_csi0':0 [1]"
						media-ctl -vl "'stf_csi0':1 -> 'stf_isp0':0 [1]"
						media-ctl -vl "'stf_isp0':1 -> 'stf_vin0_isp0_raw':0 [1]"
						# media-ctl -vl "'stf_vin0_isp0':1 -> 'stf_vin0_isp0_video1':0 [1]"
						;;
					ISP1)
						echo "csiphy0 CSIRX0 ISP1 enable pipeline:"
						media-ctl -vl "'stf_csiphy0':1 -> 'stf_csi0':0 [1]"
						media-ctl -vl "'stf_csi0':1 -> 'stf_isp1':0 [1]"
						media-ctl -vl "'stf_isp1':1 -> 'stf_vin0_isp1':0 [1]"
						# media-ctl -vl "'stf_vin0_isp1':1 -> 'stf_vin0_isp1_video2':0 [1]"
						;;
					ISP1RAW)
						echo "csiphy0 CSIRX0 ISP1RAW enable pipeline:"
						media-ctl -vl "'stf_csiphy0':1 -> 'stf_csi0':0 [1]"
						media-ctl -vl "'stf_csi0':1 -> 'stf_isp1':0 [1]"
						media-ctl -vl "'stf_isp1':1 -> 'stf_vin0_isp1_raw':0 [1]"
						# media-ctl -vl "'stf_vin0_isp1':1 -> 'stf_vin0_isp1_video2':0 [1]"
						;;

					*)
						echo $USAGE
						exit 1
						;;
				esac
				;;
			stop)
				# media-ctl -vl "'ov4689 0-0036':0 -> 'stf_csiphy0':0 [0]"
				case $2 in
					VIN)
						echo "csiphy0 CSIRX0 vin disable pipeline:"
						;;
					ISP0)
						echo "csiphy0 CSIRX0 ISP0 disable pipeline:"
						media-ctl -vl "'stf_csiphy0':1 -> 'stf_csi0':0 [0]"
						media-ctl -vl "'stf_csi0':1 -> 'stf_isp0':0 [0]"
						media-ctl -vl "'stf_isp0':1 -> 'stf_vin0_isp0':0 [0]"
						# media-ctl -vl "'stf_vin0_isp0':1 -> 'stf_vin0_isp0_video1':0 [0]"
						;;
					ISP0RAW)
						echo "csiphy0 CSIRX0 ISP0RAW disable pipeline:"
						media-ctl -vl "'stf_csiphy0':1 -> 'stf_csi0':0 [0]"
						media-ctl -vl "'stf_csi0':1 -> 'stf_isp0':0 [0]"
						media-ctl -vl "'stf_isp0':1 -> 'stf_vin0_isp0_raw':0 [0]"
						# media-ctl -vl "'stf_vin0_isp0':1 -> 'stf_vin0_isp0_video1':0 [0]"
						;;
					ISP1)
						echo "csiphy0 CSIRX0 ISP1 disable pipeline:"
						media-ctl -vl "'stf_csiphy0':1 -> 'stf_csi0':0 [0]"
						media-ctl -vl "'stf_csi0':1 -> 'stf_isp1':0 [0]"
						media-ctl -vl "'stf_isp1':1 -> 'stf_vin0_isp1':0 [0]"
						# media-ctl -vl "'stf_vin0_isp1':1 -> 'stf_vin0_isp1_video2':0 [0]"
						;;
					ISP1RAW)
						echo "csiphy0 CSIRX0 ISP1RAW disable pipeline:"
						media-ctl -vl "'stf_csiphy0':1 -> 'stf_csi0':0 [0]"
						media-ctl -vl "'stf_csi0':1 -> 'stf_isp1':0 [0]"
						media-ctl -vl "'stf_isp1':1 -> 'stf_vin0_isp1_raw':0 [0]"
						# media-ctl -vl "'stf_vin0_isp1':1 -> 'stf_vin0_isp1_video2':0 [0]"
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
		case $3 in
			start)
				# media-ctl -vl "'ov4689 2-0036':0 -> 'stf_csiphy1':0 [1]"
				case $2 in
					VIN)
						echo "csiphy1 CSIRX0 vin enable pipeline:"
						;;
					ISP0)
						echo "csiphy1 CSIRX1 ISP0 enable pipeline:"
						media-ctl -vl "'stf_csiphy1':1 -> 'stf_csi1':0 [1]"
						media-ctl -vl "'stf_csi1':1 -> 'stf_isp0':0 [1]"
						media-ctl -vl "'stf_isp0':1 -> 'stf_vin0_isp0':0 [1]"
						# media-ctl -vl "'stf_vin0_isp0':1 -> 'stf_vin0_isp0_video1':0 [1]"
						;;
					ISP0RAW)
						echo "csiphy1 CSIRX1 ISP0RAW enable pipeline:"
						media-ctl -vl "'stf_csiphy1':1 -> 'stf_csi1':0 [1]"
						media-ctl -vl "'stf_csi1':1 -> 'stf_isp0':0 [1]"
						media-ctl -vl "'stf_isp0':1 -> 'stf_vin0_isp0_raw':0 [1]"
						# media-ctl -vl "'stf_vin0_isp0':1 -> 'stf_vin0_isp0_video1':0 [1]"
						;;
					ISP1)
						echo "csiphy1 CSIRX1 ISP1 enable pipeline:"
						media-ctl -vl "'stf_csiphy1':1 -> 'stf_csi1':0 [1]"
						media-ctl -vl "'stf_csi1':1 -> 'stf_isp1':0 [1]"
						media-ctl -vl "'stf_isp1':1 -> 'stf_vin0_isp1':0 [1]"
						# media-ctl -vl "'stf_vin0_isp1':1 -> 'stf_vin0_isp1_video2':0 [1]"
						;;
					ISP1RAW)
						echo "csiphy1 CSIRX1 ISP1RAW enable pipeline:"
						media-ctl -vl "'stf_csiphy1':1 -> 'stf_csi1':0 [1]"
						media-ctl -vl "'stf_csi1':1 -> 'stf_isp1':0 [1]"
						media-ctl -vl "'stf_isp1':1 -> 'stf_vin0_isp1_raw':0 [1]"
						# media-ctl -vl "'stf_vin0_isp1':1 -> 'stf_vin0_isp1_video2':0 [1]"
						;;

					*)
						echo $USAGE
						exit 1
						;;
				esac
				;;
			stop)
				# media-ctl -vl "'ov4689 0-0036':0 -> 'stf_csiphy0':0 [0]"
				case $2 in
					VIN)
						echo "csiphy1 CSIRX0 vin disable pipeline:"
						;;
					ISP0)
						echo "csiphy1 CSIRX1 ISP0 disable pipeline:"
						media-ctl -vl "'stf_csiphy1':1 -> 'stf_csi1':0 [0]"
						media-ctl -vl "'stf_csi1':1 -> 'stf_isp0':0 [0]"
						media-ctl -vl "'stf_isp0':1 -> 'stf_vin0_isp0':0 [0]"
						# media-ctl -vl "'stf_vin0_isp0':1 -> 'stf_vin0_isp0_video1':0 [0]"
						;;
					ISP0RAW)
						echo "csiphy1 CSIRX1 ISP0RAW disable pipeline:"
						media-ctl -vl "'stf_csiphy1':1 -> 'stf_csi1':0 [0]"
						media-ctl -vl "'stf_csi1':1 -> 'stf_isp0':0 [0]"
						media-ctl -vl "'stf_isp0':1 -> 'stf_vin0_isp0_raw':0 [0]"
						# media-ctl -vl "'stf_vin0_isp0':1 -> 'stf_vin0_isp0_video1':0 [0]"
						;;
					ISP1)
						echo "csiphy1 CSIRX1 ISP1 disable pipeline:"
						media-ctl -vl "'stf_csiphy1':1 -> 'stf_csi1':0 [0]"
						media-ctl -vl "'stf_csi1':1 -> 'stf_isp1':0 [0]"
						media-ctl -vl "'stf_isp1':1 -> 'stf_vin0_isp1':0 [0]"
						# media-ctl -vl "'stf_vin0_isp1':1 -> 'stf_vin0_isp1_video2':0 [0]"
						;;
					ISP1RAW)
						echo "csiphy1 CSIRX1 ISP1RAW disable pipeline:"
						media-ctl -vl "'stf_csiphy1':1 -> 'stf_csi1':0 [0]"
						media-ctl -vl "'stf_csi1':1 -> 'stf_isp1':0 [0]"
						media-ctl -vl "'stf_isp1':1 -> 'stf_vin0_isp1_raw':0 [0]"
						# media-ctl -vl "'stf_vin0_isp1':1 -> 'stf_vin0_isp1_video2':0 [0]"
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
