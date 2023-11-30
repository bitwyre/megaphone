#include <iostream>
#include <libphone.hpp>

#include "depthl2_generated.h"

auto main() -> int {
	flatbuffers::FlatBufferBuilder builder;

	std::vector<Bitwyre::Flatbuffers::Depthl2::AskPrice> asks {Bitwyre::Flatbuffers::Depthl2::AskPrice(10000.0, 1.0),
															   Bitwyre::Flatbuffers::Depthl2::AskPrice(10100.0, 2.0)};
	std::vector<Bitwyre::Flatbuffers::Depthl2::BidPrice> bids {Bitwyre::Flatbuffers::Depthl2::BidPrice(9900.0, 1.5),
															   Bitwyre::Flatbuffers::Depthl2::BidPrice(9800.0, 0.5)};

	auto depth_data = Bitwyre::Flatbuffers::Depthl2::CreateDataDirect(builder, &asks, &bids);
	auto depth_event = Bitwyre::Flatbuffers::Depthl2::CreateDepthEventDirect(builder, "BTC/USD", 123456789, depth_data);
	auto depth_event_message = Bitwyre::Flatbuffers::Depthl2::CreateDepthEventMessage(builder, depth_event, 3);

	builder.Finish(depth_event_message);

	uint8_t* buffer_data = builder.GetBufferPointer();

	auto deserialized_depth_event_message = Bitwyre::Flatbuffers::Depthl2::GetDepthEventMessage(buffer_data);

	std::cout << "Instrument: " << deserialized_depth_event_message->depth_event()->instrument()->c_str() << std::endl;
	std::cout << "Timestamp: " << deserialized_depth_event_message->depth_event()->timestamp() << std::endl;
	std::cout << "Level: " << static_cast<int>(deserialized_depth_event_message->level()) << std::endl;

	return 0;
}
