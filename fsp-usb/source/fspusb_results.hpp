
#pragma once
#include <stratosphere.hpp>

namespace fspusb {

    R_DEFINE_NAMESPACE_RESULT_MODULE(2);

    /* Use results from 8000 to 8999, since fs uses almost to 7000 :P */
    /* 81xx -> 8100 + FATFS error, since FATFS errors aren't bigger than 20 */

    R_DEFINE_ERROR_RESULT(InvalidDriveIndex, 1);

    namespace result {

        NX_CONSTEXPR ams::Result CreateFromFRESULT(FRESULT err) {
            if(err == FR_OK) {
                return ams::ResultSuccess();
            }
            return MAKERESULT(fspusb::impl::result::ResultModuleId, 8100 + err);
        }

    }

}