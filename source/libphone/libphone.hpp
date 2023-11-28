#pragma once

#include "App.h"
#include <set>

constexpr auto PORT = 9001;

namespace LibPhone {

class Phone {
public:
	Phone();
	~Phone();

	Phone(const Phone&) = delete;
	Phone(Phone&&) noexcept = delete;

	Phone operator=(const Phone&) = delete;
	Phone operator=(Phone&&) noexcept = delete;

	auto run() -> void;

	auto publish(const std::string_view p_broadcast, const std::string_view message) -> void;

private:
	/* ws->getUserData returns one of these */
	struct PerSocketData {
		/* Fill with user data */
		std::string topic {""};
		int user {0};
	};
	uWS::App m_app;

	std::vector<std::string> m_all_topics {"bnb_usdt_spot", "busd_usd_spot"};
	int users;
};

} // namespace LibPhone
