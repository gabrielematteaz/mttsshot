#include <system_error>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <print>
#include <Windows.h>
#include "png.h"

int WINAPI wWinMain(_In_ HINSTANCE, _In_ HINSTANCE, _In_opt_ LPWSTR, _In_ int)
{
	HDC scr_DC = NULL;
	HDC scr_mem_DC = NULL;
	HBITMAP scr_bit_h = NULL;
	png_bytep scr_bit_data = nullptr;
	png_structp PNG_struct = nullptr;
	png_infop PNG_info = nullptr;
	std::FILE *PNG_file = nullptr;
	png_bytep rows = nullptr;

	try
	{
		if (RegisterHotKey(NULL, 1, MOD_CONTROL | MOD_NOREPEAT, VK_MULTIPLY) == 0)
			throw std::system_error(ERROR_GEN_FAILURE, std::system_category(), "RegisterHotKey");

		if (RegisterHotKey(NULL, 2, MOD_CONTROL | MOD_NOREPEAT, VK_SUBTRACT) == 0)
			throw std::system_error(ERROR_GEN_FAILURE, std::system_category(), "RegisterHotKey");

		BOOL res;
		MSG msg;

		while (true)
		{
			res = GetMessageW(&msg, NULL, 0, 0);

			if (res == -1 || res == 0)
				break;

			if (msg.message == WM_HOTKEY)
			{
				if (LOWORD(msg.lParam) == MOD_CONTROL)
				{
					auto virt_key = HIWORD(msg.lParam);

					if (virt_key == VK_MULTIPLY)
						break;

					if (virt_key == VK_SUBTRACT)
					{
						scr_DC = GetDC(NULL);

						if (scr_DC == NULL)
							throw std::system_error(ERROR_GEN_FAILURE, std::system_category(), "GetDC");

						scr_mem_DC = CreateCompatibleDC(scr_DC);

						if (scr_mem_DC == NULL)
							throw std::system_error(ERROR_GEN_FAILURE, std::system_category(), "CreateCompatibleDC");

						auto scr_w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
						auto scr_h = GetSystemMetrics(SM_CYVIRTUALSCREEN);

						scr_bit_h = CreateCompatibleBitmap(scr_DC, scr_w, scr_h);

						if (scr_bit_h == NULL)
							throw std::system_error(ERROR_GEN_FAILURE, std::system_category(), "CreateCompatibleBitmap");

						SelectObject(scr_mem_DC, scr_bit_h);

						auto scr_X = GetSystemMetrics(SM_XVIRTUALSCREEN);
						auto scr_Y = GetSystemMetrics(SM_YVIRTUALSCREEN);

						if (BitBlt(scr_mem_DC, 0, 0, scr_w, scr_h, scr_DC, scr_X, scr_Y, SRCCOPY) == 0)
							throw std::system_error(GetLastError(), std::system_category(), "BitBlt");

						BITMAP scr_bit;

						if (GetObject(scr_bit_h, sizeof(scr_bit), &scr_bit) == 0)
							throw std::system_error(ERROR_GEN_FAILURE, std::system_category(), "GetObject");

						BITMAPINFOHEADER bit_info = { sizeof(bit_info), scr_bit.bmWidth, -1 * scr_bit.bmHeight, 1, 24, BI_RGB, 0, 0, 0, 0, 0 };

						std::size_t scr_bit_row_size = (scr_bit.bmWidth * 3 + 3) / 4 * 4;
						std::size_t scr_bit_size = scr_bit_row_size * scr_bit.bmHeight;

						scr_bit_data = new png_byte[scr_bit_size];

						if (GetDIBits(scr_DC, scr_bit_h, 0, scr_h, scr_bit_data, reinterpret_cast < LPBITMAPINFO > (&bit_info), DIB_RGB_COLORS) == 0)
							throw std::system_error(ERROR_GEN_FAILURE, std::system_category(), "GetDIBits");

						DeleteObject(scr_bit_h);
						scr_bit_h = NULL;
						DeleteObject(scr_mem_DC);
						scr_mem_DC = NULL;
						ReleaseDC(NULL, scr_DC);
						scr_DC = NULL;

						//

						PNG_struct = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

						if (PNG_struct == NULL)
							throw std::system_error(ERROR_GEN_FAILURE, std::system_category(), "png_create_write_struct");

						PNG_info = png_create_info_struct(PNG_struct);

						if (PNG_info == NULL)
							throw std::system_error(ERROR_GEN_FAILURE, std::system_category(), "png_create_info_struct");

						if (setjmp(png_jmpbuf(PNG_struct)) != 0)
							throw std::system_error(ERROR_GEN_FAILURE, std::system_category(), "setjmp");

						SYSTEMTIME sys_time;

						GetSystemTime(&sys_time);

						auto PNG_file_name = std::format("{:0>4}{:0>2}{:0>2}T{:0>2}{:0>2}{:0>2}Z.PNG",
							sys_time.wYear, sys_time.wMonth, sys_time.wDay, sys_time.wHour, sys_time.wMinute, sys_time.wSecond);
						auto PNG_file = std::fopen(PNG_file_name.c_str(), "wb");

						if (PNG_file == NULL)
							return -9;

						png_init_io(PNG_struct, PNG_file);
						png_set_IHDR(PNG_struct, PNG_info, scr_w, scr_h, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
						png_write_info(PNG_struct, PNG_info);
						png_set_bgr(PNG_struct);

						auto rows = new png_bytep[scr_h];

						for (std::size_t row = 0; row < scr_h; ++row)
							rows[row] = scr_bit_data + row * scr_bit_row_size;

						png_write_image(PNG_struct, rows);
						delete[] scr_bit_data;
						scr_bit_data = nullptr;
						delete[] rows;
						rows = nullptr;
						png_write_end(PNG_struct, NULL);
						png_destroy_write_struct(&PNG_struct, &PNG_info);
						std::fclose(PNG_file);
						PNG_file = nullptr;
					}
				}
			}
		}

		UnregisterHotKey(NULL, 2);
		UnregisterHotKey(NULL, 1);

		return 0;
	}
	catch (const std::exception &e)
	{
		delete[] rows;
		std::fclose(PNG_file);
		png_destroy_write_struct(&PNG_struct, &PNG_info);
		delete[] scr_bit_data;
		DeleteObject(scr_bit_h);
		DeleteObject(scr_mem_DC);
		ReleaseDC(NULL, scr_DC);
		UnregisterHotKey(NULL, 2);
		UnregisterHotKey(NULL, 1);

		auto file_log = std::fopen("mttsshot.log", "w");
		SYSTEMTIME sys_time;

		GetSystemTime(&sys_time);
		std::println(file_log, "{:0>4}-{:0>2}-{:0>2}T{:0>2}:{:0>2}:{:0>2}Z {}",
			sys_time.wYear, sys_time.wMonth, sys_time.wDay, sys_time.wHour, sys_time.wMinute, sys_time.wSecond,
			e.what());
		std::fclose(file_log);
	}

	return 0;
}