# fsp-usb

## The development of this project is currently active on this [Atmosphere fork](https://github.com/XorTroll/Atmosphere/tree/fspusb). 

> Custom Nintendo Switch service to mount and access USB drives

## Information

This sysmodule was going to be part of Atmosphere, but due to lack of motivation I stopped working on it for several months.

This is an attempt to port the old code and to fix possible bugs it had, and is still a work-in-progress.

- Not many USB drives are supported, but, in general, FAT32 and exFAT drives seem to work (otherwise, fsp-usb won't detect them)

## Service

This hosts a service named `fsp-usb`, whose interfaces behave almost like `fsp-srv`'s file system interfaces.

This would be it's SwIPC definition:

```
type fspusb::FileSystemType = enum<u8> {
    FAT12 = 1;
    FAT16 = 2;
    FAT32 = 3;
    exFAT = 4;
};

interface fspusb::Service is fsp-usb {
    
    [0] GetMountedDriveCount() -> u32 drive_count;
    [1] GetDriveFileSystemType() -> fspusb::FileSystemType fs_type;
    [2] GetDriveLabel(u32 drive_idx) -> buffer<unknown, 0x6> label;
    [3] SetDriveLabel(u32 drive_idx, buffer<unknown, 0x5> label);
    [4] OpenDriveFileSystem(u32 drive_idx) -> object<nn::fssrv::sf::IFileSystem> drive_fs;
}
```

- Command 3 (SetDriveLabel)'s input string shouldn't be larger than 11 characters.

- The FileSystem returned in command 4 is identical to fsp-srv's filesystems (same commands, interfaces...)

This service's results are 2002-8XXX (FS module and 8000+ error codes):

- Specific error codes:

  - ```2002-8001``` Invalid drive index

  - ```2002-8002``` Drive unavailable (accessing a filesystem/file/directory of a drive after it got unplugged)

  - ```2002-8003``` Drive initialization failure (used internally, never returned)

- FATFS-related results are 8100 + FATFS error, if not converted to common FS results.
