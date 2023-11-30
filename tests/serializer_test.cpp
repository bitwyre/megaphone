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

	LibPhone::Serializer sz {{"bnb_usdt_spot", "busd_usd_spot"}};
	std::string result = sz.sz_depthl2message(std::move(message));

	REQUIRE(result == expected_result);
}

using MegaphoneRequest = LibPhone::Serializer::MegaphoneRequest;

TEST_CASE("The JSON request parser function is tested with correct data", "[JSON_REQ_PARSER_NOSANITIZE]") {
	std::string_view req = R"({"op" : "subscribe", "args": ["trades:bnb_usdt_spot", "depthl2:bnb_usdt_spot"]})";

	LibPhone::Serializer sz {{"bnb_usdt_spot", "busd_usd_spot"}};
	auto [req_success, res] = sz.parse_request(req);

	REQUIRE(req_success == true);
	REQUIRE(res.operation == MegaphoneRequest::Operations::SUBSCRIBE);

	REQUIRE(res.topics[0] == "trades:bnb_usdt_spot");
	REQUIRE(res.topics[1] == "depthl2:bnb_usdt_spot");
}

TEST_CASE("The JSON request parser function is tested with incorrect data", "[JSON_REQ_PARSER_SANITIZE]") {
	std::string_view req = R"({"op" : "subscribe", "args": ["trades:bnb_usdt_spot" "depthl2:bnb_usdt_spot"]})";

	LibPhone::Serializer sz {{"bnb_usdt_spot", "busd_usd_spot"}};

	auto [req_success, res] = sz.parse_request(req);

	REQUIRE(req_success == false);
	REQUIRE(res.operation == MegaphoneRequest::Operations::NO_OP);

	req = R"({"args": ["trades:bnb_usdt_spot", "depthl2:bnb_usdt_spot"]})";
	auto [req_success_noop, res_noop] = sz.parse_request(req);

	REQUIRE(req_success_noop == false);
	REQUIRE(res_noop.operation == MegaphoneRequest::Operations::NO_OP);

	req = R"({ "op": "subscribe", "args": []})";
	auto [req_success_noargs, res_noargs] = sz.parse_request(req);

	REQUIRE(req_success_noargs == false);
	REQUIRE(res_noargs.operation == MegaphoneRequest::Operations::SUBSCRIBE);
}
