#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libusb.h>
#include <errno.h>
#include "getopt.h"
#include "s5p_usb.h"
#include "s5p_boot.h"

#define BUF_MAX (50 * 1024 * 1024)

#define MIN(A, B) (((A) < (B)) ? (A) : (B))

typedef unsigned char uint8;
typedef unsigned int  uint32;

static unsigned char mem[BUF_MAX];

static void write32LE(uint8* addr, uint32 data)
{
    addr[0] = (uint8)(data & 0xff);
    addr[1] = (uint8)((data >> 8) & 0xff);
    addr[2] = (uint8)((data >> 16) & 0xff);
    addr[3] = (uint8)((data >> 24) & 0xff);
}

static unsigned read32LE(const uint8* addr)
{
    return addr[0] + (addr[1] << 8) + (addr[2] << 16) + (addr[3] << 24);
}

static void initBootloaderHeader(uint8* buf, int is64bitBoot,
    unsigned size, unsigned loadAddr, unsigned launchAddr,
    int bootMethod)
{
    static const unsigned arm64bootcode[15] = {
        0xE59F0030, // ldr r0, =0xc0011000
        0xE590113C, // ldr r1, [r0, #0x13c]
        0xE3811A0F, // orr r1, r1, #0xf000
        0xE580113C, // str r1, [r0, #0x13c]
        0xE59F1024, // ldr r1, =#0x3fffc080  @ launchAddr >> 2
        0xE5801140, // str r1, [r0, #0x140]
        0xE5101D54, // ldr r1, [r0, #-0xd54] @ 0cx00102ac
        0xE3811001, // orr r1, r1, #1
        0xE5001D54, // str r1, [r0, #-0xd54] @ 0cx00102ac
        0xE5901138, // ldr r1, [r0, #0x138]
        0xE381160F, // orr r1, r1, #0xf00000
        0xE5801138, // str r1, [r0, #0x138]
        0xE320F003, // wfi
        0xEAFFFFFE, // b   .
        0xC0011000
    };
    unsigned i;

    if (is64bitBoot) {
        for (i = 0; i < sizeof(arm64bootcode) / sizeof(arm64bootcode[0]); ++i)
            write32LE(buf + 4 * i, arm64bootcode[i]);
        write32LE(buf + 4 * i, launchAddr >> 2);
        launchAddr = loadAddr;  // launch the CPU reset
    }
    write32LE(buf + 0x44, size);        // load size
    write32LE(buf + 0x48, loadAddr);    // load address
    write32LE(buf + 0x4c, launchAddr);  // launch address
    if (bootMethod >= 0)
        buf[0x57] = bootMethod;
    memcpy(buf + 0x1fc, "NSIH", 4);     // signature
}

static int checkBootloaderHeader(uint8* buf, unsigned readSize,
    int isVerbose, int fixSize, int bootMethod)
{
    static const char bootMethods[][6] = {
        "USB", "SPI", "NAND", "SDMMC", "SDFS"
    };
    unsigned size;
    int res = !fixSize;
    unsigned headerSize, sizeOffset, loadAddrOffset, launchAddrOffset;

    headerSize = 512;
    sizeOffset = 0x44;
    loadAddrOffset = 0x48;
    launchAddrOffset = 0x4c;
    if (readSize >= headerSize) {
        if (!memcmp(buf + 0x1fc, "NSIH", 4)) {
            size = read32LE(buf + sizeOffset);
            if (fixSize)
                write32LE(buf + sizeOffset, readSize - headerSize);
            if (bootMethod >= 0)
                buf[0x57] = bootMethod;
            if (isVerbose) {
                printf("Boot Header:\n");
                printf("  size:           %u ", size);
                if (size == readSize - headerSize)
                    printf("(OK)\n");
                else {
                    printf("(real: %u%s)\n", readSize - headerSize,
                        fixSize ? ", fixed" : "");
                }
                printf("  load address:   0x%x\n",
                    read32LE(buf + loadAddrOffset));
                printf("  launch address: 0x%x\n",
                    read32LE(buf + launchAddrOffset));
                bootMethod = ((unsigned char*)buf)[0x57];
                printf("  boot method:    %d (%s)\n", bootMethod,
                    bootMethod < 5 ? bootMethods[bootMethod] : "unknown");
                printf("  signature:      OK\n");
            }
            else {
                if (!fixSize && size != readSize - headerSize) {
                    printf("warning: wrong load size in header: %u, real: %u\n",
                        size, readSize - headerSize);
                }
            }
            res = 1;
        }
        else {
            printf("%s: bad signature in header\n",
                fixSize ? "error" : "warning");
        }
    }
    else {
        printf("%s: read data size (%u) is less than boot header size\n",
            fixSize ? "error" : "warning", readSize);
    }
    return res;
}

static int readBin(uint8* buf, unsigned bufsize, const char* filepath,
    int isEnv)
{
    enum { BUF_PADDING = 16 };
    FILE* fin;
    int size;

    if (!strcmp(filepath, "-"))
        fin = stdin;
    else {
        fin = fopen(filepath, "rb");
        if (fin == NULL) {
            printf("unable to open %s: %s\n", filepath,
                strerror(errno));
            return -1;
        }
    }
    size = fread(buf, 1, bufsize, fin);
    if (fin != stdin)
        fclose(fin);
    if (size < 0) {
        fprintf(stderr, "fread: %s\n", strerror(errno));
        return size;
    }
    if (isEnv && (unsigned)size < bufsize)
        buf[size++] = '\0';
    // Pad size to be a multiple of BUF_PADDING bytes
    if (BUF_PADDING > 1)
        size = ((size - 1) | (BUF_PADDING - 1)) + 1;
    return size;
}
//
//static int hotplug_callback_attach(libusb_context* ctx, libusb_device* dev, libusb_hotplug_event event, void* user_data)
//{
//    (void)ctx; // unused
//    (void)event; // unused
//
//    libusb_device_handle** dev_handle_ptr = (libusb_device_handle**)user_data;
//    int ret;
//
//    ret = libusb_open(dev, dev_handle_ptr);
//    if (LIBUSB_SUCCESS != ret) {
//        fprintf(stderr, "libusb_open: %s\n", libusb_strerror(ret));
//        return 0;
//    }
//
//    return 1;
//}

static int device_open(libusb_context* ctx,
    libusb_device_handle** dev_handle_ptr, int isVerbose)
{
    libusb_hotplug_callback_handle hp;
    int ret;
    libusb_device** devs;
    ssize_t cnt;


    if (isVerbose)
        printf("Waiting (60 sec) for device to be available");
    int t = 60;
    while (t--)
    {
        printf(".");
        cnt = libusb_get_device_list(NULL, &devs);
        if (cnt < 0) {
            return LIBUSB_ERROR_NO_DEVICE;
        }
        for (int i = 0; i < cnt; i++)
        {
            struct libusb_device_descriptor desc;
            int r = libusb_get_device_descriptor(devs[i], &desc);
            if (r < 0) {
                fprintf(stderr, "failed to get device descriptor");
                return LIBUSB_ERROR_NO_DEVICE;
            }
            if (desc.idVendor == S5P6818_VID && desc.idProduct == S5P6818_PID)
            {
                *dev_handle_ptr = libusb_open_device_with_vid_pid(ctx, S5P6818_VID, S5P6818_PID);
                t = 0;
                break;
            }
        }
        Sleep(1000);
    }
    printf("\n");
  
    /* ret = libusb_hotplug_register_callback(ctx,
        LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, LIBUSB_HOTPLUG_ENUMERATE,
        S5P6818_VID, S5P6818_PID, LIBUSB_HOTPLUG_MATCH_ANY,
        (libusb_hotplug_callback_fn)hotplug_callback_attach, (void*)dev_handle_ptr, &hp);

    if (LIBUSB_SUCCESS != ret) {
        fprintf(stderr, "libusb_hotplug_register_callback: %s\n",
            libusb_strerror(ret));
        return ret;
    }*/

    while (NULL == *dev_handle_ptr) {
        ret = libusb_handle_events(ctx);
        if (LIBUSB_SUCCESS != ret) {
            fprintf(stderr, "libusb_handle_events: %s\n", libusb_strerror(ret));
        }
    }

    if (isVerbose)
        printf("Device opened.\n");
    //libusb_hotplug_deregister_callback(ctx, hp);

    return LIBUSB_SUCCESS;
}

static int usbBoot(uint8* buf, int size, int isVerbose)
{
    enum { TRANSFER_CHUNK = 1024 * 1024, TRANSFER_TIMEOUT_MS = 10000 };
    libusb_context* ctx;
    libusb_device_handle* dev_handle = NULL;

    int ret, boot_ret = 1;
    int transferred, remain;

    ret = libusb_init(&ctx);
    if (LIBUSB_SUCCESS != ret) {
        fprintf(stderr, "libusb_init: %s\n", libusb_strerror(ret));
        return 1;
    }

    ret = device_open(ctx, &dev_handle, isVerbose);
    if (LIBUSB_SUCCESS == ret) {
        ret = libusb_claim_interface(dev_handle, S5P6818_INTERFACE);
        if (LIBUSB_SUCCESS == ret) {
            if (isVerbose)
                printf("Start transfer, size=%d\n", size);
            remain = size;
            while (remain > 0) {
                ret = libusb_bulk_transfer(dev_handle, S5P6818_EP_OUT, buf,
                    MIN(remain, TRANSFER_CHUNK), &transferred,
                    TRANSFER_TIMEOUT_MS);
                if (LIBUSB_SUCCESS != ret)
                    break;
                buf += transferred;
                remain -= transferred;
            }
            if (LIBUSB_SUCCESS == ret) {
                if (isVerbose)
                    printf("Finish transfer, transferred=%d\n", size);
                boot_ret = 0;
            }
            else {
                fprintf(stderr, "libusb_bulk_transfer: %s\n",
                    libusb_strerror(ret));
            }
            libusb_release_interface(dev_handle, 0);
        }
        else {
            fprintf(stderr, "libusb_claim_interface: %s\n",
                libusb_strerror(ret));
        }
    }
    else {
        fprintf(stderr, "device_open failed: %s\n", libusb_strerror(ret));
    }
    libusb_close(dev_handle);
    libusb_exit(ctx);
    return boot_ret;
}

static int outTofile(const char* fname, const unsigned char* data, unsigned size)
{
    FILE* fp;
    int wr;

    if (!strcmp(fname, "-"))
        fp = stdout;
    else {
        fp = fopen(fname, "w");
        if (fp == NULL) {
            perror(fname);
            return 1;
        }
    }
    while (size) {
        if ((wr = fwrite(data, 1, size, fp)) < 0) {
            perror("fwrite");
            break;
        }
        data += wr;
        size -= wr;
    }
    if (fp != stdout)
        fclose(fp);
    return size != 0;
}

static void usage(void)
{
    printf("\n"
        "usage: nanopi-load [options...] <bootloader.bin> [<loadaddr> [<startaddr>]]\n"
        "\n"
        "options:\n"
        "  -b <num> - write the value at \"boot method\" offset (0x57) in NSIH header\n"
        "  -c - dry run (implies -v)\n"
        "  -e - pad data with at least one '\\0' byte (for env to import)\n"
        "  -f - fix load size in Boot Header\n"
        "  -h - print this help\n"
        "  -o <file> - output result to file instead of upload\n"
        "  -v - be verbose\n"
        "  -x - add code for iROMBOOT to Boot Header that switches to 64-bit\n"
        "\n"
        "If <loadaddr> is specified, image is prepended with 512-byte Boot Header\n"
        "If <startaddr> is not specified, assume <loadaddr> + 0x200\n"
        "If <bootloader.bin> is \"-\", stdin is read\n"
        "\n");
}

int main(int argc, char* argv[])
{
    int offset, size, opt;
    int is64bitBoot = 0, dryRun = 0, isVerbose = 0, fixSize = 0;
    int bootMethod = -1, isEnv = 0;
    unsigned int load_addr;
    unsigned int launch_addr;
    const char* infile = NULL, * outfile = NULL;

    if (argc == 1) {
        usage();
        return 0;
    }
    while ((opt = getopt(argc, argv, "b:cefho:vx")) > 0) {
        switch (opt) {
        case 'b':
            bootMethod = strtoul(optarg, NULL, 0);
            break;
        case 'c':
            dryRun = 1;
            isVerbose = 1;
            break;
        case 'e':
            isEnv = 1;
            break;
        case 'f':
            fixSize = 1;
            break;
        case 'h':
            usage();
            return 0;
        case 'o':
            outfile = optarg;
            break;
        case 'v':
            isVerbose = 1;
            break;
        case 'x':
            is64bitBoot = 1;
            break;
        case '?':
            return 1;
        }
    }
    if (argc == optind) {
        printf("error: input file not provided\n");
        return 1;
    }
    infile = argv[optind++];
    offset = argc == optind ? 0 : 0x200;
    memset(mem, 0, offset);
    size = readBin(mem + offset, sizeof(mem) - offset, infile, isEnv);
    if (size < 0)
        return 1;
    if (argc > optind) {
        load_addr = strtoul(argv[optind++], NULL, 16);
        launch_addr = load_addr + offset;
        if (argc > optind)
            launch_addr = strtoul(argv[optind], NULL, 16);
        initBootloaderHeader(mem, is64bitBoot, size,
            load_addr, launch_addr, bootMethod);
        if (isVerbose) {
            printf("Added Boot Header: load=0x%x, launch=0x%x size=%u%s\n",
                load_addr, launch_addr, size, is64bitBoot ? " 64-bit" : "");
        }
        size += offset;
        offset = 0;
    }
    else {
        if (!checkBootloaderHeader(mem, size, isVerbose, fixSize,
            bootMethod))
            return 1;
    }
    if (dryRun)
        return 0;
    if (outfile) {
        return outTofile(outfile, mem, size);
    }
    return usbBoot(mem, size, isVerbose);
}