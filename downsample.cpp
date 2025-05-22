#include "Header.h"

int divUp(int x, int y)
{// идея была такая (x + y - 1) / y
	//если мы добавим делитель к знамнателю и поделим на делитель, то по сравнению с прошлым результатом тут добавится единица
	// то есть, если добавить числителю делитель на 1 меньше, то результату добавится число всегда меньшее единице, но достаточное, чтобы округлить вверх
	return (x - 1) / y + 1;// упрощённая форма
}

// 1. Преобразование RGB в YCbCr

void rgb_to_ycbcr(int height, int width
	, const inputArray& R, const inputArray& G, const inputArray& B
	, inputArray& Y, inputArray& cb, inputArray& cr) {

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			double r = R[y][x];
			double g = G[y][x];
			double b = B[y][x];

			Y[y][x] = static_cast<int16_t>(0.299 * r + 0.587 * g + 0.114 * b);
			cb[y][x] = static_cast<int16_t>(128 - 0.168736 * r - 0.331264 * g + 0.5 * b);
			cr[y][x] = static_cast<int16_t>(128 + 0.5 * r - 0.418688 * g - 0.081312 * b);
		}
	}
}

void ycbcr_to_rgb(int height, int width
	, inputArray& R, inputArray& G, inputArray& B
	, const inputArray& Y, const inputArray& cb, const inputArray& cr
	, const double RG[3][3]) {

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			double y_val = Y[y][x];
			double cb_val = cb[y][x] - 128;
			double cr_val = cr[y][x] - 128;

			double r = RG[0][0] * y_val + RG[0][1] * cb_val + RG[0][2] * cr_val;
			double g = RG[1][0] * y_val + RG[1][1] * cb_val + RG[1][2] * cr_val;
			double b = RG[2][0] * y_val + RG[2][1] * cb_val + RG[2][2] * cr_val;

			R[y][x] = static_cast<int16_t>(max(0.0, min(255.0, r)));
			G[y][x] = static_cast<int16_t>(max(0.0, min(255.0, g)));
			B[y][x] = static_cast<int16_t>(max(0.0, min(255.0, b)));
		}
	}
}

// 2. Даунсэмплинг 4:2:0 (применяется к Cb и Cr)
inputArray downsample(int height, int width, const inputArray& c) {
	//булево значение, определяет нечётное ли число или нет
	bool odd_h = height % 2 != 0;//0 - нет, 1 - да, нечётное
	bool odd_w = width % 2 != 0;

	inputArray downsampled(divUp(height, 2), vector<int16_t>(divUp(width, 2), 0));

	for (size_t y = 0, h = 0; h < height - odd_h; y++, h += 2)
	{
		for (size_t x = 0, w = 0; w < width - odd_h; x++, w += 2)
		{// среднее арифметическое
			int arithmetic_mean = (c[h][w] + c[h][w + 1]
				+ c[h + 1][w] + c[h + 1][w + 1]) / 4;
			downsampled[y][x] = arithmetic_mean;
		}
	}

	if (odd_w)
	{// правый край
		// индексы правых краёв
		int w = width - 1;// const
		int x = width / 2;// width нечётное
		for (size_t y = 0, h = 0; h < height - odd_h; y++, h += 2)
		{
			int arithmetic_mean
				= (c[h][w] + 0
					+ c[h + 1][w] + 0) / 2;
			downsampled[y][x] = arithmetic_mean;
		}
	}

	if (odd_h)
	{// нижний край
		// индексы нижних краёв
		int h = height - 1;// const
		int y = height / 2;// height нечётное
		for (size_t x = 0, w = 0; w < width - odd_w; x++, w += 2)
		{
			int arithmetic_mean
				= (c[h][w] + c[h][w + 1]
					+ 0 + 0) / 2;
			downsampled[y][x] = arithmetic_mean;
		}
	}

	if (odd_h + odd_w == 2)
	{// уголок
		int w = width - 1;// const
		int h = height - 1;// const

		int y = height / 2;// height нечётное (разрешение/size - 1)
		int x = width / 2;// width нечётное (разрешение/size - 1)
		downsampled[y][x] = c[h][w];
	}

	return downsampled;
}


inputArray upScale(int height, int width, const inputArray& c) {
	inputArray up(height, vector<int16_t>(width));


	//булево значение, определяет нечётное ли число или нет
	bool odd_h = height % 2 != 0;//0 - нет, 1 - да, нечётное
	bool odd_w = width % 2 != 0;

	for (size_t y = 0, h = 0; y < height / 2; y++, h += 2)
	{
		for (size_t x = 0, w = 0; x < width / 2; x++, w += 2)
		{
			int Ycbcr_pixel = c[y][x];
			up[h][w] = Ycbcr_pixel; up[h][w + 1] = Ycbcr_pixel;
			up[h + 1][w] = Ycbcr_pixel; up[h + 1][w + 1] = Ycbcr_pixel;
		}
	}

	if (odd_w)
	{// правый край
		// индексы правых краёв
		int w = width - 1;// const
		int x = width / 2;// width нечётное
		for (size_t y = 0, h = 0; y < height / 2; y++, h += 2)
		{
			int Ycbcr_pixel = c[y][x];
			up[h][w] = Ycbcr_pixel;
			up[h + 1][w] = Ycbcr_pixel;
		}
	}

	if (odd_h)
	{// нижний край
		// индексы нижних краёв
		int h = height - 1;// const
		int y = height / 2;// height нечётное
		for (size_t x = 0, w = 0; x < width / 2; x++, w += 2)
		{
			int Ycbcr_pixel = c[y][x];
			up[h][w] = Ycbcr_pixel; up[h][w + 1] = Ycbcr_pixel;
		}
	}

	if (odd_h + odd_w == 2)
	{// уголок
		int w = width - 1;// const
		int h = height - 1;// const

		int y = height / 2;// height нечётное (разрешение/size - 1)
		int x = width / 2;// width нечётное (разрешение/size - 1)
		up[h][w] = c[y][x];
	}

	return up;
}
