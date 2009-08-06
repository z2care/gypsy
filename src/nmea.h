#ifndef NMEA_H
#define NMEA_H

#define LAYERID_TRANSPORT	 0
#define LAYERID_APPL		20

/* NMEA only allows space for 12 sats */
#define SAT_MAX_COUNT	12

/* the highest value of sat svid */
#define MAX_SAT_SVID	32

#define GSV_FIELDS 19
#define GSA_FIELDS 17
#define GGA_FIELDS 14
#define RMC_FIELDS 12

typedef enum {
	POSITION_NONE		= 0,
	POSITION_LATITUDE	= 1 << 0,
	POSITION_LONGITUDE	= 1 << 1,
	POSITION_ALTITUDE	= 1 << 2
} PositionFields;

typedef enum {
	COURSE_NONE		= 0,
	COURSE_SPEED		= 1 << 0,
	COURSE_DIRECTION	= 1 << 1,
	COURSE_CLIMB		= 1 << 2
} CourseFields;

typedef enum {
	FIX_INVALID = 0,
	FIX_NONE,
	FIX_2D,
	FIX_3D
} FixType;

typedef enum {
	ACCURACY_NONE		= 0,
	ACCURACY_POSITION	= 1 << 0, /* 3D */
	ACCURACY_HORIZONTAL	= 1 << 1, /* 2D */
	ACCURACY_VERTICAL	= 1 << 2, /* Altitude */
} AccuracyFields;

#endif

