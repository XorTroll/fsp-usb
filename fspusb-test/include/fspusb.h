/**
 * @file fspusb.h
 * @brief USB filesystem extension (fsp-usb) service IPC wrapper.
 * @author XorTroll
 * @copyright libnx Authors
 */
#pragma once
#include <switch.h>

// This is basically FATFS's file system types.
typedef enum {
    FspUsbFileSystemType_FAT12 = 1,
    FspUsbFileSystemType_FAT16 = 2,
    FspUsbFileSystemType_FAT32 = 3,
    FspUsbFileSystemType_exFAT = 4,
} FspUsbFileSystemType;

/// Initialize fsp-usb.
Result fspusbInitialize(void);

/// Exit fsp-usb.
void fspusbExit(void);

/// Gets the Service object for the actual fsp-usb service session.
Service* fspusbGetServiceSession(void);

Result fspusbGetMountedDriveCount(u32 *out_count);
Result fspusbGetDriveFileSystemType(u32 index, FspUsbFileSystemType *out_type);
Result fspusbGetDriveLabel(u32 index, char *out_label, size_t out_label_size);
Result fspusbSetDriveLabel(u32 index, const char *label);
Result fspusbOpenDriveFileSystem(u32 index, FsFileSystem *out_fs);