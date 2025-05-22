#include "Header.h"

// 5. Генерация матрицы квантования для заданного уровня качества
// Annex K стандарт JPEG (ISO/IEC 10918-1) : 1993(E)
void generate_quantization_matrix(int quality, d_Block8x8& q_matrix, const int(&Quantization_table)[8][8]) {
	// Корректируем качество (1-100)
	quality = max(1, min(100, quality));

	// Вычисляем scale_factor
	double scaleFactor;
	if (quality < 50) {
		scaleFactor = 200.0 / quality;  // Для Q < 50
	}
	else {
		scaleFactor = 8 * (1.0 - 0.01 * quality);  // Для Q >= 50
	}

	// Масштабируем стандартную матрицу Cb/Cr
	for (int y = 0; y < 8; y++) {
		for (int x = 0; x < 8; x++) {
			double q = Quantization_table[y][x] * scaleFactor;
			q_matrix[y][x] = max(1.0, min(255.0, q));
		}
	}
}

// 6.1 Квантование DCT коэффициентов
void quantize(const d_Block8x8& dct_coeffs, const d_Block8x8& q_matrix, i16_Block8x8& quantized) {
	for (int y = 0; y < N; y++) {
		for (int x = 0; x < N; x++) {
			quantized[y][x] = static_cast<int16_t>(round(dct_coeffs[y][x] / q_matrix[y][x]));
		}
	}
}

// 6.2 Обратное квантование (восстановление DCT-коэффициентов)
d_Block8x8 dequantize(const i16_Block8x8& quantized, const d_Block8x8& q_matrix) {
	d_Block8x8 dct_coeffs;
	for (int y = 0; y < N; y++) {
		for (int x = 0; x < N; x++) {
			dct_coeffs[y][x] = static_cast<double>(quantized[y][x]) * q_matrix[y][x];
		}
	}

	return dct_coeffs;
}