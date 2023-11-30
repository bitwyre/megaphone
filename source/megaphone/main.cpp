#include <iostream>
#include <libphone.hpp>

#include "depthl2_generated.h"
#include "fbhandler/fbhandler.hpp"

auto main() -> int {
	flatbuffers::FlatBufferBuilder builder;

	std::vector<flatbuffers::Offset<Bitwyre::Flatbuffers::Depthl2::AskPrice>> asks_vector {
		Bitwyre::Flatbuffers::Depthl2::CreateAskPrice(builder, 1.23, 10.0),
		Bitwyre::Flatbuffers::Depthl2::CreateAskPrice(builder, 1.24, 15.0)};

	std::vector<flatbuffers::Offset<Bitwyre::Flatbuffers::Depthl2::BidPrice>> bids_vector {
		Bitwyre::Flatbuffers::Depthl2::CreateBidPrice(builder, 1.21, 8.0),
		Bitwyre::Flatbuffers::Depthl2::CreateBidPrice(builder, 1.20, 12.0)};

	auto depth_data = Bitwyre::Flatbuffers::Depthl2::CreateDepthData(
		builder, builder.CreateString("usdt_jidr_spot"), 2913539096, false, builder.CreateVector(bids_vector),
		builder.CreateVector(asks_vector));

	auto depth_event = Bitwyre::Flatbuffers::Depthl2::CreateDepthEvent(builder, builder.CreateString("depthL2"),
																	   builder.CreateString("snapshot"), depth_data);

	builder.Finish(depth_event);

	const uint8_t* buffer = builder.GetBufferPointer();
	// int size = builder.GetSize();

	LibPhone::FBHandler fbh {};
	std::cout << fbh.flatbuf_to_json<Bitwyre::Flatbuffers::Depthl2::DepthEvent>(reinterpret_cast<const char*>(buffer))
			  << std::endl;

	return 0;
}
