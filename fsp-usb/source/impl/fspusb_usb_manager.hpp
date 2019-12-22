
#pragma once
#include "fspusb_drive.hpp"
#include <functional>

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