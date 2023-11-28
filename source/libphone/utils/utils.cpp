#include "utils.hpp"

namespace utils {
auto trim(std::string_view in) -> std::string_view {
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
} // namespace utils
