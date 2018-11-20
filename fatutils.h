#ifndef _FAT_UTILS_H
#define _FAT_UTILS_H

#include <vector>
#include <sstream>
#include <string>
#include <algorithm>
#include <functional>

using namespace std;

// ayuda aquí https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
// funcion trim para borrar espacios en blanco de la izquierda
static inline std::string ltrim(std::string s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
        return s;
}

// funcion trim para borrar espacios en blanco de la derecha
static inline std::string rtrim(std::string s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
        return s;
}

// funcion trim para borrar espacios en blanco de ambos lados
static inline std::string trim(std::string s) {
        return ltrim(rtrim(s));
}

static char unidades[] = {'B', 'K', 'M', 'G', 'T', 'P'};

// Mejorar la lectura de los tamaños
static inline string prettySize(unsigned long long bytes)
{
    double tamano = bytes;
    int n = 0;

    while (tamano >= 1024) {
        tamano /= 1024;
        n++;
    }

    ostringstream oss;
    oss << tamano << unidades[n];

    return oss.str();
}

#endif
