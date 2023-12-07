// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_DEPTHL2_BITWYRE_FLATBUFFERS_DEPTHL2_H_
#define FLATBUFFERS_GENERATED_DEPTHL2_BITWYRE_FLATBUFFERS_DEPTHL2_H_

#include "flatbuffers/flatbuffers.h"

// Ensure the included flatbuffers.h is the same version as when this file was
// generated, otherwise it may not be compatible.
static_assert(FLATBUFFERS_VERSION_MAJOR == 23 &&
              FLATBUFFERS_VERSION_MINOR == 5 &&
              FLATBUFFERS_VERSION_REVISION == 26,
             "Non-compatible flatbuffers version included");

namespace Bitwyre {
namespace Flatbuffers {
namespace Depthl2 {

struct AskPrice;
struct AskPriceBuilder;

struct BidPrice;
struct BidPriceBuilder;

struct DepthData;
struct DepthDataBuilder;

struct DepthEvent;
struct DepthEventBuilder;

inline const ::flatbuffers::TypeTable *AskPriceTypeTable();

inline const ::flatbuffers::TypeTable *BidPriceTypeTable();

inline const ::flatbuffers::TypeTable *DepthDataTypeTable();

inline const ::flatbuffers::TypeTable *DepthEventTypeTable();

struct AskPrice FLATBUFFERS_FINAL_CLASS : private ::flatbuffers::Table {
  typedef AskPriceBuilder Builder;
  struct Traits;
  static const ::flatbuffers::TypeTable *MiniReflectTypeTable() {
    return AskPriceTypeTable();
  }
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_PRICE = 4,
    VT_QTY = 6
  };
  const ::flatbuffers::String *price() const {
    return GetPointer<const ::flatbuffers::String *>(VT_PRICE);
  }
  const ::flatbuffers::String *qty() const {
    return GetPointer<const ::flatbuffers::String *>(VT_QTY);
  }
  bool Verify(::flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_PRICE) &&
           verifier.VerifyString(price()) &&
           VerifyOffset(verifier, VT_QTY) &&
           verifier.VerifyString(qty()) &&
           verifier.EndTable();
  }
};

struct AskPriceBuilder {
  typedef AskPrice Table;
  ::flatbuffers::FlatBufferBuilder &fbb_;
  ::flatbuffers::uoffset_t start_;
  void add_price(::flatbuffers::Offset<::flatbuffers::String> price) {
    fbb_.AddOffset(AskPrice::VT_PRICE, price);
  }
  void add_qty(::flatbuffers::Offset<::flatbuffers::String> qty) {
    fbb_.AddOffset(AskPrice::VT_QTY, qty);
  }
  explicit AskPriceBuilder(::flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  ::flatbuffers::Offset<AskPrice> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = ::flatbuffers::Offset<AskPrice>(end);
    return o;
  }
};

inline ::flatbuffers::Offset<AskPrice> CreateAskPrice(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    ::flatbuffers::Offset<::flatbuffers::String> price = 0,
    ::flatbuffers::Offset<::flatbuffers::String> qty = 0) {
  AskPriceBuilder builder_(_fbb);
  builder_.add_qty(qty);
  builder_.add_price(price);
  return builder_.Finish();
}

struct AskPrice::Traits {
  using type = AskPrice;
  static auto constexpr Create = CreateAskPrice;
};

inline ::flatbuffers::Offset<AskPrice> CreateAskPriceDirect(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    const char *price = nullptr,
    const char *qty = nullptr) {
  auto price__ = price ? _fbb.CreateString(price) : 0;
  auto qty__ = qty ? _fbb.CreateString(qty) : 0;
  return Bitwyre::Flatbuffers::Depthl2::CreateAskPrice(
      _fbb,
      price__,
      qty__);
}

struct BidPrice FLATBUFFERS_FINAL_CLASS : private ::flatbuffers::Table {
  typedef BidPriceBuilder Builder;
  struct Traits;
  static const ::flatbuffers::TypeTable *MiniReflectTypeTable() {
    return BidPriceTypeTable();
  }
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_PRICE = 4,
    VT_QTY = 6
  };
  const ::flatbuffers::String *price() const {
    return GetPointer<const ::flatbuffers::String *>(VT_PRICE);
  }
  const ::flatbuffers::String *qty() const {
    return GetPointer<const ::flatbuffers::String *>(VT_QTY);
  }
  bool Verify(::flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_PRICE) &&
           verifier.VerifyString(price()) &&
           VerifyOffset(verifier, VT_QTY) &&
           verifier.VerifyString(qty()) &&
           verifier.EndTable();
  }
};

struct BidPriceBuilder {
  typedef BidPrice Table;
  ::flatbuffers::FlatBufferBuilder &fbb_;
  ::flatbuffers::uoffset_t start_;
  void add_price(::flatbuffers::Offset<::flatbuffers::String> price) {
    fbb_.AddOffset(BidPrice::VT_PRICE, price);
  }
  void add_qty(::flatbuffers::Offset<::flatbuffers::String> qty) {
    fbb_.AddOffset(BidPrice::VT_QTY, qty);
  }
  explicit BidPriceBuilder(::flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  ::flatbuffers::Offset<BidPrice> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = ::flatbuffers::Offset<BidPrice>(end);
    return o;
  }
};

inline ::flatbuffers::Offset<BidPrice> CreateBidPrice(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    ::flatbuffers::Offset<::flatbuffers::String> price = 0,
    ::flatbuffers::Offset<::flatbuffers::String> qty = 0) {
  BidPriceBuilder builder_(_fbb);
  builder_.add_qty(qty);
  builder_.add_price(price);
  return builder_.Finish();
}

struct BidPrice::Traits {
  using type = BidPrice;
  static auto constexpr Create = CreateBidPrice;
};

inline ::flatbuffers::Offset<BidPrice> CreateBidPriceDirect(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    const char *price = nullptr,
    const char *qty = nullptr) {
  auto price__ = price ? _fbb.CreateString(price) : 0;
  auto qty__ = qty ? _fbb.CreateString(qty) : 0;
  return Bitwyre::Flatbuffers::Depthl2::CreateBidPrice(
      _fbb,
      price__,
      qty__);
}

struct DepthData FLATBUFFERS_FINAL_CLASS : private ::flatbuffers::Table {
  typedef DepthDataBuilder Builder;
  struct Traits;
  static const ::flatbuffers::TypeTable *MiniReflectTypeTable() {
    return DepthDataTypeTable();
  }
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_INSTRUMENT = 4,
    VT_SEQUENCE = 6,
    VT_IS_FROZEN = 8,
    VT_BIDS = 10,
    VT_ASKS = 12
  };
  const ::flatbuffers::String *instrument() const {
    return GetPointer<const ::flatbuffers::String *>(VT_INSTRUMENT);
  }
  uint64_t sequence() const {
    return GetField<uint64_t>(VT_SEQUENCE, 0);
  }
  bool is_frozen() const {
    return GetField<uint8_t>(VT_IS_FROZEN, 0) != 0;
  }
  const ::flatbuffers::Vector<::flatbuffers::Offset<Bitwyre::Flatbuffers::Depthl2::BidPrice>> *bids() const {
    return GetPointer<const ::flatbuffers::Vector<::flatbuffers::Offset<Bitwyre::Flatbuffers::Depthl2::BidPrice>> *>(VT_BIDS);
  }
  const ::flatbuffers::Vector<::flatbuffers::Offset<Bitwyre::Flatbuffers::Depthl2::AskPrice>> *asks() const {
    return GetPointer<const ::flatbuffers::Vector<::flatbuffers::Offset<Bitwyre::Flatbuffers::Depthl2::AskPrice>> *>(VT_ASKS);
  }
  bool Verify(::flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_INSTRUMENT) &&
           verifier.VerifyString(instrument()) &&
           VerifyField<uint64_t>(verifier, VT_SEQUENCE, 8) &&
           VerifyField<uint8_t>(verifier, VT_IS_FROZEN, 1) &&
           VerifyOffset(verifier, VT_BIDS) &&
           verifier.VerifyVector(bids()) &&
           verifier.VerifyVectorOfTables(bids()) &&
           VerifyOffset(verifier, VT_ASKS) &&
           verifier.VerifyVector(asks()) &&
           verifier.VerifyVectorOfTables(asks()) &&
           verifier.EndTable();
  }
};

struct DepthDataBuilder {
  typedef DepthData Table;
  ::flatbuffers::FlatBufferBuilder &fbb_;
  ::flatbuffers::uoffset_t start_;
  void add_instrument(::flatbuffers::Offset<::flatbuffers::String> instrument) {
    fbb_.AddOffset(DepthData::VT_INSTRUMENT, instrument);
  }
  void add_sequence(uint64_t sequence) {
    fbb_.AddElement<uint64_t>(DepthData::VT_SEQUENCE, sequence, 0);
  }
  void add_is_frozen(bool is_frozen) {
    fbb_.AddElement<uint8_t>(DepthData::VT_IS_FROZEN, static_cast<uint8_t>(is_frozen), 0);
  }
  void add_bids(::flatbuffers::Offset<::flatbuffers::Vector<::flatbuffers::Offset<Bitwyre::Flatbuffers::Depthl2::BidPrice>>> bids) {
    fbb_.AddOffset(DepthData::VT_BIDS, bids);
  }
  void add_asks(::flatbuffers::Offset<::flatbuffers::Vector<::flatbuffers::Offset<Bitwyre::Flatbuffers::Depthl2::AskPrice>>> asks) {
    fbb_.AddOffset(DepthData::VT_ASKS, asks);
  }
  explicit DepthDataBuilder(::flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  ::flatbuffers::Offset<DepthData> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = ::flatbuffers::Offset<DepthData>(end);
    return o;
  }
};

inline ::flatbuffers::Offset<DepthData> CreateDepthData(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    ::flatbuffers::Offset<::flatbuffers::String> instrument = 0,
    uint64_t sequence = 0,
    bool is_frozen = false,
    ::flatbuffers::Offset<::flatbuffers::Vector<::flatbuffers::Offset<Bitwyre::Flatbuffers::Depthl2::BidPrice>>> bids = 0,
    ::flatbuffers::Offset<::flatbuffers::Vector<::flatbuffers::Offset<Bitwyre::Flatbuffers::Depthl2::AskPrice>>> asks = 0) {
  DepthDataBuilder builder_(_fbb);
  builder_.add_sequence(sequence);
  builder_.add_asks(asks);
  builder_.add_bids(bids);
  builder_.add_instrument(instrument);
  builder_.add_is_frozen(is_frozen);
  return builder_.Finish();
}

struct DepthData::Traits {
  using type = DepthData;
  static auto constexpr Create = CreateDepthData;
};

inline ::flatbuffers::Offset<DepthData> CreateDepthDataDirect(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    const char *instrument = nullptr,
    uint64_t sequence = 0,
    bool is_frozen = false,
    const std::vector<::flatbuffers::Offset<Bitwyre::Flatbuffers::Depthl2::BidPrice>> *bids = nullptr,
    const std::vector<::flatbuffers::Offset<Bitwyre::Flatbuffers::Depthl2::AskPrice>> *asks = nullptr) {
  auto instrument__ = instrument ? _fbb.CreateString(instrument) : 0;
  auto bids__ = bids ? _fbb.CreateVector<::flatbuffers::Offset<Bitwyre::Flatbuffers::Depthl2::BidPrice>>(*bids) : 0;
  auto asks__ = asks ? _fbb.CreateVector<::flatbuffers::Offset<Bitwyre::Flatbuffers::Depthl2::AskPrice>>(*asks) : 0;
  return Bitwyre::Flatbuffers::Depthl2::CreateDepthData(
      _fbb,
      instrument__,
      sequence,
      is_frozen,
      bids__,
      asks__);
}

struct DepthEvent FLATBUFFERS_FINAL_CLASS : private ::flatbuffers::Table {
  typedef DepthEventBuilder Builder;
  struct Traits;
  static const ::flatbuffers::TypeTable *MiniReflectTypeTable() {
    return DepthEventTypeTable();
  }
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_TABLE = 4,
    VT_ACTION = 6,
    VT_DATA = 8
  };
  const ::flatbuffers::String *table() const {
    return GetPointer<const ::flatbuffers::String *>(VT_TABLE);
  }
  const ::flatbuffers::String *action() const {
    return GetPointer<const ::flatbuffers::String *>(VT_ACTION);
  }
  const Bitwyre::Flatbuffers::Depthl2::DepthData *data() const {
    return GetPointer<const Bitwyre::Flatbuffers::Depthl2::DepthData *>(VT_DATA);
  }
  bool Verify(::flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_TABLE) &&
           verifier.VerifyString(table()) &&
           VerifyOffset(verifier, VT_ACTION) &&
           verifier.VerifyString(action()) &&
           VerifyOffset(verifier, VT_DATA) &&
           verifier.VerifyTable(data()) &&
           verifier.EndTable();
  }
};

struct DepthEventBuilder {
  typedef DepthEvent Table;
  ::flatbuffers::FlatBufferBuilder &fbb_;
  ::flatbuffers::uoffset_t start_;
  void add_table(::flatbuffers::Offset<::flatbuffers::String> table) {
    fbb_.AddOffset(DepthEvent::VT_TABLE, table);
  }
  void add_action(::flatbuffers::Offset<::flatbuffers::String> action) {
    fbb_.AddOffset(DepthEvent::VT_ACTION, action);
  }
  void add_data(::flatbuffers::Offset<Bitwyre::Flatbuffers::Depthl2::DepthData> data) {
    fbb_.AddOffset(DepthEvent::VT_DATA, data);
  }
  explicit DepthEventBuilder(::flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  ::flatbuffers::Offset<DepthEvent> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = ::flatbuffers::Offset<DepthEvent>(end);
    return o;
  }
};

inline ::flatbuffers::Offset<DepthEvent> CreateDepthEvent(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    ::flatbuffers::Offset<::flatbuffers::String> table = 0,
    ::flatbuffers::Offset<::flatbuffers::String> action = 0,
    ::flatbuffers::Offset<Bitwyre::Flatbuffers::Depthl2::DepthData> data = 0) {
  DepthEventBuilder builder_(_fbb);
  builder_.add_data(data);
  builder_.add_action(action);
  builder_.add_table(table);
  return builder_.Finish();
}

struct DepthEvent::Traits {
  using type = DepthEvent;
  static auto constexpr Create = CreateDepthEvent;
};

inline ::flatbuffers::Offset<DepthEvent> CreateDepthEventDirect(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    const char *table = nullptr,
    const char *action = nullptr,
    ::flatbuffers::Offset<Bitwyre::Flatbuffers::Depthl2::DepthData> data = 0) {
  auto table__ = table ? _fbb.CreateString(table) : 0;
  auto action__ = action ? _fbb.CreateString(action) : 0;
  return Bitwyre::Flatbuffers::Depthl2::CreateDepthEvent(
      _fbb,
      table__,
      action__,
      data);
}

inline const ::flatbuffers::TypeTable *AskPriceTypeTable() {
  static const ::flatbuffers::TypeCode type_codes[] = {
    { ::flatbuffers::ET_STRING, 0, -1 },
    { ::flatbuffers::ET_STRING, 0, -1 }
  };
  static const char * const names[] = {
    "price",
    "qty"
  };
  static const ::flatbuffers::TypeTable tt = {
    ::flatbuffers::ST_TABLE, 2, type_codes, nullptr, nullptr, nullptr, names
  };
  return &tt;
}

inline const ::flatbuffers::TypeTable *BidPriceTypeTable() {
  static const ::flatbuffers::TypeCode type_codes[] = {
    { ::flatbuffers::ET_STRING, 0, -1 },
    { ::flatbuffers::ET_STRING, 0, -1 }
  };
  static const char * const names[] = {
    "price",
    "qty"
  };
  static const ::flatbuffers::TypeTable tt = {
    ::flatbuffers::ST_TABLE, 2, type_codes, nullptr, nullptr, nullptr, names
  };
  return &tt;
}

inline const ::flatbuffers::TypeTable *DepthDataTypeTable() {
  static const ::flatbuffers::TypeCode type_codes[] = {
    { ::flatbuffers::ET_STRING, 0, -1 },
    { ::flatbuffers::ET_ULONG, 0, -1 },
    { ::flatbuffers::ET_BOOL, 0, -1 },
    { ::flatbuffers::ET_SEQUENCE, 1, 0 },
    { ::flatbuffers::ET_SEQUENCE, 1, 1 }
  };
  static const ::flatbuffers::TypeFunction type_refs[] = {
    Bitwyre::Flatbuffers::Depthl2::BidPriceTypeTable,
    Bitwyre::Flatbuffers::Depthl2::AskPriceTypeTable
  };
  static const char * const names[] = {
    "instrument",
    "sequence",
    "is_frozen",
    "bids",
    "asks"
  };
  static const ::flatbuffers::TypeTable tt = {
    ::flatbuffers::ST_TABLE, 5, type_codes, type_refs, nullptr, nullptr, names
  };
  return &tt;
}

inline const ::flatbuffers::TypeTable *DepthEventTypeTable() {
  static const ::flatbuffers::TypeCode type_codes[] = {
    { ::flatbuffers::ET_STRING, 0, -1 },
    { ::flatbuffers::ET_STRING, 0, -1 },
    { ::flatbuffers::ET_SEQUENCE, 0, 0 }
  };
  static const ::flatbuffers::TypeFunction type_refs[] = {
    Bitwyre::Flatbuffers::Depthl2::DepthDataTypeTable
  };
  static const char * const names[] = {
    "table",
    "action",
    "data"
  };
  static const ::flatbuffers::TypeTable tt = {
    ::flatbuffers::ST_TABLE, 3, type_codes, type_refs, nullptr, nullptr, names
  };
  return &tt;
}

inline const Bitwyre::Flatbuffers::Depthl2::DepthEvent *GetDepthEvent(const void *buf) {
  return ::flatbuffers::GetRoot<Bitwyre::Flatbuffers::Depthl2::DepthEvent>(buf);
}

inline const Bitwyre::Flatbuffers::Depthl2::DepthEvent *GetSizePrefixedDepthEvent(const void *buf) {
  return ::flatbuffers::GetSizePrefixedRoot<Bitwyre::Flatbuffers::Depthl2::DepthEvent>(buf);
}

inline bool VerifyDepthEventBuffer(
    ::flatbuffers::Verifier &verifier) {
  return verifier.VerifyBuffer<Bitwyre::Flatbuffers::Depthl2::DepthEvent>(nullptr);
}

inline bool VerifySizePrefixedDepthEventBuffer(
    ::flatbuffers::Verifier &verifier) {
  return verifier.VerifySizePrefixedBuffer<Bitwyre::Flatbuffers::Depthl2::DepthEvent>(nullptr);
}

inline void FinishDepthEventBuffer(
    ::flatbuffers::FlatBufferBuilder &fbb,
    ::flatbuffers::Offset<Bitwyre::Flatbuffers::Depthl2::DepthEvent> root) {
  fbb.Finish(root);
}

inline void FinishSizePrefixedDepthEventBuffer(
    ::flatbuffers::FlatBufferBuilder &fbb,
    ::flatbuffers::Offset<Bitwyre::Flatbuffers::Depthl2::DepthEvent> root) {
  fbb.FinishSizePrefixed(root);
}

}  // namespace Depthl2
}  // namespace Flatbuffers
}  // namespace Bitwyre

#endif  // FLATBUFFERS_GENERATED_DEPTHL2_BITWYRE_FLATBUFFERS_DEPTHL2_H_
