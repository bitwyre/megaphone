#include <catch2/catch_test_macros.hpp>

#include "depthl2_generated.h"

#include "fbhandler/fbhandler.hpp"
#include "serializer/serializer.hpp"

#include <string>
using namespace std::literals;

using MegaphoneRequest = LibPhone::Serializer::MegaphoneRequest;

TEST_CASE("The JSON request parser function is tested with correct data", "[JSON_REQ_PARSER_NOSANITIZE]") {
	std::string_view req = R"({"op" : "subscribe", "args": ["trades:btd_usdt_spot", "depthl2:btd_usdt_spot"]})";

	LibPhone::Serializer sz;
	auto [req_success, res] = sz.parse_request(req);

	REQUIRE(req_success == true);
	REQUIRE(res.operation == MegaphoneRequest::Operations::SUBSCRIBE);

	REQUIRE(res.topics[0] == "trades:bnb_usdt_spot");
	REQUIRE(res.topics[1] == "depthl2:bnb_usdt_spot");
}

TEST_CASE("The JSON request parser function is tested with incorrect data", "[JSON_REQ_PARSER_SANITIZE]") {
	std::string_view req = R"({"op" : "subscribe", "args": ["trades:bnb_usdt_spot" "depthl2:bnb_usdt_spot"]})";

	LibPhone::Serializer sz;

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

TEST_CASE("Tests the FBH Class for FlatBuffer to JSON conversion", "[JSON_FBH]") {
	flatbuffers::FlatBufferBuilder builder;

	std::vector<flatbuffers::Offset<Bitwyre::Flatbuffers::Depthl2::AskPrice>> asks_vector {
		Bitwyre::Flatbuffers::Depthl2::CreateAskPrice(builder, builder.CreateString("1.23"),
													  builder.CreateString("10.0")),
		Bitwyre::Flatbuffers::Depthl2::CreateAskPrice(builder, builder.CreateString("1.24"),
													  builder.CreateString("15.0"))};

	std::vector<flatbuffers::Offset<Bitwyre::Flatbuffers::Depthl2::BidPrice>> bids_vector {
		Bitwyre::Flatbuffers::Depthl2::CreateBidPrice(builder, builder.CreateString("1.21"),
													  builder.CreateString("8.0")),
		Bitwyre::Flatbuffers::Depthl2::CreateBidPrice(builder, builder.CreateString("1.20"),
													  builder.CreateString("12.0"))};

	auto depth_data = Bitwyre::Flatbuffers::Depthl2::CreateDepthData(
		builder, builder.CreateString("usdt_jidr_spot"), 2913539096, false, builder.CreateVector(bids_vector),
		builder.CreateVector(asks_vector));

	auto depth_event = Bitwyre::Flatbuffers::Depthl2::CreateDepthEvent(builder, builder.CreateString("depthL2"),
																	   builder.CreateString("snapshot"), depth_data);

	builder.Finish(depth_event);

	REQUIRE(
		LibPhone::FBHandler::flatbuf_to_json<Bitwyre::Flatbuffers::Depthl2::DepthEvent>(builder.GetBufferPointer(),
																						builder.GetSize()) ==
		R"({ "table": "depthL2", "action": "snapshot", "data": { "instrument": "usdt_jidr_spot", "sequence": 2913539096, "bids": [ { "price": 1.21, "qty": 8.0 }, { "price": 1.2, "qty": 12.0 } ], "asks": [ { "price": 1.23, "qty": 10.0 }, { "price": 1.24, "qty": 15.0 } ] } })");
}
