#include "libphone.hpp"
#include "utils/utils.hpp"

namespace LibPhone {

Phone::Phone(zenohc::Session& session)
	: m_app(uWSAppWrapper({.passphrase = utils::ENVManager::get_instance().get_megaphone_uws_passphrase().c_str()})),
	  m_supported_instruments(utils::ENVManager::get_instance().get_megaphone_supported_instruments()),
	  m_serializer(),
	  m_zenoh_subscriber(nullptr) {

	auto keyexpr_str = utils::ENVManager::get_instance().get_megaphone_zeno_keyexpr();
	this->m_zenoh_subscriber =
		zenohc::expect<zenohc::Subscriber>(session.declare_subscriber(keyexpr_str, [&](const zenohc::Sample& sample) {
			this->m_zenoh_queue.emplace(MessageType::DEPTHL2, "bnb_usdt_spot");
		}));

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

	loop->addPreHandler(nullptr, [&](uWS::Loop*) {
		while (this->m_zenoh_queue.size() > 0 && this->m_zenoh_queue.front()) {

			auto& current_item = *this->m_zenoh_queue.front();
			auto current_topic = std::string(G_MessageMap[current_item.msg_type]) + ":" + current_item.instrument;

			// SPDLOG_INFO("Current topic: {}", current_topic);

			this->m_app.publish(current_topic, current_item.instrument, uWS::OpCode::BINARY, false);
			this->m_zenoh_queue.pop();
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

	const auto& [success, req] = this->m_serializer.parse_request(message);

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
