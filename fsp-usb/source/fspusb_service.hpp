
#pragma once
#include "impl/fspusb_usb_manager.hpp"

namespace fspusb {

    class Service : public ams::sf::IServiceObject {

        private:
            enum class CommandId {
                GetMountedDriveCount = 0,
                GetDriveFileSystemType = 1,
                GetDriveLabel = 2,

                /* TODO: filesystem-related commands! */
            };

        public:
            void GetMountedDriveCount(ams::sf::Out<u32> out_count) {
                out_count.SetValue(impl::GetAcquiredDriveCount());
            }

            ams::Result GetDriveFileSystemType(u32 drive_idx, ams::sf::Out<u8> out_fs_type) {
                R_UNLESS(impl::IsValidDriveIndex(drive_idx), ResultInvalidDriveIndex());

                impl::DoWithDriveFATFS(drive_idx, [&](FATFS *fs) {
                    out_fs_type.SetValue(fs->fs_type);
                });

                return ams::ResultSuccess();
            }

            ams::Result GetDriveLabel(u32 drive_idx, ams::sf::OutBuffer &out_label_str) {
                R_UNLESS(impl::IsValidDriveIndex(drive_idx), ResultInvalidDriveIndex());

                char mountname[0x10] = {0};
                impl::FormatDriveMountName(mountname, drive_idx);

                auto ffrc = f_getlabel(mountname, (char*)out_label_str.GetPointer(), nullptr);
                return result::CreateFromFRESULT(ffrc);
            }

            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(GetMountedDriveCount),
                MAKE_SERVICE_COMMAND_META(GetDriveFileSystemType),
                MAKE_SERVICE_COMMAND_META(GetDriveLabel),
            };
    };

}