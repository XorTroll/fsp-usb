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

    void SCSIBuffer::Write16BE(u32 val) {
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
        this->tag = 0;

        this->data_transfer_length = data_tr_len;
        this->direction = dir;
        this->lun = ln;
        this->cb_length = cb_len;

        if(dir == SCSIDirection::In) {
            flags = 0x80;
        }
        else {
            flags = 0;
        }
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

    SCSIInquiryCommand::SCSIInquiryCommand(u8 alloc_len) : SCSICommand(alloc_len, SCSIDirection::In, 0, 5) {
        this->opcode = 0x12;
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

    SCSITestUnitReadyCommand::SCSITestUnitReadyCommand() : SCSICommand(0, SCSIDirection::None, 0, 6) {
        this->opcode = 0;
    }

    SCSIBuffer SCSITestUnitReadyCommand::ProduceBuffer() {
        SCSIBuffer buf;
        this->WriteHeader(buf);

        buf.Write8(this->opcode);

        return buf;
    }

    SCSIRequestSenseCommand::SCSIRequestSenseCommand(u8 alloc_len) : SCSICommand(alloc_len, SCSIDirection::In, 0, 5) {
        this->opcode = 0x03;
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

    SCSIReadCapacityCommand::SCSIReadCapacityCommand() : SCSICommand(0x8, SCSIDirection::In, 0, 0x10) {
        this->opcode = 0x25;
    }

    SCSIBuffer SCSIReadCapacityCommand::ProduceBuffer() {
        SCSIBuffer buf;
        this->WriteHeader(buf);

        buf.Write8(this->opcode);

        return buf;
    }

    SCSIRead10Command::SCSIRead10Command(u32 block_addr, u32 block_sz, u16 xfer_blocks) : SCSICommand(block_sz * xfer_blocks, SCSIDirection::In, 0, 10) {
        this->opcode = 0x28;
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

    SCSIWrite10Command::SCSIWrite10Command(u32 block_addr, u32 block_sz, u16 xfer_blocks) : SCSICommand(block_sz * xfer_blocks, SCSIDirection::Out, 0, 10) {
        this->opcode = 0x2a;
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

    SCSICommandStatus SCSIDevice::TransferCommand(SCSICommand &c, u8 *buffer, size_t buffer_size) {
        if(this->ok) {
            Result rc = 0;
            
            this->PushCommand(c);
            u32 transfer_length = c.GetDataTransferLength();
            u32 transferred = 0;
            u32 total_transferred = 0;
            
            if (this->ok && transfer_length > 0)
            {
                if (c.GetDirection() == SCSIDirection::In) {
                    while(total_transferred < transfer_length) {
                        memset(this->buf_b, 0, BufferSize);
                        
                        rc = usbHsEpPostBuffer(this->out_endpoint, this->buf_b, transfer_length - total_transferred, &transferred);
                        if (R_FAILED(rc)) break;
                        
                        if (transferred == SCSI_CSW_SIZE)
                        {
                            u32 signature;
                            memcpy(&signature, this->buf_b, 4);
                            if(signature == SCSI_CSW_SIGNATURE) {
                                /* We weren't expecting a CSW, but we got one anyway */
                                SCSICommandStatus status = {};
                                memcpy(&status, this->buf_b, sizeof(status));
                                return status;
                            }
                        }
                        
                        memcpy(buffer + total_transferred, this->buf_b, transferred);
                        total_transferred += transferred;
                    }
                } else {
                    while(total_transferred < transfer_length) {
                        memcpy(this->buf_b, buffer + total_transferred, transfer_length - total_transferred);
                        
                        rc = usbHsEpPostBuffer(this->in_endpoint, this->buf_b, transfer_length - total_transferred, &transferred);
                        if (R_FAILED(rc)) break;
                        
                        total_transferred += transferred;
                    }
                }
            }
            
            if (this->ok && R_FAILED(rc)) this->ok = false;
        }

        return this->ReadStatus();
    }

    SCSIBlock::SCSIBlock(SCSIDevice *dev) : device(dev), ok(true) {
        SCSIInquiryCommand inquiry(0x24);
        u8 inquiry_response[0x24] = {0};
        
        SCSITestUnitReadyCommand test_unit_ready;
        
        SCSIRequestSenseCommand request_sense(0x14);
        u8 request_sense_response[0x14] = {0};
        
        SCSIReadCapacityCommand read_capacity;
        
        SCSICommandStatus status;
        
        status = this->device->TransferCommand(inquiry, inquiry_response, 0x24);
        
        status = this->device->TransferCommand(test_unit_ready, nullptr, 0);
        if (status.status == 0x01 || status.status == 0x02) // CHECK_CONDITION / CONDITION_GOOD
        {
            this->ok = true;
            SCSICommandStatus rs_status = this->device->TransferCommand(request_sense, request_sense_response, 0x14);
            if (rs_status.status == 0) status = rs_status;
            
            // We may need to further analyze these responses...
            /*FILE *rsresp = fopen("sdmc:/rsresp.bin", "wb");
            if (rsresp)
            {
                fwrite(request_sense_response, 1, 0x14, rsresp);
                fclose(rsresp);
            }*/
        }
        
        if (status.status == 0) {
            u8 read_capacity_response[8] = {0};
            u32 size_lba;
            u32 lba_bytes;
            
            status = this->device->TransferCommand(read_capacity, read_capacity_response, 8);
            
            memcpy(&size_lba, &read_capacity_response[0], 4);
            size_lba = __builtin_bswap32(size_lba);
            
            memcpy(&lba_bytes, &read_capacity_response[4], 4);
            lba_bytes = __builtin_bswap32(lba_bytes);
            
            this->capacity = (u64)(size_lba * lba_bytes);
            this->block_size = lba_bytes;
            
            if (!this->capacity || !this->block_size) this->ok = false;
        } else {
            this->ok = false;
        }
    }

    int SCSIBlock::ReadSectors(u8 *buffer, u32 sector_offset, u32 num_sectors) {
        if(!this->Ok()) {
            return 0;
        }
        SCSIRead10Command read_ten(sector_offset, this->block_size, num_sectors);
        this->device->TransferCommand(read_ten, buffer, num_sectors * this->block_size);
        return num_sectors;
    }

    int SCSIBlock::WriteSectors(const u8 *buffer, u32 sector_offset, u32 num_sectors) {
        if(!this->Ok()) {
            return 0;
        }
        SCSIWrite10Command write_ten(sector_offset, this->block_size, num_sectors);
        this->device->TransferCommand(write_ten, (u8*)buffer, num_sectors * this->block_size);
        return num_sectors;
    }
}