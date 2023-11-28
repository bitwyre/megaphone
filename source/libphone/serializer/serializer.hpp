#pragma once

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include <cstddef>
#include <string>
#include <vector>

using namespace rapidjson;

namespace LibPhone {

class Serializer {
public:
	struct DepthL2Message {
		struct Data {
			std::string instrument;
			uint64_t sequence;
			bool is_frozen;

			struct BidAsk {
				double price;
				double quantity;
			};

			std::vector<BidAsk> bids;
			std::vector<BidAsk> asks;
		};

		std::string table;
		std::string action;
		Data data;
	};

	// TODO: Implement this function
	// such that it takes in a flatbuffer and returns
	// a fully populated DepthL2Message object.
	[[nodiscard]] auto dsz_depthl2message() -> DepthL2Message;
	[[nodiscard]] auto sz_depthl2message(DepthL2Message&& message) -> std::string;

	Serializer()
		: m_document(),
		  m_buffer(),
		  m_table(kStringType),
		  m_action(kStringType),
		  m_data(kObjectType),
		  m_bids(kArrayType),
		  m_asks(kArrayType) { }

private:
	Document m_document;
	StringBuffer m_buffer;

	Value m_table;	// Set the "table" field
	Value m_action; // Set the "action" field

	Value m_data; // Set the "data" field

	Value m_bids; // Set the "bids" array
	Value m_asks; // Set the "asks" array

	auto clear_fields() -> void;
};
} // namespace LibPhone
