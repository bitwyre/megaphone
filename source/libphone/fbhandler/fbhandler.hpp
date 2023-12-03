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

	template <typename FlatBufType>
	[[nodiscard]] static constexpr auto flatbuf_to_json(const uint8_t* buf, const auto size) -> std::string {
		const flatbuffers::TypeTable* type_table = nullptr;
		auto verifier = flatbuffers::Verifier(buf, size);

		if constexpr (std::is_same_v<FlatBufType, Bitwyre::Flatbuffers::Depthl2::DepthEvent>) {
			type_table = Bitwyre::Flatbuffers::Depthl2::DepthEventTypeTable();
			if (Bitwyre::Flatbuffers::Depthl2::VerifyDepthEventBuffer(verifier))
				return flatbuffers::FlatBufferToString(buf, type_table);
			else
				return "Invalid buffer received, PLEASE REPORT THIS to the provider";

		} else if constexpr (std::is_same_v<FlatBufType, Bitwyre::Flatbuffers::L2Event::L2Event>) {
			type_table = Bitwyre::Flatbuffers::L2Event::L2EventTypeTable();
			if (Bitwyre::Flatbuffers::L2Event::VerifyL2EventBuffer(verifier))
				return flatbuffers::FlatBufferToString(buf, type_table);
			else
				return "Invalid buffer received, PLEASE REPORT THIS to the provider";

		} else if constexpr (std::is_same_v<FlatBufType, Bitwyre::Flatbuffers::L3Event::L3Event>) {
			type_table = Bitwyre::Flatbuffers::L3Event::L3EventTypeTable();
			if (Bitwyre::Flatbuffers::L3Event::VerifyL3EventBuffer(verifier))
				return flatbuffers::FlatBufferToString(buf, type_table);
			else
				return "Invalid buffer received, PLEASE REPORT THIS to the provider";

		} else if constexpr (std::is_same_v<FlatBufType, Bitwyre::Flatbuffers::trades::trades>) {
			type_table = Bitwyre::Flatbuffers::trades::tradesTypeTable();
			if (Bitwyre::Flatbuffers::trades::VerifytradesBuffer(verifier))
				return flatbuffers::FlatBufferToString(buf, type_table);
			else
				return "Invalid buffer received, PLEASE REPORT THIS to the provider";

		} else {
			static_assert(always_false_v<FlatBufType>, "This type is unsupported!");
		}
	}

private:
};

} // namespace LibPhone
