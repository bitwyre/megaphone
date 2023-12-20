#include "libphone.hpp"
#include "libusockets.h"
#include "utils/utils.hpp"

#include <chrono>
#include <thread>

#include "depthl2_generated.h"
#include "l2_events_generated.h"
#include "l3_events_generated.h"
#include "ticker_generated.h"
#include "trades_generated.h"

namespace LibPhone {

Phone::Phone()
	: m_app(uWSAppWrapper({.passphrase = utils::ENVManager::get_instance().get_megaphone_uws_passphrase().c_str()})),
	  m_supported_instruments(utils::ENVManager::get_instance().get_megaphone_supported_instruments()),
	  m_serializer(),
	  m_zenoh_subscriber(nullptr) {

	SPDLOG_INFO("Supported instruments:");
	for (auto& instrument : this->m_supported_instruments) {
		SPDLOG_INFO("\t{}", instrument);
	}

	this->m_app.ws<PerSocketData>(
		"/*",
		{.compression = uWS::DISABLED,
		 .maxPayloadLength = 16 * 2048 * 2048,
		 .idleTimeout = 960,
		 .maxBackpressure = 16 * 2048 * 2048,
		 .closeOnBackpressureLimit = false,
		 .resetIdleTimeoutOnSend = false,
		 .sendPingsAutomatically = false,
		 /* Handlers */
		 .upgrade = nullptr,
		 .open = [this](auto* ws) { this->on_ws_open(ws); },
		 .message = [this](auto* ws, std::string_view message,
						   uWS::OpCode opCode) { this->on_ws_message(ws, message, opCode); },
		 .drain = nullptr,
		 .ping = [this](auto* ws, std::string_view message) { this->on_ws_ping(ws, message); },
		 .pong = [this](auto* ws, std::string_view message) { this->on_ws_pong(ws, message); },
		 .close = [this](auto* ws, int code, std::string_view message) { this->on_ws_close(ws, code, message); }});

	this->m_loop = uWS::Loop::get();

	auto* loop_t = reinterpret_cast<struct us_loop_t*>(this->m_loop);
	auto* delay_timer = us_create_timer(loop_t, 0, 0);
	us_timer_set(
		delay_timer, [](struct us_timer_t*) {}, 1, 1);

	this->m_app.listen(PORT, [](auto* listen_socket) {
		if (listen_socket) {
			SPDLOG_INFO("Listening on port {}", PORT);
		}
	});
};

Phone::~Phone() { }

auto Phone::run(zenohc::Session& session) -> void {

	this->m_zenoh_subscriber = zenohc::expect<zenohc::Subscriber>(
		session.declare_subscriber("bitwyre/megaphone/websockets", [&](const zenohc::Sample& sample) {
			std::string data {};
			std::string instrument {};
			std::string encoding {sample.get_encoding().get_suffix().as_string_view()};
			std::string datacopy {reinterpret_cast<const char*>(sample.get_payload().start), sample.get_payload().len};

			std::transform(encoding.begin(), encoding.end(), encoding.begin(),
						   [](unsigned char c) { return std::tolower(c); });

			if (encoding == "ticker") {

				auto data_flatbuf = Bitwyre::Flatbuffers::Ticker::GetTickerEvent(datacopy.c_str());
				instrument = data_flatbuf->instrument()->str();
				data = FBHandler::flatbuf_to_json<Bitwyre::Flatbuffers::Ticker::TickerEvent>(
					sample.get_payload().start, sample.get_payload().get_len());

			} else if (encoding == "l2_events") {

				auto data_flatbuf = Bitwyre::Flatbuffers::L2Event::GetL2Event(datacopy.c_str());
				instrument = data_flatbuf->symbol()->str();
				data = FBHandler::flatbuf_to_json<Bitwyre::Flatbuffers::L2Event::L2Event>(
					sample.get_payload().start, sample.get_payload().get_len());

			} else if (encoding == "l3_events") {

				auto data_flatbuf = Bitwyre::Flatbuffers::L3Event::GetL3Event(datacopy.c_str());
				instrument = data_flatbuf->symbol()->str();
				data = FBHandler::flatbuf_to_json<Bitwyre::Flatbuffers::L3Event::L3Event>(
					sample.get_payload().start, sample.get_payload().get_len());

			} else if (encoding == "trades") {

				auto data_flatbuf = Bitwyre::Flatbuffers::trades::Gettrades(datacopy.c_str());
				instrument = data_flatbuf->symbol()->str();
				data = FBHandler::flatbuf_to_json<Bitwyre::Flatbuffers::trades::trades>(sample.get_payload().start,
																						sample.get_payload().get_len());

			} else if (encoding == "depthl2" || encoding == "depthl2_10" || encoding == "depthl2_25" ||
					   encoding == "depthl2_50" || encoding == "depthl2_100") {

				auto data_flatbuf = Bitwyre::Flatbuffers::Depthl2::GetDepthEvent(datacopy.c_str());
				instrument = data_flatbuf->data()->instrument()->str();
				data = FBHandler::flatbuf_to_json<Bitwyre::Flatbuffers::Depthl2::DepthEvent>(
					sample.get_payload().start, sample.get_payload().get_len());
			}

			SPDLOG_INFO("Event type: {}\n\tInstrument: {}\n\tData: {}", encoding, instrument, data);

			publish_result(MEMessage {encoding, instrument, data});
		}));

	this->m_app.run();
}

auto Phone::publish_result(MEMessage&& item) -> void {

	this->m_loop->defer([=]() {
		// This needs to be done as the publish function takes in a string view, ie, a reference.
		const auto data_copy {item.data};
		const auto topic = item.msg_type + ':' + item.instrument;

		// Publish to the global ticker
		if (item.msg_type == "ticker") {
			if (!this->m_app.publish(item.msg_type, data_copy, uWS::OpCode::TEXT, false)) {
				// This log is debug as publish fails if the topic doesn't exist for the user
				// that results in a lot of false-positive error logs.
				SPDLOG_DEBUG("Failed to publish to topic: {}", topic);
			}
		}

		// as well as the global ticker
		// TODO: Make a separate thread for each type of event
		if (!this->m_app.publish(topic, item.data, uWS::OpCode::TEXT, false)) {
			SPDLOG_DEBUG("Failed to publish to topic: {}", topic);
		}
	});

	return;
};

auto Phone::on_ws_open(uWSWebSocket* ws) -> void {
	/* Open event here, you may access ws->getUserData() which points to a
	 * PerSocketData struct */
	PerSocketData* perSocketData = (PerSocketData*)ws->getUserData();
	perSocketData->user = this->users++;

	SPDLOG_INFO("user {} has opened a connection.", perSocketData->user);

	ws->send("Welcome to the Bitwyre WebSocket API! Please send a JSON "
			 "request structured according to our documentation to "
			 "subscribe to a channel.");
}

auto Phone::on_ws_message(uWSWebSocket* ws, std::string_view message, uWS::OpCode opCode) -> void {
	PerSocketData* perSocketData = (PerSocketData*)ws->getUserData();

	const auto& [success, req] = this->m_serializer.parse_request(message);

	if (success) {

		SPDLOG_INFO("user {} has subscribed to topics: ", perSocketData->user);
		for (auto& topic : req.topics) {
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

auto Phone::on_ws_drain(uWSWebSocket* ws) -> void { }

auto Phone::on_ws_ping(uWSWebSocket* ws, std::string_view message) -> void { }

auto Phone::on_ws_pong(uWSWebSocket* ws, std::string_view message) -> void { }

auto Phone::on_ws_close(uWSWebSocket* ws, int code, std::string_view message) -> void { }

} // namespace LibPhone
