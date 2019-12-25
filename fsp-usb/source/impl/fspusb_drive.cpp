#include "fspusb_drive.hpp"

namespace fspusb::impl {

    Drive::Drive(UsbHsClientIfSession interface, UsbHsClientEpSession in_ep, UsbHsClientEpSession out_ep) : usb_interface(interface), usb_in_endpoint(in_ep), usb_out_endpoint(out_ep), scsi_context(nullptr), mounted(false) {
        this->scsi_context = new SCSIDriveContext(&this->usb_interface, &this->usb_in_endpoint, &this->usb_out_endpoint);
    }

    Result Drive::Mount(u32 drive_idx) {
        Result rc = 0;
        if(!this->mounted) {
            if(!this->scsi_context->Ok()) {
                return fspusb::ResultDriveInitializationFailure().GetValue();
            }

            FormatDriveMountName(this->mount_name, drive_idx);
            auto ffrc = f_mount(&this->fat_fs, this->mount_name, 1);
            rc = fspusb::result::CreateFromFRESULT(ffrc).GetValue();
            if(R_SUCCEEDED(rc)) {
                this->mounted = true;
            } 
        }
        return rc;
    }

    void Drive::Unmount() {
        if(this->mounted) {
            f_mount(nullptr, this->mount_name, 1);
            memset(&this->fat_fs, 0, sizeof(this->fat_fs));
            memset(this->mount_name, 0, 0x10);
            this->mounted = false;
        }
    }

    void Drive::Dispose() {
        if(this->scsi_context != nullptr) {
            delete this->scsi_context;
            this->scsi_context = nullptr;
        }
        usbHsIfResetDevice(&this->usb_interface);
        usbHsEpClose(&this->usb_in_endpoint);
        usbHsEpClose(&this->usb_out_endpoint);
        usbHsIfClose(&this->usb_interface);
    }

}