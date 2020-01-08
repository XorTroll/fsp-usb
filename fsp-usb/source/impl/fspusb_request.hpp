
#pragma once
#include "fspusb_drive.hpp"
#include <functional>

#define USB_REQUEST_GET_CONFIG          0x08
#define USB_REQUEST_SET_CONFIG          0x09
#define USB_REQUEST_SET_INTERFACE       0x0B
#define USB_REQUEST_EP_GET_STATUS       0x00
#define USB_REQUEST_EP_CLEAR_FEATURE    0x01
#define USB_REQUEST_BULK_GET_MAX_LUN    0xFE
#define USB_REQUEST_BULK_RESET          0xFF

#define USB_CTRLTYPE_DIR_HOST2DEVICE    (0<<7)
#define USB_CTRLTYPE_DIR_DEVICE2HOST    (1<<7)
#define USB_CTRLTYPE_TYPE_STANDARD      (0<<5)
#define USB_CTRLTYPE_TYPE_CLASS         (1<<5)
#define USB_CTRLTYPE_TYPE_VENDOR        (2<<5)
#define USB_CTRLTYPE_TYPE_RESERVED      (3<<5)
#define USB_CTRLTYPE_REC_DEVICE         0
#define USB_CTRLTYPE_REC_INTERFACE      1
#define USB_CTRLTYPE_REC_ENDPOINT       2
#define USB_CTRLTYPE_REC_OTHER          3

#define USB_FEATURE_ENDPOINT_HALT       0

#define USB_MAX_LUN                     15

namespace fspusb::impl {

    void *AllocUSBTransferMemoryBlock(u8 multiplier);
    void FreeUSBTransferMemoryBlock(void *buf);
    
    u8 GetEndpointStatus(UsbHsClientIfSession *interface, UsbHsClientEpSession *endpoint);
    Result ClearEndpointHalt(UsbHsClientIfSession *interface, UsbHsClientEpSession *endpoint);
    Result ResetBulkStorage(UsbHsClientIfSession *interface, UsbHsClientEpSession *in_endpoint, UsbHsClientEpSession *out_endpoint);
    
    u8 GetMaxLUN(UsbHsClientIfSession *interface);
    
    u8 GetUSBConfiguration(UsbHsClientIfSession *interface);
    Result SetUSBConfiguration(UsbHsClientIfSession *interface, u8 conf);
    
    Result SetUSBAlternativeInterface(UsbHsClientIfSession *interface, u8 alt_iface);
    
    Result PostUSBBuffer(UsbHsClientIfSession *interface, UsbHsClientEpSession *endpoint, void *buffer, u32 size, u32 *transferredSize);

}