#include "libphone.hpp"
#include "libusockets.h"
#include "utils/utils.hpp"

#include <chrono>
#include <thread>

#include "depthl2_generated.h"
#include "l2_events_generated.h"
#include "l3_events_generated.h"
#include "trades_generated.h"
#include "ticker_generated.h"

namespace LibPhone {

Phone::Phone(zenohc::Session& session)
	: m_app(uWSAppWrapper({.passphrase = utils::ENVManager::get_instance().get_megaphone_uws_passphrase().c_str()})),
	  m_supported_instruments(utils::ENVManager::get_instance().get_megaphone_supported_instruments()),
	  m_serializer(),
	  m_zenoh_subscriber(nullptr) {

	this->m_zenoh_subscriber = zenohc::expect<zenohc::Subscriber>(
		session.declare_subscriber("bitwyre/megaphone/websockets", [&](const zenohc::Sample& sample) {
			// Timer so that Zenoh doesn't overload the shit out of megaphone
			std::this_thread::sleep_for(std::chrono::milliseconds(1));

			std::string encoding {sample.get_encoding().get_suffix().as_string_view()};
			std::string datacopy {sample.get_payload().as_string_view()};
			std::string data {};
			std::string instrument {};

			std::transform(encoding.begin(), encoding.end(), encoding.begin(),
						   [](unsigned char c) { return std::tolower(c); });
			if (encoding == "ticker") {
				auto data_flatbuf =
					Bitwyre::Flatbuffers::Ticker::GetTickerEvent(datacopy.c_str());
				instrument = data_flatbuf->instrument()->str();
				data = FBHandler::flatbuf_to_json<Bitwyre::Flatbuffers::Ticker::TickerEvent>(
					sample.get_payload().start, sample.get_payload().get_len());

			} else if (encoding == "l2_events") {
				auto data_flatbuf =
					Bitwyre::Flatbuffers::L2Event::GetL2Event(datacopy.c_str());
				instrument = data_flatbuf->symbol()->str();
				data = FBHandler::flatbuf_to_json<Bitwyre::Flatbuffers::L2Event::L2Event>(
					sample.get_payload().start, sample.get_payload().get_len());

			} else if (encoding == "l3_events") {
				auto data_flatbuf =
					Bitwyre::Flatbuffers::L3Event::GetL3Event(datacopy.c_str());
				instrument = data_flatbuf->symbol()->str();
				data = FBHandler::flatbuf_to_json<Bitwyre::Flatbuffers::L3Event::L3Event>(
					sample.get_payload().start, sample.get_payload().get_len());

			} else if (encoding == "trades") {
				auto data_flatbuf =
					Bitwyre::Flatbuffers::trades::Gettrades(datacopy.c_str());
				instrument = data_flatbuf->symbol()->str();
				data = FBHandler::flatbuf_to_json<Bitwyre::Flatbuffers::trades::trades>(
					sample.get_payload().start, sample.get_payload().get_len());

			} else if (encoding == "depthl2" || encoding == "depthl2_10" || encoding == "depthl2_25" ||
					   encoding == "depthl2_50" || encoding == "depthl2_100") {
				auto data_flatbuf =
					Bitwyre::Flatbuffers::Depthl2::GetDepthEvent(datacopy.c_str());
				instrument = data_flatbuf->data()->instrument()->str();
				data = FBHandler::flatbuf_to_json<Bitwyre::Flatbuffers::Depthl2::DepthEvent>(
					sample.get_payload().start, sample.get_payload().get_len());
			}

			SPDLOG_INFO("Event type: {}\n\tData: {}\n\tInstrument: {}", encoding, data, instrument);

			this->m_zenoh_queue.push(MEMessage {encoding, instrument, data});
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

	loop->addPostHandler(nullptr, [this](uWS::Loop* p_loop) {
		p_loop->defer([this]() {
			if (this->m_zenoh_queue.front() != nullptr) {
				auto current_item = *this->m_zenoh_queue.front();
				auto current_topic = current_item.msg_type + ':' + current_item.instrument;
				if (this->m_app.publish(current_topic, current_item.data, uWS::OpCode::BINARY, false)) {
					
				}
				this->m_zenoh_queue.pop();
			}
		});
	});

	this->m_app.listen(PORT, [](auto* listen_socket) {
		if (listen_socket) {
			SPDLOG_INFO("Listening on port {}", PORT);
		}
	});
};

Phone::~Phone() { }

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
