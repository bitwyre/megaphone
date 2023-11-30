#include "serializer.hpp"
#include "utils/utils.hpp"

#include <spdlog/spdlog.h>

namespace LibPhone {

auto Serializer::clear_fields() -> void {
	// Clear all the fields
	this->m_buffer.Clear();

	this->m_table.Swap(Value(kStringType).Move());
	this->m_action.Swap(Value(kStringType).Move());

	this->m_data.Swap(Value(kObjectType).Move());

	this->m_bids.Swap(Value(kArrayType).Move());
	this->m_asks.Swap(Value(kArrayType).Move());

	this->m_document.SetObject();
}

[[nodiscard]] auto Serializer::sz_depthl2message(DepthL2Message&& message) -> std::string {
	this->clear_fields();

	this->m_document.AddMember("table", this->m_table, this->m_document.GetAllocator());
	this->m_document.AddMember("action", this->m_action, this->m_document.GetAllocator());

	this->m_data.AddMember("instrument", StringRef(message.data.instrument.c_str()), this->m_document.GetAllocator());
	this->m_data.AddMember("sequence", message.data.sequence, this->m_document.GetAllocator());
	this->m_data.AddMember("is_frozen", message.data.is_frozen, this->m_document.GetAllocator());

	for (const auto& bid : message.data.bids) {
		Value bidArray(kArrayType);
		bidArray.PushBack(bid.price, this->m_document.GetAllocator());
		bidArray.PushBack(bid.quantity, this->m_document.GetAllocator());
		this->m_bids.PushBack(bidArray, this->m_document.GetAllocator());
	}
	this->m_data.AddMember("bids", this->m_bids, this->m_document.GetAllocator());

	for (const auto& ask : message.data.asks) {
		Value askArray(kArrayType);
		askArray.PushBack(ask.price, this->m_document.GetAllocator());
		askArray.PushBack(ask.quantity, this->m_document.GetAllocator());
		this->m_asks.PushBack(askArray, this->m_document.GetAllocator());
	}
	this->m_data.AddMember("asks", this->m_asks, this->m_document.GetAllocator());

	this->m_document.AddMember("data", this->m_data, this->m_document.GetAllocator());

	// Convert the document to a JSON string

	Writer<StringBuffer> writer(this->m_buffer);

	this->m_document.Accept(writer);

	// Print the JSON string
	return std::string(this->m_buffer.GetString());
}

[[nodiscard]] auto Serializer::dsz_depthl2message() -> DepthL2Message {
	this->clear_fields();

	throw std::logic_error("Attempt to call un-implemented function: Serializer::dsz_depthl2message");

	// Just reutrn an empty object for now
	// so that the compiler doesn't scream.
	return DepthL2Message {};
}

[[nodiscard]] auto Serializer::parse_request(std::string_view request) -> std::pair<bool, MegaphoneRequest> {
	this->clear_fields();

	auto request_str = std::string(utils::trim(request));
	this->m_document.Parse(request_str.c_str());

	if (this->m_document.HasParseError()) {
		SPDLOG_ERROR("Error parsing JSON request: {}", request_str);
		return {};
	}

	auto result = MegaphoneRequest {};

	if (!this->m_document.HasMember("op") || !this->m_document["op"].IsString()) {
		SPDLOG_ERROR("Missing or invalid 'op' field in the JSON request");
		return {false, result};
	}

	std::string_view operation = this->m_document["op"].GetString();
	result.operation =
		(operation == "subscribe") ? MegaphoneRequest::Operations::SUBSCRIBE : MegaphoneRequest::Operations::NO_OP;

	if (result.operation == MegaphoneRequest::Operations::NO_OP) {
		SPDLOG_ERROR("Invalid value for 'op' field in the JSON request: {}", operation);
		return {false, result};
	}

	if (this->m_document.HasMember("args") && this->m_document["args"].IsArray()) {
		auto args = this->m_document["args"].GetArray();

		if (args.Size() <= 0) {
			return {false, result};
		}

		// Iterate through the array and add each string to the vector
		for (auto& arg : args) {
			if (arg.IsString()) {
				// chop the arg into 2 parts and check if the instrument is supported
				std::string_view arg_strview = arg.GetString();

				size_t pos = arg_strview.find(':');
				if (pos != std::string::npos) {
					std::string_view instrument = arg_strview.substr(pos + 1);

					bool topic_exists =
						std::find(this->m_supported_instruments.begin(), this->m_supported_instruments.end(),
								  instrument) != this->m_supported_instruments.end();

					if (!topic_exists) {
						SPDLOG_ERROR("The requested instrument does not exist: {}", instrument);
						return {false, result};
					}

				} else {
					SPDLOG_ERROR("Invalid requiest: ", request_str);
				}

				// Push it back to the object
				result.topics.emplace_back(arg_strview);
			} else {
				return {false, result};
			}
		}
	}

	return {true, result};
}

} // namespace LibPhone
