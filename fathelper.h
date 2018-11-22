#ifndef _FATREADER_FATHELPER_H
#define _FATREADER_FATHELPER_H

#include <string>

#include "fatentry.h"

using namespace std;

// Last cluster
#define FAT_LAST (-1)

// Boot sector y estructura BPB
#define BS_OEMNAME                  0x3
#define BS_OEMNAME_SIZE             8
#define BPB_BYTSPERSEC              0x0b
#define BPB_SECPERCLUS              0x0d
#define BPB_RSVDSECCNT              0x0e
#define BPB_NUMFATS                 0x10
#define BPB_ROOTENTCNT              0x11 // Sólo aplica para FAT12 Y FAT16 para FAT32 es 0
#define BPB_TOTSEC16                0x13 // Sólo aplica para FAT12 Y FAT16 para FAT32 es 0
#define BPB_MEDIA                   0x15
#define BPB_FATSZ16                 0x16 // Sólo aplica para FAT12 Y FAT16 para FAT32 es 0
#define BPB_SECPERTRK               0x18
#define BPB_NUMHEADS                0x1A
#define BPB_HIDDSEC                 0x1C
#define BPB_TOTSEC32                0x20
// FAT 32
#define BPB_FATSZ32                 0x24
#define FAT_ROOT_DIRECTORY          0x2c
#define BS_VOLID32                  0x43
#define BS_VOLID32_SIZE             4
#define BS_VOLLAB32                 0x47
#define BS_VOLLAB32_SIZE            11
#define BS_FILSYSTYPE32             0x52
#define BS_FILSYSTYPE32_SIZE        8
// FAT 12 Y 16 
#define BS_VOLID16                  0x27
#define BS_VOLID16_SIZE             4
#define BS_VOLLAB16                 0x2b
#define BS_VOLLAB16_SIZE            11
#define BS_FILSYSTYPE16             0x36
#define BS_FILSYSTYPE16_SIZE        8

#define FAT_CREATION_DATE           0x10
#define FAT_CHANGE_DATE             0x16


#define FAT32 0
#define FAT16 1

#define FAT_PATH_DELIMITER '/'

class FatHelper
{
    public:
        FatHelper(string nombrearchivo);
        ~FatHelper();
        // Inicializo el helper
        bool iniciar();
        // Mostrar información del disco, se ejecuta cuando se pasa el parametro -i
        void informacion();
        // Listar directorios
        void listar(string path);
        void listar(unsigned int cluster);
        void listar(vector<FatEntry> &entries);
        // Leer archivo
        void leer(string path, FILE *f = NULL);
        void leer(unsigned int cluster, unsigned int size, FILE * f = NULL, bool deleted = false);



        // Mostrar archivos borrados al listar una ruta
        void asignarListarEliminados(bool listarEliminados);

        // Leer información del sistema
        int leerInformacion(unsigned long long address, char *buffer, int size);

        // Encontrar un directroio
        bool encontrarDirectorio(string path, FatEntry &entry);
        bool encontrarArchivo(string path, FatEntry &entry);

        // Obtener entradas de directorio para un clúster determinado
        vector<FatEntry> getEntries(unsigned int cluster, int *clusters = NULL, bool *hasFree = NULL);

        // Devuelve el siguiente número de clúster
        unsigned int nextCluster(unsigned int cluster, int fat=0);

        // El clúster es válido?
        bool validCluster(unsigned int cluster);

        // El clúster está libre?
        bool freeCluster(unsigned int cluster);

        // Retorna el desplazamiento del cluster en el sistema de archivos
        unsigned long long clusterAddress(unsigned int cluster, bool isRoot = false);

        // Variables de descripción del archivo
        string nombrearchivo;
        int fd;

        // Variables de los valores del BS y BPB
        int tipo;
        string bs_OEMName;
        unsigned long long bpb_BytsPerSec;
        unsigned long long bpb_SecPerClus;
        unsigned long long bpb_RsvdSecCnt;
        unsigned long long bpb_NumFATs;
        unsigned long long bpb_RootEntCnt; // Solamente aplican para FAT12 y FAT16
        unsigned long long bpb_TotSec;
        unsigned long long bpb_Media;
        string bpb_Media_Type;
        unsigned long long bpb_FATSz;
        unsigned long long bpb_SecPerTrk;
        unsigned long long bpb_NumHeads;
        unsigned long long bpb_HiddSec;
        unsigned long long bs_VolID;
        string bs_VolLab;
        string bs_FilSysType;
        unsigned long long directorioRaiz;
        unsigned long long reserved;
        unsigned long long strange;
        unsigned int bits;

        unsigned long long rootEntries;
        unsigned long long rootClusters;

        // Computed values
        unsigned long long fatStart;
        unsigned long long dataStart;
        unsigned long long bytesPerCluster;
        unsigned long long tamanoTotal;
        unsigned long long dataSize;
        unsigned long long fatSize;
        unsigned long long totalClusters;
        // FAT Cache
        bool cacheEnabled;
        // Stats values
        bool statsComputed;
        unsigned long long freeClusters;
        // Flags
        bool listarEliminados;
    protected:
        void leerBS_BPB();
};

#endif