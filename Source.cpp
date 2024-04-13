#include <iostream>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <windows.h>
#include <cmath>
#include <limits>

const int CANVAS_WIDTH = 256;

void draw_line(int x0, int y0, int x1, int y1, std::vector<uint8_t>& data, int row_stride);

int main() {
    try {
        BITMAPFILEHEADER bmfh;
        BITMAPINFOHEADER bmih;
        std::vector<uint8_t> data;

        std::ofstream bmpOut("foo.bmp", std::ios::out | std::ios::binary);
        if (!bmpOut) {
            std::cout << "Could not open file, ending.";
            return -1;
        }

        int row_stride = CANVAS_WIDTH * 3;
        data.resize(row_stride * CANVAS_WIDTH);

        // Prompt user to enter coordinates
        int x1, y1, x2, y2;
        std::cout << "Enter two pairs of point coordinates in the range of 0-" << CANVAS_WIDTH - 1 << " (x1 y1 x2 y2):\n";
        std::cin >> x1 >> y1 >> x2 >> y2;

        // Validate coordinates
        if (x1 < 0 || x1 >= CANVAS_WIDTH || y1 < 0 || y1 >= CANVAS_WIDTH ||
            x2 < 0 || x2 >= CANVAS_WIDTH || y2 < 0 || y2 >= CANVAS_WIDTH) {
            std::cout << "Coordinates out of range, ending.";
            return -1;
        }

        // Fill the background with a vertical gradient
        for (int y = 0; y < CANVAS_WIDTH; y++) {
            for (int x = 0; x < CANVAS_WIDTH; x++) {
                float gradient = static_cast<float>(x) / CANVAS_WIDTH;
                uint8_t color_value = static_cast<uint8_t>(255 * gradient);
                int index = y * row_stride + x * 3;
                data[index + 0] = color_value; // Blue
                data[index + 1] = color_value; // Green
                data[index + 2] = color_value; // Red
            }
        }

        // Draw the line using Bresenham's algorithm
        draw_line(x1, y1, x2, y2, data, row_stride);

        // Prepare bitmap file headers
        bmfh.bfType = 0x4d42; // 'BM'
        bmfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
        bmfh.bfSize = bmfh.bfOffBits + data.size();
        bmfh.bfReserved1 = 0;
        bmfh.bfReserved2 = 0;

        bmih.biSize = sizeof(bmih);
        bmih.biWidth = CANVAS_WIDTH;
        bmih.biHeight = -CANVAS_WIDTH; // Negative for top-down bitmap
        bmih.biPlanes = 1;
        bmih.biBitCount = 24;
        bmih.biCompression = BI_RGB;
        bmih.biSizeImage = 0; // No compression
        bmih.biXPelsPerMeter = 2835;
        bmih.biYPelsPerMeter = 2835;
        bmih.biClrUsed = 0;
        bmih.biClrImportant = 0;

        // Write out the bitmap file header and info header
        bmpOut.write((char*)&bmfh, sizeof(bmfh));
        bmpOut.write((char*)&bmih, sizeof(bmih));
        bmpOut.write(reinterpret_cast<const char*>(data.data()), data.size());

        // Close the output file
        bmpOut.close();

        // Open the image in a default viewer
        system("mspaint foo.bmp");
    }
    catch (std::exception& e) {
        std::cout << e.what() << std::endl;
    }

    return 0;
}

void draw_line(int x0, int y0, int x1, int y1, std::vector<uint8_t>& data, int row_stride) {
    // Flip the y-coordinates
    y0 = CANVAS_WIDTH - 1 - y0;
    y1 = CANVAS_WIDTH - 1 - y1;

    int distX = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int distY = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = distX + distY, e2;

    uint8_t* data_ptr = data.data();  // Get the pointer to the vector's internal buffer

    __asm {
        mov eax, x0      // Start x
        mov ebx, y0      // Start y
        mov ecx, data_ptr  // Vector data pointer

        loop_start :
        // Check bounds
        cmp eax, 0
            jl out_of_bounds
            cmp eax, CANVAS_WIDTH
            jge out_of_bounds
            cmp ebx, 0
            jl out_of_bounds
            cmp ebx, CANVAS_WIDTH
            jge out_of_bounds

            // Calculate index = ebx * row_stride + eax * 3
            mov edi, ebx
            imul edi, row_stride
            mov esi, eax
            lea esi, [esi + esi * 2]  // esi = eax * 3
            add edi, esi              // edi = ebx * row_stride + eax * 3
            add edi, ecx              // edi = pointer to the pixel data

            // Set color to maximum values (white)
            mov byte ptr[edi], 255    // Set Blue
            mov byte ptr[edi + 1], 255  // Set Green
            mov byte ptr[edi + 2], 255  // Set Red

            out_of_bounds:
        // Bresenham's line algorithm calculations
        mov esi, err         // Current error
            shl esi, 1           // esi = 2 * err
            mov edx, distY
            neg edx              // edx = -distY
            cmp esi, edx
            jle adjust_y
            adjust_x :
        add esi, edx         // err += distY
            mov err, esi
            add eax, sx          // Increment x
            jmp check_end

            adjust_y :
        sub esi, distX       // err -= distX
            mov err, esi
            add ebx, sy          // Increment y

            check_end :
        cmp eax, x1          // Check if end of line
            jne loop_start
            cmp ebx, y1
            jne loop_start
    }
}

