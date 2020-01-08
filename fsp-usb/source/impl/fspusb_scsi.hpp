
#pragma once
#include "fspusb_utils.hpp"

#define SCSI_CBW_SIZE                           31
#define SCSI_CBW_SIGNATURE                      0x43425355
#define SCSI_CBW_IN                             0x80
#define SCSI_CBW_OUT                            0

#define SCSI_CSW_SIZE                           13
#define SCSI_CSW_SIGNATURE                      0x53425355

#define SCSI_TAG                                0xDEADBEEF

#define SCSI_CMD_STATUS_SUCCESS                 0
#define SCSI_CMD_STATUS_FAILED                  1
#define SCSI_CMD_STATUS_PHASE_ERROR             2

#define SCSI_TEST_UNIT_READY_CMD                0x00
#define SCSI_TEST_UNIT_READY_REPLY_LEN          0x00
#define SCSI_TEST_UNIT_READY_CB_LEN             0x06

#define SCSI_REQUEST_SENSE_CMD                  0x03
#define SCSI_REQUEST_SENSE_REPLY_LEN            0x12
#define SCSI_REQUEST_SENSE_CB_LEN               0x06

#define SCSI_READ_CAPACITY_10_CMD               0x25
#define SCSI_READ_CAPACITY_10_REPLY_LEN         0x08
#define SCSI_READ_CAPACITY_10_CB_LEN            0x0A

#define SCSI_SERVICE_ACTION_IN_CMD              0x9E
#define SCSI_SERVICE_ACTION_READ_CAPACITY_16    0x10
#define SCSI_READ_CAPACITY_16_REPLY_LEN         0x20
#define SCSI_READ_CAPACITY_16_CB_LEN            0x10

#define SCSI_READ_10_CMD                        0x28
#define SCSI_READ_10_CB_LEN                     0x0A

#define SCSI_READ_16_CMD                        0x88
#define SCSI_READ_16_CB_LEN                     0x10

#define SCSI_WRITE_10_CMD                       0x2A
#define SCSI_WRITE_10_CB_LEN                    0x0A

#define SCSI_WRITE_16_CMD                       0x8A
#define SCSI_WRITE_16_CB_LEN                    0x10

#define SCSI_SENSE_NO_SENSE                     0x00
#define SCSI_SENSE_RECOVERED_ERROR              0x01
#define SCSI_SENSE_NOT_READY                    0x02
#define SCSI_SENSE_MEDIUM_ERROR                 0x03
#define SCSI_SENSE_HARDWARE_ERROR               0x04
#define SCSI_SENSE_ILLEGAL_REQUEST              0x05
#define SCSI_SENSE_UNIT_ATTENTION               0x06
#define SCSI_SENSE_DATA_PROTECT                 0x07
#define SCSI_SENSE_BLANK_CHECK                  0x08
#define SCSI_SENSE_COPY_ABORTED                 0x0A
#define SCSI_SENSE_ABORTED_COMMAND              0x0B
#define SCSI_SENSE_VOLUME_OVERFLOW              0x0D
#define SCSI_SENSE_MISCOMPARE                   0x0E

#define SCSI_TRANSFER_RETRIES                   3

#define SCSI_MAX_BLOCK_10                       (u64)0xFFFFFFFF

namespace fspusb::impl {

    enum class SCSIDirection {
        None,
        In,
        Out
    };

    class SCSIBuffer {

        public:
            static constexpr size_t BufferSize = SCSI_CBW_SIZE;

        private:
            u32 idx = 0;
            u8 storage[BufferSize];

        public:
            SCSIBuffer();
            void Write8(u8 val);
            void WritePadding(u32 len);
            void Write16BE(u16 val);
            void Write32(u32 val);
            void Write32BE(u32 val);
            void Write64BE(u64 val);
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
            void SetDataTransferLength(u32 data_len);
            SCSIDirection GetDirection();
            void ToBytes(u8 *out);
            void WriteHeader(SCSIBuffer &out);
    };

    class SCSITestUnitReadyCommand : public SCSICommand {

        private:
            u8 opcode;
            
        public:
            SCSITestUnitReadyCommand(u8 lun);
            virtual SCSIBuffer ProduceBuffer();
    };
    
    class SCSIRequestSenseCommand : public SCSICommand 
    {
        private:
            u8 allocation_length;
            u8 opcode;

        public:
            SCSIRequestSenseCommand(u8 alloc_len, u8 lun);
            virtual SCSIBuffer ProduceBuffer();
    };

    class SCSIReadCapacity10Command : public SCSICommand {

        private:
            u8 opcode;
            
        public:
            SCSIReadCapacity10Command(u8 lun);
            virtual SCSIBuffer ProduceBuffer();
    };

    class SCSIReadCapacity16Command : public SCSICommand {

        private:
            u8 allocation_length;
            u8 opcode;
            
        public:
            SCSIReadCapacity16Command(u8 alloc_len, u8 lun);
            virtual SCSIBuffer ProduceBuffer();
    };

    class SCSIRead10Command : public SCSICommand {

        private:
            u8 opcode;
            u32 block_address;
            u16 transfer_blocks;
            
        public:
            SCSIRead10Command(u32 block_addr, u32 block_sz, u16 xfer_blocks, u8 lun);
            virtual SCSIBuffer ProduceBuffer();
    };

    class SCSIRead16Command : public SCSICommand {

        private:
            u8 opcode;
            u64 block_address;
            u32 transfer_blocks;
            
        public:
            SCSIRead16Command(u64 block_addr, u32 block_sz, u32 xfer_blocks, u8 lun);
            virtual SCSIBuffer ProduceBuffer();
    };

    class SCSIWrite10Command : public SCSICommand {

        private:
            u8 opcode;
            u32 block_address;
            u16 transfer_blocks;
            
        public:
            SCSIWrite10Command(u32 block_addr, u32 block_sz, u16 xfer_blocks, u8 lun);
            virtual SCSIBuffer ProduceBuffer();
    };

    class SCSIWrite16Command : public SCSICommand {

        private:
            u8 opcode;
            u64 block_address;
            u32 transfer_blocks;
            
        public:
            SCSIWrite16Command(u64 block_addr, u32 block_sz, u32 xfer_blocks, u8 lun);
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
            static constexpr size_t BufferSize = USB_TRANSFER_MEMORY_BLOCK_SIZE;

        private:
            u8 *buf_a; // Used to send SCSI commands
            u8 *buf_b; // Used to send/receive data
            u8 *buf_c; // Used to receive SCSI status
            UsbHsClientIfSession *client;
            UsbHsClientEpSession *in_endpoint;
            UsbHsClientEpSession *out_endpoint;
            bool ok;
            u8 dev_lun;
        
        public:
            SCSIDevice(UsbHsClientIfSession *iface, UsbHsClientEpSession *in_ep, UsbHsClientEpSession *out_ep, u8 lun);
            ~SCSIDevice();
            void AllocateBuffers();
            void FreeBuffers();
            SCSICommandStatus ReadStatus();
            void PushCommand(SCSICommand &cmd, u32 diff);
            SCSICommandStatus TransferCommand(SCSICommand &c, u8 *buffer);

            bool Ok() {
                return this->ok;
            }

            u8 GetDeviceLUN() {
                return this->dev_lun;
            }
    };

    class SCSIBlock {
        
        private:
            u64 capacity;
            u32 block_size;
            SCSIDevice *device;
            bool ok;

        public:
            SCSIBlock(SCSIDevice *dev);
            int ReadSectors(u8 *buffer, u64 sector_offset, u32 num_sectors);
            int WriteSectors(const u8 *buffer, u64 sector_offset, u32 num_sectors);

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
            SCSIDriveContext(UsbHsClientIfSession *interface, UsbHsClientEpSession *in_ep, UsbHsClientEpSession *out_ep, u8 lun) : device(nullptr), block(nullptr) {
                this->device = new SCSIDevice(interface, in_ep, out_ep, lun);
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