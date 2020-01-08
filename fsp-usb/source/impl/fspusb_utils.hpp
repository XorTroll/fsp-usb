
#pragma once
#include "../fspusb_results.hpp"

#include <cstdio>

//#define FSP_USB_DEBUG

#ifdef FSP_USB_DEBUG
#define FSP_USB_LOG(fmt, ...) { \
    FILE *f = fopen("sdmc:/fspusb.log", "a+"); \
    if(f) { \
        fprintf(f, fmt "\n", ##__VA_ARGS__); \
        fclose(f); \
    } \
}
#else
#define FSP_USB_LOG(fmt, ...)
#endif

#define USB_TRANSFER_MEMORY_BLOCK_SIZE      0x1000  // 1 KiB
#define USB_TRANSFER_MEMORY_MAX_MULTIPLIER  8       // 32 KiB

namespace fspusb::impl {

    inline void FormatDriveMountName(char *str, u32 drive_mounted_idx) {
        memset(str, 0, strlen(str));
        sprintf(str, "%d:", drive_mounted_idx);
    }

}