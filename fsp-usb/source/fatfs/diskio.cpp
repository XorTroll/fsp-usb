/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2016        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include "ff.h"
#include "diskio.h"
#include "../impl/fspusb_usb_manager.hpp"

static u8 GetDriveStatus(u32 drv_idx) {
	u8 status = STA_NOINIT;

	fspusb::impl::DoWithDrive(drv_idx, [&](fspusb::impl::DrivePointer &drive_ptr) {
		if(drive_ptr->IsSCSIOk()) {
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
	DWORD sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	auto res = RES_PARERR;

	fspusb::impl::DoWithDrive((u32)pdrv, [&](fspusb::impl::DrivePointer &drive_ptr) {
		auto part_idx = drive_ptr->GetValidPartitionIndex();
		if(part_idx < 4) {
			res = drive_ptr->DoReadSectors(part_idx, buff, sector, count);
		}
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
	DWORD sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	auto res = RES_PARERR;

	fspusb::impl::DoWithDrive((u32)pdrv, [&](fspusb::impl::DrivePointer &drive_ptr) {
		auto part_idx = drive_ptr->GetValidPartitionIndex();
		if(part_idx < 4) {
			res = drive_ptr->DoWriteSectors(0, buff, sector, count);
		}
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
	/* Shall we implement any ioctls here? */

	return RES_PARERR;
}
