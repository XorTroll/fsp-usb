#include "fspusb_drive.hpp"
#include "fspusb_usb_manager.hpp"

namespace fspusb::impl {

    Drive::Drive(UsbHsClientIfSession interface, UsbHsClientEpSession in_ep, UsbHsClientEpSession out_ep, u8 lun) : usb_interface(interface), usb_in_endpoint(in_ep), usb_out_endpoint(out_ep), mounted_idx(0xFF), scsi_context(nullptr), mounted(false) {
        this->scsi_context = new SCSIDriveContext(&this->usb_interface, &this->usb_in_endpoint, &this->usb_out_endpoint, lun);
    }

    Result Drive::Mount() {
        Result rc = 0;
        if(!this->mounted) {
            if(!this->scsi_context->Ok()) {
                return fspusb::ResultDriveInitializationFailure().GetValue();
            }

            /* Try to find a mountable index */
            if (FindAndMountAtIndex(&this->mounted_idx)) {
                FormatDriveMountName(this->mount_name, this->mounted_idx);
                auto ffrc = f_mount(&this->fat_fs, this->mount_name, 1);
                rc = fspusb::result::CreateFromFRESULT(ffrc).GetValue();
                if (R_SUCCEEDED(rc)) {
                    this->mounted = true;
                }
            }
        }
        return rc;
    }

    void Drive::Unmount() {
        if(this->mounted) {
            UnmountAtIndex(this->mounted_idx);
            f_mount(nullptr, this->mount_name, 1);
            memset(&this->fat_fs, 0, sizeof(this->fat_fs));
            memset(this->mount_name, 0, 0x10);
            this->mounted = false;
        }
    }

    void Drive::Dispose(bool close_usbhs) {
        if(this->scsi_context != nullptr) {
            delete this->scsi_context;
            this->scsi_context = nullptr;
        }
        
        if (close_usbhs) {
            usbHsIfResetDevice(&this->usb_interface);
            usbHsEpClose(&this->usb_in_endpoint);
            usbHsEpClose(&this->usb_out_endpoint);
            usbHsIfClose(&this->usb_interface);
        }
    }

}