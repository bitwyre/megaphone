#pragma once

#include <algorithm>
#include <array>
#include <exception>
#include <iostream>
#include <string_view>
#include <type_traits>
#include <vector>

using namespace std::literals;

namespace utils {

template <class> inline constexpr bool always_false_v = false;

constexpr auto trim(const std::string_view in) -> std::string_view {
	auto left = in.begin();
	for (;; ++left) {
		if (left == in.end())
			return std::string_view();
		if (!isspace(*left))
			break;
	}
	auto right = in.end() - 1;
	for (; right > left && isspace(*right); --right)
		;
	return std::string_view(left, std::distance(left, right) + 1);
}

inline auto split(const std::string_view str, const char delimiter) -> std::vector<std::string> {
	std::vector<std::string> tokens;
	tokens.reserve(16);
	size_t start = 0;
	size_t end = str.find(delimiter);

	while (end != std::string::npos) {
		tokens.emplace_back(str.substr(start, end - start));
		start = end + 1;
		end = str.find(delimiter, start);
	}

	tokens.emplace_back(str.substr(start));
	return tokens;
}

/**
 * @brief: Meyer's singleton to get env variables
 * defined in the header as it's a tiny, convenience class.
 * @ref https://www.modernescpp.com/index.php/thread-safe-initialization-of-a-singleton/
 */
class ENVManager {
public:
	static auto get_instance() -> ENVManager& {
		static ENVManager instance {};
		return instance;
	};

	inline auto get_megaphone_uws_passphrase() -> const std::string { return this->m_megaphone_uws_passphrase; }
	inline auto get_megaphone_supported_instruments() -> const std::vector<std::string> {
		return this->m_megaphone_supported_instruments;
	}

private:
	ENVManager()
		: m_megaphone_uws_passphrase(this->get_env<std::string>("MEGAPHONE_UWS_PASSPHRASE")),
		  m_megaphone_supported_instruments(
			  this->get_env<std::vector<std::string>>("MEGAPHONE_SUPPORTED_INSTRUMENTS")) { }
	~ENVManager() = default;

	ENVManager(const ENVManager&) = delete;
	ENVManager(ENVManager&&) noexcept = delete;

	ENVManager operator=(const ENVManager&) = delete;
	ENVManager operator=(ENVManager&&) noexcept = delete;

	template <typename T> auto get_env(const std::string& variable_name) -> const T {
		const char* value = getenv(variable_name.c_str());
		if (!value) {
			throw std::runtime_error("Environment variable "s + variable_name + " is NOT set."s);
		}

		if constexpr (std::is_same_v<T, std::string>) {
			return value;
		} else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
			return split(value, ',');
		} else {
			static_assert(always_false_v<T>("Unhandled type"));
		}
	}

private:
	const std::string m_megaphone_uws_passphrase;
	const std::vector<std::string> m_megaphone_supported_instruments;
};

} // namespace utils
