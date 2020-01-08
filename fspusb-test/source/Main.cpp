
#include <switch.h>
#include <cstdio>
#include <sys/stat.h>
#include <dirent.h>
#include <string>
#include <iostream>
#include <cmath>
#include <sstream>

extern "C" {
    #include "fspusb.h"
}

#define CONSOLE_PRINT(...) { std::cout << __VA_ARGS__ << std::endl; consoleUpdate(NULL); }

#define CONSOLE_RESULT(ctx, rc) CONSOLE_PRINT(" * An error ocurred " ctx ": 0x" << std::hex << rc << std::dec)

std::string FormatSize(u64 Bytes)
{
    std::string sufs[] = { " bytes", " KB", " MB", " GB", " TB", " PB", " EB" };
    if(Bytes == 0) return "0" + sufs[0];
    u32 plc = floor((log(Bytes) / log(1024)));
    double btnum = (double)(Bytes / pow(1024, plc));
    double rbt = ((int)(btnum * 100.0) / 100.0);
    std::stringstream strm;
    strm << rbt;
    return (strm.str() + sufs[plc]);
}

void PerformDriveTest(s32 drive_iface_id)
{
    CONSOLE_PRINT("Testing with drive (ID " << drive_iface_id << ")")

    // Labels are <=11 characters anyway
    char label[0x10] = {0};
    auto rc = fspusbGetDriveLabel(drive_iface_id, label, 0x10);
    if(R_SUCCEEDED(rc)) CONSOLE_PRINT(" - Drive label: " << label)
    else CONSOLE_RESULT("getting the drive's label", rc)

    const char *new_label = "NEW LABEL";
    CONSOLE_PRINT("Press A to set the drive's label to '" << new_label << "', or any key to skip this test.")
    while(appletMainLoop())
    {
        hidScanInput();
        auto k = hidKeysDown(CONTROLLER_P1_AUTO);
        if (k)
        {
            if(k & KEY_A)
            {
                rc = fspusbSetDriveLabel(drive_iface_id, new_label);
                if(R_SUCCEEDED(rc)) CONSOLE_PRINT("The drive's label was successfully changed.")
                else CONSOLE_RESULT("changing the drive's label", rc)
            }
            
            break;
        }
    }

    FspUsbFileSystemType fstype;
    rc = fspusbGetDriveFileSystemType(drive_iface_id, &fstype);
    if(R_SUCCEEDED(rc))
    {
        const char *fs = "<unknown type>";
        switch(fstype)
        {
            case FspUsbFileSystemType_FAT12:
                fs = "FAT12";
                break;
            case FspUsbFileSystemType_FAT16:
                fs = "FAT16";
                break;
            case FspUsbFileSystemType_FAT32:
                fs = "FAT32";
                break;
            case FspUsbFileSystemType_exFAT:
                fs = "exFAT";
                break;
            default:
                break;
        }
        CONSOLE_PRINT(" - Filesystem type: " << fs)
    }
    else CONSOLE_RESULT("getting the drive's filesystem type", rc)

    FsFileSystem drvfs;
    rc = fspusbOpenDriveFileSystem(drive_iface_id, &drvfs);
    if(R_SUCCEEDED(rc))
    {
        s64 total = 0;
        s64 free = 0;
        fsFsGetTotalSpace(&drvfs, "/", &total);
        fsFsGetFreeSpace(&drvfs, "/", &free);

        CONSOLE_PRINT(" - Total space: " << FormatSize(total))
        CONSOLE_PRINT(" - Free space: " << FormatSize(free))

        int mountres = fsdevMountDevice("usbdrv", drvfs);
        if(mountres != -1)
        {
            CONSOLE_PRINT("Listing root files and directories..." << std::endl)
            auto dir = opendir("usbdrv:/");
            if(dir)
            {
                dirent *dt;
                while(true)
                {
                    dt = readdir(dir);
                    if(dt == NULL) break;
                    CONSOLE_PRINT(" - [" << ((dt->d_type & DT_DIR) ? "D" : "F") << "] usbdrv:/" << dt->d_name)
                }
                closedir(dir);
            }
            CONSOLE_PRINT(std::endl << "Done listing...")

            fsdevUnmountDevice("usbdrv");
            CONSOLE_PRINT("Unmounted and closed drive's filesystem.")
        }
        else CONSOLE_PRINT("Failed to mount the drive's filesystem...")
        
        fsFsClose(&drvfs);
    }
    else CONSOLE_RESULT("accessing the drive's filesystem", rc)
}

void WaitForInput()
{
    while(appletMainLoop())
    {
        hidScanInput();
        if(hidKeysDown(CONTROLLER_P1_AUTO)) break;
    }
}

int main()
{
    socketInitializeDefault();
    nxlinkStdio();

    consoleInit(NULL);

    auto rc = fspusbInitialize();
    if(R_SUCCEEDED(rc))
    {
        CONSOLE_PRINT("Searching for drives...")

        s32 drive_ids[4] = {0};
        s32 drive_count = 0;
        fspusbListMountedDrives(drive_ids, 4, &drive_count);
        CONSOLE_PRINT("Drive count: " << drive_count << " drives")
        
        if (drive_count > 0) 
        {
            CONSOLE_PRINT("Press any key to start testing...")
            WaitForInput();
        }
        
        for(s32 i = 0; i < drive_count; i++)
        {
            consoleClear();
            PerformDriveTest(drive_ids[i]);
            if ((i + 1) < drive_count)
            {
                CONSOLE_PRINT("Press any key to continue to the next drive...")
                WaitForInput();
            }
        }

        CONSOLE_PRINT("Press any key to exit...")
        WaitForInput();
        consoleExit(NULL);
        socketExit();
        return 0;
    }
    else CONSOLE_RESULT("accessing fsp-usb service", rc)

    consoleExit(NULL);
    socketExit();
    
    return 0;
}