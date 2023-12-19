#pragma once

#include "App.h"
#include <set>
#include <spdlog/spdlog.h>
#include <zenohc.hxx>

#include <atomic>
#include <iostream>

#include "fbhandler/fbhandler.hpp"
#include "serializer/serializer.hpp"
#include "utils/utils.hpp"

constexpr auto PORT = 7447;

namespace LibPhone {

enum class MessageType { DEPTHL2, L2_EVENTS, L3_EVENTS, TRADE, INVALID };
struct MEMessage {
	std::string msg_type;
	std::string instrument;
	std::string data;
};

class Phone {
public:
	explicit Phone();
	~Phone();

	Phone(const Phone&) = delete;
	Phone(Phone&&) noexcept = delete;

	Phone operator=(const Phone&) = delete;
	Phone operator=(Phone&&) noexcept = delete;

	auto run(zenohc::Session& session) -> void;

private:
	/* ws->getUserData returns one of these */
	struct PerSocketData {
		/* Fill with user data */
		std::vector<std::string> topics {};
		int user {0};
	};

	// A strong type for the callback's arguments to get better intellisense support
	template <bool SSL> using WrapperSocket = uWS::WebSocket<SSL, true, PerSocketData>;

#ifdef UWS_USESSL
	using uWSWebSocket = WrapperSocket<true>;
	using uWSAppWrapper = uWS::SSLApp;
#else
	using uWSWebSocket = WrapperSocket<false>;
	using uWSAppWrapper = uWS::App;
#endif

	uWSAppWrapper m_app;

	const std::vector<std::string> m_supported_instruments;
	std::atomic_int64_t users {0};

	Serializer m_serializer;
	FBHandler m_fbhandler;
	zenohc::Subscriber m_zenoh_subscriber;

private: // Private block functions
	/**
	 * @brief Thin wrapper around publish with common args
	 * @param topic The topic to publish to
	 * @param data The data to be published
	 */
	auto publish_result(LibPhone::MEMessage&& item) noexcept -> void;

	/**
	 * @brief Called when a WebSocket connection is opened.
	 *
	 * @param ws Pointer to the WebSocket instance.
	 */
	inline auto on_ws_open(uWSWebSocket* ws) noexcept -> void;

	/**
	 * @brief Called when a WebSocket receives a message.
	 *
	 * @param ws Pointer to the WebSocket instance.
	 * @param message The received message as a string view.
	 * @param opCode The WebSocket message opcode.
	 */
	inline auto on_ws_message(uWSWebSocket* ws, std::string_view message, uWS::OpCode opCode) noexcept -> void;

	/**
	 * @brief Called when the WebSocket is ready to accept more data.
	 *
	 * @param ws Pointer to the WebSocket instance.
	 */
	inline auto on_ws_drain(uWSWebSocket* ws) noexcept -> void;

	/**
	 * @brief Called when a WebSocket receives a ping message.
	 *
	 * @param ws Pointer to the WebSocket instance.
	 * @param message The received ping message as a string view.
	 */
	inline auto on_ws_ping(uWSWebSocket* ws, std::string_view message) noexcept -> void;

	/**
	 * @brief Called when a WebSocket receives a pong message.
	 *
	 * @param ws Pointer to the WebSocket instance.
	 * @param message The received pong message as a string view.
	 */
	inline auto on_ws_pong(uWSWebSocket* ws, std::string_view message) noexcept -> void;

	/**
	 * @brief Called when a WebSocket connection is closed.
	 *
	 * @param ws Pointer to the WebSocket instance.
	 * @param code The WebSocket close code.
	 * @param message The close message as a string view.
	 */
	inline auto on_ws_close(uWSWebSocket* ws, int code, std::string_view message) noexcept -> void;
};

} // namespace LibPhone
