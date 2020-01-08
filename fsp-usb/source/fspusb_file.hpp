
#pragma once
#include "impl/fspusb_usb_manager.hpp"

namespace fspusb {

    class DriveFile : public ams::fs::fsa::IFile {

        private:
            s32 usb_iface_id;
            FIL file;

            bool IsDriveInterfaceIdValid() {
                return impl::IsDriveInterfaceIdValid(this->usb_iface_id);
            }

        public:
            DriveFile(s32 iface_id, FIL fil) : usb_iface_id(iface_id), file(fil) {}

            ~DriveFile() {
                f_close(&this->file);
            }

            virtual ams::Result ReadImpl(size_t *out, s64 offset, void *buffer, size_t size, const ams::fs::ReadOption &option) override final {
                R_UNLESS(this->IsDriveInterfaceIdValid(), ResultDriveUnavailable());

                auto ffrc = f_lseek(&this->file, (u64)offset);
                if (ffrc == FR_OK) {
                    UINT btr = (UINT)size, br = 0;
                    ffrc = f_read(&this->file, buffer, btr, &br);
                    if (ffrc == FR_OK) *out = (size_t)br;
                }

                return result::CreateFromFRESULT(ffrc);
            }

            virtual ams::Result GetSizeImpl(s64 *out) override final {
                R_UNLESS(this->IsDriveInterfaceIdValid(), ResultDriveUnavailable());

                *out = (s64)f_size(&this->file);

                return ams::ResultSuccess();
            }

            virtual ams::Result FlushImpl() override final {
                R_UNLESS(this->IsDriveInterfaceIdValid(), ResultDriveUnavailable());
                return ams::ResultSuccess();
            }

            virtual ams::Result WriteImpl(s64 offset, const void *buffer, size_t size, const ams::fs::WriteOption &option) override final {
                R_UNLESS(this->IsDriveInterfaceIdValid(), ResultDriveUnavailable());

                auto ffrc = f_lseek(&this->file, (u64)offset);
                if (ffrc == FR_OK) {
                    UINT btw = (UINT)size, bw = 0;
                    ffrc = f_write(&this->file, buffer, btw, &bw);
                }

                // We ignore the flush flag since everything is written right away
                return result::CreateFromFRESULT(ffrc);
            }

            virtual ams::Result SetSizeImpl(s64 size) override final {
                R_UNLESS(this->IsDriveInterfaceIdValid(), ResultDriveUnavailable());

                u64 new_size = (u64)size;
                u64 cur_size = f_size(&this->file);

                auto ffrc = f_lseek(&this->file, new_size);

                // f_lseek takes care of expanding the file if new_size > cur_size
                // However, if new_size < cur_size, we must also call f_truncate
                if (ffrc == FR_OK && new_size < cur_size) ffrc = f_truncate(&this->file);

                return result::CreateFromFRESULT(ffrc);
            }

            virtual ams::Result OperateRangeImpl(void *dst, size_t dst_size, ams::fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override final {
                /* TODO: How should this be handled? */
                return ams::fs::ResultNotImplemented();
            }

            virtual ams::sf::cmif::DomainObjectId GetDomainObjectId() const override {
                return ams::sf::cmif::InvalidDomainObjectId;
            }
    };

}