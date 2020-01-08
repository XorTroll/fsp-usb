
#pragma once
#include "fspusb_drive.hpp"
#include "fspusb_request.hpp"
#include <functional>

#define USB_CLASS_MASS_STORAGE      0x08
#define MASS_STORAGE_SCSI_COMMANDS  0x06
#define MASS_STORAGE_BULK_ONLY      0x50

namespace fspusb::impl {

    constexpr u32 InvalidMountedIndex = 0xFF;

    Result InitializeManager();
    void FinalizeManager();
    void DoUpdateDrives();
    
    bool FindAndMountAtIndex(u32 *out_mounted_idx);
    void UnmountAtIndex(u32 mounted_idx);
    size_t GetAcquiredDriveCount();
    bool IsDriveInterfaceIdValid(s32 drive_interface_id);
    u32 GetDriveMountedIndex(s32 drive_interface_id);
    s32 GetDriveInterfaceId(u32 drive_idx);
    void DoWithDrive(s32 drive_interface_id, std::function<void(DrivePointer&)> fn);
    void DoWithDriveMountedIndex(u32 drive_mounted_idx, std::function<void(DrivePointer&)> fn);
    void DoWithDriveFATFS(s32 drive_interface_id, std::function<void(FATFS*)> fn);
}