#include "libphone.hpp"
#include "libusockets.h"
#include "utils/utils.hpp"

namespace LibPhone {

Phone::Phone(zenohc::Session& session)
	: m_app(uWSAppWrapper({.passphrase = utils::ENVManager::get_instance().get_megaphone_uws_passphrase().c_str()})),
	  m_supported_instruments(utils::ENVManager::get_instance().get_megaphone_supported_instruments()),
	  m_serializer(),
	  m_zenoh_subscriber(nullptr) {

	this->m_zenoh_subscriber = zenohc::expect<zenohc::Subscriber>(
		session.declare_subscriber("matching_engine/*", [&](const zenohc::Sample& sample) {
			// TODO: Flatbuffers (magic + testing)
			// Try to remove this un-wanted heap allocation
			std::string lowercase_encoding {sample.get_encoding().get_suffix().as_string_view()};
			std::string data {sample.get_payload().as_string_view()};

			std::transform(lowercase_encoding.begin(), lowercase_encoding.end(), lowercase_encoding.begin(),
						   [](unsigned char c) { return std::tolower(c); });

			this->m_zenoh_queue.emplace(std::move(lowercase_encoding), "bnb_usdt_spot", std::move(data));
		}));

	this->m_app.ws<PerSocketData>(
		"/*",
		{.compression = uWS::DISABLED,
		 .maxPayloadLength = 16 * 1024 * 1024,
		 .idleTimeout = 16,
		 .maxBackpressure = 1 * 1024 * 1024,
		 .closeOnBackpressureLimit = false,
		 .resetIdleTimeoutOnSend = false,
		 .sendPingsAutomatically = false,
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
	us_timer_set(
		delay_timer, [](struct us_timer_t*) {}, 1, 1);

	uWS::Loop::get()->addPostHandler(nullptr, [this](uWS::Loop* loop) {
		if (this->m_zenoh_queue.front() != nullptr) {

			loop->defer([this]() {
				auto current_item = *this->m_zenoh_queue.front();
				auto current_topic = current_item.msg_type + ':' + current_item.instrument;
				this->m_app.publish(current_topic, current_item.data, uWS::OpCode::BINARY, false);

				// TODO: FIXME goofy issue where the thing works
				// if I put this log here, but doesn't work if I don't put
				// the log? wtf?
				SPDLOG_INFO("Current topic: {}", current_topic);
			});

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

	SPDLOG_INFO("user {} has opened a connection.", perSocketData->user);

	ws->send("Welcome to the Bitwyre WebSocket API! Please send a JSON "
			 "request structured according to our documentation to "
			 "subscribe to a channel.");
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
			} else {
				SPDLOG_ERROR("Failed to subscribe to: {}", topic);
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
