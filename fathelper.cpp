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
#include "fatentry.h"

// ayuda tomada de https://github.com/Gregwar/fatcat/
/**
 * Opens the FAT resource
 */
FatHelper::FatHelper(string nombrearchivo_)
    : strange(0),
    nombrearchivo(nombrearchivo_),
    tamanoTotal(-1),
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
        oss << "! No se a podido abrir el archivo: " << nombrearchivo << ". Asegurate que exista y que hayas escrito bien la ruta al archivo.";

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
    if (tamanoTotal != -1 && address+size > tamanoTotal) {
        cerr << "! Intentando leer fuera del disco" << endl;
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
 * Leyendo Boot Sector y estructura BPB
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
        directorioRaiz = 0;

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
        directorioRaiz = FAT_READ_LONG(buffer, FAT_ROOT_DIRECTORY)&0xffffffff;
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

    if (directorioRaiz != 2 && tipo == FAT32) {
        printf("ADVERTENCIA: El directorio raiz no es 2 (%llu)\n", directorioRaiz);
        strange++;
    }
}

bool FatHelper::iniciar()
{
    // Leer el Boot sector y la estructura BPB
    leerBS_BPB();

    // Calculando los valores
    fatStart = bpb_BytsPerSec*bpb_RsvdSecCnt;
    dataStart = fatStart + bpb_NumFATs*bpb_FATSz*bpb_BytsPerSec;
    bytesPerCluster = bpb_BytsPerSec*bpb_SecPerClus;
    tamanoTotal = bpb_TotSec*bpb_BytsPerSec;
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
    cout << "Tamaño del disco: " << tamanoTotal << " (" << prettySize(tamanoTotal) << ")" << endl;
    cout << "Cantidad de Bytes por cluster: " << bytesPerCluster << endl;
    if (tipo == FAT16) {
        cout << "Número de entradas en la Raiz: " << bpb_RootEntCnt << endl;
        cout << "Clusters de la raiz: " << rootClusters << endl;
    }
    cout << "Tamaño de FAT: " << fatSize << " (" << prettySize(fatSize) << ")" << endl;
    printf("FAT1 dirección de inicio: %016llx\n", fatStart);
    printf("FAT2 dirección de inicio: %016llx\n", fatStart+fatSize);
    printf("Data dirección de inicio: %016llx\n", dataStart);
    cout << "Número de directorios en la raiz del cluster: " << directorioRaiz << endl;
    cout << endl;
}

void FatHelper::listar(string path)
{
    FatEntry entry;

    if (encontrarDirectorio(path, entry)) {
        listar(entry.cluster);
    }
}

void FatHelper::listar(unsigned int cluster)
{
    bool hasFree = false;
    vector<FatEntry> entries = getEntries(cluster, NULL, &hasFree);
    printf("Cluster de directorios: %u\n", cluster);
    if (hasFree) {
        printf("Advertencia: este directorio tiene clústeres libres que se leen de forma contigua\n");
    }
    printf("# Entries: %zu\n", entries.size());
    listar(entries);
}

void FatHelper::listar(vector<FatEntry> &entries)
{
    vector<FatEntry>::iterator it;

    for (it=entries.begin(); it!=entries.end(); it++) {
        FatEntry &entry = *it;

        if (entry.isErased() && !listarEliminados) {
            continue;
        }

        if (entry.isDirectory()) {
            printf("dir");
        } else {
            printf("arc");
        }

        string name = entry.getFilename();
        if (entry.isDirectory()) {
            name += "/";
        }

        printf(" %-30s", name.c_str());

        printf(" c=%u", entry.cluster);

        if (!entry.isDirectory()) {
            string pretty = prettySize(entry.size);
            printf(" s=%llu (%s)", entry.size, pretty.c_str());
        }

        if (entry.isHidden()) {
            printf(" h");
        }
        if (entry.isErased()) {
            printf(" d");
        }

        printf("\n");
        fflush(stdout);
    }
}

bool FatHelper::encontrarDirectorio(string path, FatEntry &outputEntry)
{
    int cluster;
    vector<string> parts;
    split(path, FAT_PATH_DELIMITER, parts);
    cluster = directorioRaiz;
    outputEntry.cluster = cluster;

    for (int i=0; i<parts.size(); i++) {
        if (parts[i] != "") {
            parts[i] = strtolower(parts[i]);
            vector<FatEntry> entries = getEntries(cluster);
            vector<FatEntry>::iterator it;
            bool found = false;

            for (it=entries.begin(); it!=entries.end(); it++) {
                FatEntry &entry = *it;
                string name = entry.getFilename();
                if (entry.isDirectory() && strtolower(name) == parts[i]) {
                    outputEntry = entry;
                    cluster = entry.cluster;
                    found = true;
                }
            }

            if (!found) {
                cerr << "Error: el directorio " << path << " no se ha encontrado" << endl;
                return false;
            }
        }
    }

    return true;
}

bool FatHelper::encontrarArchivo(string path, FatEntry &outputEntry)
{
    bool found = false;
    vector<string> parts;
    split(path, FAT_PATH_DELIMITER, parts);
    string parentdirname = "";
    for (int i=0; i<parts.size()-1; i++) {
        parentdirname += parts[i] + "/";
    }
    string basename = parts[parts.size()-1];

    basename = strtolower(basename);
    FatEntry parentEntry;
    if (encontrarDirectorio(parentdirname, parentEntry)) {
        vector<FatEntry> entries = getEntries(parentEntry.cluster);
        vector<FatEntry>::iterator it;

        for (it=entries.begin(); it!=entries.end(); it++) {
            FatEntry &entry = (*it);
            if (strtolower(entry.getFilename()) == basename) {
                outputEntry = entry;

                if (entry.size == 0) {
                    found = true;
                } else {
                    return true;
                }
            }
        }
    }

    return found;
}

void FatHelper::leer(string path, FILE *f)
{
    FatEntry entry;
    if (encontrarArchivo(path, entry)) {
        bool contiguous = false;
        if (entry.isErased() && freeCluster(entry.cluster)) {
            fprintf(stderr, "! Este archivo es un archivo que ha sido borrado\n");
            contiguous = true;
        }
        leer(entry.cluster, entry.size, f, contiguous);
    }
}

void FatHelper::leer(unsigned int cluster, unsigned int size, FILE *f, bool deleted)
{
    bool contiguous = deleted;

    if (f == NULL) {
        f = stdout;
    }

    while ((size!=0) && cluster!=FAT_LAST) {
        int currentCluster = cluster;
        int toRead = size;
        if (toRead > bytesPerCluster || size < 0) {
            toRead = bytesPerCluster;
        }
        char buffer[bytesPerCluster];
        leerInformacion(clusterAddress(cluster), buffer, toRead);

        if (size != -1) {
            size -= toRead;
        }

        // Write file data to the given file
        fwrite(buffer, toRead, 1, f);
        fflush(f);

        if (contiguous) {
            if (deleted) {
                do {
                    cluster++;
                } while (!freeCluster(cluster));
            } else {
                if (!freeCluster(cluster)) {
                    fprintf(stderr, "! Contiguous file contains cluster that seems allocated\n");
                    fprintf(stderr, "! Trying to disable contiguous mode\n");
                    contiguous = false;
                    cluster = nextCluster(cluster);
                } else {
                    cluster++;
                }
            }
        } else {
            cluster = nextCluster(currentCluster);

            if (cluster == 0) {
                fprintf(stderr, "! One of your file's cluster is 0 (maybe FAT is broken, have a look to -2 and -m)\n");
                fprintf(stderr, "! Trying to enable contigous mode\n");
                contiguous = true;
                cluster = currentCluster+1;
            }
        }
    }
}

vector<FatEntry> FatHelper::getEntries(unsigned int cluster, int *clusters, bool *hasFree)
{
    bool isRoot = false;
    bool contiguous = false;
    int foundEntries = 0;
    int badEntries = 0;
    bool isValid = false;
    set<unsigned int> visited;
    vector<FatEntry> entries;
    string filename;

    if (clusters != NULL) {
        *clusters = 0;
    }

    if (hasFree != NULL) {
        *hasFree = false;
    }

    if (cluster == 0 && tipo == FAT32) {
        cluster = directorioRaiz;
    }

    isRoot = (tipo==FAT16 && cluster==directorioRaiz);

    if (cluster == directorioRaiz) {
        isValid = true;
    }

    if (!validCluster(cluster)) {
        return vector<FatEntry>();
    }

    do {
        bool localZero = false;
        int localFound = 0;
        int localBadEntries = 0;
        unsigned long long address = clusterAddress(cluster, isRoot);
        char buffer[ENTRY_SIZE];
        if (visited.find(cluster) != visited.end()) {
            cerr << "! Looping directory" << endl;
            break;
        }
        visited.insert(cluster);

        unsigned int i;
        for (i=0; i<bytesPerCluster; i+=sizeof(buffer)) {
            // Reading data
            leerInformacion(address, buffer, sizeof(buffer));

            // Creating entry
            FatEntry entry;

            entry.attributes = buffer[FAT_ATTRIBUTES];
            entry.address = address;

            if (entry.attributes == FAT_ATTRIBUTES_LONGFILE) {
                // Long file part
                // filename.append(buffer);
            } else {
                entry.shortName = string(buffer, 11);
                entry.longName = filename;
                entry.size = FAT_READ_LONG(buffer, FAT_FILESIZE)&0xffffffff;
                entry.cluster = (FAT_READ_SHORT(buffer, FAT_CLUSTER_LOW)&0xffff) | (FAT_READ_SHORT(buffer, FAT_CLUSTER_HIGH)<<16);
                entry.setData(string(buffer, sizeof(buffer)));

                if (!entry.isZero()) {
                    if (entry.isCorrect() && validCluster(entry.cluster)) {
                        entry.creationDate = &buffer[FAT_CREATION_DATE];
                        entry.changeDate = &buffer[FAT_CHANGE_DATE];
                        entries.push_back(entry);
                        localFound++;
                        foundEntries++;

                        if (!isValid && entry.getFilename() == "." && entry.cluster == cluster) {
                            isValid = true;
                        }
                    } else {
                        localBadEntries++;
                        badEntries++;
                    }

                    localZero = false;
                } else {
                    localZero = true;
                }
            }

            address += sizeof(buffer);
        }

        int previousCluster = cluster;

        if (isRoot) {
            if (cluster+1 < rootClusters) {
                cluster++;
            } else {
                cluster = FAT_LAST;
            }
        } else {
            cluster = nextCluster(cluster);
        }

        if (clusters) {
            (*clusters)++;
        }

        if (cluster == 0 || contiguous) {
            contiguous = true;

            if (hasFree != NULL) {
                *hasFree = true;
            }
            if (!localZero && localFound && localBadEntries<localFound) {
                cluster = previousCluster+1;
            } else {
                if (!localFound && clusters) {
                    (*clusters)--;
                }
                break;
            }
        }

        if (!isValid) {
            if (badEntries>foundEntries) {
                cerr << "! Entries don't look good, this is maybe not a directory" << endl;
                return vector<FatEntry>();
          }
        }
    } while (cluster != FAT_LAST);

    return entries;
}

/**
 * Returns the 32-bit fat value for the given cluster number
 */
unsigned int FatHelper::nextCluster(unsigned int cluster, int fat)
{
    int bytes = (bits == 32 ? 4 : 2);
    char buffer[bytes];

    if (!validCluster(cluster)) {
        return 0;
    }

    leerInformacion(fatStart+fatSize*fat+(bits*cluster)/8, buffer, sizeof(buffer));

    unsigned int next;

    if (tipo == FAT32) {
        next = FAT_READ_LONG(buffer, 0)&0x0fffffff;

        if (next >= 0x0ffffff0) {
            return FAT_LAST;
        } else {
            return next;
        }
    } else {
        next = FAT_READ_SHORT(buffer,0)&0xffff;

        if (bits == 12) {
            int bit = cluster*bits;
            if (bit%8 != 0) {
                next = next >> 4;
            }
            next &= 0xfff;
            if (next >= 0xff0) {
                return FAT_LAST;
            } else {
                return next;
            }
        } else {
            if (next >= 0xfff0) {
                return FAT_LAST;
            } else {
                return next;
            }
        }
    }
}

bool FatHelper::validCluster(unsigned int cluster)
{
    return cluster < totalClusters;
}

unsigned long long FatHelper::clusterAddress(unsigned int cluster, bool isRoot)
{
    if (tipo == FAT32 || !isRoot) {
        cluster -= 2;
    }

    unsigned long long addr = (dataStart + bpb_BytsPerSec*bpb_SecPerClus*cluster);

    if (tipo == FAT16 && !isRoot) {
        addr += bpb_RootEntCnt * ENTRY_SIZE;
    }

    return addr;
}

bool FatHelper::freeCluster(unsigned int cluster)
{
    return nextCluster(cluster) == 0;
}

void FatHelper::asignarListarEliminados(bool listarEliminados_)
{
    listarEliminados = listarEliminados_;
}