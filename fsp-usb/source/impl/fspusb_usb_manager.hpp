
#pragma once
#include "fspusb_drive.hpp"
#include <functional>

namespace fspusb::impl {

    Result InitializeManager();
    void UpdateLoop();
    void ManagerUpdateThread(void *arg);
    void FinalizeManager();
    
    u32 GetAcquiredDriveCount();
    void DoWithDriveFATFS(u32 drive_idx, std::function<void(FATFS*)> fn);
}