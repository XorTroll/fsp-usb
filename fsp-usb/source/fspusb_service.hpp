
#pragma once
#include "impl/fspusb_usb_manager.hpp"

namespace fspusb {

    class Service : public ams::sf::IServiceObject {

        private:
            enum class CommandId {
                GetAcquiredDriveCount = 0,
                TestResult = 1,
            };

        public:
            void GetAcquiredDriveCount(ams::sf::Out<u32> count) {
                count.SetValue(impl::GetAcquiredDriveCount());
            }

            ams::Result TestResult() {
                return result::MakeFATFSResult(FR_INVALID_NAME);
            }

            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(GetAcquiredDriveCount),
                MAKE_SERVICE_COMMAND_META(TestResult),
            };
    };

}