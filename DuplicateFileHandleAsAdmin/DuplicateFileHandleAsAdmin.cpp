#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>

#include <io.h>
#include <fcntl.h>

#include <optional>
#include <cstdint>
#include <string>
#include <bit>
#include <variant>

#include <fmt/core.h>
#include <nowide/convert.hpp>


namespace helper {

	template<class uint_type, std::size_t max_number_of_digits>
	constexpr std::optional<uint_type> string_base10_to_integer(const uint_type(&digits_of_max_number)[max_number_of_digits], const std::string_view& str) {
		static_assert(sizeof(digits_of_max_number) == sizeof(uint_type) * max_number_of_digits);
		static_assert(std::is_unsigned_v<uint_type>);

		if (str.empty() || str.size() > max_number_of_digits)
			return std::nullopt;


		bool must_check_for_max_num = str.size() == max_number_of_digits;
		unsigned index = 0;

		uint_type out_number = 0;
		for (char c : str) {
			static_assert('0' < '9');
			if (c < '0' || c > '9')
				return std::nullopt;

			uint_type digit = c - '0';

			if (digit == 0 && out_number == 0)
				continue;

			if (must_check_for_max_num) {
				auto max_num_digit = digits_of_max_number[index];
				++index;
				if (digit > max_num_digit)
					return std::nullopt;
				if (digit < max_num_digit)
					must_check_for_max_num = false;
			}

			out_number *= 10;
			out_number += digit;
		}
		return out_number;
	}

	template<class uint_type, std::size_t max_number_of_digits>
	constexpr bool does_array_represent_max_number(const uint_type(&digits)[max_number_of_digits]) {
		uint_type value{ 0 };
		for (int i{ 0 }; i < max_number_of_digits; ++i) {
			value *= 10;
			value += digits[i];
		}
		return value == std::numeric_limits<uint_type>::max();
	}


	template<class uint>
	constexpr std::optional<uint> string_to_uint(const std::string_view& str);

	template<>
	constexpr std::optional<std::uint32_t> string_to_uint<uint32_t>(const std::string_view& str) {

		static_assert(sizeof(uint32_t) == 4);
		// 2^32 - 1  == 4'294'967'295
		constexpr const uint32_t max_number_digits[10] = { 4,2,9,4,9,6,7,2,9,5 };
		static_assert(does_array_represent_max_number(max_number_digits));

		return string_base10_to_integer(max_number_digits, str);
	}


	template<>
	constexpr std::optional<std::uint64_t> string_to_uint<uint64_t>(const std::string_view& str) {

		// 2^64 - 1 == 18446744073709551615
		constexpr const uint64_t digits_of_max_number[] = { 1,8,4,4,6,7,4,4,0,7,3,7,0,9,5,5,1,6,1,5 };
		static_assert(does_array_represent_max_number(digits_of_max_number));

		return string_base10_to_integer(digits_of_max_number, str);
	}
}

struct handle_wrapper {
	HANDLE h{INVALID_HANDLE_VALUE};

	bool CheckedClose() {
		if (h == INVALID_HANDLE_VALUE)
			return true;
		
		bool ret = CloseHandle(h);
		h = INVALID_HANDLE_VALUE;
		return ret;
	}

	HANDLE Extract() {
		HANDLE ret = h;
		h = INVALID_HANDLE_VALUE;
		return ret;
	}

	~handle_wrapper() {
		CheckedClose();
	}
};

int main(int argc, const char** argv)
{
	_set_fmode(_O_BINARY);
	_setmode(_fileno(stdout), _O_BINARY);
	_setmode(_fileno(stdin), _O_BINARY);

	if (GetACP() != 65001) {
		fmt::print(stderr, "The Active Code Page (ACP) for this process is not UTF-8 (65001).\n"
			"Command line parsing is not supported.\n"
			"Your version of Windows might be to old, so that the manifest embedded in the executable is not read. "
			"The manifest specifies, that this executable wants UTF-8 as ACP.\n"
			"As a workaround you can activate \"Beta: Use Unicode UTF-8 for worldwide language support\":\n"
			"  - Press Win+R\n"
			"  - Type \"intl.cpl\"\n"
			"  - Goto Tab \"Administrative\"\n"
			"  - Click on \"Change system locale\"\n"
			"  - Set Checkbox \"Beta: Use Unicode UTF-8 for worldwide language support\"\n"
			"\n");
		return 1;
	}
	decltype(GetLastError()) last_error{};

	const char* const prog = argc <= 0 ? "dfhaa.exe" : argv[0];

	fmt::print(stderr,
		"Usage:\n"
		"  \"{}\" <PID> <FILE> <NAMED_PIPE>\n\n", 
		prog);

	if (argc < 4) {
		fmt::print(stderr, "Error: Not Enough arguments.\n");
		return 1;
	}

	auto pid_opt = helper::string_to_uint<uint32_t>(argv[1]);
	if (!pid_opt.has_value()) {
		fmt::print(stderr, "Error: Couldn't parse PID.\n");
		return 1;
	}
	static_assert(sizeof DWORD == sizeof (decltype(pid_opt)::value_type));

	handle_wrapper hwProcess{ .h = OpenProcess(PROCESS_DUP_HANDLE, false, *pid_opt) };
	if (hwProcess.h == INVALID_HANDLE_VALUE) {
		auto last_error = GetLastError();
		fmt::print(stderr, "Error: OpenProcess failed with {}\n", last_error);
		return 1;
	}

	std::string_view named_pipe_name{argv[3]};
	auto utf16_named_pipe_name = nowide::widen(named_pipe_name);

	handle_wrapper hwPipe{};
	while (true) {
		hwPipe.h = CreateFileW(utf16_named_pipe_name.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
		if (hwPipe.h != INVALID_HANDLE_VALUE)
			break;
		last_error = GetLastError();
		if (last_error != ERROR_PIPE_BUSY) {
			fmt::print(stderr, "Error: Failed to open pipe. Last error {}.\n", last_error);
			return 1;
		}

		if (!WaitNamedPipeW(utf16_named_pipe_name.c_str(), 20000)) {
			fmt::print(stderr, "Error: Could not open pipe. (20 seconds timeout).");
			return 1;
		}
	}


	std::string_view file_name{argv[2]};
	auto utf16_file_name = nowide::widen(file_name);

	handle_wrapper hwFile{ .h = CreateFileW(utf16_file_name.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr) };

	last_error = GetLastError();
	if (hwFile.h == INVALID_HANDLE_VALUE) {
		fmt::print(stderr, "Error: CreateFileW failed with {}\n", last_error);
		return 1;
	}

	switch (last_error) {
	case ERROR_ALREADY_EXISTS:
		fmt::print(stderr, "Info: File already exists.\n");
		break;
	case ERROR_SUCCESS:
		fmt::print(stderr, "Info: File is newly created.\n");
		break;
	default:
		break;
	}

	HANDLE hDupFile{}; // don't use handle_wrapper, because it is a handle of another process.
	if (!DuplicateHandle(GetCurrentProcess(), hwFile.h, hwProcess.h, &hDupFile, 0, false, DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE)) {
		last_error = GetLastError();
		fmt::print(stderr, "Error: DuplicateHandle failed with {}\n", last_error);
		return 1;
	}
	(void)hwFile.Extract(); // drop, because we closed it with DUPLICATE_CLOSE_SOURCE


	auto handle_as_number = std::bit_cast<uintptr_t>(hDupFile);
	fmt::print(stderr, "In the target process the handle is:\n");
	fmt::print(stdout, "{:x}", handle_as_number);

	return 0;
}

