#pragma once
#include <cstdint>
#include <fstream>
#include <vector>
#include <string>
#include <utility>

#pragma pack(push, 1)
struct BITMAPFILEHEADER
{
    uint16_t filetype;     // сигнатура
    uint32_t filesize;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offsetdata;
};
#pragma pack(pop)


#pragma pack(push, 1)
struct BITMAPINFOHEADER
{
    uint32_t Size;          // размер этой структуры
    int32_t  Width;         // ширина в пикселях
    int32_t  Height;        // высота 
    uint16_t Planes;        // число плоскостей 
    uint16_t BitCount;      // бит на пиксель
    uint32_t Compression;   // тип сжатия 
    uint32_t SizeImage;     // размер данных пикселей в байтах
    int32_t  XPelsPerMeter; // разрешение по X
    int32_t  YPelsPerMeter; // разрешение по Y
    uint32_t ClrUsed;       // число цветов в палитре
    uint32_t ClrImportant;  // число важных цветов
};
#pragma pack(pop)


class Image
{
private:
    int width = 0;
    int height = 0;
    std::vector<uint8_t> pixels;
public:
    bool load (const std::string& filename){
        std::ifstream f(filename, std::ios::binary);
        if(!f){
            return false;
        }
        BITMAPFILEHEADER fh{};
        f.read(reinterpret_cast<char*>(&fh), sizeof(fh));

        BITMAPINFOHEADER  ih{};
        f.read(reinterpret_cast<char*>(&ih), sizeof(ih));

        if(!f){
            return false;
        }
        if(fh.filetype != 0x4D42){
            return false;
        }
        bool topDown = ih.Height < 0;
        int w = ih.Width;
        int h = topDown ? -ih.Height : ih.Height;
        size_t rowBytes = static_cast<size_t>(w) * 3;
        size_t padding = (4 - rowBytes % 4) % 4;

        std::vector<uint8_t> buf(rowBytes * static_cast<size_t>(h));

        f.seekg(fh.offsetdata, std::ios::beg);
        for(int i = 0; i < h; ++i){
            int row = topDown ? i : h - 1 - i;   // в памяти строка 0 = верх картинки
            f.read(reinterpret_cast<char*>(&buf[row * rowBytes]), rowBytes);
            f.ignore(padding);
        }
        if(!f){
            return false;
        }

        width = w;
        height = h;
        pixels = std::move(buf);
        return true;
    }
    
    bool save(const std::string& filename) const{
        if(width <= 0 or height <=0){
            return false;
        }
        if(pixels.size() != width * height * 3){
            return false;
        }
        std::ofstream f(filename, std::ios::binary);
        if(!f){
            return false;
        }
        int rowBytes = width * 3;
        int padding = (4-rowBytes%4)%4;
        int rowSize = rowBytes + padding;

        BITMAPFILEHEADER fh{};
        fh.filetype = 0x4D42;
        fh.offsetdata = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER); // 54
        fh.filesize = static_cast<uint32_t>(fh.offsetdata + rowSize * height);

        BITMAPINFOHEADER ih{};
        ih.Size = sizeof(BITMAPINFOHEADER); // 40
        ih.Width = width;
        ih.Height = height;                 // положительная: строки снизу вверх
        ih.Planes = 1;
        ih.BitCount = 24;
        ih.SizeImage = static_cast<uint32_t>(rowSize * height);

        f.write(reinterpret_cast<const char*>(&fh), sizeof(fh));
        f.write(reinterpret_cast<const char*>(&ih), sizeof(ih));

        const char zeros[3] = {0, 0, 0};
        for(int i = 0; i < height; ++i){
            int row = height - 1 - i;       // в файле первой идёт нижняя строка
            f.write(reinterpret_cast<const char*>(&pixels[row * rowBytes]), rowBytes);
            f.write(zeros, padding);
        }

        return static_cast<bool>(f);
    }

};


