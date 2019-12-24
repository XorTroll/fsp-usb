
#pragma once
#include "impl/fspusb_usb_manager.hpp"

namespace fspusb {

    class DriveDirectory : public ams::fs::fsa::IDirectory {

        private:
            s32 usb_iface_id;
            DIR directory;

            bool IsDriveOk() {
                return impl::IsDriveOk(this->usb_iface_id);
            }

        public:
            DriveDirectory(s32 iface_id, DIR dir) : usb_iface_id(iface_id), directory(dir) {}

            ~DriveDirectory() {
                f_closedir(&this->directory);
            }

            virtual ams::Result ReadImpl(s64 *out_count, ams::fs::DirectoryEntry *out_entries, s64 max_entries) override final {
                R_UNLESS(this->IsDriveOk(), ResultDriveUnavailable());

                auto ffrc = FR_OK;
                ams::fs::DirectoryEntry entry = {};
                FILINFO info = {};
                s64 count = 0;
                while(true) {
                    if(count >= max_entries) {
                        break;
                    }
                    ffrc = f_readdir(&this->directory, &info);
                    if((ffrc != FR_OK) || (info.fname[0] == '\0')) {
                        break;
                    }
                    memset(&entry, 0, sizeof(ams::fs::DirectoryEntry));
                    strcpy(entry.name, info.fname);
                    /* Fill in the DirectoryEntry struct, then copy back to the buffer */
                    if(info.fattrib & AM_DIR) {
                        entry.type = FsDirEntryType_Dir;
                    }
                    else {
                        entry.type = FsDirEntryType_File;
                        entry.file_size = info.fsize;
                    }
                    memcpy(&out_entries[count], &entry, sizeof(entry));
                    count++;
                }
                *out_count = count;

                return result::CreateFromFRESULT(ffrc);
            }

            virtual ams::Result GetEntryCountImpl(s64 *out) override final {
                R_UNLESS(this->IsDriveOk(), ResultDriveUnavailable());

                s64 count = 0;
                auto ffrc = FR_OK;
                FILINFO info = {};
                while(true) {
                    ffrc = f_readdir(&this->directory, &info);
                    if((ffrc != FR_OK) || (info.fname[0] == '\0')) {
                        break;
                    }
                    count++;
                }
                *out = count;

                return result::CreateFromFRESULT(ffrc);
            }

            virtual ams::sf::cmif::DomainObjectId GetDomainObjectId() const override {
                return ams::sf::cmif::InvalidDomainObjectId;
            }

    };

}