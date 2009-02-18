/*
	Garmin protocol to NMEA 0183 converter
	Copyright (C) 2004 Manuel Kasper <mk@neon1.net>.
	All rights reserved.

	Input:
		- D800_Pvt_Data_Type (PID 51)
		- satellite data record (PID 114)

	Available output sentences:
		GPGGA, GPRMC, GPGLL, GPGSA, GPGSV

	Known caveats:
		- DOP (Dilution of Precision) information not available
		  (Garmin protocol includes EPE only)
		- DGPS information in GPGGA sentence not returned
		- speed and course over ground are calculated from the
		  north/east velocity and may not be accurate
		- magnetic variation information not available
		- Garmin 16-bit SNR scale unknown

	---------------------------------------------------------------------------
	
	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:
	
	1. Redistributions of source code must retain the above copyright notice,
	   this list of conditions and the following disclaimer.
	
	2. Redistributions in binary form must reproduce the above copyright
	   notice, this list of conditions and the following disclaimer in the
	   documentation and/or other materials provided with the distribution.
	
	THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
	INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
	AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
	AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
	OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
	SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
	INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
	CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
	POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef GARMIN_H
#define GARMIN_H

#include <sys/types.h>
#include "nmea.h"


/*
 *  Garmin device driver defines
 */

/* private layer-id to use for some ioctl-like control mechanisms */
#define GARMIN_LAYERID_PRIVATE	0x01106E4B

// packet ids used in the private layer
#define GARMIN_PRIV_PKTID_SET_DEBUG	1
#define GARMIN_PRIV_PKTID_SET_MODE	2
#define GARMIN_PRIV_PKTID_INFO_REQ	3
#define GARMIN_PRIV_PKTID_INFO_RESP	4
#define GARMIN_PRIV_PKTID_RESET_REQ	5
#define GARMIN_PRIV_PKTID_SET_DEF_MODE	6

#define GARMIN_MODE_NATIVE	0
#define GARMIN_MODE_SERIAL	1

#define GARMIN_PRIV_PKT_MAX_SIZE	32

/*
	PRIV_PKTID_INFO_RESP packet:

	pkt[0] = __cpu_to_le32(GARMIN_LAYERID_PRIVATE);
	pkt[1] = __cpu_to_le32(PRIV_PKTID_INFO_RESP);
	pkt[2] = __cpu_to_le32(12);
	pkt[3] = __cpu_to_le32(VERSION_MAJOR << 16 | VERSION_MINOR);
	pkt[4] = __cpu_to_le32(garmin_data_p->mode);
	pkt[5] = __cpu_to_le32(garmin_data_p->serial_num);
*/

#define GARMIN_PRIV_PKT_INFO_RESP_SIZE	24

/*
 *  Garmin device definitions
 */

#pragma pack(push, 1)

typedef struct {
	u_int8_t	tag;
	u_int16_t	data;
} Protocol_Data_Type;

typedef struct
{
	float	alt;
	float	epe;
	float	eph;
	float	epv;
	int16_t	fix;
	double	tow;
	double	lat;
	double	lon;
	float	east;
	float	north;
	float	up;
	float	msl_hght;
	int16_t	leap_scnds;
	int32_t	wn_days;
}
D800_Pvt_Data_Type;

typedef struct
{
	u_int8_t	svid;
	u_int16_t	snr;
	u_int8_t	elev;
	u_int16_t	azmth;
	u_int8_t	status;
}
cpo_sat_data;

/*
    The following defines have been determined empirically.  I could not find
    any definitive info on the sat data record, even in the Garmin spec.  What
    I see is a status of 0x04 when a sat is being tracked but not used for locating,
    and a status of 0x05 when the sat is good.  I have not seen any other values.
    The mask is needed to filter out extraneous bits that are unknown.
    
    Here is a snapshot of what is seen in nmea_gpgsv():

	sat 11: status = 05  SNR = 1800
	sat 13: status = 04  SNR = 2200
	sat 16: status = 05  SNR = 3200
	sat 20: status = 05  SNR = 3600
	sat 23: status = 05  SNR = 3500
	sat 25: status = 04  SNR = 1900
	sat 31: status = 05  SNR = 3000
	sat 32: status = 05  SNR = 3700
	sat 04: status = 04  SNR = 65436
	sat 30: status = 04  SNR = 65436

    The SNR value of 65436 is the 16-bit 2's complement value of -100.
*/

#define SAT_STATUS_MASK	0x07
#define SAT_STATUS_GOOD	0x05
#define SAT_SNR_BAD	65436

typedef struct
{
	u_int32_t	cycles;
	double		pr;
	u_int16_t	phase;
	u_int8_t	slp_dtct;
	u_int8_t	snr_dbhz;
	u_int8_t	svid;
	u_int8_t	valid;
}
cpo_rcv_sv_data;

typedef struct
{
	double			rcvr_tow;
	u_int16_t		rcvr_wn;
	cpo_rcv_sv_data		sv[SAT_MAX_COUNT];
}
cpo_rcv_data;

/* This is the packet definition for Garmin */

#define GARMIN_HEADER_SIZE 12

typedef struct
{
	u_int8_t	mPacketType;	// byte 0
	u_int8_t	mReserved1;	// bytes 1-3 
	u_int16_t	mReserved2;
	u_int16_t	mPacketId;	// bytes 4-5
	u_int16_t	mReserved3;	// bytes 6-7
	u_int32_t	mDataSize;	// bytes 8-11
	u_int8_t	mData[1];	// bytes 12..N
}
G_Packet_t;

#pragma pack(pop)

enum {
	Pid_Command_Data	= 10,
	Pid_Xfer_Cmplt		= 12,
	Pid_Date_Time_Data	= 14,
	Pid_Position_Data	= 17,
	Pid_Prx_Wpt_Data	= 19,
	Pid_Records		= 27,
	Pid_Rte_Hdr		= 29,
	Pid_Rte_Wpt_Data	= 30,
	Pid_Almanac_Data	= 31,
	Pid_Trk_Data		= 34,
	Pid_Wpt_Data		= 35,
	Pid_Pvt_Data		= 51,
	Pid_RMR_Data		= 52,
	Pid_Rte_Link_Data	= 98,
	Pid_Trk_Hdr		= 99,
	Pid_SatData_Record	= 114,
	Pid_FlightBook_Record	= 134,
	Pid_Lap			= 149
};

enum {
	Cmnd_Abort_Transfer      =   0,	/* abort current transfer */
	Cmnd_Transfer_Alm        =   1,	/* transfer almanac */
	Cmnd_Transfer_Posn       =   2,	/* transfer position */
	Cmnd_Transfer_Prx        =   3,	/* transfer proximity waypoints */
	Cmnd_Transfer_Rte        =   4,	/* transfer routes */
	Cmnd_Transfer_Time       =   5,	/* transfer time */
	Cmnd_Transfer_Trk        =   6,	/* transfer track log */
	Cmnd_Transfer_Wpt        =   7,	/* transfer waypoints */
	Cmnd_Turn_Off_Pwr        =   8,	/* turn off power */
	Cmnd_Start_Pvt_Data      =  49,	/* start transmitting PVT data */
	Cmnd_Stop_Pvt_Data       =  50,	/* stop transmitting PVT data */
	Cmnd_FlightBook_Transfer =  92,	/* start transferring flight records */
	Cmnd_Start_RMR           = 110,	/* start transmitting Receiver Measurement Records */
	Cmnd_Stop_RMR            = 111,	/* start transmitting Receiver Measurement Records */
	Cmnd_Transfer_Laps       = 117	/* transfer laps */
};

#endif
