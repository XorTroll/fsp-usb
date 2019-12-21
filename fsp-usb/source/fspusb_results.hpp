
#pragma once
#include <stratosphere.hpp>

namespace fspusb {

    R_DEFINE_NAMESPACE_RESULT_MODULE(2);

    /* Use results from 8000 to 8999, since fs uses almost to 7000 :P */
    /* 81xx -> 8100 + FATFS error, since FATFS errors aren't bigger than 20 */

    R_DEFINE_ERROR_RESULT(InstructionAbort,     1);
    R_DEFINE_ERROR_RESULT(DataAbort,            2);
    R_DEFINE_ERROR_RESULT(AlignmentFault,       3);
    R_DEFINE_ERROR_RESULT(DebuggerAttached,     4);
    R_DEFINE_ERROR_RESULT(BreakPoint,           5);
    R_DEFINE_ERROR_RESULT(UserBreak,            6);
    R_DEFINE_ERROR_RESULT(DebuggerBreak,        7);
    R_DEFINE_ERROR_RESULT(UndefinedSystemCall,  8);
    R_DEFINE_ERROR_RESULT(SystemMemoryError,    9);

    R_DEFINE_ERROR_RESULT(IncompleteReport,     99);

    namespace result {

        NX_CONSTEXPR Result MakeFATFSResult(FRESULT err) {
            return MAKERESULT(fspusb::impl::result::ResultModuleId, 8100 + err);
        }

    }

}