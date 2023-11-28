#include "serializer.hpp"

namespace LibPhone {

auto Serializer::clear_fields() -> void {
	// Clear all the fields
	this->m_buffer.Clear();

	this->m_table.Swap(Value(kStringType).Move());
	this->m_action.Swap(Value(kStringType).Move());

	this->m_data.Swap(Value(kObjectType).Move());

	this->m_bids.Swap(Value(kArrayType).Move());
	this->m_asks.Swap(Value(kArrayType).Move());
}

[[nodiscard]] auto Serializer::sz_depthl2message(DepthL2Message&& message) -> std::string {
	this->clear_fields();

	this->m_document.SetObject();

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
	throw std::logic_error("Attempt to call un-implemented function: Serializer::dsz_depthl2message");

	// Just reutrn an empty object for now
	// so that the compiler doesn't scream.
	return DepthL2Message {};
}
} // namespace LibPhone
