#ifndef _FAT_UTILS_H
#define _FAT_UTILS_H

#include <vector>
#include <sstream>
#include <string>
#include <algorithm>
#include <functional>

using namespace std;

// Utils
#define FAT_READ_SHORT(buffer,x) ((buffer[x]&0xff)|((buffer[x+1]&0xff)<<8))
#define FAT_READ_LONG(buffer,x) \
        ((buffer[x]&0xff)|((buffer[x+1]&0xff)<<8))| \
        (((buffer[x+2]&0xff)<<16)|((buffer[x+3]&0xff)<<24))

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

// mejora la lectura de las fechas
static inline string prettyDate(char *date)
{
    int H = FAT_READ_SHORT(date, 0);
    int D = FAT_READ_SHORT(date, 2);
    int h, i, s;
    int y, m, d;

    s = 2*(H&0x1f);
    i = (H>>5)&0x3f;
    h = (H>>11)&0x1f;

    d = D&0x1f;
    m = (D>>5)&0xf;
    y = 1980+((D>>9)&0x7f);

    char buffer[128];
    sprintf(buffer, "%d/%d/%04d %02d:%02d:%02d", d, m, y, h, i, s);

    return string(buffer);
}

// divide un string en un vector
static inline vector<string> &split(const string &s, char delim, vector<string> &elems) {
    stringstream ss(s);
    string item;
    while(getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

// Convierte una cadena string a minúsculas
static inline std::string strtolower(std::string myString)
{
  const int length = myString.length();
  for(int i=0; i!=length; ++i) {
    myString[i] = std::tolower(myString[i]);
  }

  return myString;
}

#endif
