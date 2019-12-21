
#pragma once
#include "../fspusb_results.hpp"

namespace fspusb::impl {

    inline void FormatDriveMountName(const char *str, u32 drive_idx) {
        char *cstr = const_cast<char*>(str);
        memset(cstr, 0, strlen(cstr));
        sprintf(cstr, "usb-%d:", drive_idx);
    }

}