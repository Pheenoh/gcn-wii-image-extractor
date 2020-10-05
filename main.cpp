#include <fstream>
#include <iostream>
#include "dol-header-parser/dol_header_parser.hpp"
#include "stdint.h"

using namespace std;
#define DISK_HEADER_SIZE 0x440
#define DISK_HEADER_INFO_SIZE 0x2000
#define APPLOADER_HEADER_INFO_SIZE 0x20

struct DiskHeader {
    char console_id;
    char game_code[0x2];
    char country_code;
    char maker_code[0x2];
    uint8_t disk_id;
    uint8_t version;
    bool audio_streaming;
    uint8_t stream_buffer_size;
    uint8_t padding0xA[0x12];
    char dvd_magic_word[0x4];
    char game_name[0x3E0];
    uint32_t debug_monitor_offset;
    uint32_t debug_monitor_address;
    uint8_t padding0x408[0x18];
    uint32_t main_dol_offset;
    uint32_t fst_offset;
    uint32_t fst_size;
    uint32_t max_fst_size;
    uint32_t user_position;
    uint32_t user_length;
    uint32_t unk0x438;
    uint8_t padding0x43c[0x4];
};
static_assert(sizeof(DiskHeader) == DISK_HEADER_SIZE);

struct DiskHeaderInformation {
    uint32_t debug_monitor_size;
    uint32_t simulated_memory_size;
    uint32_t argument_offset;
    uint32_t debug_flag;
    uint32_t track_location;
    uint32_t track_size;
    uint32_t country_code;
    uint32_t unk0x1c;
    uint8_t padding0x20[0x1FE0];
};
static_assert(sizeof(DiskHeaderInformation) == DISK_HEADER_INFO_SIZE);

struct ApploaderHeader {
    char revision[16];    // Revision compile date (example "2001/12/17")
    uint32_t entryPoint;  // Apploader's entrypoint
    int32_t size;         // Size of apploader code
    int32_t trailerSize;  // Size of safe trailer
    uint8_t padding[4];   // zeroes
};
static_assert(sizeof(ApploaderHeader) == APPLOADER_HEADER_INFO_SIZE);

// Full apploader size is sum of size and trailerSize

struct Disk {
    DiskHeader header;
    DiskHeaderInformation header_info;
    ApploaderHeader apploader_header;
};

void SwapEndian(uint32_t &val) {
    val = (val << 24) | ((val << 8) & 0x00ff0000) |
          ((val >> 8) & 0x0000ff00) | (val >> 24);
}

int main(int argc, char **argv) {
    fstream fh;

#define DEBUG 1

    if (DEBUG) {
        argv[1] = (char *)"GZ2P01.iso";
        argc = 2;
    }

    if (argc == 2) {
        fh.open(argv[1], ifstream::in | ifstream::binary);
    } else {
        cout << "Usage: dhp.exe <your.dol>\n";
        return 1;
    }
    if (!fh) {
        cout << "Cannot open file.\n";
        return 1;
    }

    Disk disk;

    fh.read((char *)&disk.header, DISK_HEADER_SIZE + DISK_HEADER_INFO_SIZE + APPLOADER_HEADER_INFO_SIZE);
    fh.seekg(DISK_HEADER_SIZE + DISK_HEADER_INFO_SIZE + APPLOADER_HEADER_INFO_SIZE);
    int apploader_size = disk.apploader_header.size + disk.apploader_header.trailerSize;
    uint8_t* apploader_code = new uint8_t[apploader_size];
    fh.read((char *)apploader_code, apploader_size);
    SwapEndian(disk.header.main_dol_offset);
    fh.seekg(disk.header.main_dol_offset);
    printf("0x%08X\n",disk.header.main_dol_offset);
    CDolHeader dol_header;
    fh.read((char*)&dol_header, DOL_HEADER_SIZE);
    

    dol_header.SwapEndian();
    int dol_size;
    for (int i = 0; i < DATA_SECTION_SIZE; i++) {
        if (dol_header.m_section_offsets.m_data_offset[i] != 0x0) {
            dol_size = dol_header.m_section_offsets.m_data_offset[i] + dol_header.m_section_lengths.m_data_length[i];
        }
    }

    printf("dol size: %08X\n", dol_size);
    uint8_t* main_dol = new uint8_t[dol_size];
    fh.seekg(disk.header.main_dol_offset);
    fh.read((char *)main_dol, dol_size);
    fstream fh2;
    fh2.open("main.dol", ifstream::out | ifstream::binary);
    fh2.write((char*)main_dol, dol_size);
    fh.close();
    fh2.close();

    printf("buffer size: %d\n", sizeof(*main_dol));

    
    SwapEndian(disk.header.fst_offset);
    SwapEndian(disk.header_info.simulated_memory_size);
    printf("Gamecode: %c%s\n", disk.header.console_id, disk.header.game_code, disk.header.country_code);
    printf("Game Name: %s\n", disk.header.game_name);
    printf("main.dol offset: 0x%08X\n", disk.header.main_dol_offset);
    printf("fst.bin offset: 0x%08X\n", disk.header.fst_offset);
    printf("debug monitor size: 0x%04X\n", disk.header_info.simulated_memory_size);
}