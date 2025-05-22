#include "Header.h"

void BMP_RGB(const string& filename, const inputArray& r, const inputArray& g, const inputArray& b, int width, int height) {
	ofstream file(filename, ios::binary);
	if (!file) {
		cerr << "Не удалось открыть файл для записи: " << filename << '\n';
		return;
	}

	// Размер файла (54 байта заголовок + 3 * width * height)
	const int fileSize = 54 + 3 * width * height;

	// Заголовок BMP (14 байт)
	const uint8_t bmpHeader[14] = {
		'B', 'M',                                   // Сигнатура
		static_cast<uint8_t>(fileSize),              // Размер файла (младший байт)
		static_cast<uint8_t>(fileSize >> 8),         // ...
		static_cast<uint8_t>(fileSize >> 16),        // ...
		static_cast<uint8_t>(fileSize >> 24),        // Старший байт
		0, 0, 0, 0,                                 // Зарезервировано
		54, 0, 0, 0                                 // Смещение до данных пикселей (54 байта)
	};

	// Заголовок DIB (40 байт)
	const uint8_t dibHeader[40] = {
		40, 0, 0, 0,                                // Размер DIB-заголовка
		static_cast<uint8_t>(width),                 // Ширина (младший байт)
		static_cast<uint8_t>(width >> 8),           // ...
		static_cast<uint8_t>(width >> 16),          // ...
		static_cast<uint8_t>(width >> 24),          // Старший байт
		static_cast<uint8_t>(height),                // Высота
		static_cast<uint8_t>(height >> 8),          // ...
		static_cast<uint8_t>(height >> 16),         // ...
		static_cast<uint8_t>(height >> 24),         // ...
		1, 0,                                       // Количество плоскостей (1)
		24, 0,                                      // Бит на пиксель (24 = RGB)
		0, 0, 0, 0,                                 // Сжатие (нет)
		0, 0, 0, 0,                                 // Размер изображения (можно 0)
		0, 0, 0, 0,                                 // Горизонтальное разрешение
		0, 0, 0, 0,                                 // Вертикальное разрешение
		0, 0, 0, 0,                                 // Палитра (не используется)
		0, 0, 0, 0                                  // Важные цвета (все)
	};

	// Записываем заголовки
	file.write(reinterpret_cast<const char*>(bmpHeader), 14);
	file.write(reinterpret_cast<const char*>(dibHeader), 40);

	// Выравнивание строк (BMP требует, чтобы каждая строка была кратна 4 байтам)
	const int padding = (4 - (width * 3) % 4) % 4;
	const uint8_t padBytes[3] = { 0, 0, 0 };

	// Записываем пиксели (снизу вверх, BGR-порядок)
	for (int y = height - 1; y >= 0; --y) {
		for (int x = 0; x < width; ++x) {
			// Ограничиваем значения 0-255 и конвертируем в uint8_t
			uint8_t blue = static_cast<int16_t>(max(0.0, min(255.0, b[y][x])));
			uint8_t green = static_cast<int16_t>(max(0.0, min(255.0, g[y][x])));
			uint8_t red = static_cast<int16_t>(max(0.0, min(255.0, r[y][x])));

			// Пиксель в формате BGR (не RGB!)
			file.put(blue);
			file.put(green);
			file.put(red);
		}
		// Записываем выравнивание, если нужно
		if (padding > 0) {
			file.write(reinterpret_cast<const char*>(padBytes), padding);
		}
	}

	file.close();
}

#pragma pack(push, 1)
struct BMPHeader {
	uint16_t bfType = 0x4D42;  // 'BM'
	uint32_t bfSize;
	uint16_t bfReserved1 = 0;
	uint16_t bfReserved2 = 0;
	uint32_t bfOffBits = 54;

	uint32_t biSize = 40;
	int32_t  biWidth;
	int32_t  biHeight;
	uint16_t biPlanes = 1;
	uint16_t biBitCount = 24;
	uint32_t biCompression = 0;
	uint32_t biSizeImage;
	int32_t  biXPelsPerMeter = 0;
	int32_t  biYPelsPerMeter = 0;
	uint32_t biClrUsed = 0;
	uint32_t biClrImportant = 0;
};
#pragma pack(pop)

void BMP_GR(const string& filename, const vector<uint8_t>& gray, int width, int height) {
	ofstream file(filename, ios::binary);
	if (!file) {
		cerr << "Failed to write BMP: " << filename << endl;
		return;
	}

	int row_padded = (width * 3 + 3) & (~3);
	int padding = row_padded - width * 3;

	BMPHeader header;
	header.biWidth = width;
	header.biHeight = height;
	header.biSizeImage = row_padded * height;
	header.bfSize = header.bfOffBits + header.biSizeImage;

	file.write(reinterpret_cast<const char*>(&header), sizeof(header));

	for (int y = height - 1; y >= 0; --y) {
		for (int x = 0; x < width; ++x) {
			uint8_t v = gray[y * width + x];
			uint8_t pixel[3] = { v, v, v };  // BGR
			file.write(reinterpret_cast<char*>(pixel), 3);
		}
		for (int i = 0; i < padding; ++i) file.put(0);
	}

	file.close();
}


string encode(int width, int height, int quality, string& file_in, vector<uint8_t>& output, string& name, bool rgb, bool save)
{
	ifstream ifT;

	string link;
	size_t dot_pos = file_in.rfind('.');
	name = file_in.substr(0, dot_pos);

	ifT.open(file_in, ios::binary);
	if (!ifT.is_open()) {
		cerr << "Error opening file: " << file_in << endl;
		exit(0);
	}

	ifT.seekg(0, ios::end);  // Перемещаем указатель в конец файла
	auto size = ifT.tellg();       // Получаем позицию (размер файла)
	ifT.seekg(0, ios::beg);   // Возвращаем указатель в начало
	cout << "check size: " << size << '\n';

	if ((size != 3 * height * width && size != height * width))
	{
		if (rgb)
		{
			cerr << "RGB Error the resolution does not match the size: " << size << "\nResoult count: " << 3 * height * width << "\n";
			exit(0);
		}
		else 
		{
			cerr << "Y Error the resolution does not match the size: " << size << "\nResoult count: " << height * width << "\n";
			exit(0);
		}
	}

	
	cout << "Параметры изображения:\n1) Размер: " << size << " байт\n";
	cout << "2) Количество: " << size / 3 << " pixels\n";
	cout << "3) Разрешение: " << width << "x" << height << " байт\n";


	if (rgb)
	{
		inputArray r(height, vector<int16_t>(width));
		inputArray g(height, vector<int16_t>(width));
		inputArray b(height, vector<int16_t>(width));

		uint8_t color[3];
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				if (!ifT.read(reinterpret_cast<char*>(color), 3)) {
					throw runtime_error("Ошибка чтения файла");
				}
				r[y][x] = color[0];
				g[y][x] = color[1];
				b[y][x] = color[2];
			}
		}

		// encode
		cout << "Quality: " << quality << '\n';

		inputArray Y(height, vector<int16_t>(width));
		inputArray cb(height, vector<int16_t>(width));
		inputArray cr(height, vector<int16_t>(width));

		rgb_to_ycbcr(height, width, r, g, b, Y, cb, cr);

		//4:2:0 - берём по 1 цвету в двух блоках по 4 элемента
		//цвет усреднённый среди 4 элементов
		// переписать даунсэплинг чтобы сохранялась размерность массивов
		cb = downsample(height, width, cb);
		cr = downsample(height, width, cr);

		// Разбиение на блоки и запись блоков в одномерном векторе
		// i8_Block8x8
		auto Y_blocks = splitInto8x8Blocks(height, width, Y);
		auto cb_blocks = splitInto8x8Blocks(divUp(height, 2), divUp(width, 2), cb);
		auto cr_blocks = splitInto8x8Blocks(divUp(height, 2), divUp(width, 2), cr);


		Y = marge8x8Blocks(height, width, Y_blocks, 0);
		cb = marge8x8Blocks(height, width, cb_blocks, 1);
		cr = marge8x8Blocks(height, width, cr_blocks, 1);


		Y_blocks = splitInto8x8Blocks(height, width, Y);
		cb_blocks = splitInto8x8Blocks(divUp(height, 2), divUp(width, 2), cb);
		cr_blocks = splitInto8x8Blocks(divUp(height, 2), divUp(width, 2), cr);

		size_t sizeY_Bl8x8 = Y_blocks.size();
		size_t sizeC_Bl8x8 = cb_blocks.size();// совпадает с cr_blocks.size()

		d_Block8x8 q0_matrix{};
		d_Block8x8 q1_matrix{};
		generate_quantization_matrix(quality, q0_matrix, Luminance_quantization_table);
		generate_quantization_matrix(quality, q1_matrix, Chrominance_quantization_table);

		// квантование
		for (size_t x = 0; x < sizeY_Bl8x8; x++)
		{// d_Block8x8
			auto Y_dct = dct_2d_8x8(Y_blocks[x]);// запись обработанного через dct блок
			quantize(Y_dct, q0_matrix, Y_blocks[x]);// запись в Y_blocks
		}
		for (size_t x = 0; x < sizeC_Bl8x8; x++)
		{// d_Block8x8
			auto cb_dct = dct_2d_8x8(cb_blocks[x]);
			auto cr_dct = dct_2d_8x8(cr_blocks[x]);
			quantize(cb_dct, q1_matrix, cb_blocks[x]);// запись в cb_done
			quantize(cr_dct, q1_matrix, cr_blocks[x]);// запись в cr_done
		}

		// Зиг-заг обход для каждого блока
		vector<int16_t> str1;
		vector<int16_t> str2;
		vector<int16_t> str3;
		str1.reserve(sizeY_Bl8x8 * 64);
		str2.reserve(sizeC_Bl8x8 * 64);
		str3.reserve(sizeC_Bl8x8 * 64);

		for (size_t x = 0; x < sizeY_Bl8x8; x++)
		{// Y
			auto str = zigzag_scan(Y_blocks[x]);
			copy(str.begin(), str.end(), back_inserter(str1));
		}
		for (size_t x = 0; x < sizeC_Bl8x8; x++)
		{// Cb
			auto str = zigzag_scan(cb_blocks[x]);
			copy(str.begin(), str.end(), back_inserter(str2));
		}
		for (size_t x = 0; x < sizeC_Bl8x8; x++)
		{// Cr
			auto str = zigzag_scan(cr_blocks[x]);
			copy(str.begin(), str.end(), back_inserter(str3));
		}

		preparing_for_coding_dc_and_ac(str1);
		preparing_for_coding_dc_and_ac(str2);
		preparing_for_coding_dc_and_ac(str3);

		string coder = "";
		coder += HA_encode(str1, Luminance_DC_differences, Luminance_AC);
		coder += HA_encode(str2, Chrominance_DC_differences, Chrominance_AC);
		coder += HA_encode(str3, Chrominance_DC_differences, Chrominance_AC);

		output = pack_bits_to_bytes(coder);// байтовая строка сжатых данных
		coder = "";
		// end encode


		if (save)
		{
			link = name + '/' + name + "_Orig.bmp";
			BMP_RGB(link, r, g, b, width, height);
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		link = name + '/' + name + "_E_" + to_string(quality) + ".bin";
		// Запись в файл
		writing_the_compressed_file(link, output, width, height, quality);
	}
	else
	{
		inputArray Y(height, vector<int16_t>(width));

		char color;

		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				if (!ifT.get(color)) {
					throw runtime_error("Ошибка чтения файла");
				}
				Y[y][x] = color;
			}
		}

		// encode
		cout << "Quality: " << quality << '\n';

		

		// Разбиение на блоки и запись блоков в одномерном векторе
		// i8_Block8x8
		auto Y_blocks = splitInto8x8Blocks(height, width, Y);


		Y = marge8x8Blocks(height, width, Y_blocks, 0);


		Y_blocks = splitInto8x8Blocks(height, width, Y);

		size_t sizeY_Bl8x8 = Y_blocks.size();

		d_Block8x8 q0_matrix{};

		generate_quantization_matrix(quality, q0_matrix, Luminance_quantization_table);

		// квантование
		for (size_t x = 0; x < sizeY_Bl8x8; x++)
		{// d_Block8x8
			auto Y_dct = dct_2d_8x8(Y_blocks[x]);// запись обработанного через dct блок
			quantize(Y_dct, q0_matrix, Y_blocks[x]);// запись в Y_blocks
		}

		// Зиг-заг обход для каждого блока
		vector<int16_t> str1;
		str1.reserve(sizeY_Bl8x8 * 64);

		for (size_t x = 0; x < sizeY_Bl8x8; x++)
		{// Y
			auto str = zigzag_scan(Y_blocks[x]);
			copy(str.begin(), str.end(), back_inserter(str1));
		}

		preparing_for_coding_dc_and_ac(str1);

		string coder = "";
		coder += HA_encode(str1, Luminance_DC_differences, Luminance_AC);

		output = pack_bits_to_bytes(coder);// байтовая строка сжатых данных
		coder = "";
		// end encode



		if (save)
		{
			link = name + '/' + name + "_Orig.bmp";

			vector<uint8_t> gray(width* height, 0);
			for (int y = 0; y < height; y++)
			{
				for (int x = 0; x < width; x++)
				{
					gray[y * width + x] = Y[y][x];
				}
			}

			BMP_GR(link, gray, width, height);
		}


		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		link = name + '/' + name + "_E_" + to_string(quality) + ".bin";
		// Запись в файл
		writing_the_compressed_file(link, output, width, height, quality);
	}

	return link;
}


void decode(string& link, vector<uint8_t>& output, string& name, bool rgb)
{
	int width, height, quality;
	width = height = quality = 0;
	ifstream file(link, ios::binary | ios::ate);
	streampos fileSize = file.tellg();
	cout << "\n\nРазмер сжатых данных без метаданных:\n" << fileSize << " байт\nили\n" << fileSize / 1024 << "Кб\n\n";
	file.close();

	// Считывание из файла метаданных
	int Lumin_QT[8][8];// Luminance_quantization_table
	int Chrom_QT[8][8];// Chrominance_quantization_table
	string_view Lumin_DC[12];
	string_view Chrom_DC[12];
	string_view Lumin_AC[16][11];
	string_view Chrom_AC[16][11];
	double RG[3][3];
	string str = reading_the_compressed_file(link
		, width, height, quality
		, Lumin_QT, Chrom_QT
		, Lumin_DC, Chrom_DC
		, Lumin_AC, Chrom_AC
		, RG);
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



	if (rgb)
	{
		// decode

		size_t sizeY_Bl8x8 = divUp(width, 8) * divUp(height, 8);

		vector<i16_Block8x8> vecY_Bl8x8(sizeY_Bl8x8);

		size_t sizeC_Bl8x8 = divUp(divUp(width, 2), 8) * divUp(divUp(height, 2), 8);

		vector<i16_Block8x8> vecCb_Bl8x8(sizeC_Bl8x8);
		vector<i16_Block8x8> vecCr_Bl8x8(sizeC_Bl8x8);

		// векторы блоков
		vector<array<int16_t, 64>> strY(sizeY_Bl8x8);
		vector<array<int16_t, 64>> strCb(sizeC_Bl8x8);
		vector<array<int16_t, 64>> strCr(sizeC_Bl8x8);

		// Из output создадим битовую строку
		auto size = output.size();
		str = "";
		for (uint8_t byte : output)
		{
			bitset<8> bits(byte);
			for (int8_t i = 7; i > -1; i--)
			{
				str += to_string(bits[i]);
			}
		}

		int temp = 0;
		JPEG_decode_HA_RLE(strY, str, sizeY_Bl8x8, Lumin_DC, Lumin_AC, temp);
		JPEG_decode_HA_RLE(strCb, str, sizeC_Bl8x8, Chrom_DC, Chrom_AC, temp);
		JPEG_decode_HA_RLE(strCr, str, sizeC_Bl8x8, Chrom_DC, Chrom_AC, temp);

		reverse_dc_difference(strY);
		reverse_dc_difference(strCb);
		reverse_dc_difference(strCr);

		// Обратный зиг-заг обход для всех блоков
		for (size_t i = 0; i < sizeY_Bl8x8; i++)
		{// Y
			vecY_Bl8x8[i] = inverse_zigzag_scan(strY[i]);
		}
		for (size_t i = 0; i < sizeC_Bl8x8; i++)
		{// Cb
			vecCb_Bl8x8[i] = inverse_zigzag_scan(strCb[i]);
		}
		for (size_t i = 0; i < sizeC_Bl8x8; i++)
		{// Cr
			vecCr_Bl8x8[i] = inverse_zigzag_scan(strCr[i]);
		}

		vector<i16_Block8x8> aY(sizeY_Bl8x8);
		vector<i16_Block8x8> aCb(sizeC_Bl8x8);
		vector<i16_Block8x8> aCr(sizeC_Bl8x8);

		d_Block8x8 q0_2matrix{};
		d_Block8x8 q1_2matrix{};
		generate_quantization_matrix(quality, q0_2matrix, Lumin_QT);
		generate_quantization_matrix(quality, q1_2matrix, Chrom_QT);

		for (size_t x = 0; x < sizeY_Bl8x8; x++)
		{
			d_Block8x8 doub_Y = dequantize(vecY_Bl8x8[x], q0_2matrix);
			aY[x] = idct_2d_8x8(doub_Y);
		}
		for (size_t x = 0; x < sizeC_Bl8x8; x++)
		{
			d_Block8x8 doub_Cb = dequantize(vecCb_Bl8x8[x], q1_2matrix);
			d_Block8x8 doub_Cr = dequantize(vecCr_Bl8x8[x], q1_2matrix);
			aCb[x] = idct_2d_8x8(doub_Cb);
			aCr[x] = idct_2d_8x8(doub_Cr);
		}

		inputArray Y2 = marge8x8Blocks(height, width, aY, 0);
		inputArray cb2 = marge8x8Blocks(height, width, aCb, 1);
		inputArray cr2 = marge8x8Blocks(height, width, aCr, 1);

		cb2 = upScale(height, width, cb2);
		cr2 = upScale(height, width, cr2);

		inputArray r2(height, vector<int16_t>(width));
		inputArray g2(height, vector<int16_t>(width));
		inputArray b2(height, vector<int16_t>(width));

		ycbcr_to_rgb(height, width
			, r2, g2, b2
			, Y2, cb2, cr2
			, RG);

		link = name + '/' + name + "_D_" + to_string(quality) + ".bmp";
		BMP_RGB(link, r2, g2, b2, width, height);
	}
	else
	{
		// decode

		size_t sizeY_Bl8x8 = divUp(width, 8) * divUp(height, 8);

		vector<i16_Block8x8> vecY_Bl8x8(sizeY_Bl8x8);


		// векторы блоков
		vector<array<int16_t, 64>> strY(sizeY_Bl8x8);

		// Из output создадим битовую строку
		auto size = output.size();
		str = "";
		for (uint8_t byte : output)
		{
			bitset<8> bits(byte);
			for (int8_t i = 7; i > -1; i--)
			{
				str += to_string(bits[i]);
			}
		}

		int temp = 0;
		JPEG_decode_HA_RLE(strY, str, sizeY_Bl8x8, Lumin_DC, Lumin_AC, temp);

		reverse_dc_difference(strY);

		// Обратный зиг-заг обход для всех блоков
		for (size_t i = 0; i < sizeY_Bl8x8; i++)
		{// Y
			vecY_Bl8x8[i] = inverse_zigzag_scan(strY[i]);
		}

		vector<i16_Block8x8> aY(sizeY_Bl8x8);

		d_Block8x8 q0_2matrix{};
		generate_quantization_matrix(quality, q0_2matrix, Lumin_QT);

		for (size_t x = 0; x < sizeY_Bl8x8; x++)
		{
			d_Block8x8 doub_Y = dequantize(vecY_Bl8x8[x], q0_2matrix);
			aY[x] = idct_2d_8x8(doub_Y);
		}

		inputArray Y2 = marge8x8Blocks(height, width, aY, 0);
		
		vector<uint8_t> gray(width*height, 0);
		for (int y = 0; y < height; y++)
		{
			for (int x = 0; x < width; x++)
			{
				gray[y * width + x] = Y2[y][x];
			}
		}

		link = name + '/' + name + "_D_" + to_string(quality) + ".bmp";

		BMP_GR(link, gray, width, height);
	}
	
}




int main() {
	vector<uint8_t> output;
	string name;
	bool rgb = true;
	bool save = true;

	setlocale(LC_ALL, "ru");
	SetConsoleOutputCP(CP_UTF8);

	// images
	// Lenna - 512 x 512
	// Leaf - 2048 x 2048

	string file = "Leaf_or.rgb";

	int width = 2048;
	int height = 2048;
	int quality = 100;
	

	for (int i = 0; i <= 100; i += 10)
	{
		quality = i;
		string link = encode(width, height, quality, file, output, name, rgb, save);
		decode(link, output, name, rgb);

		save = false;
	}

	

	cout << "Done!!!";
	return 0;
}