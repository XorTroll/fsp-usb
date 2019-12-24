
#pragma once
#include "impl/fspusb_usb_manager.hpp"

namespace fspusb {

    class DriveFile : public ams::fs::fsa::IFile {

        private:
            u32 idx;
            s32 usb_iface_id;
            FIL file;

            bool IsDriveOk() {
                return impl::IsDriveOk(this->usb_iface_id);
            }

        public:
            DriveFile(s32 iface_id, FIL fil) : usb_iface_id(iface_id), file(fil) {}

            ~DriveFile() {
                f_close(&this->file);
            }

            virtual ams::Result ReadImpl(size_t *out, s64 offset, void *buffer, size_t size, const ams::fs::ReadOption &option) override final {
                R_UNLESS(this->IsDriveOk(), ResultDriveUnavailable());

                auto ffrc = f_lseek(&this->file, offset);
                if(ffrc == FR_OK) {
                    UINT read = 0;
                    ffrc = f_read(&this->file, buffer, size, &read);
                    if(ffrc == FR_OK) {
                        *out = read;
                    }
                }

                return result::CreateFromFRESULT(ffrc);
            }

            virtual ams::Result GetSizeImpl(s64 *out) override final {
                R_UNLESS(this->IsDriveOk(), ResultDriveUnavailable());

                *out = f_size(&this->file);

                return ams::ResultSuccess();
            }

            virtual ams::Result FlushImpl() override final {
                R_UNLESS(this->IsDriveOk(), ResultDriveUnavailable());
                return ams::ResultSuccess();
            }

            virtual ams::Result WriteImpl(s64 offset, const void *buffer, size_t size, const ams::fs::WriteOption &option) override final {
                R_UNLESS(this->IsDriveOk(), ResultDriveUnavailable());

                auto ffrc = f_lseek(&this->file, offset);
                if(ffrc == FR_OK) {
                    UINT written = 0;
                    ffrc = f_write(&this->file, buffer, size, &written);
                    if(ffrc == FR_OK) {
                        if(option.HasFlushFlag()) {
                            R_TRY(this->FlushImpl());
                        }
                    }
                }

                return result::CreateFromFRESULT(ffrc);
            }

            virtual ams::Result SetSizeImpl(s64 size) override final {
                R_UNLESS(this->IsDriveOk(), ResultDriveUnavailable());

                auto ffrc = f_lseek(&this->file, size);

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