#include "Header.h"

// �������������� ����������� ��������� ��� DCT-II 8x8
const double PI = 3.14159265358979323846;
const double SQRT2 = 1.41421356237309504880; // sqrt(2)


// ������������ ��������������� ��� DCT-II
const double C0 = 1.0 / SQRT2;
const double C1 = 1.0;


// 3. ��������� ����������� �� ����� NxN
vector<i16_Block8x8> splitInto8x8Blocks(int height, int width, const inputArray& Y_Cb_Cr) {
	vector<i16_Block8x8> BLOCKS;// ��������� ������ ������

	// �������� �� ����������� � ����� 8�8
	//i - ������, j - �������
	for (size_t i = 0; i < height; i += 8) {
		int Nt = 8;
		if (height - i < 8) Nt = height - i;

		for (size_t j = 0; j < width; j += 8) {
			int M = 8;
			if (width - j < 8) M = width - j;

			// �������� ������ � ���� 8�8
			//bi - ������, bj - �������
			i16_Block8x8 block{};
			for (size_t bi = 0; bi < Nt; bi++) {
				for (size_t bj = 0; bj < M; bj++) {
					block[bi][bj] = Y_Cb_Cr[i + bi][j + bj];
				}
			}

			BLOCKS.push_back(block);
		}
	}

	return BLOCKS;
}

// 4.1 ������ DCT-II �������������� ��� ����� NxN
// ������� ��������� ��� ��������� ����������
d_Block8x8 precompute_cos_table() {
	d_Block8x8 cos_table;
	for (int u = 0; u < N; u++) {
		for (int x = 0; x < N; x++) {
			cos_table[u][x] = cos((2 * x + 1) * u * PI / (2 * N));
		}
	}
	return cos_table;
}

const auto COS_TABLE = precompute_cos_table();

// 1D DCT-II ��� ������/�������
void dct_1d(const array<double, N>& input, array<double, N>& output) {
	for (int u = 0; u < N; u++) {
		double sum = 0.0;
		double cu = (u == 0) ? C0 : C1;
		for (int x = 0; x < N; x++) {
			sum += input[x] * COS_TABLE[u][x];
		}
		output[u] = sum * cu * 0.5; // ������������
	}
}

// 2D DCT-II ��� ����� 8x8
d_Block8x8 dct_2d_8x8(const i16_Block8x8& block) {
	d_Block8x8 temp;
	d_Block8x8 coeffs;

	// ��������� 1D DCT � ������ ������ (�������������� ��������������)
	for (int y = 0; y < N; y++) {
		array<double, N> row;
		for (int x = 0; x < N; x++) {
			row[x] = block[y][x] - 128;
		}
		array<double, N> dct_row{};
		dct_1d(row, dct_row);
		temp[y] = dct_row;
	}

	// ��������� 1D DCT � ������� ������� (������������ ��������������)
	for (int u = 0; u < N; u++) {
		array<double, N> column{};
		for (int y = 0; y < N; y++) {
			column[y] = temp[y][u];
		}
		array<double, N> dct_column{};
		dct_1d(column, dct_column);
		for (int v = 0; v < N; v++) {
			coeffs[v][u] = dct_column[v];
		}
	}

	return coeffs;
}

// 4.2 �������� DCT-II �������������� ��� ����� NxN
// VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV
// 1D DCT-III �������� �������������� ��� ������/�������
void idct_1d(const array<double, N>& input, array<double, N>& output) {
	for (int x = 0; x < N; x++) {
		double sum = 0.0;
		for (int u = 0; u < N; ++u) {
			double cu = (u == 0) ? C0 : C1;
			sum += cu * input[u] * COS_TABLE[u][x];
		}
		output[x] = sum * 0.5; // ������������
	}
}

// 2D IDCT ��� ����� 8x8
i16_Block8x8 idct_2d_8x8(const d_Block8x8& coeffs) {
	d_Block8x8 temp;
	i16_Block8x8 block;

	// ��������� 1D IDCT � ������� ������� (������������ ��������������)
	for (int u = 0; u < N; u++) {
		array<double, N> column{};
		for (int v = 0; v < N; v++) {
			column[v] = coeffs[v][u];
		}
		array<double, N> idct_column{};
		idct_1d(column, idct_column);
		for (int y = 0; y < N; y++) {
			temp[y][u] = idct_column[y];
		}
	}

	// ��������� 1D IDCT � ������ ������ (�������������� ��������������)
	for (int y = 0; y < N; y++) {
		array<double, N> row{};
		for (int u = 0; u < N; u++) {
			row[u] = temp[y][u];
		}
		array<double, N> idct_row{};
		idct_1d(row, idct_row);
		for (int x = 0; x < N; x++) {
			block[y][x] = static_cast<int16_t>(round(idct_row[x])) + 128;
		}
	}

	return block;
}