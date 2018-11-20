#include <unistd.h>
#include <time.h>
#include <string>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <set>

#include <FatUtils.h>
#include "fathelper.h"

// Utils
#define FAT_READ_SHORT(buffer,x) ((buffer[x]&0xff)|((buffer[x+1]&0xff)<<8))
#define FAT_READ_LONG(buffer,x) \
        ((buffer[x]&0xff)|((buffer[x+1]&0xff)<<8))| \
        (((buffer[x+2]&0xff)<<16)|((buffer[x+3]&0xff)<<24))

/**
 * Opens the FAT resource
 */
FatHelper::FatHelper(string nombrearchivo_)
    : strange(0),
    nombrearchivo(nombrearchivo_),
    totalSize(-1),
    listarEliminados(false),
    statsComputed(false),
    freeClusters(0),
    cacheEnabled(false),
    tipo(FAT32),
    bpb_RootEntCnt(0)
{
    fd = open(nombrearchivo.c_str(), O_RDONLY|O_LARGEFILE);

    if (fd < 0) {
        ostringstream oss;
        oss << "! Unable to open the input file: " << nombrearchivo << " for reading";

        throw oss.str();
    }
}

FatHelper::~FatHelper()
{
    close(fd);
}

/**
 * Reading some data
 */
int FatHelper::leerInformacion(unsigned long long address, char *buffer, int size)
{
    if (totalSize != -1 && address+size > totalSize) {
        cerr << "! Trying to read outside the disk" << endl;
    }

    lseek64(fd, address, SEEK_SET);

    int n;
    int pos = 0;
    do {
        n = read(fd, buffer+pos, size);

        if (n > 0) {
            pos += n;
            size -= n;
        }
    } while ((size>0) && (n>0));

    return n;
}

/**
 * Parses FAT header
 */
void FatHelper::leerBS_BPB()
{
    char buffer[128];

    leerInformacion(0x0, buffer, sizeof(buffer));
    bs_OEMName = string(buffer + BS_OEMNAME, BS_OEMNAME_SIZE);
    bpb_BytsPerSec = FAT_READ_SHORT(buffer, BPB_BYTSPERSEC)&0xffff;
    bpb_SecPerClus = buffer[BPB_SECPERCLUS]&0xff;
    bpb_RsvdSecCnt = FAT_READ_SHORT(buffer, BPB_RSVDSECCNT)&0xffff;
    bpb_NumFATs = buffer[BPB_NUMFATS];
    bpb_Media = FAT_READ_SHORT(buffer, BPB_MEDIA)&0xffff;

    bpb_FATSz = FAT_READ_SHORT(buffer, BPB_FATSZ16)&0xffff;
    bpb_SecPerTrk = FAT_READ_SHORT(buffer, BPB_SECPERTRK)&0xffff;
    bpb_NumHeads = FAT_READ_SHORT(buffer, BPB_NUMHEADS)&0xffff;
    bpb_HiddSec = FAT_READ_SHORT(buffer, BPB_HIDDSEC)&0xffff;

    if (bpb_FATSz != 0) {
        tipo = FAT16;
        bits = 16;
        bs_VolID = FAT_READ_SHORT(buffer, BS_VOLID16)&0xffff;
        bs_VolLab = string(buffer+BS_VOLLAB16, BS_VOLLAB16_SIZE);
        bs_FilSysType = string(buffer+BS_FILSYSTYPE16, BS_FILSYSTYPE16_SIZE);
        bpb_RootEntCnt = FAT_READ_SHORT(buffer, BPB_ROOTENTCNT)&0xffff;
        rootDirectory = 0;

        if (trim(bs_FilSysType) == "FAT12") {
            bits = 12;
        }

        bpb_TotSec = FAT_READ_SHORT(buffer, BPB_TOTSEC16)&0xffff;
        if (!bpb_TotSec) {
            bpb_TotSec = FAT_READ_LONG(buffer, BPB_TOTSEC32)&0xffffffff;
        }
    } else {
        tipo = FAT32;
        bits = 32;
        bpb_FATSz = FAT_READ_LONG(buffer, BPB_FATSZ32)&0xffffffff;
        bpb_TotSec = FAT_READ_LONG(buffer, BPB_TOTSEC32)&0xffffffff;
        bs_VolID = FAT_READ_LONG(buffer, BS_VOLID32)&0xffffffff;
        bs_VolLab = string(buffer+BS_VOLLAB32, BS_VOLLAB32_SIZE);
        rootDirectory = FAT_READ_LONG(buffer, FAT_ROOT_DIRECTORY)&0xffffffff;
        bs_FilSysType = string(buffer+BS_FILSYSTYPE32, BS_FILSYSTYPE32_SIZE);
    }

    if (bpb_BytsPerSec != 512) {
        printf("ADVERTENCIA: Los Bytes por sector no son 512 (%llu)\n", bpb_BytsPerSec);
        strange++;
    }

    if (bpb_SecPerClus > 128) {
        printf("ADVERTENCIA: Sectors per cluster high (%llu)\n", bpb_SecPerClus);
        strange++;
    }

    if (bpb_NumFATs != 2) {
        printf("ADVERTENCIA: El número de FATs es diferente de 2 (%llu)\n", bpb_NumFATs);
        strange++;
    }

    if (rootDirectory != 2 && tipo == FAT32) {
        printf("ADVERTENCIA: El directorio raiz no es 2 (%llu)\n", rootDirectory);
        strange++;
    }
}

void FatHelper::listar(string path)
{
    // FatEntry entry;

    /*if (findDirectory(path, entry)) {
        list(entry.cluster);
    }*/
}

bool FatHelper::iniciar()
{
    // Leer el Boot sector y la estructura BPB
    leerBS_BPB();

    // Calculando los valores
    fatStart = bpb_BytsPerSec*bpb_RsvdSecCnt;
    dataStart = fatStart + bpb_NumFATs*bpb_FATSz*bpb_BytsPerSec;
    bytesPerCluster = bpb_BytsPerSec*bpb_SecPerClus;
    totalSize = bpb_TotSec*bpb_BytsPerSec;
    fatSize = bpb_FATSz*bpb_BytsPerSec;
    totalClusters = (fatSize*8)/bits;
    dataSize = totalClusters*bytesPerCluster;
    if (bpb_Media == 248) {
        bpb_Media_Type = "fijo";
    }
    if (bpb_Media == 240) {
        bpb_Media_Type = "removible";
    }
    if (tipo == FAT16) {
        int rootBytes = bpb_RootEntCnt*32;
        rootClusters = rootBytes/bytesPerCluster + ((rootBytes%bytesPerCluster) ? 1 : 0);
    }

    return strange == 0;
}

void FatHelper::informacion()
{
    cout << "Información del sistema de archivos FAT" << endl << endl;

    cout << "Tipo de sistema de archivos: " << bs_FilSysType << endl;
    cout << "ID del disco: " << bs_VolID << endl;
    cout << "Label del disco: " << bs_VolLab << endl;
    cout << "Nombre del OEM: " << bs_OEMName << endl;
    cout << "Número de Bytes por sector: " << bpb_BytsPerSec << endl;
    cout << "Número de sectores por cluster: " << bpb_SecPerClus << endl;
    cout << "Número de sectores reservados: " << bpb_RsvdSecCnt << endl;
    cout << "Número de FATs: " << bpb_NumFATs << endl;
    cout << "Tipo de media: " << bpb_Media_Type << endl;
    cout << "Número de sectores por FAT: " << bpb_FATSz << endl;
    cout << "Número de sectores por Track: " << bpb_SecPerTrk << endl;
    cout << "Número de cabezas: " << bpb_NumHeads << endl;
    cout << "Número de sectores escondidos: " << bpb_HiddSec << endl;
    cout << "Número total de sectores: " << bpb_TotSec << endl;
    cout << "Número total de clusters: " << totalClusters << endl;
    cout << "Tamaño de datos: " << dataSize << " (" << prettySize(dataSize) << ")" << endl;
    cout << "Tamaño del disco: " << totalSize << " (" << prettySize(totalSize) << ")" << endl;
    cout << "Cantidad de Bytes por cluster: " << bytesPerCluster << endl;
    if (tipo == FAT16) {
        cout << "Número de entradas en la Raiz: " << bpb_RootEntCnt << endl;
        cout << "Clusters de la raiz: " << rootClusters << endl;
    }
    cout << "Tamaño de FAT: " << fatSize << " (" << prettySize(fatSize) << ")" << endl;
    printf("FAT1 dirección de inicio: %016llx\n", fatStart);
    printf("FAT2 dirección de inicio: %016llx\n", fatStart+fatSize);
    printf("Data dirección de inicio: %016llx\n", dataStart);
    cout << "Root directory cluster: " << rootDirectory << endl;
    cout << endl;
}

void FatHelper::asignarListarEliminados(bool listarEliminados_)
{
    listarEliminados = listarEliminados_;
}