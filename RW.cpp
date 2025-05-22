#include "Header.h"

inputArray marge8x8Blocks(int height, int width, const vector<i16_Block8x8>& blocks/*Y/Cb/Cr*/, int lever) {
	if (lever)
	{
		height = divUp(height, 2);
		width = divUp(width, 2);
	}

	inputArray out(height, vector<int16_t>(width));
	int h_blocks = divUp(height, 8);
	int w_blocks = divUp(width, 8);
	int position = 0;

	for (size_t i = 0; i < h_blocks; i++)
	{
		for (size_t u = 0; u < w_blocks; u++)
		{
			for (size_t bi = 0; bi < 8; bi++)
			{
				if (i * 8 + bi >= height) continue;

				for (size_t bu = 0; bu < 8; bu++)
				{
					if (u * 8 + bu >= width) continue;

					out[i * 8 + bi][u * 8 + bu] = blocks[position][bi][bu];
				}
			}

			position++;
		}
	}

	return out;
}

void JPEG_decode_HA_RLE(vector<array<int16_t, 64>>& out, string str, int size_Bl8x8, const string_view(&DC)[12], const string_view(&AC)[16][11], int& tmp) {
	ofstream outp("console.txt");

	for (size_t num_block = 0; num_block < size_Bl8x8; num_block++)
	{
		// DC coeffs
		if (str.substr(tmp, 2) == "00")
		{
			tmp += 2;// сдвигаем курсор
			outp << tmp << '\n';
		}
		else
		{
			bool search = true;
			int length = 2;// длина кода

			while (search)
			{
				if (length > 11)
				{
					cout << "DC Difference length ERROR: not found. tmp = " << tmp << '\n';
				}

				string_view code(str.data() + tmp, length);

				// поиск по таблице
				for (size_t d = 1; d < 12; d++)
				{
					if (length == DC[d].length())
					{
						if (code == DC[d])
						{
							search = false;
							tmp += length;// сдвигаем курсор на кол. битов отведённых на код категории
							outp << tmp << '\n';

							// перевод числа 2->10 систему счисления
							string bits = str.substr(tmp, d);
							int minus = 1;

							if (bits[0] == '0')
							{
								for (char& c : bits)
								{// char& — ссылка, меняет исходные данные
									c ^= 1;// Инвертируем биты
								}
								minus = -1;
							}

							tmp += d;// сдвигаем курсор на кол. битов отведённых двоичное число
							outp << tmp << '\n';
							out[num_block][0] = minus * stoi(bits, nullptr, 2);// nullptr нужен просто, чтобы функция не сохраняла лишний раз pos, т.к. не нужен
							break;
						}
					}
					else if (length < DC[d].length())
					{
						break;
					}
				}
				length++;
			}
		}

		// AC coeffs
		int EOB = false;
		int count = 0;// count должен стоять на последнем записанном числе и никак иначе

		// 63 это индекс последнего коэффициента массива
		while (count != 63)// count никогда не будет больше > 63
		{
			bool search = true;
			int length = 2;// длина кода

			while (search)
			{

				string_view code(str.data() + tmp, length);

				if (length > 16)
				{
					cout << "AC length ERROR: not found. tmp = " << tmp << '\n';
				}

				if (code == AC[0][0])
				{
					EOB = true;
					tmp += AC[0][0].length();
					outp << tmp << '\n';
					break;
				}

				// поиск по таблице
				for (size_t a = 0; a < 16; a++)
				{
					for (size_t c = 0; c < 11; c++)
					{
						if (length == AC[a][c].length())
						{
							if (code == AC[a][c])
							{
								search = false;
								tmp += length;// сдвигаем курсор на кол. битов отведённых на код (кол. нулей/категория)
								outp << tmp << '\n';
								count += a;// прибавил количество нулей

								if (a == 15 && c == 0)
								{
									break;
								}

								string bits = str.substr(tmp, c);
								int minus = 1;

								if (bits[0] == '0')
								{
									for (char& c : bits)
									{// char& — ссылка, меняет исходные данные
										c ^= 1;// Инвертируем биты
									}
									minus = -1;
								}

								// перевод числа 2->10 систему счисления
								tmp += c;// сдвигаем курсор на кол. битов отведённых двоичное число
								outp << tmp << '\n';
								count++;
								out[num_block][count] = minus * stoi(bits, nullptr, 2);// nullptr нужен просто, чтобы функция не сохраняла лишний раз pos, т.к. не нужен
								break;
							}
						}
						else if (length < AC[a][c].length())
						{
							if (!(a == 0 && c == 0))
							{
								break;
							}
						}
					}

					if (!search) break;
				}

				length++;
			}

			if (EOB) break;
		}
	}

	outp.close();
}

// Запись в файл
void write_quant_table(const int(&quant_table)[8][8], ofstream& out) {
	for (size_t i = 0; i < 8; i++)
	{
		for (size_t u = 0; u < 8; u++)
		{
			char sym = quant_table[i][u];// Байт в ASCII или просто обычный символ char
			out.write(&sym, 1);// Записываем 1 байт (sizeof(char) = 1 byte)
		}
	}
}

void write_DC_coeff(const string_view(&DC)[12], string& text) {
	for (size_t i = 0; i < 12; i++)
	{
		int8_t length = DC[i].length() - 1;// записываем длину - 1
		bitset<4> bits(length);
		text += bits.to_string();
		text += DC[i];
	}
}

void write_AC_coeff(const string_view(&AC)[16][11], string& text) {
	// EOB
	int8_t length = AC[0][0].length() - 1;// записываем длину - 1
	bitset<4> bits(length);
	text += bits.to_string();
	text += AC[0][0];

	// ZRL
	length = AC[15][0].length() - 1;// записываем длину - 1
	bits = bitset<4>(length);
	text += bits.to_string();
	text += AC[15][0];

	// остальные коды
	for (size_t i = 0; i < 16; i++)
	{
		for (size_t u = 1; u < 11; u++)// пропускаем первый элемент, т.к. он пустой (u = 1)
		{
			int8_t length = AC[i][u].length() - 1;// записываем длину - 1
			bits = bitset<4>(length);
			text += bits.to_string();
			text += AC[i][u];
		}
	}
}

void writing_the_compressed_file(string link, vector<uint8_t> out_bytes, int width, int height, int quality) {
	ofstream out(link, ios::binary);

	// 1. Запись таблиц квантования
	write_quant_table(Luminance_quantization_table, out);
	write_quant_table(Chrominance_quantization_table, out);

	// 2. Разрешение
	out.write(reinterpret_cast<char*>(&width), sizeof(int));
	out.write(reinterpret_cast<char*>(&height), sizeof(int));

	// 3. Качество
	out.write(reinterpret_cast<char*>(&quality), sizeof(int));

	// 4. Запись таблиц Хаффмана
	string text;

	write_DC_coeff(Luminance_DC_differences, text);
	write_DC_coeff(Chrominance_DC_differences, text);

	write_AC_coeff(Luminance_AC, text);
	write_AC_coeff(Chrominance_AC, text);

	vector<uint8_t> coeff = pack_bits_to_bytes(text);
	text = "";

	int size_coeff = coeff.size();
	for (size_t i = 0; i < size_coeff; i++)
	{
		char sym = coeff[i];// Байт в ASCII или просто обычный символ char
		out.write(&sym, 1);// Записываем 1 байт и sizeof = 8 (const)
	}

	// 5. Запись формулы для RGB to YCbCr
	for (size_t i = 0; i < 3; i++)
	{
		for (size_t u = 0; u < 3; u++)
		{
			out.write(reinterpret_cast<const char*>(&PR[i][u]), sizeof(double));
		}
	}

	// 6. Запись сжатых данных
	size_t size_output = out_bytes.size();
	cout << size_output;

	for (size_t i = 0; i < size_output; i++)
	{
		char sym = out_bytes[i];
		out.write(&sym, 1);// Записываем 1 байт (sizeof(char) = 1 byte)
	}

	out.close();
}

// Бинарное чтение метаданных и сжатых данных
void read_quant_table(ifstream& in, int quant_table[8][8]) {
	for (size_t i = 0; i < 8; i++)
	{
		for (size_t u = 0; u < 8; u++)
		{
			char sym;
			in.read(&sym, 1);
			quant_table[i][u] = sym;
		}
	}
}

void check(int& cursor, ifstream& in, bitset<8>& bits) {
	if (cursor < 0)
	{
		cursor = 7;
		char sym;
		in.read(&sym, 1);
		bits = bitset<8>(sym);
	}
}

string code(int& cursor, ifstream& in, bitset<8>& bits) {
	int num = 0;
	bits.to_string();

	for (int i = 3; i >= 0; i--)
	{
		cursor--;
		check(cursor, in, bits);
		num += bits[cursor] << i;
	}

	int length = num + 1;// длина кода
	string str;
	for (size_t i = 0; i < length; i++)
	{
		cursor--;
		check(cursor, in, bits);
		str += bits[cursor] == 1 ? '1' : '0';
	}

	return str;
}

void read_DC_coeff(ifstream& in, int& cursor, bitset<8>& bits, string DC[12]) {
	for (int count = 0; count < 12; count++)
	{
		DC[count] = code(cursor, in, bits);
	}
}

void read_AC_coeff(ifstream& in, int& cursor, bitset<8>& bits, string AC[16][11]) {
	int f = 0;

	for (size_t t = 0; t < 2; t++)
	{//EOB ZRL
		AC[f][0] = code(cursor, in, bits);
		f = 15;
	}
	for (int count = 1; count <= 14; count++)
	{
		AC[count][0] = "";
	}

	for (int count = 0; count < 16; count++)
	{
		for (size_t cnt = 1; cnt < 11; cnt++)
		{
			AC[count][cnt] = code(cursor, in, bits);
		}
	}
}

void read_coeff_ycbcr_to_rgb(ifstream& in, double RG[3][3]) {
	for (size_t i = 0; i < 3; i++)
	{
		for (size_t u = 0; u < 3; u++)
		{
			in.read(reinterpret_cast<char*>(&RG[i][u]), 8);
		}
	}
}

string read_compressed_data(ifstream& in) {
	string str = "";

	char byte;
	while (in.read(&byte, 1))
	{
		bitset<8> bits(byte);
		for (int8_t i = 7; i > -1; i--)
		{
			str += to_string(bits[i]);
		}
	}

	return str;
}

string reading_the_compressed_file(string link
	, int& width, int& height, int& quality
	, int Lumin_QT[8][8], int Chrom_QT[8][8]
	, string_view L_DC[12], string_view C_DC[12]
	, string_view L_AC[16][11], string_view C_AC[16][11],
	double RG[3][3]) {
	ifstream in(link, ios::binary);

	read_quant_table(in, Lumin_QT);
	read_quant_table(in, Chrom_QT);

	in.read(reinterpret_cast<char*>(&width), sizeof(int));
	in.read(reinterpret_cast<char*>(&height), sizeof(int));
	in.read(reinterpret_cast<char*>(&quality), sizeof(int));

	int cursor = 0;
	bitset<8> bits;

	static string Lumin_DC[12];
	static string Chrom_DC[12];
	read_DC_coeff(in, cursor, bits, Lumin_DC);
	read_DC_coeff(in, cursor, bits, Chrom_DC);
	for (size_t i = 0; i < 12; i++) {
		L_DC[i] = Lumin_DC[i];
		C_DC[i] = Chrom_DC[i];
	}

	static string Lumin_AC[16][11];
	static string Chrom_AC[16][11];
	read_AC_coeff(in, cursor, bits, Lumin_AC);
	read_AC_coeff(in, cursor, bits, Chrom_AC);
	for (size_t i = 0; i < 16; i++) {
		for (size_t u = 0; u < 11; u++) {
			L_AC[i][u] = Lumin_AC[i][u];
			C_AC[i][u] = Chrom_AC[i][u];
		}
	}

	read_coeff_ycbcr_to_rgb(in, RG);

	string str = read_compressed_data(in);

	in.close();
	return str;
}