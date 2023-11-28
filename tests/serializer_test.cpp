#include <catch2/catch_test_macros.hpp>

#include "serializer/serializer.hpp"

TEST_CASE("The JSON serializer is tested", "[JSON_SZ]") {
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

	std::string expected_result =
		R"({"table":"","action":"","data":{"instrument":"usdt_jidr_spot","sequence":2913539096,"is_frozen":false,"bids":[[15862.0,2.42],[15861.0,2.08]],"asks":[[15879.0,2.32],[15892.0,2.02]]}})";

	LibPhone::Serializer sz {};
	std::string result = sz.sz_depthl2message(std::move(message));

	REQUIRE(result == expected_result);
}
