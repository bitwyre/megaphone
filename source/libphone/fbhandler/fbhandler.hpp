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
	[[nodiscard]] static constexpr auto flatbuf_to_json(const std::string& flatbuf) noexcept -> std::string {
		const flatbuffers::TypeTable* type_table = nullptr;

		if constexpr (std::is_same_v<FlatBufType, Bitwyre::Flatbuffers::Depthl2::DepthEvent>)
			type_table = Bitwyre::Flatbuffers::Depthl2::DepthEventTypeTable();
		else if constexpr (std::is_same_v<FlatBufType, Bitwyre::Flatbuffers::L2Event::L2Event>)
			type_table = Bitwyre::Flatbuffers::L2Event::L2EventTypeTable();
		else if constexpr (std::is_same_v<FlatBufType, Bitwyre::Flatbuffers::L3Event::L3Event>)
			type_table = Bitwyre::Flatbuffers::L3Event::L3EventTypeTable();
		else if constexpr (std::is_same_v<FlatBufType, Bitwyre::Flatbuffers::trades::trades>)
			type_table = Bitwyre::Flatbuffers::trades::tradesTypeTable();
		else
			static_assert(always_false_v<FlatBufType>, "This type is unsupported!");

		return flatbuffers::FlatBufferToString(reinterpret_cast<const uint8_t*>(flatbuf.c_str()), type_table);
	}

private:
};

} // namespace LibPhone
