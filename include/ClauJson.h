
#ifndef CLAUJSON_H
#define CLAUJSON_H

#if _WIN32
#include <Windows.h>
#endif

#include <iostream>
#include <vector>
#include <set>
#include <stack>
#include <string>

#include <fstream>

#include <algorithm>

#include <thread>

namespace wiz {
	template <typename T>
	inline T pos_1(const T x, const int base = 10)
	{
		if (x >= 0) { return x % base; }// x - ( x / 10 ) * 10; }
		else { return (x / base) * base - x; }
		// -( x - ( (x/10) * 10 ) )
	}


	template <typename T> /// T <- char, int, long, long long...
	std::string toStr(const T x) /// chk!!
	{
		const int base = 10;
		if (base < 2 || base > 16) { return "base is not valid"; }
		T i = x;

		const int INT_SIZE = sizeof(T) << 3; ///*8
		char* temp = new char[INT_SIZE + 1 + 1]; /// 1 NULL, 1 minus
		std::string tempString;
		int k;
		bool isMinus = (i < 0);
		temp[INT_SIZE + 1] = '\0'; //

		for (k = INT_SIZE; k >= 1; k--) {
			T val = pos_1<T>(i, base); /// 0 ~ base-1
									   /// number to ['0'~'9'] or ['A'~'F']
			if (val < 10) { temp[k] = val + '0'; }
			else { temp[k] = val - 10 + 'A'; }

			i /= base;

			if (0 == i) { // 
				k--;
				break;
			}
		}

		if (isMinus) {
			temp[k] = '-';
			tempString = std::string(temp + k);//
		}
		else {
			tempString = std::string(temp + k + 1); //
		}
		delete[] temp;

		return tempString;
	}


	class LoadDataOption
	{
	public:
		const char Left = '{';	// objectt start
		const char Right = '}';	// object last
		const char Left2 = '['; // array start
		const char Right2 = ']'; // array last
		const char Assignment = ':'; //
		const char Comma = ',';
	};

	inline bool isWhitespace(const char ch)
	{
		switch (ch)
		{
		case ' ':
		case '\t':
		case '\r':
		case '\n':
		case '\v':
		case '\f':
			return true;
			break;
		}
		return false;
	}


	inline int Equal(const long long x, const long long y)
	{
		if (x == y) {
			return 0;
		}
		return -1;
	}

	class InFileReserver
	{
	private:

		class BomInfo
		{
		public:
			size_t bom_size;
			char seq[5];
		};

		const static size_t BOM_COUNT = 1;

		enum class BomType { UTF_8, ANSI };

		const static BomInfo bomInfo[1]; 

		static BomType ReadBom(std::ifstream& file) {
			char btBom[5] = { 0, };
			file.read(btBom, 5);
			size_t readSize = file.gcount();

			if (0 == readSize) {
				file.clear();
				file.seekg(0, std::ios_base::beg);
				return BomType::ANSI;
			}

			BomInfo stBom = { 0, };
			BomType type = ReadBom(btBom, readSize, stBom);

			if (type == BomType::ANSI) { // ansi
				file.clear();
				file.seekg(0, std::ios_base::beg);
				return BomType::ANSI;
			}

			file.clear();
			file.seekg(stBom.bom_size, std::ios_base::beg);
			return type;
		}

		static BomType ReadBom(const char* contents, size_t length, BomInfo& outInfo) {
			char btBom[5] = { 0, };
			size_t testLength = length < 5 ? length : 5;
			memcpy(btBom, contents, testLength);

			size_t i, j;
			for (i = 0; i < BOM_COUNT; ++i) {
				const BomInfo& bom = bomInfo[i];

				if (bom.bom_size > testLength) {
					continue;
				}

				bool matched = true;

				for (j = 0; j < bom.bom_size; ++j) {
					if (bom.seq[j] == btBom[j]) {
						continue;
					}

					matched = false;
					break;
				}

				if (!matched) {
					continue;
				}

				outInfo = bom;

				return (BomType)i;
			}

			return BomType::ANSI;
		}


	private:
		// todo - rename.
		static long long Get(long long position, long long length, char ch, const wiz::LoadDataOption& option) {
			long long x = (position << 32) + (length << 3) + 0;

			if (length != 1) {
				return x;
			}

			if (option.Left == ch || option.Left2 == ch) {
				x += 2; // 010
			}
			else if (option.Right == ch || option.Right2 == ch) {
				x += 4; // 100
			}
			else if (option.Assignment == ch) {
				x += 6;
			}

			return x;
		}

		static long long GetIdx(long long x) {
			return (x >> 32) & 0x00000000FFFFFFFF;
		}
		static long long GetLength(long long x) {
			return (x & 0x00000000FFFFFFF8) >> 3;
		}
		static long long GetType(long long x) { //to enum or enum class?
			return (x & 6) >> 1;
		}
		static bool IsToken2(long long x) {
			return (x & 1);
		}

		static void _Scanning(char* text, long long num, const long long length,
			long long*& token_arr, long long& _token_arr_size, const LoadDataOption& option) {

			long long token_arr_size = 0;

			{
				int state = 0;

				long long token_first = 0;
				long long token_last = -1;

				long long token_arr_count = 0;

				for (long long i = 0; i < length; ++i) {
					const char ch = text[i];

					switch (ch) {
					case '\"':
						token_last = i - 1;
						if (token_last - token_first + 1 > 0) {
							token_arr[num + token_arr_count] = Get(token_first + num, token_last - token_first + 1, text[token_first], option);
							token_arr_count++;
						}

						token_first = i;
						token_last = i;

						token_first = i + 1;
						token_last = i + 1;

						{//
							token_arr[num + token_arr_count] = 1;
							token_arr[num + token_arr_count] += Get(i + num, 1, ch, option);
							token_arr_count++;
						}
						break;
					case '\\':
					{//
						token_arr[num + token_arr_count] = 1;
						token_arr[num + token_arr_count] += Get(i + num, 1, ch, option);
						token_arr_count++;
					}
					break;
					case '\n':
					case '\0':
					case ' ':
					case '\t':
					case '\r':
					case '\v':
					case '\f':
						token_last = i - 1;
						if (token_last - token_first + 1 > 0) {
							token_arr[num + token_arr_count] = Get(token_first + num, token_last - token_first + 1, text[token_first], option);
							token_arr_count++;
						}
						token_first = i + 1;
						token_last = i + 1;

						break;
					case '{':
					case '[':
						token_last = i - 1;
						if (token_last - token_first + 1 > 0) {
							token_arr[num + token_arr_count] = Get(token_first + num, token_last - token_first + 1, text[token_first], option);
							token_arr_count++;
						}

						token_first = i;
						token_last = i;

						token_arr[num + token_arr_count] = Get(token_first + num, token_last - token_first + 1, text[token_first], option);
						token_arr_count++;

						token_first = i + 1;
						token_last = i + 1;
						break;
					case '}':
					case ']':
						token_last = i - 1;
						if (token_last - token_first + 1 > 0) {
							token_arr[num + token_arr_count] = Get(token_first + num, token_last - token_first + 1, text[token_first], option);
							token_arr_count++;
						}
						token_first = i;
						token_last = i;

						token_arr[num + token_arr_count] = Get(token_first + num, token_last - token_first + 1, text[token_first], option);
						token_arr_count++;

						token_first = i + 1;
						token_last = i + 1;
						break;
					case ':':
						token_last = i - 1;
						if (token_last - token_first + 1 > 0) {
							token_arr[num + token_arr_count] = Get(token_first + num, token_last - token_first + 1, text[token_first], option);
							token_arr_count++;
						}
						token_first = i;
						token_last = i;

						token_arr[num + token_arr_count] = Get(token_first + num, token_last - token_first + 1, text[token_first], option);
						token_arr_count++;

						token_first = i + 1;
						token_last = i + 1;
						break;
					case ',':
						token_last = i - 1;
						if (token_last - token_first + 1 > 0) {
							token_arr[num + token_arr_count] = Get(token_first + num, token_last - token_first + 1, text[token_first], option);
							token_arr_count++;
						}
						token_first = i;
						token_last = i;

						token_arr[num + token_arr_count] = Get(token_first + num, token_last - token_first + 1, text[token_first], option);
						token_arr_count++;

						token_first = i + 1;
						token_last = i + 1;
						break;
					}
				}

				if (length - 1 - token_first + 1 > 0) {
					token_arr[num + token_arr_count] = Get(token_first + num, length - 1 - token_first + 1, text[token_first], option);
					token_arr_count++;
				}
				token_arr_size = token_arr_count;
			}

			{
				_token_arr_size = token_arr_size;
			}
		}

		static void ScanningNew(char* text, const size_t length, const int thr_num,
			long long*& _token_arr, long long& _token_arr_size, const LoadDataOption& option)
		{
			std::vector<std::thread> thr(thr_num);
			std::vector<long long> start(thr_num);
			std::vector<long long> last(thr_num);

			{
				start[0] = 0;

				for (int i = 1; i < thr_num; ++i) {
					start[i] = length / thr_num * i;

					for (size_t x = start[i]; x <= length; ++x) {
						if (isWhitespace(text[x]) || '\0' == text[x] ||
							option.Left == text[x] || option.Right == text[x] ||
							option.Left2 == text[x] || option.Right2 == text[x] ||
							option.Assignment == text[x]) {
							start[i] = x;
							break;
						}
					}
				}
				for (int i = 0; i < thr_num - 1; ++i) {
					last[i] = start[i + 1];
					for (size_t x = last[i]; x <= length; ++x) {
						if (isWhitespace(text[x]) || '\0' == text[x] ||
							option.Left == text[x] || option.Right == text[x] ||
							option.Left2 == text[x] || option.Right2 == text[x] ||
							option.Assignment == text[x]) {
							last[i] = x;
							break;
						}
					}
				}
				last[thr_num - 1] = length + 1;
			}
			long long real_token_arr_count = 0;

			long long* tokens = new long long[length + 1];
			long long token_count = 0;

			std::vector<long long> token_arr_size(thr_num);

			for (int i = 0; i < thr_num; ++i) {
				thr[i] = std::thread(_Scanning, text + start[i], start[i], last[i] - start[i], std::ref(tokens), std::ref(token_arr_size[i]), option);
			}

			for (int i = 0; i < thr_num; ++i) {
				thr[i].join();
			}

			{
				long long _count = 0;
				for (int i = 0; i < thr_num; ++i) {
					for (long long j = 0; j < token_arr_size[i]; ++j) {
						tokens[token_count] = tokens[start[i] + j];
						token_count++;
					}
				}
			}

			bool var = false;
			bool val = false;
			bool parse_state = 0;
			bool expect_comma = false;
			bool comma_on = false;

			std::stack<char> _stack;
			_stack.push(option.Left);

			int state = 0;
			long long qouted_start;
			long long slush_start;

			for (long long i = 0; i < token_count; ++i) {
				long long len = GetLength(tokens[i]);
				char ch = text[GetIdx(tokens[i])];
				long long idx = GetIdx(tokens[i]);
				const bool isToken2 = IsToken2(tokens[i]);
				const long long type = GetType(tokens[i]);

				bool pass = false;

				if (isToken2) {
					if (0 == state && '\"' == ch) {
						state = 1;
						qouted_start = i;
					}
					else if (1 == state && '\\' == ch) {
						state = 3;
						slush_start = idx;
					}
					else if (1 == state && '\"' == ch) {
						state = 0;

						{
							long long _idx = GetIdx(tokens[qouted_start]);
							long long _len = GetLength(tokens[qouted_start]);

							_len = GetIdx(tokens[i]) - _idx + 1;

							tokens[real_token_arr_count] = Get(_idx, _len, text[_idx], option);
							real_token_arr_count++;

							pass = true;

							len = _len;
							idx = _idx;
							ch = text[_idx];
						}
					}
					else if (3 == state) {
						if (idx != slush_start + 1) {
							--i;
						}
						state = 1;
					}
				}
				else if (0 == state) {
					tokens[real_token_arr_count] = tokens[i];
					real_token_arr_count++;
					
					pass = true;
				}

				if (pass) {
					if (_stack.empty()) {
						std::cout << "Syntax Error in pre-parsing1\n";
					}

					if (expect_comma) {
						if (len == 1 && ch == ',') {
							real_token_arr_count--;
							expect_comma = false;
							comma_on = true;
							continue;
						}
						else {
							if (type == 2 || type == 3) {
								//
							}
							else {
								std::cout << "Syntax Error in pre-parsing2\n";
							}
						}
					}
					else {
						if (len == 1 && ch == ',') {
							std::cout << "Syntax Error in pre-parsing3.5\n";
						}
					}

					expect_comma = false;


					// pre-parsing? - checking syntax errors. for comma.
					if (0 == parse_state) {
						if (1 == type) { // LEFT
							_stack.push(ch);
							comma_on = false;
						}
						else if (2 == type) { // RIGHT
							if (comma_on) {
								std::cout << "Syntax Error in pre-parsing3.75\n";
							}

							if (_stack.top() == option.Left && ch == option.Right) {
								_stack.pop();
							}
							else if (_stack.top() == option.Left2 && ch == option.Right2) {
								_stack.pop();
							}
							else {
								std::cout << "Syntax Error in pre-parsing4\n";
							}

							expect_comma = true;
						}
						else if (3 == type) { // ASSIGN
							if (comma_on) {
								std::cout << "Syntax Error in pre-parsing4.5\n";
							}

							if (_stack.top() == option.Left2) { // array
								std::cout << "Syntax Error in pre-parsing5\n";
							}
							if (!var) {
								std::cout << "Syntax Error in pre-parsing6\n";
							}
							parse_state = 1;
						}
						else {
							if (1 == len && ch == ',') {
								std::cout << "Syntax Error in pre-parsing7\n";
							}

							if (_stack.top() == option.Left) {
								var = true;
								val = false;
							}
							else { // _stack.top() == option.Left2
								var = false;
								val = true;
							}

							if (var) {
								if (text[idx] == '\"' &&
									text[idx + len - 1] == '\"' && len >= 2) {
									//
								}
								else {
									std::cout << "Syntax Error in pre-parsing8\n";
								}
							}
							else if (val) {
								expect_comma = true;
								val = false;
							}

							comma_on = false;
						}
					}
					else if (1 == parse_state) {
						if (comma_on) {
							std::cout << "Syntax Error in pre-parsing8.5\n";
						}

						if (1 == type) { // LEFT
							_stack.push(ch);
							var = false;
							val = false;
						}
						else if (0 == type) {
							if (len == 1 && ch == ',') {
								std::cout << "Syntax Error in pre-parsing9\n";
							}
							else {
								var = false;
								val = true;

								if (val) {
									expect_comma = true;
									val = false;
								}
							}
						}
						else {
							// error
							std::cout << "Syntax Error in pre-parsing10\n";
						}

						parse_state = 0;
					}
				}
			}
			
			if (comma_on) {
				std::cout << "Syntax Error in pre-parsing11\n";
			}

			{
				if (1 != _stack.size()) {
					std::cout << "_stack.size() != 1\n";
				}

				if (0 != state) {
					std::cout << "[ERROR] state [" << state << "] is not zero \n";
				}
			}


			{
				_token_arr = tokens;
				_token_arr_size = real_token_arr_count;
			}
		}


		static void Scanning(char* text, const size_t length,
			long long*& _token_arr, long long& _token_arr_size, const LoadDataOption& option) {

			long long* token_arr = new long long[length + 1];
			long long token_arr_size = 0;

			{
				int state = 0;

				long long token_first = 0;
				long long token_last = -1;

				long long token_arr_count = 0;

				for (size_t i = 0; i <= length; ++i) {
					const char ch = text[i];

					if (0 == state) {
						if ('\"' == ch) {
							state = 1;
						}
						else if (isWhitespace(ch) || '\0' == ch) {
							token_last = i - 1;
							if (token_last - token_first + 1 > 0) {
								token_arr[token_arr_count] = Get(token_first, token_last - token_first + 1, text[token_first], option);
								token_arr_count++;
							}
							token_first = i + 1;
							token_last = i + 1;
						}
						else if (option.Left == ch || option.Left2 == ch) {
							token_last = i - 1;
							if (token_last - token_first + 1 > 0) {
								token_arr[token_arr_count] = Get(token_first, token_last - token_first + 1, text[token_first], option);
								token_arr_count++;
							}

							token_first = i;
							token_last = i;

							token_arr[token_arr_count] = Get(token_first, token_last - token_first + 1, text[token_first], option);
							token_arr_count++;

							token_first = i + 1;
							token_last = i + 1;
						}
						else if (option.Right == ch || option.Right2 == ch) {
							token_last = i - 1;
							if (token_last - token_first + 1 > 0) {
								token_arr[token_arr_count] = Get(token_first, token_last - token_first + 1, text[token_first], option);
								token_arr_count++;
							}
							token_first = i;
							token_last = i;

							token_arr[token_arr_count] = Get(token_first, token_last - token_first + 1, text[token_first], option);
							token_arr_count++;

							token_first = i + 1;
							token_last = i + 1;

						}
						else if (option.Assignment == ch) {
							token_last = i - 1;
							if (token_last - token_first + 1 > 0) {
								token_arr[token_arr_count] = Get(token_first, token_last - token_first + 1, text[token_first], option);
								token_arr_count++;
							}
							token_first = i;
							token_last = i;

							token_arr[token_arr_count] = Get(token_first, token_last - token_first + 1, text[token_first], option);
							token_arr_count++;

							token_first = i + 1;
							token_last = i + 1;
						}
						else if (option.Comma == ch) {
							token_last = i - 1;
							if (token_last - token_first + 1 > 0) {
								token_arr[token_arr_count] = Get(token_first, token_last - token_first + 1, text[token_first], option);
								token_arr_count++;
							}
							token_first = i;
							token_last = i;

							token_arr[token_arr_count] = Get(token_first, token_last - token_first + 1, text[token_first], option);
							token_arr_count++;

							token_first = i + 1;
							token_last = i + 1;
						}
					}
					else if (1 == state) {
						if ('\\' == ch) {
							state = 2;
						}
						else if ('\"' == ch) {
							state = 0;
						}
					}
					else if (2 == state) {
						state = 1;
					}
					else if (3 == state) {
						if ('\n' == ch || '\0' == ch) {
							state = 0;

							token_first = i + 1;
							token_last = i + 1;
						}
					}

				}
				
				std::stack<char> _stack; 
				_stack.push(option.Left);
				bool expect_comma = false;
				bool comma_on = false;
				int parse_state = 0;
				bool var = false;
				bool val = false;
				long long real_token_arr_count = 0;

				for(long long i=0; i < token_arr_count; ++i) {
					if (_stack.empty()) {
						std::cout << "Syntax Error in pre-parsing1\n";
					}

					const long long type = GetType(token_arr[i]);
					const char ch = text[GetIdx(token_arr[i])];
					const long long len = GetLength(token_arr[i]);
					const long long idx = GetIdx(token_arr[i]);


					if (expect_comma) {
						if (len == 1 && ch == ',') {
							real_token_arr_count--;
							expect_comma = false;
							comma_on = true;
							continue;
						}
						else {
							if (type == 2 || type == 3) {
								//
							}
							else {
								std::cout << "Syntax Error in pre-parsing2\n";
							}
						}
					}
					else {
						if (len == 1 && ch == ',') {
							std::cout << "Syntax Error in pre-parsing3.5\n";
						}
					}

					expect_comma = false;


					// pre-parsing? - checking syntax errors. for comma.
					if (0 == parse_state) {
						if (1 == type) { // LEFT
							_stack.push(ch);
							comma_on = false;
						}
						else if (2 == type) { // RIGHT
							if (comma_on) {
								std::cout << "Syntax Error in pre-parsing3.75\n";
							}

							if (_stack.top() == option.Left && ch == option.Right) {
								_stack.pop();
							}
							else if (_stack.top() == option.Left2 && ch == option.Right2) {
								_stack.pop();
							}
							else {
								std::cout << "Syntax Error in pre-parsing4\n";
							}

							expect_comma = true;
						}
						else if (3 == type) { // ASSIGN
							if (comma_on) {
								std::cout << "Syntax Error in pre-parsing4.5\n";
							}

							if (_stack.top() == option.Left2) { // array
								std::cout << "Syntax Error in pre-parsing5\n";
							}
							if (!var) {
								std::cout << "Syntax Error in pre-parsing6\n";
							}
							parse_state = 1;
						}
						else {
							if (1 == len && ch == ',') {
								std::cout << "Syntax Error in pre-parsing7\n";
							}

							if (_stack.top() == option.Left) {
								var = true;
								val = false;
							}
							else {
								var = false;
								val = true;
							}

							if (var) {
								if (text[idx] == '\"' &&
									text[idx + len - 1] == '\"' && len >= 2) {
									//
								}
								else {
									std::cout << "Syntax Error in pre-parsing8\n";
								}
							}
							else if (val) {
								expect_comma = true;
								val = false;
							}

							comma_on = false;
						}
					}
					else if (1 == parse_state) {
						if (comma_on) {
							std::cout << "Syntax Error in pre-parsing8.5\n";
						}

						if (1 == type) { // LEFT
							_stack.push(ch);
							var = false;
							val = false;
						}
						else if (0 == type) {
							if (len == 1 && ch == ',') {
								std::cout << "Syntax Error in pre-parsing9\n";
							}
							else {
								var = false;
								val = true;

								if (val) {
									expect_comma = true;
									val = false;
								}
							}
						}
						else {
							// error
							std::cout << "Syntax Error in pre-parsing10\n";
						}

						parse_state = 0;
					}
				}

				if (comma_on) {
					std::cout << "Syntax Error in pre-parsing11\n";
				}

				token_arr_size = real_token_arr_count;

				if (0 != state) {
					std::cout << "[" << state << "] state is not zero.\n";
				}
			}

			{
				_token_arr = token_arr;
				_token_arr_size = token_arr_size;
			}
		}

	

		static std::pair<bool, int> Scan(std::ifstream& inFile, const int num, const wiz::LoadDataOption& option, int thr_num,
			char*& _buffer, long long* _buffer_len, long long*& _token_arr, long long* _token_arr_len)
		{
			if (inFile.eof()) {
				return { false, 0 };
			}

			long long* arr_count = nullptr; //
			long long arr_count_size = 0;

			std::string temp;
			char* buffer = nullptr;
			long long file_length;

			{
				inFile.seekg(0, inFile.end);
				long long length = inFile.tellg();
				inFile.seekg(0, inFile.beg);

				BomType x = ReadBom(inFile);
				//	wiz::Out << "length " << length << "\n";
				if (x == BomType::UTF_8) {
					length = length - 3;
#if _WIN32
					SetConsoleOutputCP(65001); // UTF-8 Codepage 
#endif
				}

				file_length = length;
				buffer = new char[file_length + 1]; // 

				// read data as a block:
				inFile.read(buffer, file_length);

				buffer[file_length] = '\0';

				{
					//int a = clock();
					long long* token_arr;
					long long token_arr_size;

					if (thr_num == 1) {
						Scanning(buffer, file_length, token_arr, token_arr_size, option);
					}
					else {
						ScanningNew(buffer, file_length, thr_num, token_arr, token_arr_size, option);
					}

					//int b = clock();
				//	std::cout << b - a << "ms\n";
					_buffer = buffer;
					_token_arr = token_arr;
					*_token_arr_len = token_arr_size;
					*_buffer_len = file_length;
				}
			}

			return{ true, 1 };
		}

	private:
		std::ifstream* pInFile;
	public:
		int Num;
	public:
		explicit InFileReserver(std::ifstream& inFile)
		{
			pInFile = &inFile;
			Num = 1;
		}
		bool end()const { return pInFile->eof(); } //
	public:
		bool operator() (const wiz::LoadDataOption& option, int thr_num, char*& buffer, long long* buffer_len, long long*& token_arr, long long* token_arr_len)
		{
			bool x = Scan(*pInFile, Num, option, thr_num, buffer, buffer_len, token_arr, token_arr_len).second > 0;

			//	std::cout << *token_arr_len << "\n";
			return x;
		}
	};



	class Type {
	private:
		std::string name;

	public:
		explicit Type(const std::string& name = "", const bool valid = true) : name(name) { }//chk();  }
		explicit Type(std::string&& name, const bool valid = true) : name(move(name)) { }//chk(); }
		Type(const Type& type)
			: name(type.name)
		{
			//chk();
		}
		virtual ~Type() { }
		bool IsFail() const { // change body
			return "" == name;
		}
		std::string& GetName() {
			return name;
		}
		const std::string& GetName() const {
			return name;
		}
		void SetName(const std::string& name)
		{
			this->name = name;

			//chk();
		}
		void SetName(std::string&& name)
		{
			this->name = name;

			//chk();
		}
		bool operator==(const Type& t) const {
			return name == t.name;
		}
		bool operator<(const Type& t) const {
			return name < t.name;
		}
		Type& operator=(const Type& type)
		{
			name = type.name;
			return *this;
		}
		void operator=(Type&& type) noexcept
		{
			name = std::move(type.name);
		}
	};

	template < class T >
	class ItemType : public Type {
	public:
		typedef T item_type; //
	private:
		//std::vector<T> arr;
		T data;
		bool inited;
	public:
		ItemType(const ItemType<T>& ta) : Type(ta), data(ta.data), inited(ta.inited)
		{

		}
		ItemType(ItemType<T>&& ta) noexcept : Type(std::move(ta))
		{
			data = std::move(ta.data);
			inited = ta.inited;
		}
	public:
		explicit ItemType()
			: Type("", true), inited(false) { }
		explicit ItemType(const std::string& name, const T& value, const bool valid = true)
			:Type(name, valid), data(value), inited(true)
		{

		}
		explicit ItemType(std::string&& name, T&& value, const bool valid = true)
			:Type(move(name), valid), data(move(value)), inited(true)
		{

		}
		virtual ~ItemType() { }
	public:
		void Remove(const size_t  idx = 0)
		{
			data = T();
			inited = false;
		}
		bool Push(const T& val) { /// do not change..!!
			if (inited) { throw "ItemType already inited"; }
			data = val;
			inited = true;

			return true;
		}
		bool Push(T&& val) {
			if (inited) { throw "ItemType already inited"; }
			data = std::move(val);
			inited = true;

			return true;
		}
		T& Get(const size_t index = 0) {
			if (!inited) {
				throw "ItemType, not inited";
			}
			return data;
		}
		const T& Get(const size_t index = 0) const {
			if (!inited) {
				throw "ItemType, not inited";
			}
			return data;
		}
		void Set(const size_t index, const T& val) {
			if (!inited) {
				throw "ItemType, not inited";
			} // removal?
			data = val;
		}
		void Set(const size_t index, T&& val) {
			if (!inited) {
				throw "ItemType, not inited";
			} // removal?
			data = std::move(val);
		}
		size_t size()const {
			return inited ? 1 : 0;
		}
		bool empty()const { return !inited; }
		std::string ToString()const
		{
			if (Type::GetName().empty()) {
				return Get(0);
			}
			return Type::GetName() + " = " + Get(0);
		}
	public:
		ItemType<T>& operator=(const ItemType<T>& ta)
		{
			Type::operator=(ta);
			ItemType<T> temp = ta;

			data = std::move(temp.data);
			inited = temp.inited;
			return *this;
		}
		void operator=(ItemType<T>&& ta) noexcept
		{
			Type::operator=(ta);
			if (data == ta.data) { return; }

			data = std::move(ta.data);
			inited = ta.inited;
			return;
		}

		Type* ToType() {
			return this;
		}
		const Type* ToType()const {
			return this;
		}

	};

	class UserType : public Type {
	private:
		class UserTypeCompare
		{
		public:
			bool operator() (const UserType* x, const UserType* y) const {
				return x->GetName() < y->GetName();
			}
		};
		class ItemTypeStringPtrCompare {
		public:
			bool operator() (const ItemType<std::string>* x, const ItemType<std::string>* y) const {
				return x->GetName() < y->GetName();
			}
		};

		int Dif(const std::string& x, const std::string& y) const {
			return x.compare(y); // strcmp
		}
		size_t binary_find_ut(const std::vector<UserType*>& arr, const UserType& x, bool& err) const
		{
			err = false;

			if (arr.empty()) { err = true;  return -1; }

			size_t left = 0, right = arr.size() - 1;
			size_t middle = (left + right) / 2;

			while (left <= right) {
				const int dif = Dif(arr[middle]->GetName(), x.GetName());

				if (dif == 0) { //arr[middle]->GetName() == x.GetName()) {
					return middle;
				}
				else if (dif < 0) { //arr[middle]->GetName() < x.GetName()) {
					left = middle + 1;
				}
				else {
					if (0 == middle) {
						break;
					}
					right = middle - 1;
				}

				middle = (left + right) / 2;
			}
			err = true;
			return -1;
		}

		size_t binary_find_it(const std::vector<ItemType<std::string>*>& arr, const ItemType<std::string>& x, bool& err) const {
			err = false;
			if (arr.empty()) { err = true;  return -1; }

			size_t left = 0, right = arr.size() - 1;
			size_t middle = (left + right) / 2;

			while (left <= right) {
				const int dif = Dif(arr[middle]->GetName(), x.GetName());

				if (dif == 0) { //arr[middle]->GetName() == x.GetName()) {
					return middle;
				}
				else if (dif < 0) { //arr[middle]->GetName() < x.GetName()) {
					left = middle + 1;
				}
				else {
					if (0 == middle) {
						break;
					}
					right = middle - 1;
				}

				middle = (left + right) / 2;
			}

			err = true;
			return -1;
		}

	public:
		size_t GetIListSize()const { return ilist.size(); }
		size_t GetItemListSize()const { return itemList.size(); }
		size_t GetUserTypeListSize()const { return userTypeList.size(); }
		ItemType<std::string>& GetItemList(const size_t idx) { return itemList[idx]; }
		const ItemType<std::string>& GetItemList(const size_t idx) const { return itemList[idx]; }
		UserType*& GetUserTypeList(const size_t idx) { return userTypeList[idx]; }
		const UserType*& GetUserTypeList(const size_t idx) const { return const_cast<const UserType*&>(userTypeList[idx]); }

		Type* ToType() {
			return this;
		}
		const Type* ToType()const {
			return this;
		}

		bool IsItemList(const size_t idx) const
		{
			return ilist[idx] == 1;
		}
		bool IsUserTypeList(const size_t idx) const
		{
			return ilist[idx] == 2;
		}
		/*
		void AddItemList(const ItemType<std::string>& strTa)
		{
			for (int i = 0; i < strTa.size(); ++i) {
				this->AddItem(strTa.GetName(), strTa.Get(i));
			}
		}
		void AddItemList(ItemType<std::string>&& strTa)
		{
			for (int i = 0; i < strTa.size(); ++i) {
				this->AddItem(std::move(strTa.GetName()), std::move(strTa.Get(i)));
			}
		}
		*/
	public:
		void Clear() {
			itemList.clear();
			ilist.clear();
			userTypeList.clear();

			sortedItemList.clear();
			sortedUserTypeList.clear();
		}

		void SetParent(UserType* other)
		{
			parent = other;
		}
		UserType* GetParent() { return parent; }
		const UserType* GetParent()const { return parent; }

		void LinkUserType(UserType* ut) // friend?
		{
			userTypeList.push_back(ut);
			ilist.push_back(2);
			ut->parent = this;

			useSortedUserTypeList = false;
		}
	private:
		UserType* parent = nullptr;
		std::vector<int> ilist;
		std::vector< ItemType<std::string> > itemList;
		std::vector< UserType* > userTypeList;
		mutable std::vector< ItemType<std::string>* > sortedItemList;
		mutable std::vector< UserType* > sortedUserTypeList;
		mutable bool useSortedItemList = false;
		mutable bool useSortedUserTypeList = false;
		bool isVirtual = false;
	public:	
		bool IsObject() const {
			return isObject;
		}
		bool IsArray() const {
			return !isObject;
		}
		bool isObject = true;

		void SetIsObject(bool val) {
			isObject = val;
		}
		void SetIsVirtual(bool val) {
			isVirtual = val;
		}
		bool IsVirtual() const {
			return isVirtual;
		}
	public:
		explicit UserType(const char* str, size_t len) : Type(std::string(str, len)), parent(nullptr) { }
		explicit UserType(std::string&& name) : Type(std::move(name)), parent(nullptr) { }
		explicit UserType(const std::string& name = "") : Type(name), parent(nullptr) { }
		UserType(const UserType& ut) : Type(ut.GetName()) {
			Reset(ut);  // Initial
		}
		UserType(UserType&& ut) noexcept : Type(std::move(ut.GetName())) {
			Reset2(std::move(ut));
		}
		virtual ~UserType() {
			_Remove();
		}
		UserType& operator=(const UserType& ut) {
			if (this == &ut) { return *this; }
			Type::operator=(ut);

			RemoveUserTypeList();
			Reset(ut);
			return *this;
		}
		void operator=(UserType&& ut) noexcept {
			if (this == &ut) { return; }

			Type::operator=(ut);
			RemoveUserTypeList();
			Reset2(std::move(ut));
			return;
		}
	private:
		void Reset(const UserType& ut) {
			ilist = ut.ilist;
			itemList = ut.itemList;

			//sortedItemList = ut.sortedItemList;
			//sortedUserTypeList = ut.sortedUserTypeList;

			useSortedItemList = false; // ut.useSortedItemList;
			useSortedUserTypeList = false; //ut.useSortedUserTypeList;

			isObject = ut.isObject;
			isVirtual = ut.isVirtual;

			userTypeList.reserve(ut.userTypeList.size());

			for (size_t i = 0; i < ut.userTypeList.size(); ++i) {
				userTypeList.push_back(new UserType(*ut.userTypeList[i]));
				userTypeList.back()->parent = this;
			}



			sortedItemList.clear();
			sortedUserTypeList.clear();
		}
		void Reset2(UserType&& ut) {
			ilist = std::move(ut.ilist);
			itemList = std::move(ut.itemList);

			//sortedItemList = std::move(ut.sortedItemList);
			//sortedUserTypeList = std::move(ut.sortedUserTypeList);

			useSortedItemList = false; // ut.useSortedItemList;
			useSortedUserTypeList = false; // ut.useSortedUserTypeList;

			userTypeList.reserve(ut.userTypeList.size());

			for (size_t i = 0; i < ut.userTypeList.size(); ++i) {
				userTypeList.push_back(std::move(ut.userTypeList[i]));
				ut.userTypeList[i] = nullptr;
				userTypeList.back()->parent = this;
			}
			ut.userTypeList.clear();

			std::swap(isObject, ut.isObject);
			std::swap(isVirtual, ut.isVirtual);

			sortedItemList.clear();
			sortedUserTypeList.clear();

			ut.sortedItemList.clear();
			ut.sortedUserTypeList.clear();
		}

		void _Remove()
		{
			//parent = nullptr;
			ilist = std::vector<int>();
			itemList = std::vector< ItemType<std::string> >();

			sortedItemList.clear();
			sortedUserTypeList.clear();

			useSortedItemList = false;
			useSortedUserTypeList = false;
			RemoveUserTypeList();
		}
	public:
		size_t  GetIlistIndex(const size_t  index, const int type, bool& err)
		{
			return _GetIlistIndex(ilist, index, type, err);
		}
		size_t GetUserTypeIndexFromIlistIndex(const size_t ilist_idx, bool& err)
		{
			return _GetUserTypeIndexFromIlistIndex(ilist, ilist_idx, err);
		}
		size_t GetItemIndexFromIlistIndex(const size_t  ilist_idx, bool& err)
		{
			return _GetItemIndexFromIlistIndex(ilist, ilist_idx, err);
		}
	private:
		/// val : 1 or 2
		size_t  _GetIndex(const std::vector<int>& ilist, const int val, bool& err, const size_t  start = 0) {
			err = false;
			for (size_t i = start; i < ilist.size(); ++i) {
				if (ilist[i] == val) { return i; }
			}
			err = true;
			return -1;
		}
		// test? - need more thinking!
		size_t  _GetItemIndexFromIlistIndex(const std::vector<int>& ilist, const size_t  ilist_idx, bool& _err) {
			if (ilist.size() == ilist_idx) { return ilist.size(); }
			bool err = false;
			_err = false;
			size_t idx = _GetIndex(ilist, 1, err, 0);
			size_t item_idx = -1;

			while (!err) {
				item_idx++;
				if (ilist_idx == idx) { return item_idx; }
				idx = _GetIndex(ilist, 1, err, idx + 1);
			}
			_err = true;
			return -1;
		}
		size_t  _GetUserTypeIndexFromIlistIndex(const std::vector<int>& ilist, const size_t  ilist_idx, bool& _err) {
			_err = false;
			bool err = false;
			if (ilist.size() == ilist_idx) { return ilist.size(); }
			size_t  idx = _GetIndex(ilist, 2, err, 0);
			size_t  usertype_idx = -1;

			while (!err) {
				usertype_idx++;
				if (ilist_idx == idx) { return usertype_idx; }
				idx = _GetIndex(ilist, 2, err, idx + 1);
			}
			_err = true;
			return -1;
		}
		/// type : 1 or 2
		size_t _GetIlistIndex(const std::vector<int>& ilist, const size_t  index, const int type, bool& err) {
			size_t  count = -1;
			err = false;
			for (size_t i = 0; i < ilist.size(); ++i) {
				if (ilist[i] == type) {
					count++;
					if (index == count) {
						return i;
					}
				}
			}
			err = true;
			return -1;
		}
	public:
		void RemoveItemList(const size_t  idx)
		{
			// left shift start idx, to end, at itemList. and resize!
			for (size_t i = idx + 1; i < GetItemListSize(); ++i) {
				itemList[i - 1] = std::move(itemList[i]);
			}
			itemList.resize(itemList.size() - 1);
			//  ilist left shift and resize - count itemType!
			size_t count = 0;
			for (size_t i = 0; i < ilist.size(); ++i) {
				if (ilist[i] == 1) { count++; }
				if (count == idx + 1) {
					// left shift!and resize!
					for (size_t k = i + 1; k < ilist.size(); ++k) {
						ilist[k - 1] = std::move(ilist[k]);
					}
					ilist.resize(ilist.size() - 1);
					break;
				}
			}

			useSortedItemList = false;
		}
		void RemoveUserTypeList(const size_t idx, const bool chk = true)
		{
			if (chk && userTypeList[idx]) {
				delete userTypeList[idx];
			}

			// left shift start idx, to end, at itemList. and resize!
			for (size_t i = idx + 1; i < GetUserTypeListSize(); ++i) {
				userTypeList[i - 1] = std::move(userTypeList[i]);
			}
			userTypeList.resize(userTypeList.size() - 1);
			//  ilist left shift and resize - count itemType!
			size_t  count = 0;
			for (size_t i = 0; i < ilist.size(); ++i) {
				if (ilist[i] == 2) { count++; }
				if (count == idx + 1) {
					// left shift! and resize!
					for (size_t k = i + 1; k < ilist.size(); ++k) {
						ilist[k - 1] = std::move(ilist[k]);
					}
					ilist.resize(ilist.size() - 1);
					break;
				}
			}

			useSortedUserTypeList = false;
		}
		void RemoveItemList(const std::string& varName)
		{
			bool err = false;
			size_t k = _GetIndex(ilist, 1, err, 0);

			std::vector<ItemType<std::string>> tempDic;
			for (size_t i = 0; i < itemList.size(); ++i) {
				if (varName != itemList[i].GetName()) {
					tempDic.push_back(itemList[i]);
					k = _GetIndex(ilist, 1, err, k + 1);
				}
				else {
					// remove item, ilist left shift 1.
					for (size_t j = k + 1; j < ilist.size(); ++j) {
						ilist[j - 1] = ilist[j];
					}
					ilist.resize(ilist.size() - 1);
					k = _GetIndex(ilist, 1, err, k);
				}
			}
			itemList = std::move(tempDic);

			useSortedItemList = false;
		}
		void RemoveItemList() /// ALL
		{
			itemList = std::vector<ItemType<std::string>>();
			//
			std::vector<int> temp;
			for (size_t i = 0; i < ilist.size(); ++i) {
				if (ilist[i] == 2)
				{
					temp.push_back(2);
				}
			}
			ilist = move(temp);

			useSortedItemList = false;
		}
		void RemoveEmptyItem()
		{
			bool err = false;
			size_t k = _GetIndex(ilist, 1, err, 0);
			std::vector<ItemType<std::string>> tempDic;
			for (size_t i = 0; i < itemList.size(); ++i) {
				if (itemList[i].size() > 0) {
					tempDic.push_back(itemList[i]);
					k = _GetIndex(ilist, 1, err, k + 1);
				}
				else {
					// remove item, ilist left shift 1.
					for (size_t j = k + 1; j < ilist.size(); ++j) {
						ilist[j - 1] = ilist[j];
					}
					ilist.resize(ilist.size() - 1);
					k = _GetIndex(ilist, 1, err, k);
				}
			}
			itemList = move(tempDic);

			useSortedItemList = false;
		}
		void Remove()
		{
			ilist = std::vector<int>();
			itemList = std::vector< ItemType<std::string> >();

			RemoveUserTypeList();

			sortedItemList.clear();
			sortedUserTypeList.clear();

			useSortedItemList = false;
			useSortedUserTypeList = false;
		}
		void RemoveUserTypeList() {
			for (size_t i = 0; i < userTypeList.size(); i++) {
				if (nullptr != userTypeList[i]) {
					delete userTypeList[i]; //
					userTypeList[i] = nullptr;
				}
			}

			userTypeList.clear();

			std::vector<int> temp;
			for (size_t i = 0; i < ilist.size(); ++i) {
				if (ilist[i] == 1)
				{
					temp.push_back(1);
				}
			}
			ilist = move(temp);

			useSortedUserTypeList = false;
		}
		void RemoveUserTypeList(const std::string& varName, const bool chk = true)
		{
			bool err = false;
			size_t k = _GetIndex(ilist, 2, err, 0);
			std::vector<UserType*> tempDic;
			for (size_t i = 0; i < userTypeList.size(); ++i) {
				if (varName != userTypeList[i]->GetName()) {
					tempDic.push_back(userTypeList[i]);
					k = _GetIndex(ilist, 2, err, k + 1);
				}
				else {
					if (chk && userTypeList[i]) {
						delete userTypeList[i];
					}
					// remove usertypeitem, ilist left shift 1.
					for (size_t j = k + 1; j < ilist.size(); ++j) {
						ilist[j - 1] = ilist[j];
					}
					ilist.resize(ilist.size() - 1);
					k = _GetIndex(ilist, 2, err, k);
				}
			}
			userTypeList = move(tempDic);

			useSortedUserTypeList = false;
		}
		//			
		void RemoveList(const size_t idx) // ilist_idx!
		{
			// chk whether item or usertype.
			// find item_idx or usertype_idx.
			// remove item or remove usertype.
			if (ilist[idx] == 1) {
				size_t  item_idx = 0; // -1

				for (size_t i = 0; i < ilist.size() && i <= idx; ++i) {
					if (ilist[i] == 1) { item_idx++; }
				}

				RemoveItemList(item_idx - 1); // item_idx
			}
			else {
				size_t  usertype_idx = 0;

				for (size_t i = 0; i < ilist.size() && i <= idx; ++i) {
					if (ilist[i] == 2) { usertype_idx++; }
				}

				RemoveUserTypeList(usertype_idx - 1);
			}
		}
	public:
		bool empty()const { return ilist.empty(); }

		void InsertItemByIlist(const size_t ilist_idx, const std::string& name, const std::string& item) {
			if (empty() && name.empty()) {
				isObject = false;
			}
			else if (empty() && !name.empty()) {
				isObject = true;
			}

			if (isObject && name.empty()) {
			//	std::cout << "it is not object\n"; return; 
			}
			else if (!isObject && !name.empty()) {
			//	std::cout << "it is not array\n"; return;
			}

			ilist.push_back(1);
			for (size_t i = ilist.size() - 1; i > ilist_idx; --i) {
				ilist[i] = ilist[i - 1];
			}
			ilist[ilist_idx] = 1;

			bool err = false;
			size_t  itemIndex = _GetItemIndexFromIlistIndex(ilist, ilist_idx, err);

			itemList.emplace_back("", std::string(""));

			if (!err) {
				for (size_t i = itemList.size() - 1; i > itemIndex; --i) {
					itemList[i] = std::move(itemList[i - 1]);
				}
				itemList[itemIndex] = ItemType<std::string>(name, item);
			}
			else {
				itemList[0] = ItemType<std::string>(name, item);
			}

			useSortedItemList = false;
		}
		void InsertItemByIlist(const size_t  ilist_idx, std::string&& name, std::string&& item) {
			if (empty() && name.empty()) {
				isObject = false;
			}
			else if (empty() && !name.empty()) {
				isObject = true;
			}

			if (isObject && name.empty()) {
			//	std::cout << "it is not object\n"; return; 
			}
			else if (!isObject && !name.empty()) {
			//	std::cout << "it is not array\n"; return;
			}


			ilist.push_back(1);

			bool err = false;

			for (size_t i = ilist.size() - 1; i > ilist_idx; --i) {
				ilist[i] = ilist[i - 1];
			}
			ilist[ilist_idx] = 1;

			size_t  itemIndex = _GetItemIndexFromIlistIndex(ilist, ilist_idx, err);

			itemList.emplace_back("", std::string(""));
			if (!err) {
				for (size_t i = itemList.size() - 1; i > itemIndex; --i) {
					itemList[i] = std::move(itemList[i - 1]);
				}
				itemList[itemIndex] = ItemType<std::string>(std::move(name), std::move(item));
			}
			else {
				itemList[0] = ItemType<std::string>(std::move(name), std::move(item));
			}


			useSortedItemList = false;
		}

		void InsertUserTypeByIlist(const int ilist_idx, UserType&& item) {
			if (empty() && item.GetName().empty()) {
				isObject = false;
			}
			else if (empty() && !item.GetName().empty()) {
				isObject = true;
			}

			if (isObject && item.GetName().empty()) {
			//	std::cout << "it is not object\n"; return; 
			}
			else if (!isObject && !item.GetName().empty()) {
			//	std::cout << "it is not array\n"; return;
			}


			ilist.push_back(2);
			UserType* temp = new UserType(std::move(item));

			temp->parent = this;

			for (size_t i = ilist.size() - 1; i > ilist_idx; --i) {
				ilist[i] = ilist[i - 1];
			}
			ilist[ilist_idx] = 2;

			bool err = false;
			size_t userTypeIndex = _GetUserTypeIndexFromIlistIndex(ilist, ilist_idx, err);
			userTypeList.push_back(nullptr);
			if (!err) {
				for (size_t i = userTypeList.size() - 1; i > userTypeIndex; --i) {
					userTypeList[i] = std::move(userTypeList[i - 1]);
				}
				userTypeList[userTypeIndex] = temp;
			}
			else {
				userTypeList[0] = temp;
			}


			useSortedUserTypeList = false;
		}
		void InsertUserTypeByIlist(const int ilist_idx, const UserType& item) {
			if (empty() && item.GetName().empty()) {
				isObject = false;
			}
			else if (empty() && !item.GetName().empty()) {
				isObject = true;
			}

			if (isObject && item.GetName().empty()) {
			//	std::cout << "it is not object\n"; return; 
			}
			else if (!isObject && !item.GetName().empty()) {
			//	std::cout << "it is not array\n"; return;
			}
			
			ilist.push_back(2);
			UserType* temp = new UserType(item);

			temp->parent = this;


			for (size_t i = ilist.size() - 1; i > ilist_idx; --i) {
				ilist[i] = ilist[i - 1];
			}
			ilist[ilist_idx] = 2;

			bool err = false;
			size_t  userTypeIndex = _GetUserTypeIndexFromIlistIndex(ilist, ilist_idx, err);
			userTypeList.push_back(nullptr);
			if (!err) {
				for (size_t i = userTypeList.size() - 1; i > userTypeIndex; --i) {
					userTypeList[i] = std::move(userTypeList[i - 1]);
				}
				userTypeList[userTypeIndex] = temp;
			}
			else {
				userTypeList[0] = temp;
			}

			useSortedUserTypeList = false;
		}

		void ReserveIList(long long offset)
		{
			if (offset > 0) {
				ilist.reserve(offset);
			}
		}
		void ReserveItemList(long long offset)
		{
			if (offset > 0) {
				itemList.reserve(offset);
			}
		}
		void ReserveUserTypeList(long long offset)
		{
			if (offset > 0) {
				userTypeList.reserve(offset);
			}
		}
		void AddItem(const char* str1, size_t len1, const char* str2, size_t len2) {
			if (empty() && len1 == 0) {
				isObject = false;
			}
			else if (empty() && len1 != 0) {
				isObject = true;
			}

			if (isObject && len1 == 0) {
			//	std::cout << "it is not object\n"; return; 
			}
			else if (!isObject && len1 != 0) {
			//	std::cout << "it is not array\n"; return;
			}
			
			itemList.emplace_back(std::string(str1, len1), std::string(str2, len2));
			ilist.push_back(1);

			useSortedItemList = false;
		}

		void AddItem(std::string&& name, std::string&& item) {
			if (empty() && name.empty()) {
				isObject = false;
			}
			else if (empty() && !name.empty()) {
				isObject = true;
			}

			if (isObject && name.empty()) {
			//	std::cout << "it is not object\n"; return; 
			}
			else if (!isObject && !name.empty()) {
			//	std::cout << "it is not array\n"; return;
			}
			
			itemList.emplace_back(std::move(name), std::move(item));
			ilist.push_back(1);

			useSortedItemList = false;
		}
		void AddItem(const std::string& name, const std::string& item) {
			if (empty() && name.empty()) {
				isObject = false;
			}
			else if (empty() && !name.empty()) {
				isObject = true;
			}

			if (isObject && name.empty()) {
			//	std::cout << "it is not object\n"; return; 
			}
			else if (!isObject && !name.empty()) {
			//	std::cout << "it is not array\n"; return;
			}
			
			itemList.emplace_back(name, item);
			ilist.push_back(1);

			useSortedItemList = false;
		}
		void AddItemType(wiz::ItemType<std::string>&& item) {
			if (empty() && item.GetName().empty()) {
				isObject = false;
			}
			else if (empty() && !item.GetName().empty()) {
				isObject = true;
			}

			if (isObject && item.GetName().empty()) {
			//	std::cout << "it is not object\n"; return; 
			}
			else if (!isObject && !item.GetName().empty()) {
			//	std::cout << "it is not array\n"; return;
			}
			
			itemList.push_back(std::move(item));
			ilist.push_back(1);

			useSortedItemList = false;
		}

		void AddItemType(const wiz::ItemType<std::string>& item) {
			if (empty() && item.GetName().empty()) {
				isObject = false;
			}
			else if (empty() && !item.GetName().empty()) {
				isObject = true;
			}

			if (isObject && item.GetName().empty()) {
			////	std::cout << "it is not object\n"; return; 
			}
			else if (!isObject && !item.GetName().empty()) {
			//	std::cout << "it is not array\n"; return;
			}
			itemList.push_back(item);
			ilist.push_back(1);

			useSortedItemList = false;
		}
		void AddUserTypeItem(UserType&& item) {
			if (empty() && item.GetName().empty()) {
				isObject = false;
			}
			else if (empty() && !item.GetName().empty()) {
				isObject = true;
			}

			if (isObject && item.GetName().empty()) {
			////	std::cout << "it is not object\n"; return; 
			}
			else if (!isObject && !item.GetName().empty()) {
			//	std::cout << "it is not array\n"; return;
			}
			
			UserType* temp = new UserType(std::move(item.GetName()));

			if (item.IsVirtual()) {
				temp->SetIsVirtual(true);
			}
			if (item.IsArray()) {
				temp->SetIsObject(false);
			}

			temp->parent = this;

			ilist.push_back(2);

			userTypeList.push_back(temp);

			useSortedUserTypeList = false;
			useSortedItemList = false;


			{
				temp->itemList = std::move(item.itemList);
				temp->ilist = std::move(item.ilist);
				temp->userTypeList = std::move(item.userTypeList);

				for (size_t i = 0; i < item.GetUserTypeListSize(); ++i) {
					item.userTypeList[i] = nullptr;
				}
				for (size_t i = 0; i < temp->GetUserTypeListSize(); ++i) {
					temp->userTypeList[i]->parent = temp;
				}
			}
		}
		void AddUserTypeItem(const UserType& item) {
			if (empty() && item.GetName().empty()) {
				isObject = false;
			}
			else if (empty() && !item.GetName().empty()) {
				isObject = true;
			}

			if (isObject && item.GetName().empty()) {
			//	std::cout << "it is not object\n"; return; 
			}
			else if (!isObject && !item.GetName().empty()) {
			//	std::cout << "it is not array\n"; return;
			}
			
			UserType* temp = new UserType(item);
			temp->parent = this;

			ilist.push_back(2);

			userTypeList.push_back(temp);

			useSortedUserTypeList = false;
		}
		void AddItemAtFront(std::string&& name, std::string&& item) {
			if (empty() && name.empty()) {
				isObject = false;
			}
			else if (empty() && !name.empty()) {
				isObject = true;
			}

			if (isObject && name.empty()) {
			//	std::cout << "it is not object\n"; return; 
			}
			else if (!isObject && !name.empty()) {
			//	std::cout << "it is not array\n"; return;
			}
			
			itemList.emplace(itemList.begin(), name, item);

			ilist.insert(ilist.begin(), 1);

			useSortedItemList = false;
		}
		void AddItemAtFront(const std::string& name, const std::string& item) {
			if (empty() && name.empty()) {
				isObject = false;
			}
			else if (empty() && !name.empty()) {
				isObject = true;
			}

			if (isObject && name.empty()) {
			//	std::cout << "it is not object\n"; return; 
			}
			else if (!isObject && !name.empty()) {
			//	std::cout << "it is not array\n"; return;
			}
			
			itemList.emplace(itemList.begin(), name, item);

			ilist.insert(ilist.begin(), 1);

			useSortedItemList = false;
		}
		void AddUserTypeItemAtFront(const UserType& item) {
			if (empty() && item.GetName().empty()) {
				isObject = false;
			}
			else if (empty() && !item.GetName().empty()) {
				isObject = true;
			}

			if (isObject && item.GetName().empty()) {
			//	std::cout << "it is not object\n"; return; 
			}
			else if (!isObject && !item.GetName().empty()) {
			//	std::cout << "it is not array\n"; return;
			}
			
			UserType* temp = new UserType(item);
			temp->parent = this;

			ilist.insert(ilist.begin(), 2);

			userTypeList.insert(userTypeList.begin(), temp);

			useSortedUserTypeList = false;
		}
		void AddUserTypeItemAtFront(UserType&& item) {
			if (empty() && item.GetName().empty()) {
				isObject = false;
			}
			else if (empty() && !item.GetName().empty()) {
				isObject = true;
			}

			if (isObject && item.GetName().empty()) {
			//	std::cout << "it is not object\n"; return; 
			}
			else if (!isObject && !item.GetName().empty()) {
			//	std::cout << "it is not array\n"; return;
			}
			
			UserType* temp = new UserType(item);
			temp->parent = this;

			ilist.insert(ilist.begin(), 2);

			userTypeList.insert(userTypeList.begin(), temp);

			useSortedUserTypeList = false;
		}

		std::vector<ItemType<std::string>> GetItem(const std::string& name) const {
			std::vector<ItemType<std::string>> temp;
			/*if (String::startsWith(name, "$.") && name.size() >= 5) {
				// later, change to binary search?
				std::string str = name.substr(3, name.size() - 4);
				std::regex rgx(str);

				for (int i = 0; i < itemList.size(); ++i) {
					if (regex_match(itemList[i].GetName(), rgx)) {
						temp.push_back(itemList[i]);
					}
				}
			}
			else*/
			{
				if (false == useSortedItemList) {
					sortedItemList.clear();
					for (size_t i = 0; i < itemList.size(); ++i) {
						sortedItemList.push_back((ItemType<std::string>*) & itemList[i]);
					}

					std::sort(sortedItemList.begin(), sortedItemList.end(), ItemTypeStringPtrCompare());

					useSortedItemList = true;
				}
				// binary search
				{
					bool err = false;
					ItemType<std::string> x = ItemType<std::string>(name, "");
					size_t  idx = binary_find_it(sortedItemList, x, err);
					if (!err) {
						size_t  start = idx;
						size_t  last = idx;

						for (size_t i = idx; i > 0; --i) {
							if (name == sortedItemList[i - 1]->GetName()) {
								start--;
							}
							else {
								break;
							}
						}
						for (size_t i = idx + 1; i < sortedItemList.size(); ++i) {
							if (name == sortedItemList[i]->GetName()) {
								last++;
							}
							else {
								break;
							}
						}

						for (size_t i = start; i < last + 1; ++i) {
							temp.push_back(*sortedItemList[i]);
						}
					}
					else {
						//std::cout << "no found" << "\n";
					}
				}
			}
			return temp;
		}
		// regex to SetItem?
		bool SetItem(const std::string& name, const std::string& value) {
			size_t index = -1;
			bool err = true;

			for (size_t i = 0; i < itemList.size(); ++i) {
				if (itemList[i].GetName() == name)
				{
					itemList[i].Set(0, value);
					index = i;
					err = false;
				}
			}

			return err;
		}
		/// add set Data
		bool SetItem(const long long  var_idx, const std::string& value) {
			itemList[var_idx].Set(0, value);
			return true;
		}
	
private:
		/// save1 - like EU4 savefiles.
		void Save1(std::ostream& stream, const UserType* ut, const int depth = 0) const {
			size_t  itemListCount = 0;
			size_t userTypeListCount = 0;

			const bool existUserType = ut->GetUserTypeListSize() > 0;

			for (size_t i = 0; i < ut->ilist.size(); ++i) {
				//std::cout << "ItemList" << endl;
				if (ut->ilist[i] == 1) {
					for (size_t j = 0; j < ut->itemList[itemListCount].size(); j++) {
						std::string temp;
						if (existUserType) {
							for (int k = 0; k < depth; ++k) {
								temp += "\t";
							}
						}
						if (ut->itemList[itemListCount].GetName() != "") {
							temp += ut->itemList[itemListCount].GetName();
							temp += " = ";
						}
						temp += ut->itemList[itemListCount].Get(j);
						if (j != ut->itemList[itemListCount].size() - 1) {
							temp += " ";
						}
						stream << temp;
					}
					if (i != ut->ilist.size() - 1) {
						if (existUserType) {
							stream << "\n";
						}
						else {
							stream << " "; // \n
						}
					}
					itemListCount++;
				}
				else if (ut->ilist[i] == 2) {
					// std::cout << "UserTypeList" << endl;
					for (int k = 0; k < depth; ++k) {
						stream << "\t";
					}

					if (ut->userTypeList[userTypeListCount]->GetName() != "") {
						stream << ut->userTypeList[userTypeListCount]->GetName() << " = ";
					}

					stream << "{\n";

					Save1(stream, ut->userTypeList[userTypeListCount], depth + 1);
					stream << "\n";

					for (int k = 0; k < depth; ++k) {
						stream << "\t";
					}
					stream << "}";
					if (i != ut->ilist.size() - 1) {
						stream << "\n";
					}

					userTypeListCount++;
				}
			}
		}
		/// save2 - for more speed loading data!?
		void Save2(std::ostream& stream, const UserType* ut, const int depth = 0) const {
			size_t  itemListCount = 0;
			size_t  userTypeListCount = 0;


			for (size_t i = 0; i < ut->ilist.size(); ++i) {
				//std::cout << "ItemList" << endl;
				if (ut->ilist[i] == 1) {
					for (size_t j = 0; j < ut->itemList[itemListCount].size(); j++) {
						//for (int k = 0; k < depth; ++k) {
						//	stream << "\t";
						//}
						if (ut->itemList[itemListCount].GetName() != "")
							stream << ut->itemList[itemListCount].GetName() << " : ";
						stream << ut->itemList[itemListCount].Get(j);
						if (j != ut->itemList[itemListCount].size() - 1)
							stream << " ";
					}
					if (i != ut->ilist.size() - 1) {
						stream << " ";//"\n";
					}
					itemListCount++;
				}
				else if (ut->ilist[i] == 2) {
					// std::cout << "UserTypeList" << endl;
					if (ut->userTypeList[userTypeListCount]->GetName() != "")
					{
						stream << ut->userTypeList[userTypeListCount]->GetName() << " ";
					}

					if (ut->userTypeList[userTypeListCount]->IsObject()) {
						stream << "{\n";
					}
					else {
						stream << "[\n";
					}

					Save2(stream, ut->userTypeList[userTypeListCount], depth + 1);
					stream << "\n";

					for (int k = 0; k < depth; ++k) {
						stream << "\t";
					}

					if (ut->userTypeList[userTypeListCount]->IsObject()) {
						stream << "}\n";
					}
					else {
						stream << "]\n";
					}
					
					if (i != ut->ilist.size() - 1) {
						stream << "\n";
					}
					userTypeListCount++;
				}
			}
		}

	public:
		void Save1(std::ostream& stream, int depth = 0) const {
			Save1(stream, this, depth);
		}

		void Save2(std::ostream& stream, int depth = 0) const {
			Save2(stream, this, depth);
		}

		std::string ToString()const
		{
			std::string temp;
			size_t itemListCount = 0;
			size_t  userTypeListCount = 0;

			for (size_t i = 0; i < ilist.size(); ++i) {
				//std::cout << "ItemList" << endl;
				if (ilist[i] == 1) {
					for (size_t j = 0; j < itemList[itemListCount].size(); j++) {
						if (itemList[itemListCount].GetName() != "") {
							temp.append(itemList[itemListCount].GetName());
							temp.append(" = ");
						}
						temp.append(itemList[itemListCount].Get(j));
						if (j != itemList[itemListCount].size() - 1)
						{
							temp.append(" ");
						}
					}
					if (i != ilist.size() - 1) {
						temp.append(" ");
					}
					itemListCount++;
				}
				else if (ilist[i] == 2) {
					// std::cout << "UserTypeList" << endl;
					if (userTypeList[userTypeListCount]->GetName() != "") {
						temp.append(userTypeList[userTypeListCount]->GetName());
						temp.append(" = ");
					}
					temp.append(" { ");
					temp.append(userTypeList[userTypeListCount]->ToString());
					temp.append(" ");
					temp.append(" }");
					if (i != ilist.size() - 1) {
						temp.append(" ");
					}

					userTypeListCount++;
				}
			}
			return temp;
		}
	public:
	};

	// LoadData
	class LoadData
	{
		enum {
			TYPE_LEFT = 1, // 01
			TYPE_RIGHT = 2, // 10
			TYPE_ASSIGN = 3 // 11
		};
	private:
		static long long check_syntax_error1(long long str, int* err) {
			long long len = GetLength(str);
			long long type = GetType(str);

			if (1 == len && (type == TYPE_LEFT || type == TYPE_RIGHT ||
				type == TYPE_ASSIGN)) {
				*err = -4;
			}
			return str;
		}
	public:
		static int Merge(UserType* next, UserType* ut, UserType** ut_next)
		{
			//check!!
			while (ut->GetIListSize() >= 1 && ut->GetUserTypeListSize() >= 1
				&& (ut->GetUserTypeList(0)->IsVirtual()))
			{
				ut = ut->GetUserTypeList(0);
			}

			bool chk_ut_next = false;

			while (true) {
				int itCount = 0;
				int utCount = 0;

				UserType* _ut = ut;
				UserType* _next = next;


				if (ut_next && _ut == *ut_next) {
					*ut_next = _next;
					chk_ut_next = true;
				}

				for (int i = 0; i < _ut->GetIListSize(); ++i) {
					if (_ut->IsUserTypeList(i)) {
						if (_ut->GetUserTypeList(utCount)->IsVirtual()) {
							_ut->GetUserTypeList(utCount)->SetName("");
						}
						else {
							{
								_next->LinkUserType(_ut->GetUserTypeList(utCount));
								_ut->GetUserTypeList(utCount) = nullptr;
							}
						}
						utCount++;
					}
					else if (_ut->IsItemList(i)) {
						_next->AddItemType(std::move(_ut->GetItemList(itCount)));
						itCount++;
					}
				}
				_ut->Remove();

				ut = ut->GetParent();
				next = next->GetParent();


				if (next && ut) {
					//
				}
				else {
					// right_depth > left_depth
					if (!next && ut) {
						return -1;
					}
					else if (next && !ut) {
						return 1;
					}

					return 0;
				}
			}
		}
	private:
		static int GetIdx(long long x) {
			return (x >> 32) & 0x00000000FFFFFFFF;
		}
		static int GetLength(long long x) {
			return (x & 0x00000000FFFFFFF8) >> 3;
		}
		static int GetType(long long x) { //to enum or enum class?
			return (x & 6) >> 1;
		}
	private:
		static bool __LoadData(const char* buffer, const long long* token_arr, long long token_arr_len, UserType* _global, const wiz::LoadDataOption* _option,
			int start_state, int last_state, UserType** next, int* err)
		{

			std::vector<long long> varVec;
			std::vector<long long> valVec;


			if (token_arr_len <= 0) {
				return false;
			}

			UserType& global = *_global;
			wiz::LoadDataOption option = *_option;

			int state = start_state;
			int braceNum = 0;
			std::vector< UserType* > nestedUT(1);
			long long var = 0, val = 0;

			nestedUT.reserve(10);
			nestedUT[0] = &global;


			long long count = 0;
			const long long* x = token_arr;
			const long long* x_next = x;

			for (long long i = 0; i < token_arr_len; ++i) {
				x = x_next;
				{
					x_next = x + 1;
				}
				if (count > 0) {
					count--;
					continue;
				}
				long long len = GetLength(token_arr[i]);

				switch (state)
				{
				case 0:
				{
					// Left 1
					if (len == 1 && (-1 != Equal(TYPE_LEFT, GetType(token_arr[i])))) {
						if (!varVec.empty()) {
							nestedUT[braceNum]->ReserveIList(nestedUT[braceNum]->GetIListSize() + varVec.size());
							nestedUT[braceNum]->ReserveItemList(nestedUT[braceNum]->GetItemListSize() + varVec.size());

							for (size_t x = 0; x < varVec.size(); ++x) {
								nestedUT[braceNum]->AddItem(buffer + GetIdx(varVec[x]), GetLength(varVec[x]),
									buffer + GetIdx(valVec[x]), GetLength(valVec[x]));
							}

							varVec.clear();
							valVec.clear();
						}

						UserType temp("");

						nestedUT[braceNum]->AddUserTypeItem(temp);
						UserType* pTemp = nestedUT[braceNum]->GetUserTypeList(nestedUT[braceNum]->GetUserTypeListSize() - 1);

						if (buffer[GetIdx(token_arr[i])] == option.Left) {
							pTemp->SetIsObject(true); // Object
						}
						else if (buffer[GetIdx(token_arr[i])] == option.Left2) {
							pTemp->SetIsObject(false); // Array
						}

						braceNum++;

						/// new nestedUT
						if (nestedUT.size() == braceNum) { /// changed 2014.01.23..
							nestedUT.push_back(nullptr);
						}

						/// initial new nestedUT.
						nestedUT[braceNum] = pTemp;
						///

						state = 0;
					}
					// Right 2
					else if (len == 1 && (-1 != Equal(TYPE_RIGHT, GetType(token_arr[i])))) {
						/*
						if (!nestedUT[braceNum]->empty() && buffer[GetIdx(token_arr[i])] == option.Right && nestedUT[braceNum]->IsArray()) {
							*err = -5;
							i = token_arr_len - 1;
							continue;
						}
						if (!nestedUT[braceNum]->empty() && buffer[GetIdx(token_arr[i])] == option.Right2 && nestedUT[braceNum]->IsObject()) {
							*err = -6;
							i = token_arr_len - 1;
							continue;
						}
						*/

						state = 0;

						if (!varVec.empty()) {

							{
								nestedUT[braceNum]->ReserveIList(nestedUT[braceNum]->GetIListSize() + varVec.size());
								nestedUT[braceNum]->ReserveItemList(nestedUT[braceNum]->GetItemListSize() + varVec.size());

								for (size_t x = 0; x < varVec.size(); ++x) {
									nestedUT[braceNum]->AddItem(buffer + GetIdx(varVec[x]), GetLength(varVec[x]),
										buffer + GetIdx(valVec[x]), GetLength(valVec[x]));
								}
							}

							varVec.clear();
							valVec.clear();
						}

						if (braceNum == 0) {
							UserType ut;
							ut.AddUserTypeItem(UserType()); // json -> "var_name" = val  // clautext, # is line comment delimiter.
							UserType* pTemp = ut.GetUserTypeList(0);
							pTemp->SetIsVirtual(true);

							if (buffer[GetIdx(token_arr[i])] == option.Right) {
								pTemp->SetIsObject(true); // Object
							}
							else if (buffer[GetIdx(token_arr[i])] == option.Right2) {
								pTemp->SetIsObject(false); // Array
							}

							int utCount = 0;
							int itCount = 0;
							auto max = nestedUT[braceNum]->GetIListSize();
							for (auto i = 0; i < max; ++i) {
								if (nestedUT[braceNum]->IsUserTypeList(i)) {
									ut.GetUserTypeList(0)->AddUserTypeItem(std::move(*(nestedUT[braceNum]->GetUserTypeList(utCount))));
									utCount++;
								}
								else {
									ut.GetUserTypeList(0)->AddItemType(std::move(nestedUT[braceNum]->GetItemList(itCount)));
									itCount++;
								}
							}

							nestedUT[braceNum]->Remove();
							nestedUT[braceNum]->AddUserTypeItem(std::move(*(ut.GetUserTypeList(0))));

							braceNum++;
						}

						{
							if (braceNum < nestedUT.size()) {
								nestedUT[braceNum] = nullptr;
							}
							braceNum--;
						}
					}
					else {
						if (x < token_arr + token_arr_len - 1) {
							long long _len = GetLength(token_arr[i + 1]);
							// EQ 3
							if (_len == 1 && -1 != Equal(TYPE_ASSIGN, GetType(token_arr[i + 1]))) {
								var = token_arr[i];

								state = 1;

								{
									count = 1;
								}
							}
							else {
								// val
								if (x <= token_arr + token_arr_len - 1) {

									val = token_arr[i];

									varVec.push_back(check_syntax_error1(var, err));
									valVec.push_back(check_syntax_error1(val, err));

									val = 0;

									state = 0;

								}
							}
						}
						else
						{
							// var1
							if (x <= token_arr + token_arr_len - 1)
							{
								val = token_arr[i];
								varVec.push_back(check_syntax_error1(var, err));
								valVec.push_back(check_syntax_error1(val, err));
								val = 0;

								state = 0;
							}
						}
					}
				}
				break;
				case 1:
				{
					// LEFT 1
					if (len == 1 && (-1 != Equal(TYPE_LEFT, GetType(token_arr[i])))) {
						nestedUT[braceNum]->ReserveIList(nestedUT[braceNum]->GetIListSize() + varVec.size());
						nestedUT[braceNum]->ReserveItemList(nestedUT[braceNum]->GetItemListSize() + varVec.size());

						for (size_t x = 0; x < varVec.size(); ++x) {
							nestedUT[braceNum]->AddItem(buffer + GetIdx(varVec[x]), GetLength(varVec[x]),
								buffer + GetIdx(valVec[x]), GetLength(valVec[x]));
						}


						varVec.clear();
						valVec.clear();

						///
						{
							nestedUT[braceNum]->AddUserTypeItem(UserType(buffer + GetIdx(var), GetLength(var)));
							UserType* pTemp = nestedUT[braceNum]->GetUserTypeList(nestedUT[braceNum]->GetUserTypeListSize() - 1);
							var = 0;
							braceNum++;


							if (buffer[GetIdx(token_arr[i])] == option.Left) {
								pTemp->SetIsObject(true); // Object
							}
							else if (buffer[GetIdx(token_arr[i])] == option.Left2) {
								pTemp->SetIsObject(false); // Array
							}

							/// new nestedUT
							if (nestedUT.size() == braceNum) {
								nestedUT.push_back(nullptr);
							}

							/// initial new nestedUT.
							nestedUT[braceNum] = pTemp;
						}
						///
						state = 0;
					}
					else {
						if (x <= token_arr + token_arr_len - 1) {
							val = token_arr[i];

							varVec.push_back(check_syntax_error1(var, err));
							valVec.push_back(check_syntax_error1(val, err));
							var = 0; val = 0;

							state = 0;
						}
					}
				}
				break;
				default:
					// syntax err!!
					*err = -1;
					return false; // throw "syntax error ";
					break;
				}
			}

			if (next) {
				*next = nestedUT[braceNum];
			}

			if (varVec.empty() == false) {
				nestedUT[braceNum]->ReserveIList(nestedUT[braceNum]->GetIListSize() + varVec.size());
				nestedUT[braceNum]->ReserveItemList(nestedUT[braceNum]->GetItemListSize() + varVec.size());

				for (size_t x = 0; x < varVec.size(); ++x) {
					nestedUT[braceNum]->AddItem(buffer + GetIdx(varVec[x]), GetLength(varVec[x]),
						buffer + GetIdx(valVec[x]), GetLength(valVec[x]));
				}


				varVec.clear();
				valVec.clear();
			}

			if (state != last_state) {
				*err = -2;
				return false;
				// throw std::string("error final state is not last_state!  : ") + toStr(state);
			}
			if (x > token_arr + token_arr_len) {
				*err = -3;
				return false;
				//throw std::string("error x > buffer + buffer_len: ");
			}

			return true;
		}


		static long long FindDivisionPlace(const char* buffer, const long long* token_arr, long long start, long long last, const wiz::LoadDataOption& option)
		{
			for (long long a = last; a >= start; --a) {
				long long len = GetLength(token_arr[a]);
				long long val = GetType(token_arr[a]);


				if (len == 1 && (-1 != Equal(TYPE_RIGHT, val))) { // right
					return a;
				}

				bool pass = false;
				if (len == 1 && (-1 != Equal(TYPE_LEFT, val))) { // left
					return a;
				}
				else if (len == 1 && -1 != Equal(TYPE_ASSIGN, val)) { // assignment
					//
					pass = true;
				}

				if (a < last && pass == false) {
					long long len = GetLength(token_arr[a + 1]);
					long long val = GetType(token_arr[a + 1]);

					if (!(len == 1 && -1 != Equal(TYPE_ASSIGN, val))) // assignment
					{ // NOT
						return a;
					}
				}
			}
			return -1;
		}

		static bool _LoadData(InFileReserver& reserver, UserType& global, wiz::LoadDataOption option, const int lex_thr_num, const int parse_num) // first, strVec.empty() must be true!!
		{
			const int pivot_num = parse_num - 1;
			char* buffer = nullptr;
			long long* token_arr = nullptr;
			long long buffer_total_len;
			long long token_arr_len = 0;

			{
				int a = clock();

				bool success = reserver(option, lex_thr_num, buffer, &buffer_total_len, token_arr, &token_arr_len);


				int b = clock();
				std::cout << "scan " << b - a << "ms\n";

				//	{
				//		for (long long i = 0; i < token_arr_len; ++i) {
				//			std::string(buffer + GetIdx(token_arr[i]), GetLength(token_arr[i]));
			//				if (0 == GetIdx(token_arr[i])) {
				//				std::cout << "chk";
				//			}
				//		}
				//	}

				if (!success) {
					return false;
				}
				if (token_arr_len <= 0) {
					if (buffer) {
						delete[] buffer;
					}
					if (token_arr) {
						delete[] token_arr;
					}
					return true;
				}
			}

			UserType* before_next = nullptr;
			UserType _global;

			bool first = true;
			long long sum = 0;

			{
				std::set<long long> _pivots;
				std::vector<long long> pivots;
				const long long num = token_arr_len; //

				if (pivot_num > 0) {
					std::vector<long long> pivot;
					pivots.reserve(pivot_num);
					pivot.reserve(pivot_num);

					for (int i = 0; i < pivot_num; ++i) {
						pivot.push_back(FindDivisionPlace(buffer, token_arr, (num / (pivot_num + 1)) * (i), (num / (pivot_num + 1)) * (i + 1) - 1, option));
					}

					for (size_t i = 0; i < pivot.size(); ++i) {
						if (pivot[i] != -1) {
							_pivots.insert(pivot[i]);
						}
					}

					for (auto& x : _pivots) {
						pivots.push_back(x);
					}
				}

				std::vector<UserType*> next(pivots.size() + 1, nullptr);

				{
					std::vector<UserType> __global(pivots.size() + 1);

					std::vector<std::thread> thr(pivots.size() + 1);
					std::vector<int> err(pivots.size() + 1, 0);
					{
						long long idx = pivots.empty() ? num - 1 : pivots[0];
						long long _token_arr_len = idx - 0 + 1;

						thr[0] = std::thread(__LoadData, buffer, token_arr, _token_arr_len, &__global[0], &option, 0, 0, &next[0], &err[0]);
					}

					for (size_t i = 1; i < pivots.size(); ++i) {
						long long _token_arr_len = pivots[i] - (pivots[i - 1] + 1) + 1;

						thr[i] = std::thread(__LoadData, buffer, token_arr + pivots[i - 1] + 1, _token_arr_len, &__global[i], &option, 0, 0, &next[i], &err[i]);

					}

					if (pivots.size() >= 1) {
						long long _token_arr_len = num - 1 - (pivots.back() + 1) + 1;

						thr[pivots.size()] = std::thread(__LoadData, buffer, token_arr + pivots.back() + 1, _token_arr_len, &__global[pivots.size()],
							&option, 0, 0, &next[pivots.size()], &err[pivots.size()]);
					}

					// wait
					for (size_t i = 0; i < thr.size(); ++i) {
						thr[i].join();
					}

					for (size_t i = 0; i < err.size(); ++i) {
						switch (err[i]) {
						case 0:
							break;
						case -1:
						case -4:
							std::cout << "Syntax Error\n";
							break;
						case -2:
							std::cout << "error final state is not last_state!\n";
							break;
						case -3:
							std::cout << "error x > buffer + buffer_len:\n";
							break;
							// -4, -5?
						default:
							std::cout << "unknown parser error\n";
							break;
						}
					}

					// Merge
					try {
						if (__global[0].GetUserTypeListSize() > 0 && __global[0].GetUserTypeList(0)->IsVirtual()) {
							std::cout << "not valid file1\n";
							throw 1;
						}
						if (next.back()->GetParent() != nullptr) {
							std::cout << "not valid file2\n";
							throw 2;
						}

						int err = Merge(&_global, &__global[0], &next[0]);
						if (-1 == err || (pivots.size() == 0 && 1 == err)) {
							std::cout << "not valid file3\n";
							throw 3;
						}

						for (size_t i = 1; i < pivots.size() + 1; ++i) {
							// linearly merge and error check...
							int err = Merge(next[i - 1], &__global[i], &next[i]);
							if (-1 == err) {
								std::cout << "not valid file4\n";
								throw 4;
							}
							else if (i == pivots.size() && 1 == err) {
								std::cout << "not valid file5\n";
								throw 5;
							}
						}
					}
					catch (...) {
						delete[] buffer;
						delete[] token_arr;
						buffer = nullptr;
						throw "in Merge, error";
					}

					before_next = next.back();
				}
			}

			delete[] buffer;
			delete[] token_arr;

			if (!(_global.GetIListSize() == 1)) {
				return false;
			}

			global = std::move(_global);

			return true;
		}
	public:
		static bool LoadDataFromFile(const std::string& fileName, UserType& global, int lex_thr_num, int parse_thr_num) /// global should be empty
		{
			if (lex_thr_num <= 0) {
				lex_thr_num = std::thread::hardware_concurrency();
			}
			if (lex_thr_num <= 0) {
				lex_thr_num = 1;
			}

			if (parse_thr_num <= 0) {
				parse_thr_num = std::thread::hardware_concurrency();
			}
			if (parse_thr_num <= 0) {
				parse_thr_num = 1;
			}

			bool success = true;
			std::ifstream inFile;
			inFile.open(fileName, std::ios::binary);


			if (true == inFile.fail())
			{
				inFile.close(); return false;
			}

			UserType globalTemp;

			try {

				InFileReserver ifReserver(inFile);
				wiz::LoadDataOption option;

				char* buffer = nullptr;
				ifReserver.Num = 1 << 19;
				//	strVec.reserve(ifReserver.Num);
				// cf) empty file..
				if (false == _LoadData(ifReserver, globalTemp, option, lex_thr_num, parse_thr_num))
				{
					inFile.close();
					return false; // return true?
				}

				inFile.close();
			}
			catch (const char* err) { std::cout << err << "\n"; inFile.close(); return false; }
			catch (const std::string& e) { std::cout << e << "\n"; inFile.close(); return false; }
			catch (const std::exception& e) { std::cout << e.what() << "\n"; inFile.close(); return false; }
			catch (...) { std::cout << "not expected error" << "\n"; inFile.close(); return false; }


			global = std::move(globalTemp);

			return true;
		}

		static bool LoadWizDB(UserType& global, const std::string& fileName, const int thr_num) {
			UserType globalTemp = UserType("global");

			// Scan + Parse 
			if (false == LoadDataFromFile(fileName, globalTemp, thr_num, thr_num)) { return false; }
			//std::cout << "LoadData End" << "\n";

			global = std::move(globalTemp);
			return true;
		}
		// SaveQuery
		static bool SaveWizDB(const UserType& global, const std::string& fileName, const bool append = false) {
			std::ofstream outFile;
			if (fileName.empty()) { return false; }
			if (false == append) {
				outFile.open(fileName);
				if (outFile.fail()) { return false; }
			}
			else {
				outFile.open(fileName, std::ios::app);
				if (outFile.fail()) { return false; }

				outFile << "\n";
			}


#if _WIN32
			if (65001 == GetConsoleOutputCP()) {
				outFile << (char)0xEF << (char)0xBB << (char)0xBF;
			}
#endif

			/// saveFile
			global.Save1(outFile); // cf) friend

			outFile.close();

			return true;
		}

		static bool SaveWizDB2(const UserType& global, const std::string& fileName, const bool append = false) {
			std::ofstream outFile;
			if (fileName.empty()) { return false; }
			if (false == append) {
				outFile.open(fileName);
				if (outFile.fail()) { return false; }
			}
			else {
				outFile.open(fileName, std::ios::app);
				if (outFile.fail()) { return false; }

				outFile << "\n";
			}

#if _WIN32
			if (65001 == GetConsoleOutputCP()) {
				outFile << (char)0xEF << (char)0xBB << (char)0xBF;
			}
#endif

			/// saveFile
			global.Save2(outFile); // cf) friend

			outFile.close();

			return true;
		}
	};

}

#endif
