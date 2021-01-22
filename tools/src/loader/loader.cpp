#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libusb.h>
#include <errno.h>
#include "getopt.h"
#include "s5p_usb.h"
#include "s5p_boot.h"

#define MIN(A, B) (((A) < (B)) ? (A) : (B))

typedef unsigned char uint8;
typedef unsigned int  uint32;

static unsigned char mem[BOOT_LOADER_MAX_SIZE];

static unsigned char nsih64[BOOT_LOADER_HEADER_SIZE];

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

static void initBootloaderHeader(uint8* buf, uint8* nsihbuf, unsigned headerType, unsigned size, unsigned loadAddr, unsigned launchAddr)
{
    unsigned i;
    if (headerType == 64)
    {
        for (i = 0; i < BOOT_LOADER_HEADER_SIZE; i++)
            buf[i] = nsihbuf[i];
    }
    write32LE(buf + BOOT_LOADER_HEADER_SIZE_OFFSET, size - BOOT_LOADER_HEADER_SIZE);  // actual boot loader size
    write32LE(buf + BOOT_LOADER_HEADER_LOADADDR_OFFSET, loadAddr);  // load address
    write32LE(buf + BOOT_LOADER_HEADER_LAUNCHADDR_OFFSET, launchAddr);  // launch address
    write32LE(buf + BOOT_LOADER_HEADER_BOOTMETHOD_OFFSET, 0);  // USB boot method
    *(int*)(buf + BOOT_LOADER_HEADER_NSIH_OFFSET) = BOOT_LOADER_HEADER_NSIH;
}

static int checkBootloaderHeader(uint8* buf, unsigned readSize, int isVerbose)
{
    unsigned size;
    int res = 0;
    unsigned actualBootloaderSize = readSize - BOOT_LOADER_HEADER_SIZE;
    if (readSize >= BOOT_LOADER_HEADER_SIZE) {
        if (*(int*)(buf + BOOT_LOADER_HEADER_NSIH_OFFSET) == BOOT_LOADER_HEADER_NSIH) {
            size = read32LE(buf + BOOT_LOADER_HEADER_SIZE_OFFSET);
            if (isVerbose) {
                printf("Boot Header:\n");
                printf("  size:           %u ", size);
                if (size == actualBootloaderSize)
                    printf("(OK)\n");
                else {
                    printf("(real: %u)\n", actualBootloaderSize);
                }
                printf("  load address:   0x%x\n", read32LE(buf + BOOT_LOADER_HEADER_LOADADDR_OFFSET));
                printf("  launch address: 0x%x\n", read32LE(buf + BOOT_LOADER_HEADER_LAUNCHADDR_OFFSET));
                unsigned char bootMethod = ((unsigned char*)buf)[BOOT_LOADER_HEADER_BOOTMETHOD_OFFSET];
                printf("  boot method:    %d (%s)\n", bootMethod, bootMethod < 5 ? BOOT_METHODS[bootMethod] : "unknown");
                printf("  signature:      OK\n");
            }
            else {
                if (size != actualBootloaderSize) {
                    printf("warning: wrong load size in header: %u, real: %u\n",size, actualBootloaderSize);
                }
            }
            res = 1;
        }
    }
    else {
        printf("error: read data size (%u) is less than boot header size\n", readSize);
    }
    return res;
}

static int readBin(uint8* buf, unsigned bufsize, const char* filepath)
{
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
    return size;
}

static int outTofile(const char* fname, const unsigned char* data, unsigned size)
{
    FILE* fp;
    int wr;

    if (!strcmp(fname, "-"))
        fp = stdout;
    else {
        fp = fopen(fname, "wb");
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
  
    while (NULL == *dev_handle_ptr) {
        ret = libusb_handle_events(ctx);
        if (LIBUSB_SUCCESS != ret) {
            fprintf(stderr, "libusb_handle_events: %s\n", libusb_strerror(ret));
        }
    }

    if (isVerbose)
        printf("Device opened.\n");

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

static void usage(void)
{
    printf("\n"
        "usage: loader [options...] <bootloader.bin> [<loadaddr> [<startaddr>]]\n"
        "\n"
        "options:\n"
        "  -h - print this help\n"
        "  -v - be verbose\n"
        "  -t <32|64> - add code for iROMBOOT for corresponding Boot Header type, 32 or 64 bit respectively\n"
        "  -s <file> - nsih file\n"
        "  -o <file> - output result to file instead of upload\n"
        "\n"
        "If <loadaddr> is specified, image is prepended with 512-byte Boot Header\n"
        "If <startaddr> is not specified, assume <loadaddr> + 0x200\n"
        "If <bootloader.bin> is \"-\", stdin is read\n"
        "\n");
}

int main(int argc, char* argv[])
{
    int size, opt;
    int headerType = 0, isVerbose = 0;
    unsigned int load_addr = BOOT_LOADER_LAUNCHADDR_DEFAULT;
    unsigned int launch_addr = BOOT_LOADER_LAUNCHADDR_DEFAULT;
    const char* infile = NULL, * outfile = NULL, * nsihfile = NULL;

    if (argc == 1) {
        usage();
        return 0;
    }
    while ((opt = getopt(argc, argv, "hvt:s:o:")) > 0) {
        switch (opt) {
        case 'h':
            usage();
            return 0;
        case 'v':
            isVerbose = 1;
            break;
        case 't':
            headerType = (strtoul(optarg, NULL, 10) == 64) ? 64 :32;
            break;
        case 's':
            nsihfile = optarg;
            break;
        case 'o':
            outfile = optarg;
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
    size = readBin(mem, sizeof(mem), infile);
    if (size < 0)
        return 1;
    if (argc > optind) {
        load_addr = strtoul(argv[optind++], NULL, 16);
        launch_addr = load_addr + 0x200;
        if (argc > optind)
            launch_addr = strtoul(argv[optind], NULL, 16);
    }
    if (headerType)
    {
        // check if header already defined
        if (!checkBootloaderHeader(mem, size, isVerbose))
        {
            if (headerType == 64)
            {
                if (nsihfile == NULL) {
                    printf("error: Boot header file name has not been provided!");
                    return 1;
                }
                if (readBin(nsih64, sizeof(nsih64), nsihfile) < 0) {
                    printf("error: Boot header file not found !");
                    return 1;
                }
            }
            initBootloaderHeader(mem, nsih64, headerType, size, load_addr, launch_addr);
            if (isVerbose) {
                printf("Added Boot Header: load=0x%x, launch=0x%x size=%u%s\n", load_addr, launch_addr, size, headerType == 64 ? " 64-bit" : "32-bit");
            }
        }
        else
        {
            printf("warning: Boot Header already defined!");
        }
    }
    else
    {
        if (!checkBootloaderHeader(mem, size, isVerbose))
        {
            printf("error: Boot header not found or incorrect\n");
            return 1;
        }
    }
    if (outfile) {
       return outTofile(outfile, mem, size);
    }

    return usbBoot(mem, size, isVerbose);
}