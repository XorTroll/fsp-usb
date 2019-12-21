
#pragma once
#include <thread>
#include <memory>
#include "../fatfs/ff.h"
#include "fspusb_utils.hpp"
#include "fspusb_scsi.hpp"

namespace fspusb::impl {

    // Maximum amount of drives
    constexpr u32 DriveMax = 4;

    class Drive {
            NON_COPYABLE(Drive);
            NON_MOVEABLE(Drive);

        private:
            UsbHsClientIfSession usb_interface;
            UsbHsClientEpSession usb_in_endpoint;
            UsbHsClientEpSession usb_out_endpoint;
            FATFS fat_fs;
            char mount_name[0x10];
            SCSIDriveContext *scsi_context;
            bool mounted;

        public:
            Drive(UsbHsClientIfSession interface, UsbHsClientEpSession in_ep, UsbHsClientEpSession out_ep);
            Result Mount(u32 drive_idx);
            void Unmount();
            void Dispose();

            UsbHsClientIfSession *GetInterfaceAccess() {
                return &this->usb_interface;
            }

            SCSIDriveContext *GetSCSIContext() {
                return this->scsi_context;
            }

            FATFS *GetFATFSAccess() {
                return &this->fat_fs;
            }

            const char *GetMountName() {
                return this->mount_name;
            }
    };

    /* For convenience :P */
    using DrivePointer = std::unique_ptr<Drive>;
}