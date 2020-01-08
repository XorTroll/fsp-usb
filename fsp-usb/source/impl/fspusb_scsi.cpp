#include "fspusb_scsi.hpp"
#include "fspusb_request.hpp"

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
    
    void SCSIBuffer::Write64BE(u64 val)
    {
        u64 tmpval = __builtin_bswap64(val);
        memcpy(&this->storage[this->idx], &tmpval, sizeof(u64));
        this->idx += sizeof(u64);
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

    void SCSICommand::SetDataTransferLength(u32 data_len) {
        this->data_transfer_length = data_len;
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

    SCSITestUnitReadyCommand::SCSITestUnitReadyCommand(u8 lun) : SCSICommand(SCSI_TEST_UNIT_READY_REPLY_LEN, SCSIDirection::Out, lun, SCSI_TEST_UNIT_READY_CB_LEN) {
        this->opcode = SCSI_TEST_UNIT_READY_CMD;
    }

    SCSIBuffer SCSITestUnitReadyCommand::ProduceBuffer() {
        SCSIBuffer buf;
        this->WriteHeader(buf);

        buf.Write8(this->opcode);
        buf.WritePadding(5);

        return buf;
    }

    SCSIRequestSenseCommand::SCSIRequestSenseCommand(u8 alloc_len, u8 lun) : SCSICommand(alloc_len, SCSIDirection::In, lun, SCSI_REQUEST_SENSE_CB_LEN) {
        this->opcode = SCSI_REQUEST_SENSE_CMD;
        this->allocation_length = alloc_len;
    }

    SCSIBuffer SCSIRequestSenseCommand::ProduceBuffer() {
        SCSIBuffer buf;
        this->WriteHeader(buf);

        buf.Write8(this->opcode);
        buf.WritePadding(3);
        buf.Write8(this->allocation_length);
        buf.WritePadding(1);

        return buf;
    }

    SCSIReadCapacity10Command::SCSIReadCapacity10Command(u8 lun) : SCSICommand(SCSI_READ_CAPACITY_10_REPLY_LEN, SCSIDirection::In, lun, SCSI_READ_CAPACITY_10_CB_LEN) {
        this->opcode = SCSI_READ_CAPACITY_10_CMD;
    }

    SCSIBuffer SCSIReadCapacity10Command::ProduceBuffer() {
        SCSIBuffer buf;
        this->WriteHeader(buf);

        buf.Write8(this->opcode);
        buf.WritePadding(9);

        return buf;
    }

    SCSIReadCapacity16Command::SCSIReadCapacity16Command(u8 alloc_len, u8 lun) : SCSICommand(alloc_len, SCSIDirection::In, lun, SCSI_READ_CAPACITY_16_CB_LEN) {
        this->opcode = SCSI_SERVICE_ACTION_IN_CMD;
        this->allocation_length = alloc_len;
    }

    SCSIBuffer SCSIReadCapacity16Command::ProduceBuffer() {
        SCSIBuffer buf;
        this->WriteHeader(buf);

        buf.Write8(this->opcode);
        buf.Write8(SCSI_SERVICE_ACTION_READ_CAPACITY_16);
        buf.WritePadding(8);
        buf.Write32BE((u32)this->allocation_length);
        buf.WritePadding(2);

        return buf;
    }

    SCSIRead10Command::SCSIRead10Command(u32 block_addr, u32 block_sz, u16 xfer_blocks, u8 lun) : SCSICommand(block_sz * xfer_blocks, SCSIDirection::In, lun, SCSI_READ_10_CB_LEN) {
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

    SCSIRead16Command::SCSIRead16Command(u64 block_addr, u32 block_sz, u32 xfer_blocks, u8 lun) : SCSICommand((u32)(block_sz * xfer_blocks), SCSIDirection::In, lun, SCSI_READ_16_CB_LEN) {
        this->opcode = SCSI_READ_16_CMD;
        this->block_address = block_addr;
        this->transfer_blocks = xfer_blocks;
    }

    SCSIBuffer SCSIRead16Command::ProduceBuffer() {
        SCSIBuffer buf;
        this->WriteHeader(buf);

        buf.Write8(this->opcode);
        buf.WritePadding(1);
        buf.Write64BE(this->block_address);
        buf.Write32BE(this->transfer_blocks);
        buf.WritePadding(2);

        return buf;
    }

    SCSIWrite10Command::SCSIWrite10Command(u32 block_addr, u32 block_sz, u16 xfer_blocks, u8 lun) : SCSICommand(block_sz * xfer_blocks, SCSIDirection::Out, lun, SCSI_WRITE_10_CB_LEN) {
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

    SCSIWrite16Command::SCSIWrite16Command(u64 block_addr, u32 block_sz, u32 xfer_blocks, u8 lun) : SCSICommand((u32)(block_sz * xfer_blocks), SCSIDirection::Out, lun, SCSI_WRITE_16_CB_LEN) {
        this->opcode = SCSI_WRITE_16_CMD;
        this->block_address = block_addr;
        this->transfer_blocks = xfer_blocks;
    }

    SCSIBuffer SCSIWrite16Command::ProduceBuffer() {
        SCSIBuffer buf;
        this->WriteHeader(buf);

        buf.Write8(this->opcode);
        buf.WritePadding(1);
        buf.Write64BE(this->block_address);
        buf.Write32BE(this->transfer_blocks);
        buf.WritePadding(2);

        return buf;
    }

    SCSIDevice::SCSIDevice(UsbHsClientIfSession *iface, UsbHsClientEpSession *in_ep, UsbHsClientEpSession *out_ep, u8 lun) : buf_a(nullptr), buf_b(nullptr), buf_c(nullptr), client(iface), in_endpoint(in_ep), out_endpoint(out_ep), ok(true), dev_lun(lun) {
        this->AllocateBuffers();
    }

    SCSIDevice::~SCSIDevice() {
        this->FreeBuffers();
    }

    void SCSIDevice::AllocateBuffers() {
        if(this->buf_a == nullptr) {
            this->buf_a = (u8*)AllocUSBTransferMemoryBlock(1);
        }
        if(this->buf_b == nullptr) {
            this->buf_b = (u8*)AllocUSBTransferMemoryBlock(USB_TRANSFER_MEMORY_MAX_MULTIPLIER);
        }
        if(this->buf_c == nullptr) {
            this->buf_c = (u8*)AllocUSBTransferMemoryBlock(1);
        }
    }

    void SCSIDevice::FreeBuffers() {
        if(this->buf_a != nullptr) {
            FreeUSBTransferMemoryBlock(this->buf_a);
        }
        if(this->buf_b != nullptr) {
            FreeUSBTransferMemoryBlock(this->buf_b);
        }
        if(this->buf_c != nullptr) {
            FreeUSBTransferMemoryBlock(this->buf_c);
        }
    }

    SCSICommandStatus SCSIDevice::ReadStatus() {
        u32 in_len = 0;
        SCSICommandStatus status;
        memset(&status, 0, sizeof(SCSICommandStatus));

        if (this->ok) {
            FSP_USB_LOG("%s (interface ID %d): OK to proceed.", __func__, this->client->ID);
            
            Result rc = PostUSBBuffer(this->client, this->out_endpoint, this->buf_c, SCSI_CSW_SIZE, &in_len);
            
            FSP_USB_LOG("%s (interface ID %d): PostUSBBuffer returned 0x%08X (in_len -> %u) (%s).", __func__, this->client->ID, rc, in_len, (R_SUCCEEDED(rc) && in_len == SCSI_CSW_SIZE ? "succeeded" : "failed"));
            
            if (R_SUCCEEDED(rc) && in_len == SCSI_CSW_SIZE) {
                memcpy(&status, this->buf_c, SCSI_CSW_SIZE);
                
                FSP_USB_LOG("%s (interface ID %d): CSW signature -> 0x%08X (%s) | CSW tag -> 0x%08X (%s).", __func__, this->client->ID, status.signature, (status.signature == SCSI_CSW_SIGNATURE ? "valid" : "invalid"), status.tag, (status.tag == SCSI_TAG ? "valid" : "invalid"));
                
                if (status.signature == SCSI_CSW_SIGNATURE && status.tag == SCSI_TAG)
                {
                    FSP_USB_LOG("%s (interface ID %d): CSW status -> 0x%02X (%s).", __func__, this->client->ID, status.status, (status.status == SCSI_CMD_STATUS_SUCCESS ? "success" : (status.status == SCSI_CMD_STATUS_FAILED ? "failed" : (status.status == SCSI_CMD_STATUS_PHASE_ERROR ? "phase error" : "unknown / invalid"))));
                    
                    if (status.status == SCSI_CMD_STATUS_PHASE_ERROR)
                    {
                        // Perform a full bulk-only reset recovery if we got a phase error
                        FSP_USB_LOG("%s (interface ID %d): performing bulk-only mass storage reset recovery.", __func__, this->client->ID);
                        ResetBulkStorage(this->client, this->in_endpoint, this->out_endpoint);
                    }
                } else {
                    // Perform a full bulk-only reset recovery if we got an invalid CSW signature or tag
                    FSP_USB_LOG("%s (interface ID %d): performing bulk-only mass storage reset recovery.", __func__, this->client->ID);
                    ResetBulkStorage(this->client, this->in_endpoint, this->out_endpoint);
                    this->ok = false;
                }
            } else {
                this->ok = false;
            }
        } else {
            FSP_USB_LOG("%s (interface ID %d): not OK to proceed.", __func__, this->client->ID);
        }

        return status;
    }

    void SCSIDevice::PushCommand(SCSICommand &cmd, u32 diff) {
        if(this->ok) {
            FSP_USB_LOG("%s (interface ID %d): OK to proceed.", __func__, this->client->ID);
            
            memset(this->buf_a, 0, BufferSize);

            if (diff > 0) {
                FSP_USB_LOG("%s (interface ID %d): data transfer length difference provided -> %u.", __func__, this->client->ID, diff);
                u32 data_len = cmd.GetDataTransferLength();
                FSP_USB_LOG("%s (interface ID %d): current data transfer length -> %u.", __func__, this->client->ID, data_len);
                if (data_len > 0 && diff < data_len) {
                    // Update data transfer length field
                    data_len -= diff;
                    cmd.SetDataTransferLength(data_len);
                    FSP_USB_LOG("%s (interface ID %d): updated data transfer length -> %u.", __func__, this->client->ID, cmd.GetDataTransferLength());
                } else {
                    FSP_USB_LOG("%s (interface ID %d): ignoring data transfer length difference.", __func__, this->client->ID);
                }
            }

            cmd.ToBytes(this->buf_a);
            
            u32 out_len = 0;
            auto rc = PostUSBBuffer(this->client, this->in_endpoint, this->buf_a, SCSIBuffer::BufferSize, &out_len);
            this->ok = (R_SUCCEEDED(rc) && out_len == SCSIBuffer::BufferSize);
            
            FSP_USB_LOG("%s (interface ID %d): PostUSBBuffer returned 0x%08X (out_len -> %u) (%s).", __func__, this->client->ID, rc, out_len, (R_SUCCEEDED(rc) && out_len == SCSIBuffer::BufferSize ? "succeeded" : "failed"));
            
            if (R_FAILED(rc)) {
                // Check if the endpoint is stalled
                if (GetEndpointStatus(this->client, this->in_endpoint)) {
                    // Perform a full bulk-only reset recovery
                    FSP_USB_LOG("%s (interface ID %d): endpoint stalled. Performing bulk-only mass storage reset recovery.", __func__, this->client->ID);
                    ResetBulkStorage(this->client, this->in_endpoint, this->out_endpoint);
                }
            }
        } else {
            FSP_USB_LOG("%s (interface ID %d): not OK to proceed.", __func__, this->client->ID);
        }
    }

    SCSICommandStatus SCSIDevice::TransferCommand(SCSICommand &c, u8 *buffer) {
        SCSICommandStatus status;
        memset(&status, 0, sizeof(SCSICommandStatus));
        
        Result rc = 0;
        
        if (this->ok) {
            FSP_USB_LOG("%s (interface ID %d): OK to proceed.", __func__, this->client->ID);
            
            u32 retries = SCSI_TRANSFER_RETRIES;
            u32 total_transferred = 0;
            u32 transferred = 0;
            u32 block_size = (u32)(BufferSize * USB_TRANSFER_MEMORY_MAX_MULTIPLIER);
            u32 transfer_length = c.GetDataTransferLength();
            u32 cur_transfer_size = 0;
            
            FSP_USB_LOG("%s (interface ID %d): block size -> %u | data transfer length -> %u | %s buffer | direction -> %s.", __func__, this->client->ID, block_size, transfer_length, (buffer == nullptr ? "invalid" : "valid"), (c.GetDirection() == SCSIDirection::In ? "in" : "out"));
            
            do {
                retries--;
                this->ok = true;
                
                FSP_USB_LOG("%s (interface ID %d): attempt #%u.", __func__, this->client->ID, SCSI_TRANSFER_RETRIES - retries);
                
                this->PushCommand(c, total_transferred);
                if (!this->ok) {
                    continue;
                }
                
                if (buffer != nullptr && transfer_length > 0) {
                    if (c.GetDirection() == SCSIDirection::In) {
                        while(total_transferred < transfer_length) {
                            cur_transfer_size = ((transfer_length - total_transferred) > block_size ? block_size : (transfer_length - total_transferred));
                            FSP_USB_LOG("%s (interface ID %d): total transferred data length -> %u | current transfer size -> %u.", __func__, this->client->ID, total_transferred, cur_transfer_size);
                            
                            transferred = 0;
                            rc = PostUSBBuffer(this->client, this->out_endpoint, this->buf_b, cur_transfer_size, &transferred);
                            
                            FSP_USB_LOG("%s (interface ID %d): PostUSBBuffer returned 0x%08X (transferred -> %u) (%s).", __func__, this->client->ID, rc, transferred, (R_SUCCEEDED(rc) && transferred == cur_transfer_size ? "succeeded" : "failed"));
                            
                            if (R_FAILED(rc) || transferred != cur_transfer_size) break;
                            
                            if (transferred == SCSI_CSW_SIZE)
                            {
                                memcpy(&status, this->buf_b, sizeof(status));
                                if (status.signature == SCSI_CSW_SIGNATURE && status.tag == SCSI_TAG) {
                                    /* We weren't expecting a CSW, but we got one anyway */
                                    FSP_USB_LOG("%s (interface ID %d): received unexpected (but valid) CSW.", __func__, this->client->ID);
                                    return status;
                                }
                            }
                            
                            memcpy(buffer + total_transferred, this->buf_b, transferred);
                            total_transferred += transferred;
                        }
                    } else {
                        while(total_transferred < transfer_length) {
                            cur_transfer_size = ((transfer_length - total_transferred) > block_size ? block_size : (transfer_length - total_transferred));
                            FSP_USB_LOG("%s (interface ID %d): total transferred data length -> %u | current transfer size -> %u.", __func__, this->client->ID, total_transferred, cur_transfer_size);
                            
                            memcpy(this->buf_b, buffer + total_transferred, cur_transfer_size);
                            
                            transferred = 0;
                            rc = PostUSBBuffer(this->client, this->in_endpoint, this->buf_b, cur_transfer_size, &transferred);
                            
                            FSP_USB_LOG("%s (interface ID %d): PostUSBBuffer returned 0x%08X (transferred -> %u) (%s).", __func__, this->client->ID, rc, transferred, (R_SUCCEEDED(rc) && transferred == cur_transfer_size ? "succeeded" : "failed"));
                            
                            if (R_FAILED(rc) || transferred != cur_transfer_size) break;
                            
                            total_transferred += transferred;
                        }
                    }
                    
                    if (total_transferred < transfer_length) continue;
                }
                
                status = this->ReadStatus();
                if (!this->ok) {
                    continue;
                }
                
                break;
            } while(retries > 0);
        } else {
            status.status = SCSI_CMD_STATUS_FAILED;
            FSP_USB_LOG("%s (interface ID %d): not OK to proceed.", __func__, this->client->ID);
        }
        
        return status;
    }

    SCSIBlock::SCSIBlock(SCSIDevice *dev) : capacity(0), block_size(0), device(dev), ok(true) {
        SCSICommandStatus status, rs_status;
        u8 lun = this->device->GetDeviceLUN();
        
        SCSITestUnitReadyCommand test_unit_ready(lun);
        
        SCSIRequestSenseCommand request_sense(SCSI_REQUEST_SENSE_REPLY_LEN, lun);
        u8 request_sense_response[SCSI_REQUEST_SENSE_REPLY_LEN] = {0};
        
        SCSIReadCapacity10Command read_capacity_10(lun);
        u8 read_capacity_10_response[SCSI_READ_CAPACITY_10_REPLY_LEN] = {0};
        
        SCSIReadCapacity16Command read_capacity_16(SCSI_READ_CAPACITY_16_REPLY_LEN, lun);
        u8 read_capacity_16_response[SCSI_READ_CAPACITY_16_REPLY_LEN] = {0};
        
        u64 size_lba = 0;
        u32 lba_bytes = 0;
        
        FSP_USB_LOG("%s: sending TestUnitReady command.", __func__);
        status = this->device->TransferCommand(test_unit_ready, nullptr);
        if (status.status != SCSI_CMD_STATUS_SUCCESS)
        {
            FSP_USB_LOG("%s: TestUnitReady command failed (0x%02X).", __func__, status.status);
            
            this->ok = true; // Manually set back to true
            FSP_USB_LOG("%s: sending RequestSense command.", __func__);
            rs_status = this->device->TransferCommand(request_sense, request_sense_response);
            if (rs_status.status == SCSI_CMD_STATUS_SUCCESS)
            {
                u8 sense_key = (request_sense_response[2] & 0x0F);
                
                FSP_USB_LOG("%s: RequestSense command succeeded. Sense key: 0x%02X.", __func__, sense_key);
                
                switch(sense_key)
                {
                    case SCSI_SENSE_NO_SENSE:
                    case SCSI_SENSE_RECOVERED_ERROR:
                    case SCSI_SENSE_UNIT_ATTENTION:
                        // Use Request Sense command status
                        status = rs_status;
                        FSP_USB_LOG("%s: using RequestSense command status to proceed.", __func__);
                        break;
                    case SCSI_SENSE_ABORTED_COMMAND:
                    case SCSI_SENSE_NOT_READY:
                        if (sense_key == SCSI_SENSE_NOT_READY) {
                            // Wait 3 seconds
                            FSP_USB_LOG("%s: waiting for drive to become ready.", __func__);
                            svcSleepThread((u64)3000000000);
                        }
                        
                        // Retry command
                        FSP_USB_LOG("%s: retrying TestUnitReady command.", __func__);
                        status = this->device->TransferCommand(test_unit_ready, nullptr);
                        break;
                    case SCSI_SENSE_MEDIUM_ERROR:
                    case SCSI_SENSE_HARDWARE_ERROR:
                    case SCSI_SENSE_ILLEGAL_REQUEST:
                    case SCSI_SENSE_DATA_PROTECT:
                    case SCSI_SENSE_BLANK_CHECK:
                    case SCSI_SENSE_COPY_ABORTED:
                    case SCSI_SENSE_VOLUME_OVERFLOW:
                    case SCSI_SENSE_MISCOMPARE:
                        this->ok = false;
                        FSP_USB_LOG("%s: unrecoverable error.", __func__);
                        break;
                    default:
                        this->ok = false;
                        FSP_USB_LOG("%s: unknown sense key.", __func__);
                        break;
                }
                
                // We may need to further analyze these responses...
                /*FILE *rsresp = fopen("sdmc:/rsresp.bin", "wb");
                if (rsresp)
                {
                    fwrite(request_sense_response, 1, 0x14, rsresp);
                    fclose(rsresp);
                }*/
            } else {
                this->ok = false;
                FSP_USB_LOG("%s: RequestSense command failed (0x%02X).", __func__, rs_status.status);
            }
        } else {
            FSP_USB_LOG("%s: TestUnitReady command succeeded.", __func__);
        }
        
        if (this->ok && status.status == SCSI_CMD_STATUS_SUCCESS) {
            FSP_USB_LOG("%s: sending ReadCapacity10 command.", __func__);
            status = this->device->TransferCommand(read_capacity_10, read_capacity_10_response);
            if (this->ok && status.status == SCSI_CMD_STATUS_SUCCESS) {
                FSP_USB_LOG("%s: ReadCapacity10 command succeeded.", __func__);
                
                memcpy(&lba_bytes, &read_capacity_10_response[0], 4);
                size_lba = (u64)__builtin_bswap32(lba_bytes);
                
                if (size_lba > 0 && size_lba < SCSI_MAX_BLOCK_10) {
                    FSP_USB_LOG("%s: valid total block count returned by ReadCapacity10 command.", __func__);
                    
                    memcpy(&lba_bytes, &read_capacity_10_response[4], 4);
                    lba_bytes = __builtin_bswap32(lba_bytes);
                    
                    this->capacity = (size_lba * (u64)lba_bytes);
                    this->block_size = lba_bytes;
                } else {
                    // Issue a Read Capacity 16 command
                    FSP_USB_LOG("%s: invalid or maxed out total block count returned by ReadCapacity10 command.", __func__);
                    FSP_USB_LOG("%s: sending ReadCapacity16 command.", __func__);
                    status = this->device->TransferCommand(read_capacity_16, read_capacity_16_response);
                    if (this->ok && status.status == SCSI_CMD_STATUS_SUCCESS) {
                        FSP_USB_LOG("%s: ReadCapacity16 command succeeded.", __func__);
                        
                        memcpy(&size_lba, &read_capacity_16_response[0], 8);
                        size_lba = __builtin_bswap64(size_lba);
                        
                        memcpy(&lba_bytes, &read_capacity_16_response[8], 4);
                        lba_bytes = __builtin_bswap32(lba_bytes);
                        
                        this->capacity = (size_lba * (u64)lba_bytes);
                        this->block_size = lba_bytes;
                    } else {
                        this->ok = false;
                        FSP_USB_LOG("%s: ReadCapacity16 command failed (0x%02X).", __func__, status.status);
                    }
                }
                
                FSP_USB_LOG("%s: total block count -> 0x%016lX | block size -> 0x%08X | capacity -> 0x%016lX.", __func__, size_lba, lba_bytes, this->capacity);
                
                if (this->ok && (!this->capacity || !this->block_size)) this->ok = false;
            } else {
                this->ok = false;
                FSP_USB_LOG("%s: ReadCapacity10 command failed (0x%02X).", __func__, status.status);
            }
        }
    }

    int SCSIBlock::ReadSectors(u8 *buffer, u64 sector_offset, u32 num_sectors) {
        if(!this->Ok()) {
            return 0;
        }
        
        FSP_USB_LOG("%s: LBA address -> 0x%016lX | sector count -> 0x%08X.", __func__, sector_offset, num_sectors);
        
        if ((sector_offset + num_sectors) > SCSI_MAX_BLOCK_10) {
            FSP_USB_LOG("%s: end LBA address exceeds U32_MAX. Sending Read16 command.", __func__);
            SCSIRead16Command read_16(sector_offset, this->block_size, num_sectors, this->device->GetDeviceLUN());
            this->device->TransferCommand(read_16, buffer);
        } else {
            FSP_USB_LOG("%s: sending Read10 command.", __func__);
            SCSIRead10Command read_10((u32)sector_offset, this->block_size, (u16)num_sectors, this->device->GetDeviceLUN());
            this->device->TransferCommand(read_10, buffer);
        }
        
        return num_sectors;
    }

    int SCSIBlock::WriteSectors(const u8 *buffer, u64 sector_offset, u32 num_sectors) {
        if(!this->Ok()) {
            return 0;
        }
        
        FSP_USB_LOG("%s: LBA address -> 0x%016lX | sector count -> 0x%08X.", __func__, sector_offset, num_sectors);
        
        if ((sector_offset + num_sectors) > SCSI_MAX_BLOCK_10) {
            FSP_USB_LOG("%s: end LBA address exceeds U32_MAX. Sending Write16 command.", __func__);
            SCSIWrite16Command write_16(sector_offset, this->block_size, num_sectors, this->device->GetDeviceLUN());
            this->device->TransferCommand(write_16, (u8*)buffer);
        } else {
            FSP_USB_LOG("%s: sending Write10 command.", __func__);
            SCSIWrite10Command write_10((u32)sector_offset, this->block_size, (u16)num_sectors, this->device->GetDeviceLUN());
            this->device->TransferCommand(write_10, (u8*)buffer);
        }
        
        return num_sectors;
    }
}