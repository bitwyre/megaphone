#include "serializer.hpp"
#include "utils/utils.hpp"

#include <spdlog/spdlog.h>

namespace LibPhone {

auto Serializer::clear_fields() -> void {
	// Clear all the fields
	this->m_document.SetObject();
}

[[nodiscard]] auto Serializer::parse_request(const std::string_view request) -> std::pair<bool, MegaphoneRequest> {
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
