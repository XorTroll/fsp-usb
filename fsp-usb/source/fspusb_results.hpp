
#pragma once
#include <stratosphere.hpp>
#include "fatfs/ff.h"

namespace fspusb {

    R_DEFINE_NAMESPACE_RESULT_MODULE(2);

    /* Use custom results from 8000 to 8999, since FS uses almost to 7000 :P */
    /* FATFS errors are converted below, those who aren't handled are returned as 8100 + the error */

    R_DEFINE_ERROR_RESULT(InvalidDriveIndex,           8001);
    R_DEFINE_ERROR_RESULT(DriveUnavailable,            8002);
    R_DEFINE_ERROR_RESULT(DriveInitializationFailure,  8003);

    namespace result {

        NX_CONSTEXPR ams::Result CreateFromFRESULT(FRESULT err) {
            switch(err) {
                case FR_OK:
                    return ams::ResultSuccess();

                case FR_NO_FILE:
                case FR_NO_PATH:
                case FR_INVALID_NAME:
                    return ams::fs::ResultPathNotFound();

                case FR_EXIST:
                    return ams::fs::ResultPathAlreadyExists();

                case FR_WRITE_PROTECTED:
                    return ams::fs::ResultUnsupportedOperation();

                case FR_INVALID_DRIVE:
                    return ams::fs::ResultInvalidMountName();

                case FR_INVALID_PARAMETER:
                    return ams::fs::ResultInvalidArgument();

                /* TODO: more FATFS errors */

                default:
                    return MAKERESULT(fspusb::impl::result::ResultModuleId, 8100 + err);
            }
        }

    }

}