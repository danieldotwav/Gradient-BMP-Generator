#include <iostream>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <windows.h>
#include <cmath>
#include <limits>

const int CANVAS_WIDTH = 256;

void drawLine(int x0, int y0, int x1, int y1, std::vector<uint8_t>& data, int row_stride);

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
        drawLine(x1, y1, x2, y2, data, row_stride);

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

    // Pre-compute all required values before assembly
    int distX = std::abs(x1 - x0);
    int distY = -std::abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = distX + distY;
    int e2;

    uint8_t* data_ptr = data.data();  // Get the pointer to the vector's internal buffer

    __asm {
        mov eax, x0
        mov ebx, y0
        mov ecx, x1
        mov edx, y1
        mov esi, err

        _Bresenham :
        // Check if the loop should end
        cmp eax, ecx
            jg _end_loop
            cmp ebx, edx
            jg _end_loop

            // Calculate the index into the 'data' buffer
            push eax  // Save x
            mov eax, ebx  // eax = y
            mul dword ptr[row_stride]  // eax = y * row_stride
            pop ebx  // Restore x
            add eax, ebx  // eax = y * row_stride + x
            mov ebx, data_ptr  // ebx = base address of 'data'
            add ebx, eax  // ebx now points to the pixel location

            // Set the pixel
            mov byte ptr[ebx], 255

            // Calculate error and update positions
            mov eax, esi  // eax = err
            shl eax, 1
            add eax, distY
            jl _increment_y
            add eax, distX
            jge _next_step

            _increment_y :
            add ebx, sy  // Increment y

            _next_step :
            add eax, sx  // Increment x
            mov esi, eax  // Update err
            jmp _Bresenham

            _end_loop :
    }
}

// CPP Implementation
void drawLine(int x0, int y0, int x1, int y1, std::vector<uint8_t>& data, int row_stride) {
    // Flip the y-coordinates since BMP format starts from the bottom-left corner
    y0 = CANVAS_WIDTH - 1 - y0;
    y1 = CANVAS_WIDTH - 1 - y1;
    
    // Bresenham's line algorithm
    int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    while (true) {
        // Set the pixel
        if (x0 >= 0 && x0 < CANVAS_WIDTH && y0 >= 0 && y0 < CANVAS_WIDTH) {
            data[y0 * row_stride + x0 * 3 + 0] = 255; // Blue
            data[y0 * row_stride + x0 * 3 + 1] = 255; // Green
            data[y0 * row_stride + x0 * 3 + 2] = 255; // Red
        }

        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}
