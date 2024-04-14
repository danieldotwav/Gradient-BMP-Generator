#include <iostream>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <windows.h>
#include <cmath>
#include <limits>

const int CANVAS_WIDTH = 256;

void draw_line(int x0, int y0, int x1, int y1, std::vector<uint8_t>& data, int row_stride);
void draw_lineCPP(int x0, int y0, int x1, int y1, std::vector<uint8_t>& data, int row_stride);

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
        //draw_line(x1, y1, x2, y2, data, row_stride);
        draw_lineCPP(x1, y1, x2, y2, data, row_stride);

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

void draw_line(int xa, int ya, int xb, int yb, std::vector<uint8_t>& data, int row_stride) {
    unsigned char bits[CANVAS_WIDTH][CANVAS_WIDTH]; // Array to hold pixel values

    int x, y, xEnd;
    int deltaX = abs(xa - xb);
    int deltaY = abs(ya - yb);
    int p = 2 * deltaY - deltaX;
    int twoDy = 2 * deltaY;
    int twoDyDx = 2 * (deltaY - deltaX);

    // Initialize your bits array or other preparations as needed
    memset(bits, 0, sizeof(bits));  // Example: clear the bits array

    __asm {

        _Bresenham:
            mov         eax, dword ptr[xa]
            sub         eax, dword ptr[xb]
            push        eax
            mov         ebx, eax
            neg         eax
            cmovl       eax, ebx
            add         esp,4
            mov         dword ptr[deltaX], eax

            //int deltaY = abs(ya - yb);
            mov         eax, dword ptr[xa]
            sub         eax, dword ptr[xb]
            push        eax
            mov         ebx, eax
            neg         eax
            cmovl       eax, ebx
            add         esp, 4
            mov         dword ptr[deltaX], eax

            //int p = 2 * deltaY - deltaX;
            mov         eax, dword ptr[deltaY]
            shl         eax, 1
            sub         eax, dword ptr[deltaX]
            mov         dword ptr[p], eax

            //int twoDy = 2 * deltaY, twoDyDx = 2 * (deltaY - deltaX);
            mov         eax, dword ptr[deltaY]
            shl         eax, 1
            mov         dword ptr[twoDy], eax
            mov         eax, dword ptr[deltaY]
            sub         eax, dword ptr[deltaX]
            shl         eax, 1
            mov         dword ptr[twoDyDx], eax

            //if (xa > xb);
            mov         eax, dword ptr[xa]
            cmp         eax, dword ptr[xb]
            jle         else1

            //x = xb;
            mov         eax, dword ptr[xb]
            mov         dword ptr[x], eax

            //y = yb;
            mov         eax, dword ptr[yb]
            mov         dword ptr[y], eax

            //xEnd = xa;
            mov         eax, dword ptr[xa]
            mov         dword ptr[xEnd], eax

            //bits[y][x] = 0 - bits[y][x];
            mov         eax, dword ptr[y]
            shl         eax, 8
            lea         ecx, bits[eax]
            mov         edx, dword ptr[x]
            movsx       eax, byte ptr[ecx + edx]
            xor ecx, ecx
            sub         ecx, eax
            mov         edx, dword ptr[y]
            shl         edx, 8
            lea         eax, bits[edx]
            mov         edx, dword ptr[x]
            mov         byte ptr[eax + edx], cl

            jmp         while1

        else1:
            //x = xa;
            mov         eax, dword ptr[xa]
            mov         dword ptr[x], eax

            //y = ya;
            mov         eax, dword ptr[ya]
            mov         dword ptr[y], eax

            //xEnd = xb;
            mov         eax, dword ptr[xb]
            mov         dword ptr[xEnd], eax

            //bits[y][x] = 0 - bits[y][x];
            mov         eax, dword ptr[y]
            shl         eax, 8
            lea         ecx, bits[eax]
            mov         edx, dword ptr[x]
            movsx       eax, byte ptr[ecx + edx]
            xor ecx, ecx
            sub         ecx, eax
            mov         edx, dword ptr[y]
            shl         edx, 8
            lea         eax, bits[edx]
            mov         edx, dword ptr[x]
            mov         byte ptr[eax + edx], cl

            jmp         while1

        while1:
            //while (x < xEnd)
            mov         eax, dword ptr[x]
            cmp         eax, dword ptr[xEnd]
            jge         stop_program

            //x++
            mov         eax, dword ptr[x]
            add         eax, 1
            mov         dword ptr[x], eax

            //if (p < 0)
            cmp         dword ptr[p], 0
            jge         else2

            //p = p + twoDy
            mov         eax, dword ptr[p]
            add         eax, dword ptr[twoDy]
            mov         dword ptr[p], eax

            //bits[y][x] = 0 - bits[y][x];
            mov         eax, dword ptr[y]
            shl         eax, 8
            lea         ecx, bits[eax]
            mov         edx, dword ptr[x]
            movsx       eax, byte ptr[ecx + edx]
            xor ecx, ecx
            sub         ecx, eax
            mov         edx, dword ptr[y]
            shl         edx, 8
            lea         eax, bits[edx]
            mov         edx, dword ptr[x]
            mov         byte ptr[eax + edx], cl

            jmp         while1

        else2:

            //y++
            mov         eax, dword ptr[y]
            add         eax, 1
            mov         dword ptr[y], eax

            //p = p + twoDyDx
            mov         eax, dword ptr[p]
            add         eax, dword ptr[twoDyDx]
            mov         dword ptr[p], eax


            //bits[y][x] = 0 - bits[y][x];
            mov         eax, dword ptr[y]
            shl         eax, 8
            lea         ecx, bits[eax]
            mov         edx, dword ptr[x]
            movsx       eax, byte ptr[ecx + edx]
            xor ecx, ecx
            sub         ecx, eax
            mov         edx, dword ptr[y]
            shl         edx, 8
            lea         eax, bits[edx]
            mov         edx, dword ptr[x]
            mov         byte ptr[eax + edx], cl

            jmp         while1


        stop_program:
    }
}

void draw_lineCPP(int x0, int y0, int x1, int y1, std::vector<uint8_t>& data, int row_stride) {
    // Flip the y-coordinates since BMP format starts from the bottom-left corner
    // Flip the y-coordinates
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
