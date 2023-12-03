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

constexpr auto split(const std::string_view str, const char delimiter) -> std::vector<std::string> {
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

template <typename K, typename V, std::size_t N> struct ConstexprMap {
	using key_type = K;
	using value_type = V;
	using const_key_type = std::add_const_t<key_type>;
	using pair_type = std::pair<const_key_type, V>;

	std::array<pair_type, N> map_;

	[[nodiscard]] constexpr auto at(const_key_type& lookedForKey) const -> value_type {
		auto it = std::find_if(map_.begin(), map_.end(),
							   [&lookedForKey](const pair_type& e) { return e.first == lookedForKey; });
		if (it != map_.end()) {
			return it->second;
		}
		throw std::runtime_error("Element with the specified key not found");
	}

	constexpr auto operator[](const_key_type& k) const -> value_type { return at(k); }
};

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

	inline auto get_megaphone_zeno_keyexpr() -> const std::string { return this->m_megaphone_zeno_keyexpr; }
	inline auto get_megaphone_uws_passphrase() -> const std::string { return this->m_megaphone_uws_passphrase; }
	inline auto get_megaphone_supported_instruments() -> const std::vector<std::string> {
		return this->m_megaphone_supported_instruments;
	}

private:
	ENVManager()
		: m_megaphone_zeno_keyexpr(this->get_env<std::string>("MEGAPHONE_ZENO_KEYEXPR")),
		  m_megaphone_uws_passphrase(this->get_env<std::string>("MEGAPHONE_UWS_PASSPHRASE")),
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
	const std::string m_megaphone_zeno_keyexpr;
	const std::string m_megaphone_uws_passphrase;
	const std::vector<std::string> m_megaphone_supported_instruments;
};

} // namespace utils
