#include "libphone.hpp"
#include "utils/utils.hpp"

#include "spdlog/spdlog.h"
#include <iostream>

namespace LibPhone {
Phone::Phone() : m_app(uWS::App({.passphrase = "1234"})) {

	this->m_app.ws<PerSocketData>(
		"/*", {.compression = uWS::SHARED_COMPRESSOR,
			   .maxPayloadLength = 16 * 1024 * 1024,
			   .idleTimeout = 16,
			   .maxBackpressure = 1 * 1024 * 1024,
			   .closeOnBackpressureLimit = false,
			   .resetIdleTimeoutOnSend = false,
			   .sendPingsAutomatically = true,
			   /* Handlers */
			   .upgrade = nullptr,
			   .open =
				   [this](auto* ws) {
					   /* Open event here, you may access ws->getUserData() which points to a
						* PerSocketData struct */
					   PerSocketData* perSocketData = (PerSocketData*)ws->getUserData();
					   perSocketData->user = this->users++;
				   },
			   .message =
				   [this](auto* ws, std::string_view message, uWS::OpCode opCode) {
					   PerSocketData* perSocketData = (PerSocketData*)ws->getUserData();

					   auto trimmed_msg = utils::trim(message);
					   bool topic_exists = std::find(this->m_all_topics.begin(), this->m_all_topics.end(),
													 trimmed_msg) != this->m_all_topics.end();

					   if (topic_exists) {

						   perSocketData->topic = trimmed_msg;
						   ws->subscribe(trimmed_msg);

						   SPDLOG_INFO("user {} has subscribed to topic {}", perSocketData->user, perSocketData->topic);

					   } else {
						   // return error
						   SPDLOG_ERROR("Topic {} does not exist", trimmed_msg);
					   }
				   },
			   .drain =
				   [](auto* /*ws*/) {
					   /* Check ws->getBufferedAmount() here */
				   },
			   .ping =
				   [](auto* /*ws*/, std::string_view) {
					   /* Not implemented yet */
				   },
			   .pong =
				   [](auto* /*ws*/, std::string_view) {
					   /* Not implemented yet */
				   },
			   .close =
				   [](auto* /*ws*/, int /*code*/, std::string_view /*message*/) {
					   /* You may access ws->getUserData() here */
				   }});

	auto loop = uWS::Loop::get();

	struct us_loop_t* loop_t = (struct us_loop_t*)loop;
	struct us_timer_t* delay_timer = us_create_timer(loop_t, 0, 0);

	// broadcast the unix time as millis every 8 millis
	us_timer_set(
		delay_timer, [](struct us_timer_t*) {}, 1, 1);

	loop->addPostHandler(nullptr, [&](uWS::Loop*) {
		for (auto& topic : this->m_all_topics) {
			this->m_app.publish(topic, topic, uWS::OpCode::BINARY, false);
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

} // namespace LibPhone
