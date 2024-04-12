#include <iostream>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <windows.h>
#include <cmath> // For std::abs
#include <limits>

const int CANVAS_WIDTH = 256; // Assuming BITMAP_SIZE is defined somewhere

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
#pragma pack(pop)

class BMP {
public:
    BMPFileHeader file_header;
    BMPInfoHeader bmp_info_header;
    std::vector<uint8_t> data;
    int row_stride{ 0 };

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

    void fill_background_white() {
        for (int y = 0; y < bmp_info_header.height; y++) {
            for (int x = 0; x < bmp_info_header.width; x++) {
                // Set each pixel's color to white
                data[y * row_stride + x * 3 + 0] = 255; // Blue
                data[y * row_stride + x * 3 + 1] = 255; // Green
                data[y * row_stride + x * 3 + 2] = 255; // Red
            }
        }
    }

    void fill_background_black() {
        for (int y = 0; y < bmp_info_header.height; y++) {
            for (int x = 0; x < bmp_info_header.width; x++) {
                // Set each pixel's color to white
                data[y * row_stride + x * 3 + 0] = 0; // Blue
                data[y * row_stride + x * 3 + 1] = 0; // Green
                data[y * row_stride + x * 3 + 2] = 0; // Red
            }
        }
    }

    void draw_line(int x0, int y0, int x1, int y1) {
        // Bresenham's line algorithm
        int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
        int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
        int err = dx + dy, e2;

        while (true) {
            set_pixel(x0, y0);
            if (x0 == x1 && y0 == y1) break;
            e2 = 2 * err;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
        }
    }

    void drawLine(int x1, int y1, int x2, int y2) {
        int distX = std::abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
        int distY = -std::abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
        int err = distX + distY, e2;

        while (true) {
            if (x1 >= 0 && x1 < bmp_info_header.width && y1 >= 0 && y1 < bmp_info_header.height) {
                // Calculate the offset in the BMP's data vector
                int pixelOffset = ((bmp_info_header.height - 1 - y1) * bmp_info_header.width + x1) * 3;

                // Ensure the offset is within bounds
                if (pixelOffset >= 0 && pixelOffset < data.size() - 3) {
                    // Use inline assembly to set the pixel color to white
                    __asm {
                        mov eax, dword ptr[this];    Move the this pointer into eax
                        mov eax, [eax]BMP.data;      Access the data member of BMP
                        add eax, pixelOffset;        Add the pixelOffset to the base address of data
                        mov byte ptr[eax], 255;      Set the blue component to 255
                        mov byte ptr[eax + 1], 255;  Set the green component to 255
                        mov byte ptr[eax + 2], 255;  Set the red component to 255
                    }
                }
            }

            if (x1 == x2 && y1 == y2) break;

            e2 = 2 * err;
            if (e2 >= distY) {
                if (x1 == x2) break;
                err += distY;
                x1 += sx;
            }
            if (e2 <= distX) {
                if (y1 == y2) break;
                err += distX;
                y1 += sy;
            }
        }
    }

    void write_headers(std::ofstream& of) {
        of.write((const char*)&file_header, sizeof(file_header));
        of.write((const char*)&bmp_info_header, sizeof(bmp_info_header));
    }

    void write_data(std::ofstream& of) {
        of.write((const char*)data.data(), data.size());
    }

    void set_pixel(int x, int y) {
        if (x >= 0 && x < bmp_info_header.width && y >= 0 && y < bmp_info_header.height) {
            data[y * row_stride + x * 3 + 0] = 255; // Blue
            data[y * row_stride + x * 3 + 1] = 255; // Green
            data[y * row_stride + x * 3 + 2] = 255; // Red
        }
    }
};

int main() {
    try {
        int x1, y1, x2, y2;

        std::cout << "Enter two pairs of point coordinates in the range of 0-" << CANVAS_WIDTH << ".\n";
        std::cin >> x1 >> y1 >> x2 >> y2;

        // Print appropriate error messages if applicable
        bool is_valid_input = 1;
        if (x1 < 0 || x1 > CANVAS_WIDTH) {
            std::cout << "Value " << x1 << " out of range, ending.\n";
            is_valid_input = 0;
        }
        if (y1 < 0 || y1 > CANVAS_WIDTH) {
            std::cout << "Value " << y1 << " out of range, ending.\n";
            is_valid_input = 0;
        }
        if (x2 < 0 || x2 > CANVAS_WIDTH) {
            std::cout << "Value " << x2 << " out of range, ending.\n";
            is_valid_input = 0;
        }
        if (y2 < 0 || y2 > CANVAS_WIDTH) {
            std::cout << "Value " << y2 << " out of range, ending.\n";
            is_valid_input = 0;
        }

        BITMAPFILEHEADER bmfh;
        BITMAPINFOHEADER bmih;
        char colorTable[CANVAS_WIDTH];
        char bits[CANVAS_WIDTH][CANVAS_WIDTH] = { 0 };

        std::ofstream bmpOut("foo.bmp", std::ios::out | std::ios::binary);
        if (!bmpOut) {
            std::cout << "...could not open file, ending.";
            return -1;
        }

        // Initialize the bitmap file header
        bmfh.bfType = 0x4d42;  // 'BM'
        bmfh.bfSize = sizeof(bmfh) + sizeof(bmih) + sizeof(colorTable) + IMAGE_SIZE * IMAGE_SIZE;
        bmfh.bfReserved1 = 0;
        bmfh.bfReserved2 = 0;
        bmfh.bfOffBits = sizeof(bmfh) + sizeof(bmih) + sizeof(colorTable);

        // Initialize the bitmap info header
        bmih.biSize = sizeof(bmih);
        bmih.biWidth = IMAGE_SIZE;
        bmih.biHeight = IMAGE_SIZE;
        bmih.biPlanes = 1;
        bmih.biBitCount = 8;
        bmih.biCompression = BI_RGB;
        bmih.biSizeImage = 0;  // No compression
        bmih.biXPelsPerMeter = 2835;
        bmih.biYPelsPerMeter = 2835;
        bmih.biClrUsed = 256;
        bmih.biClrImportant = 0;

        if (is_valid_input) {
            BMP bmp(256, 256); // Specify the dimensions of the bitmap image
            bmp.fill_gradient_vertical();
            //bmp.fill_background_white();
            //bmp.fill_background_black();
            //bmp.draw_line(x0, y0, x1, y1);
            bmp.drawLine(x1, y1, x2, y2);
            bmp.write("line.bmp");
            std::cout << "Line BMP file created." << std::endl;

            // Open the BMP file in Microsoft Paint
            system("start mspaint line.bmp");
        }
    }
    catch (std::exception& e) {
        std::cout << e.what() << std::endl;
    }

    return 0;
}