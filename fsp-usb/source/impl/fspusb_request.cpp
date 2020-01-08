#include "fspusb_request.hpp"

namespace fspusb::impl {

    void *AllocUSBTransferMemoryBlock(u8 multiplier) {
        u32 block_size = (USB_TRANSFER_MEMORY_BLOCK_SIZE * ((multiplier < 1 || multiplier > USB_TRANSFER_MEMORY_MAX_MULTIPLIER) ? 1 : multiplier));
        void *buf = new (std::align_val_t(USB_TRANSFER_MEMORY_BLOCK_SIZE)) u8[block_size]();
        return buf;
    }

    void FreeUSBTransferMemoryBlock(void *buf) {
        if (buf == nullptr) return;
        operator delete[](buf, std::align_val_t(USB_TRANSFER_MEMORY_BLOCK_SIZE));
    }

    // 0 = Not Halted, 1 = Halted
    u8 GetEndpointStatus(UsbHsClientIfSession *interface, UsbHsClientEpSession *endpoint) {
        u8 *status = (u8*)AllocUSBTransferMemoryBlock(1);
        if (status == nullptr) return 0;
        
        u8 ret_status;
        u32 transferredSize = 0;
        u16 ep_addr = (u16)(endpoint->desc.bEndpointAddress);
        
        Result rc = usbHsIfCtrlXfer(interface, (USB_CTRLTYPE_DIR_DEVICE2HOST | USB_CTRLTYPE_TYPE_STANDARD | USB_CTRLTYPE_REC_ENDPOINT), USB_REQUEST_EP_GET_STATUS, 0, ep_addr, 2, status, &transferredSize);
        
        svcSleepThread((u64)120000000); // 120 mS
        
        if (R_SUCCEEDED(rc) && transferredSize == 2) {
            ret_status = (*status & 0x01);
        } else {
            ret_status = 0;
        }
        
        FreeUSBTransferMemoryBlock(status);
        
        return ret_status;
    }

    Result ClearEndpointHalt(UsbHsClientIfSession *interface, UsbHsClientEpSession *endpoint) {
        // First check if this endpoint really is stalled
        if (!GetEndpointStatus(interface, endpoint)) return 0;
        
        u32 transferredSize = 0;
        u16 ep_addr = (u16)(endpoint->desc.bEndpointAddress);
        
        Result rc = usbHsIfCtrlXfer(interface, (USB_CTRLTYPE_DIR_HOST2DEVICE | USB_CTRLTYPE_TYPE_STANDARD | USB_CTRLTYPE_REC_ENDPOINT), USB_REQUEST_EP_CLEAR_FEATURE, USB_FEATURE_ENDPOINT_HALT, ep_addr, 0, nullptr, &transferredSize);
        
        svcSleepThread((u64)120000000); // 120 mS
        
        return rc;
    }

    Result ResetBulkStorage(UsbHsClientIfSession *interface, UsbHsClientEpSession *in_endpoint, UsbHsClientEpSession *out_endpoint) {
        u32 transferredSize = 0;
        u16 iface_num = (u16)(interface->inf.inf.interface_desc.bInterfaceNumber);
        
        Result rc = usbHsIfCtrlXfer(interface, (USB_CTRLTYPE_DIR_HOST2DEVICE | USB_CTRLTYPE_TYPE_CLASS | USB_CTRLTYPE_REC_INTERFACE), USB_REQUEST_BULK_RESET, 0, iface_num, 0, nullptr, &transferredSize);
        
        svcSleepThread((u64)120000000); // 120 mS
        
        if (R_SUCCEEDED(rc)) {
            ClearEndpointHalt(interface, in_endpoint);
            ClearEndpointHalt(interface, out_endpoint);
        }
        
        return rc;
    }

    u8 GetMaxLUN(UsbHsClientIfSession *interface) {
        u8 *max_lun = (u8*)AllocUSBTransferMemoryBlock(1);
        if (max_lun == nullptr) return 1;
        
        u8 ret_lun = 0;
        u32 transferredSize = 0;
        u16 iface_num = (u16)(interface->inf.inf.interface_desc.bInterfaceNumber);
        
        Result rc = usbHsIfCtrlXfer(interface, (USB_CTRLTYPE_DIR_DEVICE2HOST | USB_CTRLTYPE_TYPE_CLASS | USB_CTRLTYPE_REC_INTERFACE), USB_REQUEST_BULK_GET_MAX_LUN, 0, iface_num, 1, max_lun, &transferredSize);
        
        svcSleepThread((u64)120000000); // 120 mS
        
        if (R_SUCCEEDED(rc) && transferredSize == 1 && *max_lun < USB_MAX_LUN) {
            ret_lun = (*max_lun + 1);
        } else {
            // Assume this a drive with a single LUN
            ret_lun = 1;
        }
        
        FreeUSBTransferMemoryBlock(max_lun);
        
        return ret_lun;
    }

    u8 GetUSBConfiguration(UsbHsClientIfSession *interface) {
        u8 *conf = (u8*)AllocUSBTransferMemoryBlock(1);
        if (conf == nullptr) return 0;
        
        u8 ret_conf = 0;
        u32 transferredSize = 0;
        
        Result rc = usbHsIfCtrlXfer(interface, (USB_CTRLTYPE_DIR_DEVICE2HOST | USB_CTRLTYPE_TYPE_STANDARD | USB_CTRLTYPE_REC_DEVICE), USB_REQUEST_GET_CONFIG, 0, 0, 1, conf, &transferredSize);
        
        svcSleepThread((u64)120000000); // 120 mS
        
        if (R_SUCCEEDED(rc) && transferredSize == 1) {
            ret_conf = *conf;
        } else {
            ret_conf = 0;
        }
        
        FreeUSBTransferMemoryBlock(conf);
        
        return ret_conf;
    }
    
    Result SetUSBConfiguration(UsbHsClientIfSession *interface, u8 conf) {
        u32 transferredSize = 0;
        
        Result rc = usbHsIfCtrlXfer(interface, (USB_CTRLTYPE_DIR_HOST2DEVICE | USB_CTRLTYPE_TYPE_STANDARD | USB_CTRLTYPE_REC_DEVICE), USB_REQUEST_SET_CONFIG, conf, 0, 0, nullptr, &transferredSize);
        
        svcSleepThread((u64)120000000); // 120 mS
        
        return rc;
    }

    Result SetUSBAlternativeInterface(UsbHsClientIfSession *interface, u8 alt_iface) {
        u32 transferredSize = 0;
        u16 iface_num = (u16)(interface->inf.inf.interface_desc.bInterfaceNumber);
        
        Result rc = usbHsIfCtrlXfer(interface, (USB_CTRLTYPE_DIR_HOST2DEVICE | USB_CTRLTYPE_TYPE_STANDARD | USB_CTRLTYPE_REC_INTERFACE), USB_REQUEST_SET_INTERFACE, alt_iface, iface_num, 0, nullptr, &transferredSize);
        
        svcSleepThread((u64)120000000); // 120 mS
        
        return rc;
    }

    Result PostUSBBuffer(UsbHsClientIfSession *interface, UsbHsClientEpSession *endpoint, void *buffer, u32 size, u32 *transferredSize) {
        Result rc = usbHsEpPostBuffer(endpoint, buffer, size, transferredSize);
        if (R_FAILED(rc)) ClearEndpointHalt(interface, endpoint);
        return rc;
    }

}