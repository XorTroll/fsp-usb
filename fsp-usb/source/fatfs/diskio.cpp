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
#include <switch.h>
#include <vector>
#include <thread>
#include "ff.h"
#include "diskio.h"
#include "../impl/fspusb_drive.hpp"

namespace fspusb::impl {
	extern ams::os::Mutex g_usb_manager_lock;
	extern std::vector<fspusb::impl::DrivePointer> g_usb_manager_drives;
}

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

extern "C" DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	return 0;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

extern "C" DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	return 0;
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
	std::scoped_lock lk(fspusb::impl::g_usb_manager_lock);
	int wres = fspusb::impl::g_usb_manager_drives[pdrv]->GetSCSIContext()->GetBlockPartition(0).ReadSectors(buff, sector, count);
	if(wres != 0) {
		return RES_OK;
	}

	return RES_PARERR;
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
	std::scoped_lock lk(fspusb::impl::g_usb_manager_lock);
	int wres = fspusb::impl::g_usb_manager_drives[pdrv]->GetSCSIContext()->GetBlockPartition(0).WriteSectors(buff, sector, count);
	if(wres != 0) {
		return RES_OK;
	}

	return RES_PARERR;
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
	return RES_OK;
}
