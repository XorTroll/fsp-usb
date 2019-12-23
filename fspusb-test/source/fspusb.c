#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "fspusb.h"
#include "service_guard.h"

#include <string.h>

static Service g_fspusbSrv;

NX_GENERATE_SERVICE_GUARD(fspusb);

Result _fspusbInitialize(void) {
    return smGetService(&g_fspusbSrv, "fsp-usb");
}

void _fspusbCleanup(void) {
    serviceClose(&g_fspusbSrv);
}

Service* fspusbGetServiceSession(void) {
    return &g_fspusbSrv;
}

Result fspusbGetMountedDriveCount(u32 *out_count) {
    return serviceDispatchOut(&g_fspusbSrv, 0, *out_count);
}

Result fspusbGetDriveFileSystemType(u32 index, FspUsbFileSystemType *out_type) {
    return serviceDispatchInOut(&g_fspusbSrv, 1, index, *out_type);
}

Result fspusbGetDriveLabel(u32 index, char *out_label, size_t out_label_size) {
    return serviceDispatchIn(&g_fspusbSrv, 2, index,
        .buffer_attrs = { SfBufferAttr_Out | SfBufferAttr_HipcMapAlias },
        .buffers = { { out_label, out_label_size } },
    );
}

Result fspusbSetDriveLabel(u32 index, const char *label) {
    char inputlbl[11 + 1] = {0}; // Actual limit is 11 characters
    strncpy(inputlbl, label, 11);
    return serviceDispatchIn(&g_fspusbSrv, 3, index,
        .buffer_attrs = { SfBufferAttr_In | SfBufferAttr_HipcMapAlias },
        .buffers = { { inputlbl, 11 + 1 } },
    );
}

Result fspusbOpenDriveFileSystem(u32 index, FsFileSystem *out_fs) {
    return serviceDispatchIn(&g_fspusbSrv, 4, index,
        .out_num_objects = 1,
        .out_objects = &out_fs->s,
    );
}