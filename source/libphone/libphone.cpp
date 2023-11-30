#include "libphone.hpp"
#include "utils/utils.hpp"

#include "spdlog/spdlog.h"
#include <iostream>

namespace LibPhone {
Phone::Phone() : m_app(uWSAppWrapper({.passphrase = "1234"})), m_serializer(this->m_supported_instruments) {

	this->m_app.ws<PerSocketData>(
		"/*",
		{.compression = uWS::SHARED_COMPRESSOR,
		 .maxPayloadLength = 16 * 1024 * 1024,
		 .idleTimeout = 16,
		 .maxBackpressure = 1 * 1024 * 1024,
		 .closeOnBackpressureLimit = false,
		 .resetIdleTimeoutOnSend = false,
		 .sendPingsAutomatically = true,
		 /* Handlers */
		 .upgrade = nullptr,
		 .open = [this](auto* ws) { this->on_ws_open(ws); },
		 .message = [this](auto* ws, std::string_view message,
						   uWS::OpCode opCode) { this->on_ws_message(ws, message, opCode); },
		 .drain = [this](auto* ws) { this->on_ws_drain(ws); },
		 .ping = [this](auto* ws, std::string_view message) { this->on_ws_ping(ws, message); },
		 .pong = [this](auto* ws, std::string_view message) { this->on_ws_pong(ws, message); },
		 .close = [this](auto* ws, int code, std::string_view message) { this->on_ws_close(ws, code, message); }});

	auto* loop = uWS::Loop::get();

	auto* loop_t = reinterpret_cast<struct us_loop_t*>(loop);
	auto* delay_timer = us_create_timer(loop_t, 0, 0);

	// broadcast the unix time as millis every millisecond
	us_timer_set(
		delay_timer, [](struct us_timer_t*) {}, 1, 1);

	loop->addPostHandler(nullptr, [&](uWS::Loop*) {
		// TODO: Change with respect to ME

		for (auto& topic : this->m_supported_instruments) {
			for (auto& action : {"trades:", "depthl2:"})
				this->m_app.publish(action + topic, "topic", uWS::OpCode::BINARY, false);
		}
	});

	this->m_app.listen(PORT, [](auto* listen_socket) {
		if (listen_socket) {
			SPDLOG_INFO("Listening on port {}", PORT);
		}
	});
};

Phone::~Phone() { uWS::Loop::get()->free(); }

auto Phone::run() -> void { this->m_app.run(); }

auto Phone::on_ws_open(uWSWebSocket* ws) noexcept -> void {
	/* Open event here, you may access ws->getUserData() which points to a
	 * PerSocketData struct */
	PerSocketData* perSocketData = (PerSocketData*)ws->getUserData();
	perSocketData->user = this->users++;
}

auto Phone::on_ws_message(uWSWebSocket* ws, std::string_view message, uWS::OpCode opCode) noexcept -> void {
	PerSocketData* perSocketData = (PerSocketData*)ws->getUserData();

	auto [success, req] = this->m_serializer.parse_request(message);

	if (success) {

		perSocketData->topics = std::move(req.topics);

		SPDLOG_INFO("user {} has subscribed to topics: ", perSocketData->user);
		for (auto& topic : perSocketData->topics) {
			if (ws->subscribe(topic)) {
				SPDLOG_INFO("\t - {}", topic);
			}
		}

	} else {
		// return error
		ws->send("An error has occoured in executing the request, please check the request.");
	}
}
auto Phone::on_ws_drain(uWSWebSocket* ws) noexcept -> void { }

auto Phone::on_ws_ping(uWSWebSocket* ws, std::string_view message) noexcept -> void { }

auto Phone::on_ws_pong(uWSWebSocket* ws, std::string_view message) noexcept -> void { }

auto Phone::on_ws_close(uWSWebSocket* ws, int code, std::string_view message) noexcept -> void { }

} // namespace LibPhone
