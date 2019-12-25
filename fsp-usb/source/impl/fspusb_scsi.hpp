
#pragma once
#include "fspusb_utils.hpp"

#define SCSI_CBW_SIGNATURE 0x43425355
#define SCSI_COMMAND_PASSED 0
#define SCSI_COMMAND_FAILED 1
#define SCSI_PHASE_ERROR 2
#define SCSI_CSW_SIZE 13
#define SCSI_CSW_SIGNATURE 0x53425355

namespace fspusb::impl {

    enum class SCSIDirection {
        None,
        In,
        Out
    };

    class SCSIBuffer {

        public:
            static constexpr size_t BufferSize = 31;

        private:
            u32 idx = 0;
            u8 storage[BufferSize];

        public:
            SCSIBuffer();
            void Write8(u8 val);
            void WritePadding(u32 len);
            void Write16BE(u32 val);
            void Write32(u32 val);
            void Write32BE(u32 val);
            u8 *GetStorage();
    };

    class SCSICommand {

        private:
            u32 tag;
            u32 data_transfer_length;
            u8 flags;
            u8 lun;
            u8 cb_length;
            SCSIDirection direction;

        public:
            SCSICommand(u32 data_tr_len, SCSIDirection dir, u8 ln, u8 cb_len);
            virtual SCSIBuffer ProduceBuffer() = 0;
            u32 GetDataTransferLength();
            SCSIDirection GetDirection();
            void ToBytes(u8 *out);
            void WriteHeader(SCSIBuffer &out);
    };

    class SCSIInquiryCommand : public SCSICommand 
    {
        private:
            u8 allocation_length;
            u8 opcode;

        public:
            SCSIInquiryCommand(u8 alloc_len);
            virtual SCSIBuffer ProduceBuffer();
    };

    class SCSITestUnitReadyCommand : public SCSICommand {

        private:
            u8 opcode;
            
        public:
            SCSITestUnitReadyCommand();
            virtual SCSIBuffer ProduceBuffer();
    };


    class SCSIReadCapacityCommand : public SCSICommand {

        private:
            u8 opcode;
            
        public:
            SCSIReadCapacityCommand();
            virtual SCSIBuffer ProduceBuffer();
    };


    class SCSIRead10Command : public SCSICommand {

        private:
            u8 opcode;
            u32 block_address;
            u16 transfer_blocks;
            
        public:
            SCSIRead10Command(u32 block_addr, u32 block_sz, u16 xfer_blocks);
            virtual SCSIBuffer ProduceBuffer();
    };

    class SCSIWrite10Command : public SCSICommand {

        private:
            u8 opcode;
            u32 block_address;
            u16 transfer_blocks;
            
        public:
            SCSIWrite10Command(u32 block_addr, u32 block_sz, u16 xfer_blocks);
            virtual SCSIBuffer ProduceBuffer();
    };

    struct SCSICommandStatus {
        u32 signature;
        u32 tag;
        u32 data_residue;
        u8 status;
    };

    class SCSIDevice {

        public:
            static constexpr size_t BufferSize = 0x1000; 

        private:
            u8 *buf_a;
            u8 *buf_b;
            u8 *buf_c;
            UsbHsClientIfSession *client;
            UsbHsClientEpSession *in_endpoint;
            UsbHsClientEpSession *out_endpoint;
            bool ok;
        
        public:
            SCSIDevice(UsbHsClientIfSession *iface, UsbHsClientEpSession *in_ep, UsbHsClientEpSession *out_ep);
            ~SCSIDevice();
            void AllocateBuffers();
            void FreeBuffers();
            SCSICommandStatus ReadStatus();
            void PushCommand(SCSICommand &cmd);
            SCSICommandStatus TransferCommand(SCSICommand &c, u8 *buffer, size_t buffer_size);

            bool Ok() {
                return this->ok;
            }
    };

    struct MBRPartition {
        u8 active;
        u8 pad1[3];
        u8 partition_type;
        u8 pad2[3];
        u32 start_lba;
        u32 num_sectors;
    };

    class SCSIBlock;

    class SCSIBlockPartition {
        
        private:
            SCSIBlock *block;
            u32 start_lba_offset;
            u32 lba_size;
            bool empty;

        public:
            SCSIBlockPartition() : block(nullptr), start_lba_offset(0), lba_size(0), empty(true) {}
            SCSIBlockPartition(SCSIBlock *blk, MBRPartition partition_info) : block(blk), start_lba_offset(partition_info.start_lba), lba_size(partition_info.num_sectors), empty(false) {}

            int ReadSectors(u8 *buffer, u32 sector_offset, u32 num_sectors);
            int WriteSectors(const u8 *buffer, u32 sector_offset, u32 num_sectors);

            bool IsEmpty() {
                return this->empty;
            }
    };

    class SCSIBlock {
        
        private:
            u64 capacity;
            u32 block_size;
            SCSIBlockPartition partitions[4];
            MBRPartition partition_infos[4];
            SCSIDevice *device;
            bool ok;

        public:
            SCSIBlock(SCSIDevice *dev);
            int ReadPartitionSectors(u32 part_idx, u8 *buffer, u32 sector_offset, u32 num_sectors);
            int WritePartitionSectors(u32 part_idx, const u8 *buffer, u32 sector_offset, u32 num_sectors);
            int ReadSectors(u8 *buffer, u32 sector_offset, u32 num_sectors);
            int WriteSectors(const u8 *buffer, u32 sector_offset, u32 num_sectors);

            u32 GetFirstValidPartitionIndex() {
                for(u32 i = 0; i < 4; i++) {
                    if(!this->partitions[i].IsEmpty()) {
                        return i;
                    }
                }
                return 0xFF;
            }

            u32 GetBlockSize() {
                return this->block_size;
            }

            bool Ok() {
                if(this->device == nullptr) {
                    return false;
                }
                return this->ok && this->device->Ok();
            }
    };

    class SCSIDriveContext {

        private:
            SCSIDevice *device;
            SCSIBlock *block;

        public:
            SCSIDriveContext(UsbHsClientIfSession *interface, UsbHsClientEpSession *in_ep, UsbHsClientEpSession *out_ep) : device(nullptr), block(nullptr) {
                this->device = new SCSIDevice(interface, in_ep, out_ep);
                this->block = new SCSIBlock(this->device);
            }

            ~SCSIDriveContext() {
                if(this->block != nullptr) {
                    delete this->block;
                    this->block = nullptr;
                }
                if(this->device != nullptr) {
                    delete this->device;
                    this->device = nullptr;
                }
            }

            SCSIDevice *GetDevice() {
                return this->device;
            }

            SCSIBlock *GetBlock() {
                return this->block;
            }

            bool Ok() {
                return this->block->Ok();
            }
    };
}