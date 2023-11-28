#include "libphone.hpp"

#include "spdlog/spdlog.h"
#include <iostream>

namespace LibPhone {
Phone::Phone() : m_app(uWS::App({.passphrase = "1234"})) {

	this->m_app.ws<PerSocketData>("/*", {.compression = uWS::SHARED_COMPRESSOR,
										 .maxPayloadLength = 16 * 1024 * 1024,
										 .idleTimeout = 16,
										 .maxBackpressure = 1 * 1024 * 1024,
										 .closeOnBackpressureLimit = false,
										 .resetIdleTimeoutOnSend = false,
										 .sendPingsAutomatically = true,
										 /* Handlers */
										 .upgrade = nullptr,
										 .open =
											 [](auto* ws) {
												 /* Open event here, you may access ws->getUserData() which points to a
												  * PerSocketData struct */
												 ws->subscribe("broadcast");
											 },
										 .message =
											 [](auto* /*ws*/, std::string_view /*message*/, uWS::OpCode /*opCode*/) {

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

	this->m_app.listen(PORT, [](auto* listen_socket) {
		if (listen_socket) {
			SPDLOG_INFO("Listening on port {}", PORT);
		}
	});
};

auto Phone::get_app() noexcept -> uWS::App& { return this->m_app; }

auto Phone::run() -> void { this->m_app.run(); }

} // namespace LibPhone
