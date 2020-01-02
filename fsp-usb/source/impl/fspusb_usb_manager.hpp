
#pragma once
#include "fspusb_drive.hpp"
#include <functional>

#define USB_CLASS_MASS_STORAGE      0x08
#define MASS_STORAGE_SCSI_COMMANDS  0x06
#define MASS_STORAGE_BULK_ONLY      0x50

namespace fspusb::impl {

    Result InitializeManager();
    void UpdateLoop();
    void ManagerUpdateThread(void *arg);
    void FinalizeManager();
    
    u32 GetAcquiredDriveCount();
    bool IsValidDriveIndex(u32 drive_idx);
    bool IsDriveOk(s32 drive_interface_id);
    void DoWithDrive(u32 drive_idx, std::function<void(DrivePointer&)> fn);
    void DoWithDriveFATFS(u32 drive_idx, std::function<void(FATFS*)> fn);
}