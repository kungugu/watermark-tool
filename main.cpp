#include <iostream>
#include "image.h"

const double kTransparencyCoefficient = 0.3;

void WatermarkMaker(Image* original_image, Image* watermark_image, Image* result_image) {
    if (!result_image) {
        std::cerr << "Ошибка: указатель на результат (result_image) равен nullptr." << std::endl;
        return;
    }

    int original_width = original_image->getWidth();
    int original_height = original_image->getHeight();
    int watermark_width = watermark_image->getWidth();
    int watermark_height = watermark_image->getHeight();

    if (original_width * original_height * watermark_width * watermark_height <= 0) {
        std::cerr << "Ошибка: одно из изображений имеет недопустимые размеры." << std::endl;
        return;
    }

    int overlap_width = std::min(original_width, watermark_width);
    int overlap_height = std::min(original_height, watermark_height);

    result_image->create(original_width, original_height);

    for (int y = 0; y < original_height; y++) {
        for (int x = 0; x < original_width; x++) {
            uint8_t original_blue, original_red, original_green;
            original_image->getPixel(x, y, original_blue, original_green, original_red);

            uint8_t result_blue = original_blue;
            uint8_t result_green = original_green;
            uint8_t result_red = original_red;

            if (x < overlap_width && y < overlap_height) {
                uint8_t watermark_blue, watermark_red, watermark_green;
                watermark_image->getPixel(x, y, watermark_blue, watermark_green, watermark_red);

                result_blue = static_cast<uint8_t>(
                    original_blue * (1.0 - kTransparencyCoefficient) + watermark_blue * kTransparencyCoefficient);
                result_green = static_cast<uint8_t>(
                    original_green * (1.0 - kTransparencyCoefficient) + watermark_green * kTransparencyCoefficient);
                result_red = static_cast<uint8_t>(
                    original_red * (1.0 - kTransparencyCoefficient) + watermark_red * kTransparencyCoefficient);
            }

            result_image->setPixel(x, y, result_blue, result_green, result_red);
        }
    }
}

int main() {
    Image orig;
    Image mark;
    Image res;

    if (!orig.load("test_data/original.bmp")) {
        std::cerr << "Не удалось загрузить original.bmp. Убедитесь, что файл существует." << std::endl;
        return 1;
    }

    if (mark.load("test_data/watermark.bmp")) {
        WatermarkMaker(&orig, &mark, &res);
        if (res.save("test_data/result.bmp")) {
            std::cout << "Тест пройден" << std::endl;
        }
    } else {
        std::cout << "watermark.bmp не найден" << std::endl;
    }
    return 0;
}