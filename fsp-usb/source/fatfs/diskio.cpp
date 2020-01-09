/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>

#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */

#include "../impl/fspusb_usb_manager.hpp"

// Reference for needed FATFS impl functions: http://irtos.sourceforge.net/FAT32_ChaN/doc/en/appnote.html#port

static u8 GetDriveStatus(u32 mounted_idx) {
	u8 status = STA_NOINIT;

	fspusb::impl::DoWithDriveMountedIndex(mounted_idx, [&](fspusb::impl::DrivePointer &drive_ptr) {
		if (drive_ptr->IsSCSIOk()) {
			status = 0;
		}
	});

	return status;
}

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

extern "C" DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	return GetDriveStatus((u32)pdrv);
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

extern "C" DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	return GetDriveStatus((u32)pdrv);
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

extern "C" DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	auto res = RES_PARERR;

	fspusb::impl::DoWithDriveMountedIndex((u32)pdrv, [&](fspusb::impl::DrivePointer &drive_ptr) {
        res = drive_ptr->DoReadSectors(buff, sector, count);
	});

	return res;
}


/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

extern "C" DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	auto res = RES_PARERR;

	fspusb::impl::DoWithDriveMountedIndex((u32)pdrv, [&](fspusb::impl::DrivePointer &drive_ptr) {
		res = drive_ptr->DoWriteSectors(buff, sector, count);
	});
	
	return res;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

extern "C" DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
    switch(cmd) {
        case GET_SECTOR_SIZE:
            fspusb::impl::DoWithDriveMountedIndex((u32)pdrv, [&](fspusb::impl::DrivePointer &drive_ptr) {
                *(WORD*)buff = (WORD)drive_ptr->GetBlockSize();
            });
            
            break;
        default:
            break;
    }
    
    return RES_OK;
}

#if !FF_FS_READONLY && !FF_FS_NORTC /* Get system time */
DWORD get_fattime(void)
{
    u64 timestamp = 0;
    DWORD output = 0;
    
    Result rc = timeGetCurrentTime(TimeType_LocalSystemClock, &timestamp);
    if (R_SUCCEEDED(rc))
    {
        time_t rawtime = (time_t)timestamp;
        struct tm *timeinfo = localtime(&rawtime);
        output = FAT_TIMESTAMP(timeinfo->tm_year, timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    } else {
        // Fallback to FF_NORTC definitions if the call failed
        output = FAT_TIMESTAMP(FF_NORTC_YEAR, FF_NORTC_MON, FF_NORTC_MDAY, 0, 0, 0);
    }
    
    return output;
}
#endif
