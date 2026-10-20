#pragma once
#include <cstdint>
#include <fstream>
#include <vector>
#include <string>
#include <utility>
#include <climits>

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
    int bitCount = 24;                 // бит на пиксель (поддерживаем только 24)
    std::vector<uint8_t> pixels;       // B,G,R подряд, строка 0 = ВЕРХ картинки, без padding

public:
    // Загружает 24-битный несжатый BMP. При любой ошибке возвращает false,
    // а текущее изображение остаётся нетронутым.
    bool load(const std::string& filename) {
        std::ifstream f(filename, std::ios::binary);
        if (!f) {
            return false;
        }

        // Читаем оба заголовка целиком (структуры упакованы #pragma pack(1))
        BITMAPFILEHEADER fh{};
        f.read(reinterpret_cast<char*>(&fh), sizeof(fh));
        BITMAPINFOHEADER ih{};
        f.read(reinterpret_cast<char*>(&ih), sizeof(ih));
        if (!f) {
            return false;
        }

        // Проверка сигнатуры "BM"
        if (fh.filetype != 0x4D42) {
            return false;
        }

        // Проверка полей информационного заголовка до любых вычислений и выделения памяти
        if (ih.Width <= 0 ||                // ширина должна быть положительной
            ih.Height == 0 ||               // нулевая высота бессмысленна
            ih.Height == INT_MIN ||         
            ih.BitCount != 24 ||            // умеем читать только 24 бита на пиксель
            ih.Compression != 0) {          // только несжатый BMP (BI_RGB)
            return false;
        }

        // Отрицательная высота в BMP означает, что строки лежат сверху вниз
        bool topDown = ih.Height < 0;
        int w = ih.Width;
        int h = topDown ? -ih.Height : ih.Height;

        // Каждая строка в файле дополняется нулями до кратности 4 байтам
        size_t rowBytes = static_cast<size_t>(w) * 3;
        size_t padding = (4 - rowBytes % 4) % 4;

        // Защита от заголовка с гигантскими размерами: данные должны помещаться в файл
        f.seekg(0, std::ios::end);
        std::streamoff fileSize = f.tellg();
        unsigned long long needed = static_cast<unsigned long long>(fh.offsetdata) +
            static_cast<unsigned long long>(rowBytes + padding) * static_cast<unsigned long long>(h);
        if (fileSize < 0 || needed > static_cast<unsigned long long>(fileSize)) {
            return false;
        }

        std::vector<uint8_t> buf(rowBytes * static_cast<size_t>(h));

        // Переходим к началу пиксельных данных
        f.seekg(fh.offsetdata, std::ios::beg);
        for (int i = 0; i < h; ++i) {
            // Если высота положительная, первой в файле идёт нижняя строка -> переворачиваем,
            // чтобы в памяти строка 0 была верхней
            int row = topDown ? i : h - 1 - i;
            f.read(reinterpret_cast<char*>(&buf[static_cast<size_t>(row) * rowBytes]),
                   static_cast<std::streamsize>(rowBytes));
            f.ignore(static_cast<std::streamsize>(padding));   // пропускаем padding
        }
        if (!f) {
            return false;
        }

        width = w;
        height = h;
        bitCount = 24;
        pixels = std::move(buf);
        return true;
    }

    // Сохраняет изображение как 24-битный BMP
    bool save(const std::string& filename) const {
        if (width <= 0 || height <= 0) {
            return false;
        }
        if (pixels.size() != static_cast<size_t>(width) * static_cast<size_t>(height) * 3) {
            return false;
        }
        std::ofstream f(filename, std::ios::binary);
        if (!f) {
            return false;
        }

        size_t rowBytes = static_cast<size_t>(width) * 3;
        size_t padding = (4 - rowBytes % 4) % 4;
        size_t rowSize = rowBytes + padding;

        BITMAPFILEHEADER fh{};
        fh.filetype = 0x4D42;
        fh.offsetdata = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
        fh.filesize = static_cast<uint32_t>(fh.offsetdata + rowSize * static_cast<size_t>(height));

        BITMAPINFOHEADER ih{};
        ih.Size = sizeof(BITMAPINFOHEADER);
        ih.Width = width;
        ih.Height = height;               
        ih.Planes = 1;
        ih.BitCount = 24;
        ih.SizeImage = static_cast<uint32_t>(rowSize * static_cast<size_t>(height));

        f.write(reinterpret_cast<const char*>(&fh), sizeof(fh));
        f.write(reinterpret_cast<const char*>(&ih), sizeof(ih));

        const char zeros[3] = { 0, 0, 0 };
        for (int i = 0; i < height; ++i) {
            int row = height - 1 - i;     // в файле первой идёт нижняя строка
            f.write(reinterpret_cast<const char*>(&pixels[static_cast<size_t>(row) * rowBytes]),
                    static_cast<std::streamsize>(rowBytes));
            f.write(zeros, static_cast<std::streamsize>(padding));
        }

        return static_cast<bool>(f);
    }

    int getWidth() const { return width; }
    int getHeight() const { return height; }
    int getBitCount() const { return bitCount; }

    // Создаёт чёрное изображение w x h. Возвращает false при некорректных размерах.
    bool create(int w, int h) {
        if (w <= 0 || h <= 0) {
            return false;
        }
        width = w;
        height = h;
        bitCount = 24;
        pixels.assign(static_cast<size_t>(w) * static_cast<size_t>(h) * 3, 0);
        return true;
    }

    // Читает цвет пикселя (x, y); (0,0) - верхний левый угол.
    // За границами возвращает чёрный.
    void getPixel(int x, int y, uint8_t& b, uint8_t& g, uint8_t& r) const {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            size_t index = (static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x)) * 3;
            b = pixels[index];
            g = pixels[index + 1];
            r = pixels[index + 2];
        } else {
            b = g = r = 0;
        }
    }

    // Записывает цвет пикселя (x, y). За границами ничего не делает.
    void setPixel(int x, int y, uint8_t b, uint8_t g, uint8_t r) {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            size_t index = (static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x)) * 3;
            pixels[index] = b;
            pixels[index + 1] = g;
            pixels[index + 2] = r;
        }
    }
};