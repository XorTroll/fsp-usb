
#pragma once
#include "impl/fspusb_usb_manager.hpp"
#include "fspusb_filesystem.hpp"
#include <stratosphere/fssrv/fssrv_interface_adapters.hpp>

using IFileSystemInterface = ams::fssrv::impl::FileSystemInterfaceAdapter;

namespace fspusb {

    class Service : public ams::sf::IServiceObject {

        private:
            enum class CommandId {
                GetMountedDriveCount = 0,
                GetDriveFileSystemType = 1,
                GetDriveLabel = 2,
                SetDriveLabel = 3,
                OpenDriveFileSystem = 4,
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

            ams::Result SetDriveLabel(u32 drive_idx, ams::sf::InBuffer &label_str) {
                R_UNLESS(impl::IsValidDriveIndex(drive_idx), ResultInvalidDriveIndex());

                char mountname[0x10] = {0};
                impl::FormatDriveMountName(mountname, drive_idx);

                char newname[0x100] = {0};
                sprintf(newname, "%s%s", mountname, (char*)label_str.GetPointer());

                auto ffrc = f_setlabel(newname);
                return result::CreateFromFRESULT(ffrc);
            }

            ams::Result OpenDriveFileSystem(u32 drive_idx, ams::sf::Out<std::shared_ptr<IFileSystemInterface>> out_fs) {
                R_UNLESS(impl::IsValidDriveIndex(drive_idx), ResultInvalidDriveIndex());

                std::shared_ptr<ams::fs::fsa::IFileSystem> drv_fs = std::make_shared<DriveFileSystem>(drive_idx);
                out_fs.SetValue(std::make_shared<IFileSystemInterface>(std::move(drv_fs), false));

                return ams::ResultSuccess();
            }

            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(GetMountedDriveCount),
                MAKE_SERVICE_COMMAND_META(GetDriveFileSystemType),
                MAKE_SERVICE_COMMAND_META(GetDriveLabel),
                MAKE_SERVICE_COMMAND_META(SetDriveLabel),
                MAKE_SERVICE_COMMAND_META(OpenDriveFileSystem),
            };
    };

}