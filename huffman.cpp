#include "Header.h"

// 11. ����������� ���������  DC ������������� � �������������������  Run/Size  �� ������� ����� �������� � �������� ���������� � �������� ������.
string HA_encode(const vector<int16_t>& data, const string_view(&DC_differences)[12], const string_view(&AC)[16][11]) {
	string encoded;
	size_t size = data.size();
	for (size_t i = 0; i < size; i++)
	{
		// DC
		encoded += DC_differences[data[i]];// ��� ���������
		int k_size = data[i];// ����� ������� ������

		for (int k = 0; k < k_size; k++)
		{// ������� ����
			i++;
			encoded += to_string(data[i]);
		}

		// AC
		int count = 1;// �� ��� ���������� DC, ������� 1.
		while (count < 64)// ���� �� 64 (��������� ������ = 63)
		{
			i++;

			if (data[i] == 0 && data[i + 1] == 0)
			{// EOB
				encoded += AC[0][0];
				i++;
				break;
			}

			if (data[i] == 15 && data[i + 1] == 0)
			{// ZRL
				encoded += AC[15][0];
				count += 15;
				i++;
				continue;
			}

			count += 1 + data[i];// �������� ����� + ��� ����� � ������� �����
			encoded += AC[data[i]][data[i + 1]];// ������ ���� ������� "���-�� �����/���������"

			i++;
			int k_size = data[i];
			for (int k = 0; k < k_size; k++)
			{// ������� ���� 0/1
				i++;
				encoded += to_string(data[i]);
			}
			int up = 0;

		}
	}

	return encoded;
}

// �������� ������� ������ � �����
vector<uint8_t> pack_bits_to_bytes(const string& bit_str) {
	vector<uint8_t> output;
	size_t len = bit_str.length();

	for (size_t i = 0; i < len; i += 8) {
		string byte_str = bit_str.substr(i, 8);

		if (byte_str.length() < 8) {
			uint8_t zero_bits = 8 - byte_str.length();
			byte_str.append(zero_bits, '0');// ��������� ������
		}

		bitset<8> bits(byte_str);
		int8_t num = static_cast<uint8_t>(bits.to_ulong());
		output.push_back(num);
	}

	return output;
}