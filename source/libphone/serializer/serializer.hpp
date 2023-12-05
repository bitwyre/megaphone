#pragma once

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "utils/utils.hpp"

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

	// this object is only constructed
	// like once per request so it's
	// fine if it's a bit heavy
	struct MegaphoneRequest {
		enum class Operations { SUBSCRIBE, NO_OP };

		Operations operation {Operations::NO_OP};
		std::vector<std::string> topics;
	};

public: // Methods
	explicit Serializer()
		: m_document(),
		  m_supported_instruments(utils::ENVManager::get_instance().get_megaphone_supported_instruments()) { }

	Serializer(const Serializer&) = delete;
	Serializer(Serializer&&) noexcept = delete;

	Serializer operator=(const Serializer&) = delete;
	Serializer operator=(Serializer&&) noexcept = delete;

	[[nodiscard]] auto parse_request(const std::string_view request) -> std::pair<bool, MegaphoneRequest>;

private:
	Document m_document;
	std::vector<std::string> m_supported_instruments;

	auto clear_fields() -> void;
};
} // namespace LibPhone
