#include <iostream>
#include <fstream>
#include <vector>
#include <windows.h>

#pragma pack(push, 1)
struct BMPFileHeader {
    uint16_t file_type{ 0x4D42 };          // File type always BM which is 0x4D42
    uint32_t file_size{ 0 };               // Size of the file (in bytes)
    uint16_t reserved1{ 0 };               // Reserved, always 0
    uint16_t reserved2{ 0 };               // Reserved, always 0
    uint32_t offset_data{ 0 };             // Start position of pixel data (bytes from the beginning of the file)
};

struct BMPInfoHeader {
    uint32_t size{ 0 };                    // Size of this header (in bytes)
    int32_t width{ 0 };                    // width of bitmap in pixels
    int32_t height{ 0 };                   // width of bitmap in pixels
    uint16_t planes{ 1 };                  // No. of planes for the target device, this is always 1
    uint16_t bit_count{ 0 };               // No. of bits per pixel
    uint32_t compression{ 0 };             // 0 or 3 - uncompressed. This program does not produce compressed images
    uint32_t size_image{ 0 };              // 0 - for uncompressed images
    int32_t x_pixels_per_meter{ 0 };
    int32_t y_pixels_per_meter{ 0 };
    uint32_t colors_used{ 0 };             // No. color indexes in the color table. Use 0 for the max number of colors allowed by bit_count
    uint32_t colors_important{ 0 };        // No. of colors used for displaying the bitmap. If 0 all colors are required
};

struct BMPColorHeader {
    uint32_t red_mask{ 0x00ff0000 };       // Bit mask for the red channel
    uint32_t green_mask{ 0x0000ff00 };     // Bit mask for the green channel
    uint32_t blue_mask{ 0x000000ff };      // Bit mask for the blue channel
    uint32_t alpha_mask{ 0xff000000 };     // Bit mask for the alpha channel
    uint32_t color_space_type{ 0x73524742 }; // Default "sRGB" (0x73524742)
    uint32_t unused[16]{ 0 };              // Unused data for sRGB color space
};
#pragma pack(pop)

class BMP {
public:
    BMP(int32_t width, int32_t height, bool has_alpha = false) {
        if (width <= 0 || height <= 0) {
            throw std::runtime_error("The image width and height must be positive numbers.");
        }

        bmp_info_header.width = width;
        bmp_info_header.height = height;
        bmp_info_header.size = sizeof(BMPInfoHeader);
        file_header.offset_data = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader);

        if (has_alpha) {
            bmp_info_header.bit_count = 32;
            bmp_info_header.compression = 3; // BI_BITFIELDS, no compression
        }
        else {
            bmp_info_header.bit_count = 24;
            bmp_info_header.compression = 0; // BI_RGB, no compression
        }

        row_stride = width * bmp_info_header.bit_count / 8;
        data.resize(row_stride * height);
        file_header.file_size = file_header.offset_data + data.size();
    }

    void write(const char* fname) {
        std::ofstream of{ fname, std::ios_base::binary };
        if (!of) {
            throw std::runtime_error("Cannot open the output file.");
        }
        write_headers(of);
        write_data(of);
    }

    void fill_gradient_vertical() {
        for (int y = 0; y < bmp_info_header.height; y++) {
            for (int x = 0; x < bmp_info_header.width; x++) {
                float gradient = static_cast<float>(x) / bmp_info_header.width;
                uint8_t color_value = static_cast<uint8_t>(255 * gradient);
                data[y * row_stride + x * 3 + 0] = color_value; // Blue
                data[y * row_stride + x * 3 + 1] = color_value; // Green
                data[y * row_stride + x * 3 + 2] = color_value; // Red
            }
        }
    }

private:
    BMPFileHeader file_header;
    BMPInfoHeader bmp_info_header;
    std::vector<uint8_t> data;
    int row_stride{ 0 };

    void write_headers(std::ofstream& of) {
        of.write((const char*)&file_header, sizeof(file_header));
        of.write((const char*)&bmp_info_header, sizeof(bmp_info_header));
    }

    void write_data(std::ofstream& of) {
        of.write((const char*)data.data(), data.size());
    }
};

int main() {
    try {
        BMP bmp(1256, 1256); //.Specify the dimensions of the paint image
        bmp.fill_gradient_vertical();
        bmp.write("gradient_stripes.bmp");
        std::cout << "Gradient BMP file created." << std::endl;

        // Open the BMP file in Microsoft Paint
        system("start mspaint gradient_stripes.bmp");
    }
    catch (std::exception& e) {
        std::cout << e.what() << std::endl;
    }

    return 0;
}