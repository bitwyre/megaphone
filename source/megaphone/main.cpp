#include <libphone.hpp>

#include "serializer/serializer.hpp"

auto main() -> int {

	// Create an instance of the data structures
	LibPhone::Serializer::DepthL2Message message {};

	message.table = "depthL2";
	message.action = "snapshot";

	message.data.instrument = "usdt_jidr_spot";
	message.data.sequence = 2913539096;
	message.data.is_frozen = false;

	message.data.bids.push_back({15862.000000, 2.420000});
	message.data.bids.push_back({15861.000000, 2.080000});

	message.data.asks.push_back({15879.000000, 2.320000});
	message.data.asks.push_back({15892.000000, 2.020000});

	LibPhone::Serializer serializer {};

	std::cout << serializer.sz_depthl2message(std::move(message)) << std::endl;

	// LibPhone::Phone p {};
	// p.run();
}
