#pragma once

#include <string>
#include <type_traits>

#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/idl.h"
#include "flatbuffers/minireflect.h"
#include "flatbuffers/table.h"
#include "flatbuffers/util.h"

#include "depthl2_generated.h"
#include "l2_events_generated.h"
#include "l3_events_generated.h"
#include "ticker_generated.h"
#include "trades_generated.h"

#include <iostream>

template <class> inline constexpr bool always_false_v = false;

namespace LibPhone {

class FBHandler {
public:
	FBHandler() = default;
	~FBHandler() = default;

	FBHandler(const FBHandler&) = delete;
	FBHandler(FBHandler&&) noexcept = delete;

	FBHandler operator=(const FBHandler&) = delete;
	FBHandler operator=(FBHandler&&) noexcept = delete;

	template <typename flat_buf_type>
	[[nodiscard]] static auto flatbuf_to_json(const uint8_t* buf, const auto size) -> std::string {
		const flatbuffers::TypeTable* type_table = nullptr;
		flatbuffers::ToStringVisitor flatbuf_visitor {" ", true, ""};
		auto verifier = flatbuffers::Verifier(buf, size);

		auto handle_invalid_buffer = [&]() { return "Invalid buffer received, PLEASE REPORT THIS to the provider"; };

		auto process_valid_buffer = [&](auto&& data_flatbuf) {
			IterateFlatBuffer(buf, type_table, &flatbuf_visitor);
			return flatbuf_visitor.s;
		};

		if constexpr (std::is_same_v<flat_buf_type, Bitwyre::Flatbuffers::Depthl2::DepthEvent>) {
			type_table = Bitwyre::Flatbuffers::Depthl2::DepthEventTypeTable();
			if (Bitwyre::Flatbuffers::Depthl2::VerifyDepthEventBuffer(verifier)) {
				return process_valid_buffer(Bitwyre::Flatbuffers::Depthl2::GetDepthEvent(buf));
			} else {
				auto event_data = Bitwyre::Flatbuffers::Depthl2::GetDepthEvent(buf)->data();
				if (event_data->asks()->size() <= 0 || event_data->bids()->size() <= 0 || event_data->sequence() == 0) {
					return process_valid_buffer(Bitwyre::Flatbuffers::Depthl2::GetDepthEvent(buf));
				} else {
					return handle_invalid_buffer();
				}
			}
		} else if constexpr (std::is_same_v<flat_buf_type, Bitwyre::Flatbuffers::L2Event::L2Event>) {
			type_table = Bitwyre::Flatbuffers::L2Event::L2EventTypeTable();
			return Bitwyre::Flatbuffers::L2Event::VerifyL2EventBuffer(verifier) ? process_valid_buffer(nullptr)
																				: handle_invalid_buffer();
		} else if constexpr (std::is_same_v<flat_buf_type, Bitwyre::Flatbuffers::L3Event::L3Event>) {
			type_table = Bitwyre::Flatbuffers::L3Event::L3EventTypeTable();
			return Bitwyre::Flatbuffers::L3Event::VerifyL3EventBuffer(verifier) ? process_valid_buffer(nullptr)
																				: handle_invalid_buffer();
		} else if constexpr (std::is_same_v<flat_buf_type, Bitwyre::Flatbuffers::trades::trades>) {
			type_table = Bitwyre::Flatbuffers::trades::tradesTypeTable();
			return Bitwyre::Flatbuffers::trades::VerifytradesBuffer(verifier) ? process_valid_buffer(nullptr)
																			  : handle_invalid_buffer();
		} else if constexpr (std::is_same_v<flat_buf_type, Bitwyre::Flatbuffers::Ticker::TickerEvent>) {
			type_table = Bitwyre::Flatbuffers::Ticker::TickerEventTypeTable();
			return Bitwyre::Flatbuffers::Ticker::VerifyTickerEventBuffer(verifier) ? process_valid_buffer(nullptr)
																				   : handle_invalid_buffer();
		} else {
			static_assert(always_false_v<flat_buf_type>, "This type is unsupported!");
		}
	}
};

} // namespace LibPhone
