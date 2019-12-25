
#pragma once
#include "fspusb_file.hpp" 
#include "fspusb_directory.hpp"

namespace fspusb {

    class DriveFileSystem : public ams::fs::fsa::IFileSystem {

        private:
            u32 idx;
            s32 usb_iface_id;
            char mount_name[0x10];

            void DoWithDrive(std::function<void(impl::DrivePointer&)> fn) {
                impl::DoWithDrive(this->idx, fn);
            }

            void DoWithDriveFATFS(std::function<void(FATFS*)> fn) {
                this->DoWithDrive([&](impl::DrivePointer &drive_ptr) {
                    drive_ptr->DoWithFATFS(fn);
                });
            }

            bool IsDriveOk() {
                return impl::IsDriveOk(this->usb_iface_id);
            }

            void NormalizePath(char *out_path, const char *input_path) {
                memset(out_path, 0, strlen(out_path));
                sprintf(out_path, "%s%s", this->mount_name, input_path);
            }

        public:
            DriveFileSystem(u32 drive_idx) : idx(drive_idx) {
                this->DoWithDrive([&](impl::DrivePointer &drive_ptr) {
                    this->usb_iface_id = drive_ptr->GetInterfaceId();
                });
                impl::FormatDriveMountName(this->mount_name, this->idx);
            }

            virtual ams::Result CreateFileImpl(const char *path, s64 size, int flags) override final {
                R_UNLESS(this->IsDriveOk(), ResultDriveUnavailable());
                
                char ffpath[FS_MAX_PATH] = {0};
                this->NormalizePath(ffpath, path);

                auto ffrc = FR_OK;
                this->DoWithDriveFATFS([&](FATFS *fatfs) {
                    FIL fp = {};
                    ffrc = f_open(&fp, ffpath, FA_CREATE_NEW | FA_WRITE);
                    if(ffrc == FR_OK) {
                        f_lseek(&fp, size);
                        f_close(&fp);
                    }
                });

                return result::CreateFromFRESULT(ffrc);
            }

            virtual ams::Result DeleteFileImpl(const char *path) override final {
                R_UNLESS(this->IsDriveOk(), ResultDriveUnavailable());
                
                char ffpath[FS_MAX_PATH] = {0};
                this->NormalizePath(ffpath, path);

                auto ffrc = FR_OK;
                this->DoWithDriveFATFS([&](FATFS *fatfs) {
                    ffrc = f_unlink(ffpath);
                });

                return result::CreateFromFRESULT(ffrc);
            }

            virtual ams::Result CreateDirectoryImpl(const char *path) override final {
                R_UNLESS(this->IsDriveOk(), ResultDriveUnavailable());
                
                char ffpath[FS_MAX_PATH] = {0};
                this->NormalizePath(ffpath, path);

                auto ffrc = FR_OK;
                this->DoWithDriveFATFS([&](FATFS *fatfs) {
                    ffrc = f_mkdir(ffpath);
                });

                return result::CreateFromFRESULT(ffrc);
            }

            virtual ams::Result DeleteDirectoryImpl(const char *path) override final {
                R_UNLESS(this->IsDriveOk(), ResultDriveUnavailable());
                
                char ffpath[FS_MAX_PATH] = {0};
                this->NormalizePath(ffpath, path);

                auto ffrc = FR_OK;
                this->DoWithDriveFATFS([&](FATFS *fatfs) {
                    ffrc = f_rmdir(ffpath);
                });

                return result::CreateFromFRESULT(ffrc);
            }

            virtual ams::Result DeleteDirectoryRecursivelyImpl(const char *path) override final {
                /* TODO: this isn't recursive, make a proper implementation */
                return this->DeleteDirectoryImpl(path);
            }

            virtual ams::Result RenameFileImpl(const char *old_path, const char *new_path) override final {
                R_UNLESS(this->IsDriveOk(), ResultDriveUnavailable());
                
                char ffoldpath[FS_MAX_PATH] = {0};
                this->NormalizePath(ffoldpath, old_path);
                char ffnewpath[FS_MAX_PATH] = {0};
                this->NormalizePath(ffnewpath, new_path);

                auto ffrc = FR_OK;
                this->DoWithDriveFATFS([&](FATFS *fatfs) {
                    ffrc = f_rename(ffoldpath, ffnewpath);
                });

                return result::CreateFromFRESULT(ffrc);
            }

            virtual ams::Result RenameDirectoryImpl(const char *old_path, const char *new_path) override final {
                /* TODO: are these really the same? */
                return this->RenameFileImpl(old_path, new_path);
            }

            virtual ams::Result GetEntryTypeImpl(ams::fs::DirectoryEntryType *out, const char *path) override final {
                R_UNLESS(this->IsDriveOk(), ResultDriveUnavailable());
                
                char ffpath[FS_MAX_PATH] = {0};
                this->NormalizePath(ffpath, path);

                auto ffrc = FR_OK;
                this->DoWithDriveFATFS([&](FATFS *fatfs) {
                    FILINFO finfo = {};
                    ffrc = f_stat(ffpath, &finfo);
                    if(ffrc == FR_OK) {
                        if(finfo.fattrib & AM_DIR) {
                            *out = ams::fs::DirectoryEntryType_Directory;
                        }
                        else {
                            *out = ams::fs::DirectoryEntryType_File;
                        }
                    }
                });

                return result::CreateFromFRESULT(ffrc);
            }

            virtual ams::Result OpenFileImpl(std::unique_ptr<ams::fs::fsa::IFile> *out_file, const char *path, ams::fs::OpenMode mode) override final {
                R_UNLESS(this->IsDriveOk(), ResultDriveUnavailable());
                
                char ffpath[FS_MAX_PATH] = {0};
                this->NormalizePath(ffpath, path);

                BYTE openmode = FA_OPEN_ALWAYS;
                if(mode & ams::fs::OpenMode_Append) {
                    openmode = FA_OPEN_APPEND;
                }
                if(mode & ams::fs::OpenMode_Read) {
                    openmode |= FA_READ;
                }
                if(mode & ams::fs::OpenMode_Write) {
                    openmode |= FA_WRITE;
                }

                FIL fil = {};
                auto ffrc = FR_OK;
                this->DoWithDriveFATFS([&](FATFS *fatfs) {
                    ffrc = f_open(&fil, ffpath, openmode);
                });

                if(ffrc == FR_OK) {
                    *out_file = std::make_unique<DriveFile>(this->usb_iface_id, fil);
                }

                return result::CreateFromFRESULT(ffrc);
            }

            virtual ams::Result OpenDirectoryImpl(std::unique_ptr<ams::fs::fsa::IDirectory> *out_dir, const char *path, ams::fs::OpenDirectoryMode mode) override final {
                R_UNLESS(this->IsDriveOk(), ResultDriveUnavailable());
                
                char ffpath[FS_MAX_PATH] = {0};
                this->NormalizePath(ffpath, path);

                DIR dir = {};
                auto ffrc = FR_OK;
                this->DoWithDriveFATFS([&](FATFS *fatfs) {
                    ffrc = f_opendir(&dir, ffpath);
                });

                if(ffrc == FR_OK) {
                    *out_dir = std::make_unique<DriveDirectory>(this->usb_iface_id, dir);
                }

                return result::CreateFromFRESULT(ffrc);
            }

            virtual ams::Result CommitImpl() override final {
                R_UNLESS(this->IsDriveOk(), ResultDriveUnavailable());
                return ams::ResultSuccess();
            }

            virtual ams::Result GetFreeSpaceSizeImpl(s64 *out, const char *path) {
                R_UNLESS(this->IsDriveOk(), ResultDriveUnavailable());

                char ffpath[FS_MAX_PATH] = {0};
                this->NormalizePath(ffpath, path);

                u32 block_size = 0;
                this->DoWithDrive([&](impl::DrivePointer &drive_ptr) {
                    block_size = drive_ptr->GetBlockSize();
                });

                auto ffrc = FR_OK;
                this->DoWithDriveFATFS([&](FATFS *fatfs) {
                    FATFS *fs = nullptr;
                    DWORD clstrs = 0;
                    ffrc = f_getfree(ffpath, &clstrs, &fs);
                    if(ffrc == FR_OK) {
                        if(fs) {
                            *out = (s64)(clstrs * fs->csize) * block_size;
                        }
                    }
                });
                
                return result::CreateFromFRESULT(ffrc);
            }

            virtual ams::Result GetTotalSpaceSizeImpl(s64 *out, const char *path) {
                R_UNLESS(this->IsDriveOk(), ResultDriveUnavailable());

                char ffpath[FS_MAX_PATH] = {0};
                this->NormalizePath(ffpath, path);

                u32 block_size = 0;
                this->DoWithDrive([&](impl::DrivePointer &drive_ptr) {
                    block_size = drive_ptr->GetBlockSize();
                });

                auto ffrc = FR_OK;
                this->DoWithDriveFATFS([&](FATFS *fatfs) {
                    FATFS *fs = nullptr;
                    DWORD clstrs = 0;
                    ffrc = f_getfree(ffpath, &clstrs, &fs);
                    if(ffrc == FR_OK) {
                        if(fs) {
                            *out = (s64)((fs->n_fatent - 2) * fs->csize) * block_size;
                        }
                    }
                });
                
                return result::CreateFromFRESULT(ffrc);
            }

            virtual ams::Result CleanDirectoryRecursivelyImpl(const char *path) {
                /* TODO: clean = delete everything except the dir itself? */
                auto rc = this->DeleteDirectoryRecursivelyImpl(path);
                if(R_SUCCEEDED(rc)) {
                    rc = this->CreateDirectoryImpl(path);
                }
                return rc;
            }

            virtual ams::Result GetFileTimeStampRawImpl(ams::fs::FileTimeStampRaw *out, const char *path) {
                R_UNLESS(this->IsDriveOk(), ResultDriveUnavailable());
                /* TODO */
                return ams::ResultSuccess();
            }

            virtual ams::Result QueryEntryImpl(char *dst, size_t dst_size, const char *src, size_t src_size, ams::fs::fsa::QueryId query, const char *path) {
                R_UNLESS(this->IsDriveOk(), ResultDriveUnavailable());
                /* TODO */
                return ams::ResultSuccess();
            }
    };

}