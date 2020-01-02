#include "fspusb_scsi.hpp"

namespace fspusb::impl {

    SCSIBuffer::SCSIBuffer() {
        memset(this->storage, 0, BufferSize);
    }

    void SCSIBuffer::Write8(u8 val) {
        memcpy(&this->storage[this->idx], &val, sizeof(u8));
        this->idx += sizeof(u8);
    }

    void SCSIBuffer::WritePadding(u32 len) {
        this->idx += len;
    }

    void SCSIBuffer::Write16BE(u16 val) {
        u16 tmpval = __builtin_bswap16(val);
        memcpy(&this->storage[this->idx], &tmpval, sizeof(u16));
        this->idx += sizeof(u16);
    }

    void SCSIBuffer::Write32(u32 val) {
        memcpy(&this->storage[this->idx], &val, sizeof(u32));
        this->idx += sizeof(u32);
    }

    void SCSIBuffer::Write32BE(u32 val)
    {
        u32 tmpval = __builtin_bswap32(val);
        memcpy(&this->storage[this->idx], &tmpval, sizeof(u32));
        this->idx += sizeof(u32);
    }

    u8 *SCSIBuffer::GetStorage() {
        return this->storage;
    }

    SCSICommand::SCSICommand(u32 data_tr_len, SCSIDirection dir, u8 ln, u8 cb_len) {
        this->tag = SCSI_TAG;

        this->data_transfer_length = data_tr_len;
        this->direction = dir;
        this->lun = ln;
        this->cb_length = cb_len;

        flags = (dir == SCSIDirection::In ? SCSI_CBW_IN : SCSI_CBW_OUT);
    }

    u32 SCSICommand::GetDataTransferLength() {
        return this->data_transfer_length;
    }

    SCSIDirection SCSICommand::GetDirection() {
        return this->direction;
    }

    void SCSICommand::ToBytes(u8 *out) {
        auto buffer = this->ProduceBuffer();
        memcpy(out, buffer.GetStorage(), SCSIBuffer::BufferSize);
    }

    void SCSICommand::WriteHeader(SCSIBuffer &out) {
        out.Write32(SCSI_CBW_SIGNATURE);
        out.Write32(this->tag);
        out.Write32(this->data_transfer_length);
        out.Write8(this->flags);
        out.Write8(this->lun);
        out.Write8(this->cb_length);
    };

    SCSITestUnitReadyCommand::SCSITestUnitReadyCommand() : SCSICommand(SCSI_TEST_UNIT_READY_REPLY_LEN, SCSIDirection::Out, SCSI_DEFAULT_LUN, SCSI_TEST_UNIT_READY_CB_LEN) {
        this->opcode = SCSI_TEST_UNIT_READY_CMD;
    }

    SCSIBuffer SCSITestUnitReadyCommand::ProduceBuffer() {
        SCSIBuffer buf;
        this->WriteHeader(buf);

        buf.Write8(this->opcode);

        return buf;
    }

    SCSIRequestSenseCommand::SCSIRequestSenseCommand(u8 alloc_len) : SCSICommand(alloc_len, SCSIDirection::In, SCSI_DEFAULT_LUN, SCSI_REQUEST_SENSE_CB_LEN) {
        this->opcode = SCSI_REQUEST_SENSE_CMD;
        this->allocation_length = alloc_len;
    }

    SCSIBuffer SCSIRequestSenseCommand::ProduceBuffer() {
        SCSIBuffer buf;
        this->WriteHeader(buf);

        buf.Write8(this->opcode);
        buf.WritePadding(4);
        buf.Write8(this->allocation_length);

        return buf;
    }

    SCSIInquiryCommand::SCSIInquiryCommand(u8 alloc_len) : SCSICommand(alloc_len, SCSIDirection::In, SCSI_DEFAULT_LUN, SCSI_INQUIRY_CB_LEN) {
        this->opcode = SCSI_INQUIRY_CMD;
        this->allocation_length = alloc_len;
    }

    SCSIBuffer SCSIInquiryCommand::ProduceBuffer() {
        SCSIBuffer buf;
        this->WriteHeader(buf);

        buf.Write8(this->opcode);
        buf.WritePadding(3);
        buf.Write8(this->allocation_length);

        return buf;
    }

    SCSIReadCapacityCommand::SCSIReadCapacityCommand() : SCSICommand(SCSI_READ_CAPACITY_REPLY_LEN, SCSIDirection::In, SCSI_DEFAULT_LUN, SCSI_READ_CAPACITY_CB_LEN) {
        this->opcode = SCSI_READ_CAPACITY_CMD;
    }

    SCSIBuffer SCSIReadCapacityCommand::ProduceBuffer() {
        SCSIBuffer buf;
        this->WriteHeader(buf);

        buf.Write8(this->opcode);

        return buf;
    }

    SCSIRead10Command::SCSIRead10Command(u32 block_addr, u32 block_sz, u16 xfer_blocks) : SCSICommand(block_sz * xfer_blocks, SCSIDirection::In, SCSI_DEFAULT_LUN, SCSI_READ_10_CB_LEN) {
        this->opcode = SCSI_READ_10_CMD;
        this->block_address = block_addr;
        this->transfer_blocks = xfer_blocks;
    }

    SCSIBuffer SCSIRead10Command::ProduceBuffer() {
        SCSIBuffer buf;
        this->WriteHeader(buf);

        buf.Write8(this->opcode);
        buf.WritePadding(1);
        buf.Write32BE(this->block_address);
        buf.WritePadding(1);
        buf.Write16BE(this->transfer_blocks);
        buf.WritePadding(1);

        return buf;
    }

    SCSIWrite10Command::SCSIWrite10Command(u32 block_addr, u32 block_sz, u16 xfer_blocks) : SCSICommand(block_sz * xfer_blocks, SCSIDirection::Out, SCSI_DEFAULT_LUN, SCSI_WRITE_10_CB_LEN) {
        this->opcode = SCSI_WRITE_10_CMD;
        this->block_address = block_addr;
        this->transfer_blocks = xfer_blocks;
    }

    SCSIBuffer SCSIWrite10Command::ProduceBuffer() {
        SCSIBuffer buf;
        this->WriteHeader(buf);

        buf.Write8(this->opcode);
        buf.WritePadding(1);
        buf.Write32BE(this->block_address);
        buf.WritePadding(1);
        buf.Write16BE(this->transfer_blocks);
        buf.WritePadding(1);

        return buf;
    }

    SCSIDevice::SCSIDevice(UsbHsClientIfSession *iface, UsbHsClientEpSession *in_ep, UsbHsClientEpSession *out_ep) : buf_a(nullptr), buf_b(nullptr), buf_c(nullptr), client(iface), in_endpoint(in_ep), out_endpoint(out_ep), ok(true) {
        this->AllocateBuffers();
    }

    SCSIDevice::~SCSIDevice() {
        this->FreeBuffers();
    }

    void SCSIDevice::AllocateBuffers() {
        if(this->buf_a == nullptr) {
            this->buf_a = new (std::align_val_t(0x1000)) u8[BufferSize]();
        }
        if(this->buf_b == nullptr) {
            this->buf_b = new (std::align_val_t(0x1000)) u8[BufferSize]();
        }
        if(this->buf_c == nullptr) {
            this->buf_c = new (std::align_val_t(0x1000)) u8[BufferSize]();
        }
    }

    void SCSIDevice::FreeBuffers() {
        if(this->buf_a != nullptr) {
            operator delete[](this->buf_a, std::align_val_t(0x1000));
        }
        if(this->buf_b != nullptr) {
            operator delete[](this->buf_b, std::align_val_t(0x1000));
        }
        if(this->buf_c != nullptr) {
            operator delete[](this->buf_c, std::align_val_t(0x1000));
        }
    }

    SCSICommandStatus SCSIDevice::ReadStatus() {
        u32 out_len;
        SCSICommandStatus status = {};

        if(this->ok) {
            auto rc = usbHsEpPostBuffer(this->out_endpoint, this->buf_c, 0x10, &out_len);
            if(R_SUCCEEDED(rc)) {
                memcpy(&status, this->buf_c, sizeof(status));
            }
            else {
                this->ok = false;
            }
        }

        return status;
    }

    void SCSIDevice::PushCommand(SCSICommand &cmd) {
        if(this->ok) {
            memset(this->buf_a, 0, BufferSize);
            cmd.ToBytes(this->buf_a);

            u32 out_len;
            auto rc = usbHsEpPostBuffer(this->in_endpoint, this->buf_a, SCSIBuffer::BufferSize, &out_len);
            this->ok = R_SUCCEEDED(rc);
        }
    }

    SCSICommandStatus SCSIDevice::TransferCommand(SCSICommand &c, u8 *buffer) {
        SCSICommandStatus status = {};
        
        if(this->ok) {
            Result rc = 0;
            
            this->PushCommand(c);
            u32 transfer_length = c.GetDataTransferLength();
            u32 transferred = 0;
            u32 total_transferred = 0;
            u32 block_size = (u32)BufferSize;
            
            if (this->ok && buffer != nullptr && transfer_length > 0)
            {
                if (c.GetDirection() == SCSIDirection::In) {
                    while(total_transferred < transfer_length) {
                        u32 cur_transfer_size = ((transfer_length - total_transferred) > block_size ? block_size : (transfer_length - total_transferred));
                        
                        rc = usbHsEpPostBuffer(this->out_endpoint, this->buf_b, cur_transfer_size, &transferred);
                        if (R_FAILED(rc) || !transferred) {
                            this->ok = false;
                            break;
                        }
                        
                        if (transferred == SCSI_CSW_SIZE)
                        {
                            memcpy(&status, this->buf_b, sizeof(status));
                            if (status.signature == SCSI_CSW_SIGNATURE && status.tag == SCSI_TAG) {
                                /* We weren't expecting a CSW, but we got one anyway */
                                return status;
                            }
                        }
                        
                        memcpy(buffer + total_transferred, this->buf_b, transferred);
                        total_transferred += transferred;
                    }
                } else {
                    while(total_transferred < transfer_length) {
                        u32 cur_transfer_size = ((transfer_length - total_transferred) > block_size ? block_size : (transfer_length - total_transferred));
                        
                        memcpy(this->buf_b, buffer + total_transferred, cur_transfer_size);
                        
                        rc = usbHsEpPostBuffer(this->in_endpoint, this->buf_b, cur_transfer_size, &transferred);
                        if (R_FAILED(rc) || !transferred) {
                            this->ok = false;
                            break;
                        }
                        
                        total_transferred += transferred;
                    }
                }
            }
        }
        
        status = this->ReadStatus();
        return status;
    }

    SCSIBlock::SCSIBlock(SCSIDevice *dev) : device(dev), ok(true) {
        SCSICommandStatus status;
        
        //SCSIInquiryCommand inquiry(SCSI_INQUIRY_REPLY_LEN);
        //u8 inquiry_response[SCSI_INQUIRY_REPLY_LEN] = {0};
        
        SCSITestUnitReadyCommand test_unit_ready;
        
        SCSIRequestSenseCommand request_sense(SCSI_REQUEST_SENSE_REPLY_LEN);
        u8 request_sense_response[SCSI_REQUEST_SENSE_REPLY_LEN] = {0};
        
        SCSIReadCapacityCommand read_capacity;
        u8 read_capacity_response[SCSI_READ_CAPACITY_REPLY_LEN] = {0};
        
        //status = this->device->TransferCommand(inquiry, inquiry_response);
        
        status = this->device->TransferCommand(test_unit_ready, nullptr);
        if (status.status != SCSI_CMD_STATUS_SUCCESS)
        {
            this->ok = true; // Manually set back to true, just in case
            SCSICommandStatus rs_status = this->device->TransferCommand(request_sense, request_sense_response);
            if (rs_status.status == SCSI_CMD_STATUS_SUCCESS)
            {
                switch(request_sense_response[2] & 0x0F)
                {
                    case SCSI_SENSE_NO_SENSE:
                    case SCSI_SENSE_RECOVERED_ERROR:
                    case SCSI_SENSE_UNIT_ATTENTION:
                        // Use Request Sense command status
                        status = rs_status;
                        break;
                    case SCSI_SENSE_ABORTED_COMMAND:
                        // Retry command
                        status = this->device->TransferCommand(test_unit_ready, nullptr);
                        break;
                    case SCSI_SENSE_NOT_READY:
                    case SCSI_SENSE_MEDIUM_ERROR:
                    case SCSI_SENSE_HARDWARE_ERROR:
                    case SCSI_SENSE_ILLEGAL_REQUEST:
                    case SCSI_SENSE_DATA_PROTECT:
                    case SCSI_SENSE_BLANK_CHECK:
                    case SCSI_SENSE_COPY_ABORTED:
                    case SCSI_SENSE_VOLUME_OVERFLOW:
                    case SCSI_SENSE_MISCOMPARE:
                        this->ok = false;
                        break;
                    default:
                        break;
                }
            } else {
                this->ok = false;
            }
            
            // We may need to further analyze these responses...
            /*FILE *rsresp = fopen("sdmc:/rsresp.bin", "wb");
            if (rsresp)
            {
                fwrite(request_sense_response, 1, 0x14, rsresp);
                fclose(rsresp);
            }*/
        }
        
        if (this->ok && status.status == SCSI_CMD_STATUS_SUCCESS) {
            u32 size_lba;
            u32 lba_bytes;
            
            status = this->device->TransferCommand(read_capacity, read_capacity_response);
            if (this->ok && status.status == SCSI_CMD_STATUS_SUCCESS) {
                memcpy(&size_lba, &read_capacity_response[0], 4);
                size_lba = __builtin_bswap32(size_lba);
                
                memcpy(&lba_bytes, &read_capacity_response[4], 4);
                lba_bytes = __builtin_bswap32(lba_bytes);
                
                this->capacity = (u64)(size_lba * lba_bytes);
                this->block_size = lba_bytes;
                
                if (!this->capacity || !this->block_size) this->ok = false;
            }
        }
    }

    int SCSIBlock::ReadSectors(u8 *buffer, u32 sector_offset, u32 num_sectors) {
        if(!this->Ok()) {
            return 0;
        }
        SCSIRead10Command read_ten(sector_offset, this->block_size, num_sectors);
        this->device->TransferCommand(read_ten, buffer);
        return num_sectors;
    }

    int SCSIBlock::WriteSectors(const u8 *buffer, u32 sector_offset, u32 num_sectors) {
        if(!this->Ok()) {
            return 0;
        }
        SCSIWrite10Command write_ten(sector_offset, this->block_size, num_sectors);
        this->device->TransferCommand(write_ten, (u8*)buffer);
        return num_sectors;
    }
}