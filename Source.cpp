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
    // Flip the y-coordinates since BMP format starts from the bottom-left corner
    y0 = CANVAS_WIDTH - 1 - y0;
    y1 = CANVAS_WIDTH - 1 - y1;

    int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    __asm {
        mov eax, x0      // Move x0 to eax
        mov ebx, y0      // Move y0 to ebx
        mov ecx, x1      // Move x1 to ecx
        mov edx, y1      // Move y1 to edx
        mov esi, err     // Move err to esi
        mov edi, dy      // Move dy to edi
        jmp check        // Jump to the beginning of loop checking condition
        loop_start :
        // Check pixel bounds and set color
        cmp eax, 0
            jl out_of_bounds
            cmp eax, CANVAS_WIDTH
            jge out_of_bounds
            cmp ebx, 0
            jl out_of_bounds
            cmp ebx, CANVAS_WIDTH
            jge out_of_bounds

            // Calculate index: index = ebx * row_stride + eax * 3
            mov edi, ebx
            imul edi, row_stride
            mov esi, eax
            lea esi, [esi + esi * 2]  // eax * 3
            add edi, esi          // edi = ebx * row_stride + eax * 3
            mov esi, [data]       // pointer to data vector storage
            add esi, edi          // esi = address to write to

            mov byte ptr[esi], 255     // Set Blue to max
            mov byte ptr[esi + 1], 255   // Set Green to max
            mov byte ptr[esi + 2], 255   // Set Red to max

            out_of_bounds:
        // Check end condition
        cmp eax, ecx
            jne continue_loop
            cmp ebx, edx
            je loop_end

            continue_loop :
        // Calculate error
        mov edi, esi         // Move err to edi for calculations
            shl edi, 1           // edi = 2 * err
            mov esi, edi         // Copy to esi for comparison
            cmp esi, edx         // Compare with dy
            jg adjust_x
            jmp adjust_y

            adjust_x :
        add err, dy          // err += dy
            add eax, sx          // x0 += sx

            adjust_y :
        cmp esi, dx          // Compare with dx
            jl no_y_adjust       // Jump if no adjustment needed
            add err, dx          // err += dx
            add ebx, sy          // y0 += sy
            no_y_adjust :
        jmp loop_start       // Continue loop

            loop_end :
    check:
        // This is to ensure the condition is checked before first iteration
        cmp eax, ecx         // Compare x0 with x1
            jne loop_start       // Jump if not equal
            cmp ebx, edx         // Compare y0 with y1
            jne loop_start
    }

}

