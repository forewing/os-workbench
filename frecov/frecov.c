#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define KB *1024
#define MB KB * 1024
#define GB MB * 1024

#define FILE_SIZE_MAX (96 MB)

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

typedef union fat32_boot_sector {
    uint8_t data[512];
    struct {
        uint8_t BS_jmpBoot[3];
        uint8_t BS_OEMName[8];
        uint16_t BPB_BytsPerSec;
        uint8_t BPB_SecPerClus;
        uint16_t BPB_ResvdSecCnt;
        uint8_t BPB_NumFATs;
        uint16_t BPB_RootEntCnt;
        uint16_t BPB_TotSec16;
        uint8_t BPB_Media;
        uint16_t BPB_FATSz16;
        uint16_t BPB_SecPerTrk;
        uint16_t BPB_NumHeads;
        uint32_t BPB_HiddSec;
        uint32_t BPB_TotSec32;
        uint32_t BPB_FATSz32;
        uint16_t BPB_ExtFlags;
        uint16_t BPB_FSVer;
        uint32_t BPB_RootClus;
        uint16_t BPB_FSInfo;
        uint16_t BPB_BkBootSec;
        uint8_t BPB_Reserved[12];
        uint8_t BS_DrvNum;
        uint8_t BS_Reserved1;
        uint8_t BS_BootSig;
        uint32_t BS_VolID;
        uint8_t BS_VolLab[11];
        uint8_t BS_FilSysType[8];
    } __attribute__((packed));
    struct {
        uint8_t prefix[510];
        uint8_t end_mark[2];
    } __attribute__((packed));
} fat32_bs_t;

#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20
#define ATTR_LAST_LONG_ENTRY 0x40
#define ATTR_LONG_NAME \
    ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID
#define ATTR_LONG_NAME_MASK                                       \
    ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID | \
        ATTR_DIRECTORY | ATTR_ARCHIVE

#define INSIDE_IMG(addr)                 \
    (((addr) >= (unsigned long)image) && \
     ((addr) < (unsigned long)(&image[image_size])))

typedef union fat32_dir_entry {
    uint8_t data[32];
    struct {
        uint8_t DIR_Name[11];
        uint8_t DIR_Attr;
        uint8_t DIR_NTRes;
        uint8_t DIR_CrtTimeTenth;
        uint16_t DIR_CrtTime;
        uint16_t DIR_CrtDate;
        uint16_t DIR_LstAccDate;
        uint16_t DIR_FstClusHI;
        uint16_t DIR_WrtTime;
        uint16_t DIR_WrtDate;
        uint16_t DIR_FstClusLO;
        uint32_t DIR_FileSize;
    } __attribute__((packed));
} fat32_dirent_t;

typedef union fat32_long_dir_entry {
    uint8_t data[32];
    struct {
        uint8_t LDIR_Ord;
        uint8_t LDIR_Name1[10];
        uint8_t LDIR_Attr;
        uint8_t LDIR_Type;
        uint8_t LDIR_Chksum;
        uint8_t LDIR_Name2[12];
        uint16_t LDIR_FstClusLO;
        uint8_t LDIR_Name3[4];
    } __attribute__((packed));
} fat32_ldirent_t;

typedef struct bmp_file {
    uint8_t BM[2];
    uint32_t total_size;
    uint16_t rezvd1;
    uint16_t rezvd2;
    uint32_t offset_bits;
    uint32_t header_size;
    uint32_t width;
    uint32_t height;
    uint16_t planes;
    uint16_t bits_per_pixel;
    uint32_t compression;
    uint32_t image_size;
    uint32_t x_pixel_per_meter;
    uint32_t y_pixel_per_meter;
    uint32_t number_of_color;
    uint32_t color_important;
} __attribute__((packed)) bmp_t;

long image_size = 0;
uint8_t* image = NULL;
fat32_bs_t* image_bs;
uint8_t (*image_cluster)[512];
uint8_t (*image_dirs)[32];

uint8_t bmp_name_buf[1024];
uint8_t bmp_name_buf_u[1024];
#define NAME_SHORT 1
#define NAME_LONG 2
int bmp_name_len;

int kmp_size = 0;
int* kmp_list = NULL;

int preprocess();
uint8_t dirent_ChkSum(uint8_t* str);
#define LEVEL_SOFT 1
#define LEVEL_HARD 2
int check_dir(int pos, int level);
int process_bmp(int pos);

int main(int argc, char* argv[]) {
    if (argc < 2 || argv[1] == NULL) {
        printf("Usage: frecov FILE\n");
        return 0;
    }

    FILE* img_fp = fopen(argv[1], "rb");
    if (img_fp == NULL) {
        perror("fopen");
        return 0;
    }

    fseek(img_fp, 0, SEEK_END);
    image_size = ftell(img_fp);
    // printf("image size: %ld\n", image_size);
    fseek(img_fp, 0, SEEK_SET);

    image = mmap(NULL, image_size, PROT_READ, MAP_PRIVATE, fileno(img_fp), 0);
    fclose(img_fp);
    if (image == MAP_FAILED) {
        perror("mmap");
        return 0;
    }

    preprocess();
    // for (int i = 0; i < 100; i++) {
    for (int i = 0; INSIDE_IMG((unsigned long)&image_dirs[i]); i++) {
        int pos = check_dir(i, LEVEL_HARD);
        if (pos != -1) {
            process_bmp(pos);
            // break;
        }
    }

    // scan again in soft mode, for deleted file only
    for (int i = 0; INSIDE_IMG((unsigned long)&image_dirs[i]); i++) {
        int pos = check_dir(i, LEVEL_SOFT);
        if (pos != -1) {
            process_bmp(pos);
            // break;
        }
    }

    return 0;
}

int preprocess() {
    image_bs = (fat32_bs_t*)image;

    uint32_t begin = (image_bs->BPB_FATSz32 * image_bs->BPB_NumFATs +
                      image_bs->BPB_ResvdSecCnt - 2) *
                     image_bs->BPB_BytsPerSec;
    // printf("%x %u\n", begin, begin);

    image_cluster = (uint8_t(*)[512])(&image[begin]);
    image_dirs = (uint8_t(*)[32])(&image[begin]);

    return 0;
}

uint8_t dirent_ChkSum(uint8_t* str) {
    uint8_t sum = 0;
    for (int i = 0; i < 11; i++) {
        sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1) + str[i];
    }
    return sum;
}

int check_dir(int pos, int level) {
    fat32_dirent_t* dirent = (fat32_dirent_t*)(&image_dirs[pos]);
    uint32_t entry = 0;
    entry = dirent->DIR_FstClusLO | (dirent->DIR_FstClusHI << 16);
    // printf("%lx\t", (unsigned long)&image_dirs[pos]);
    if (!INSIDE_IMG((unsigned long)&image_cluster[entry]) || entry <= 3) {
        return -1;
    }
    bmp_t* bmp = (bmp_t*)(&image_cluster[entry]);
    if (bmp->BM[0] != 'B' || bmp->BM[1] != 'M') {
        return -1;
    }
    if (dirent->DIR_FileSize != bmp->total_size) {
        return -1;
    }
    uint8_t dir_name[20];
    memcpy(dir_name, dirent->DIR_Name, 11);
    if (dir_name[0] == 0xe5) {
        if (level == LEVEL_HARD) {
            // freed
            return -1;
        } else {
            fat32_ldirent_t* ldirent = (fat32_ldirent_t*)(dirent);
            ldirent--;
            // repair the broken 1st byte
            dir_name[0] = ldirent->LDIR_Name1[0];
            if (dir_name[0] >= 'a' && dir_name[0] <= 'z') {
                dir_name[0] += ('A' - 'a');
            }
        }
    } else {
        if (level == LEVEL_SOFT) {
            // hard have done
            return -1;
        }
    }
    uint8_t sum = dirent_ChkSum(dir_name);
    // if (dirent->DIR_Name[6] == '~' && dirent->DIR_Name[7] >= '1' &&
    // dirent->DIR_Name[7] <= '9') {
    // if (1) {
    // in fact long name
    // uint8_t longname[512];
    bmp_name_len = NAME_LONG;
    long bmp_name_ptr = 0;
    fat32_ldirent_t* ldirent = (fat32_ldirent_t*)(dirent);
    for (int i = 1; i < 32; i++) {
        ldirent--;
        if (ldirent->LDIR_Chksum != sum) {
            // sum wrong
            if (level == LEVEL_HARD) {
                return -1;
            } else {
                break;
            }
        }
        if ((ldirent->LDIR_Ord & (~ATTR_LAST_LONG_ENTRY)) != i) {
            // order wrong
            if (level == LEVEL_HARD) {
                return -1;
            }
        }

        memcpy(bmp_name_buf_u + bmp_name_ptr, ldirent->LDIR_Name1,
               sizeof(ldirent->LDIR_Name1));
        bmp_name_ptr += sizeof(ldirent->LDIR_Name1);
        memcpy(bmp_name_buf_u + bmp_name_ptr, ldirent->LDIR_Name2,
               sizeof(ldirent->LDIR_Name2));
        bmp_name_ptr += sizeof(ldirent->LDIR_Name2);
        memcpy(bmp_name_buf_u + bmp_name_ptr, ldirent->LDIR_Name3,
               sizeof(ldirent->LDIR_Name3));
        bmp_name_ptr += sizeof(ldirent->LDIR_Name3);

        if ((ldirent->LDIR_Ord & ATTR_LAST_LONG_ENTRY) != 0) {
            // last long name
            if (level == LEVEL_HARD) {
                break;
            }
        }
    }
    bmp_name_buf_u[bmp_name_ptr++] = '\0';
    bmp_name_buf_u[bmp_name_ptr++] = '\0';

    for (int i = 0; i < bmp_name_ptr; i += 2) {
        bmp_name_buf[i / 2] = bmp_name_buf_u[i];
    }

    for (int i = 0; i < (bmp_name_ptr - 1) / 2 - 4; i++) {
        if (strncmp((const char*)bmp_name_buf + i, ".bmp", 4) == 0 ||
            strncmp((const char*)bmp_name_buf + i, ".BMP", 4) == 0) {
            bmp_name_buf[i + 4] = '\0';
            break;
        }
    }

    // } else {
    // DEPRECATED: every short name always have a long name
    // actually short name
    // if (dirent->DIR_Name[0] == '\xe5') {
    //     // space freed
    //     return -1;
    // }
    // bmp_name_len = NAME_SHORT;
    // int i;
    // for (i = 0; i < 8; i++) {
    //     if (dirent->DIR_Name[i] != ' ') {
    //         bmp_name_buf[i] = dirent->DIR_Name[i];
    //     } else {
    //         break;
    //     }
    // }
    // bmp_name_buf[i++] = '.';
    // for (int j = 8; j < 11; j++) {
    //     if (dirent->DIR_Name[j] != ' ') {
    //         bmp_name_buf[i++] = dirent->DIR_Name[j];
    //     } else {
    //         break;
    //     }
    // }
    // bmp_name_buf[i] = '\0';
    // }
    return entry;
}

// static int cnt = 0;
int process_bmp(int pos) {
    // if (cnt != 0) {
    //     return -1;
    // } else {
    //     cnt = 1;
    // }
    bmp_t* bmp = (bmp_t*)(&image_cluster[pos]);
    if (!INSIDE_IMG((unsigned long)bmp)) {
        // not in image
        return -1;
    }
    if (!INSIDE_IMG((unsigned long)bmp + bmp->total_size)) {
        // too big
        return -1;
    }
    // printf("%d\t%s\n", pos, bmp_name_buf);

    // sprintf((char*)bmp_name_buf_u, "%s", bmp_name_buf);
    FILE* bmp_fp = fopen((char*)bmp_name_buf, "wb");
    if (bmp_fp == NULL) {
        perror("bmp fopen");
        return 0;
    }

    fwrite((void*)bmp, bmp->total_size, 1, bmp_fp);
    fflush(bmp_fp);
    fclose(bmp_fp);
    char cmd[1024];
    sprintf(cmd, "sha1sum %s", (char*)bmp_name_buf);
    system(cmd);
    fflush(stdout);

    remove((char*)bmp_name_buf);
    return 0;
}