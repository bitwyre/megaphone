#pragma once

#include <algorithm>
#include <array>
#include <exception>
#include <iostream>
#include <string_view>
#include <type_traits>

using namespace std::literals;

namespace utils {

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
} // namespace utils
