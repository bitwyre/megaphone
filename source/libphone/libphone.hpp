#pragma once

#include "App.h"

constexpr auto PORT = 9001;

namespace LibPhone {

class Phone {
public:
	Phone();
	~Phone() = default;

	Phone(const Phone&) = delete;
	Phone(Phone&&) noexcept = delete;

	Phone operator=(const Phone&) = delete;
	Phone operator=(Phone&&) noexcept = delete;

	auto run() -> void;

	auto get_app() noexcept -> uWS::App&;

private:
	/* ws->getUserData returns one of these */
	struct PerSocketData {
		/* Fill with user data */
	};

	uWS::App m_app;
};

} // namespace LibPhone
