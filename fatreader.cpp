#include <stdlib.h>
#include <stdio.h>
#include <argp.h>
#include <string>
#include <iostream>
#include <vector>

#include "fathelper.h"

using namespace std;

void ayuda()
{
    cout << "Lector FAT construido para la práctica final de Sistemas Operativos 2018-2" << endl;
    cout << "Construido por:" << endl;
    cout << "Christian Barrero  - 201117501010" << endl;
    cout << "Ivan Vargas - " << endl;
    cout << endl;
    cout << "Uso: ./a.out nombrearchivo.img [opciones]" << endl;
    cout << endl;
    cout << "Listado de opciones:" << endl;
    cout << "  -i: Muestra información general acerca del disco" << endl;
    cout << "  -l [dir]: Litar los archivos y directorios en la ruta seleccionada" << endl;
    cout << "  -d: Permite listar los archivos eliminados" << endl;
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    vector<string> argumentos;
    char *image = NULL;
    int index;

    // -i, mostrar la información del disco
    bool mostrarInformacion = false;

    // -l, listar los directorios dada una ruta
    bool listarDirectorios = false;
    string rutaListar;

    // -d, listar archivos eliminados
    bool listarEliminados = false;

    // Leyendo argumentos de la linea de comandos
    while ((index = getopt(argc, argv, "il:L:r:R:s:dc:hx:2@:ob:p:w:v:mt:Sze:O:fk:a:")) != -1) {
        switch (index) {
            case 'i':
                mostrarInformacion = true;
                break;
            case 'l':
                listarDirectorios = true;
                rutaListar = string(optarg);
                break;
            case 'd':
                listarEliminados = true;
                break;
            case 'h':
                ayuda();
                break;
        }
    }

    // Obteniendo el archivo FAT
    if (optind != argc) {
        image = argv[optind];
    }

    if (!image) {
        ayuda();
    }

    // Obteniendo argumentos extra
    for (index=optind+1; index<argc; index++) {
        argumentos.push_back(argv[index]);
    }

    // Si el usuario no ha solicitado alguna información
    if (!(mostrarInformacion || listarDirectorios)) {
        ayuda();
    }

    try {
        // Abriendo la imagen con la clase FatHelper
        FatHelper fathelper(image);

        fathelper.asignarListarEliminados(listarEliminados);

        if (fathelper.iniciar()) {
            if (mostrarInformacion) {
                fathelper.informacion();
            } else if (listarDirectorios) {
                cout << "Listando la ruta " << rutaListar << endl;
                fathelper.listar(rutaListar);
            }
        } else {
            cout << "Error al iniciar el sistema de archivos FAT" << endl;
        }
    } catch (string error) {
        cerr << "Error: " << error << endl;
    }

    exit(EXIT_SUCCESS);
}