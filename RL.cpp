#include "Header.h"

// 7.1 Зигзаг-сканирование блока
constexpr int zigzag_sequence[64] = {
	0,
	1, 8,
	16, 9, 2,
	3, 10, 17, 24,
	32, 25, 18, 11, 4,
	5, 12, 19, 26, 33, 40,
	48, 41, 34, 27, 20, 13, 6,
	7, 14, 21, 28, 35, 42, 49, 56,
	57, 50, 43, 36, 29, 22, 15,
	23, 30, 37, 44, 51, 58,
	59, 52, 45, 38, 31,
	39, 46, 53, 60,
	61, 54, 47,
	55, 62,
	63
};

vector<int16_t> zigzag_scan(const i16_Block8x8& block) {
	vector<int16_t> str(64);

	for (int i = 0; i < N * N; i++) {
		int idx = zigzag_sequence[i];
		str[i] = block[idx / N][idx % N];
	}

	return str;
}

// 7.2 Обратное зигзаг-сканирование блока
i16_Block8x8 inverse_zigzag_scan(const array<int16_t, 64>& str) {
	i16_Block8x8 block{};

	for (int i = 0; i < 64; i++) {
		int idx = zigzag_sequence[i];
		block[idx / 8][idx % 8] = str[i];
	}

	return block;
}

// после зиг-заг обхода обрабатываем полученный массив с DC и AC коэффициентами.
// 8.1 Разностное кодирование DC коэффициентов
void dc_difference(vector<int16_t>& data) {
	size_t size = data.size();
	vector<int16_t> temp(size / 64);

	for (size_t i = 0, t = 0; i < size; i += 64, t++) {
		temp[t] = data[i];
	}

	for (size_t i = 64, t = 0; i < size; i += 64, t++) {
		data[i] -= temp[t];
	}
}

// 8.2 Обратное разностное кодирование DC коэффициентов
void reverse_dc_difference(vector<array<int16_t, 64>>& data) {
	size_t size = data.size();

	for (size_t i = 1; i < size; i++) {
		data[i][0] += data[i - 1][0];
	}
}

// 9. Переменного кодирования разностей DC и AC коэффициентов.
vector<int16_t> intToBinaryVector(int16_t num, int positive1_or_negative0 = 1/*влияет на биты, будут ли биты инвертивными*/) {
	vector<int16_t> bits;

	if (positive1_or_negative0 == 0) num *= -1;

	// Разложение числа на биты
	while (num > 0) {
		bits.push_back(num % 2 == positive1_or_negative0); // Младший бит
		num /= 2;
	}

	// Чтобы биты шли в привычном порядке (старший бит первым), развернём вектор
	reverse(bits.begin(), bits.end());

	return bits;
}

// 10. RLE кодирование AC коэффициентов
bool rle_encode_ac(int16_t cur, vector<int16_t>& out_rle, int& zero_count, bool& EOB, bool& ZRL, vector<int16_t>& tempZRL) {
	if (cur == 0)
	{// попался 0
		zero_count++;
		EOB = true;
		if (zero_count == 15)
		{//ZRL or EOB. Если ZRL то мы не записываем код, т.к. там 0
			tempZRL.push_back(15);// запись НУЛЕЙ
			tempZRL.push_back(0);// запись КАТЕГОРИИ
			zero_count = 0;
			ZRL = true;
		}
		return true;
	}
	else
	{
		// tempZRL
		if (ZRL)
		{// если был ZRL, то он не пустой
			// запись ZRL
			copy(tempZRL.begin(), tempZRL.end(), back_inserter(out_rle));

			tempZRL.clear();
			ZRL = false;
		}

		out_rle.push_back(zero_count);// запись НУЛЕЙ AC
		zero_count = 0;
		EOB = false;
		return false;
	}
}

// типо продолжение разностного кодирования DC, а также мы кодируем AC
void preparing_for_coding_dc_and_ac(vector<int16_t>& data) {
	vector<int16_t> output;
	output.reserve(data.size());

	dc_difference(data);

	//вся запись будет не побитовой
	size_t size = data.size();

	for (size_t i = 0; i < size; i += 64)// блок
	{
		// запись DC
		vector<int16_t> temp;
		// {DC coeffs}
		if (data[i] == 0)
		{
			output.push_back(0);// запись КАТЕГОРИИ = 0 без записи кода
		}
		else
		{
			if (data[i] > 0)
			{//запись категорий DC и сам DC ввиде бинарного кода в вектор, но запись не на самом деле не бинарная.
				temp = intToBinaryVector(data[i], 1);
			}
			else
			{//если число отрицательно, то мы инвертируем его биты
				temp = intToBinaryVector(data[i], 0);
			}
			output.push_back(static_cast<int16_t>(temp.size()));// запись КАТЕГОРИИ
			copy(temp.begin(), temp.end(), back_inserter(output));// запись КОДА
		}




		// {AC coeffs}
		int zero_count = 0;
		bool EOB = false;
		bool ZRL = false;
		vector<int16_t> tempZRL;
		for (size_t j = 1; j < 64; j++)// оставшиеся 63 AC коэффициенты блока
		{// подсчёт нулей и его запись, запись категорий AC с самим коэффициентом и так заного пока не 0,0 (EOB).
			if (rle_encode_ac(data[j + i], output, zero_count, EOB, ZRL, tempZRL)) continue;
			// запись НУЛЕЙ AC происхлдит в функции rle_encode_ac

			// запись AC
			if (data[j + i] >= 0)
			{//запись категорий AC и сам AC ввиде бинарного кода в вектор, но запись на самом деле не бинарная.
				temp = intToBinaryVector(data[j + i], 1);
			}
			else
			{//если число отрицательно, то мы инвертируем его биты
				temp = intToBinaryVector(data[j + i], 0);
			}

			output.push_back(static_cast<int16_t>(temp.size()));// запись КАТЕГОРИИ
			copy(temp.begin(), temp.end(), back_inserter(output));// запись КОДА
		}

		// когда до конца блока все нули
		if (EOB)
		{
			output.push_back(0);
			output.push_back(0);
		}
	}

	data = output;
}