// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#include "encoding.h"
#include "detail/rapidjson_helper.h"
#include "rapidjson/document.h"
#include "rapidjson/reader.h"
#include <boost/call_traits.hpp>
#include <boost/make_shared.hpp>

namespace bond
{

template <typename BufferT>
class SimpleJsonWriter;


/// @brief Reader for Simple JSON
template <typename BufferT>
class SimpleJsonReader
{
public:
    typedef BufferT                         Buffer;
    typedef DOMParser<SimpleJsonReader&>    Parser;
    typedef SimpleJsonWriter<Buffer>        Writer;
    typedef rapidjson::Value                Field;

    BOND_STATIC_CONSTEXPR uint16_t magic = SIMPLE_JSON_PROTOCOL;
    BOND_STATIC_CONSTEXPR uint16_t version = 0x0001;

    /// @brief Construct from input buffer/stream containing serialized data.
    SimpleJsonReader(typename boost::call_traits<Buffer>::param_type input)
        : _input(input),
          _stream(_input),
          _document(boost::make_shared<rapidjson::Document>()),
          _value(NULL)
    {}

    SimpleJsonReader(const SimpleJsonReader& that, const Field& value)
        : _stream(that._stream),
          _document(that._document),
          _value(&value)
    {}

    /// @brief Copy constructor
    SimpleJsonReader(const SimpleJsonReader& that)
        : _input(that._input),
          _stream(that._stream, _input),
          _document(that._document),
          _value(that._value)
    {}

    bool ReadVersion()
    {
        return false;
    }

    void Parse()
    {
        // Don't need to reparse for nested fields
        if (!_value || _value == _document.get())
        {
            const unsigned parseFlags = rapidjson::kParseIterativeFlag | rapidjson::kParseStopWhenDoneFlag;

            _document->ParseStream<parseFlags>(_stream);
            BOOST_ASSERT(!_document->HasParseError());
            _value = _document.get();
        }
    }

    const Field* FindField(uint16_t id, const Metadata& metadata, BondDataType type)
    {
        // BT_INT32 may be an enum. This allows us to decode symbolic enum values
        // when parsing using runtime schema. The assumption is that runtime schema
        // matches JSON payload. If it doesn't, nothing horrible will happen, but
        // we might not indicate a required field missing for an int32 field if we
        // mistake a string member with matching name for it.
        return FindField(id, metadata, type, type == BT_INT32);
    }

    const Field* FindField(uint16_t id, const Metadata& metadata, BondDataType type, bool is_enum);


    template <typename T>
    void Read(T& var)
    {
        detail::Read(*_value, var);
    }

    template <typename T>
    void ReadContainerBegin(uint32_t&, T&)
    {
        BOOST_ASSERT(false);
    }

    void ReadContainerEnd()
    {
        BOOST_ASSERT(false);
    }

    template <typename T>
    void Skip()
    {}

    template <typename T>
    void Skip(const T&)
    {}


    bool operator==(const SimpleJsonReader& rhs) const
    {
        return _value == rhs._value;
    }

    /// @brief Access to underlying buffer
    typename boost::call_traits<Buffer>::const_reference
    GetBuffer() const
    {
        return _input;
    }

    /// @brief Access to underlying buffer
    typename boost::call_traits<Buffer>::reference
    GetBuffer()
    {
        return _input;
    }

private:
    rapidjson::Value::ConstMemberIterator MemberBegin() const
    {
        return _value->IsObject() ? _value->MemberBegin() : rapidjson::Value::ConstMemberIterator();
    }

    rapidjson::Value::ConstMemberIterator MemberEnd() const
    {
        return _value->IsObject() ? _value->MemberEnd() : rapidjson::Value::ConstMemberIterator();
    }

    rapidjson::Value::ConstValueIterator ArrayBegin() const
    {
        return _value->IsArray() ? _value->Begin() : rapidjson::Value::ConstValueIterator();
    }

    rapidjson::Value::ConstValueIterator ArrayEnd() const
    {
        return _value->IsArray() ? _value->End() : rapidjson::Value::ConstValueIterator();
    }

    uint32_t ArraySize() const
    {
        return _value->IsArray() ? _value->Size() : 0;
    }

    template <typename Input>
    friend struct base_input;

    template <typename Protocols, typename A, typename T, typename Buffer>
    friend void DeserializeContainer(std::vector<bool, A>&, const T&, SimpleJsonReader<Buffer>&);

    template <typename Protocols, typename T, typename Buffer>
    friend void DeserializeContainer(blob&, const T&, SimpleJsonReader<Buffer>&);

    template <typename Protocols, typename X, typename T, typename Buffer>
    friend typename boost::enable_if<is_list_container<X> >::type
    DeserializeContainer(X&, const T&, SimpleJsonReader<Buffer>&);

    template <typename Protocols, typename X, typename T, typename Buffer>
    friend typename boost::enable_if<is_set_container<X> >::type
    DeserializeContainer(X&, const T&, SimpleJsonReader<Buffer>&);

    template <typename Protocols, typename X, typename T, typename Buffer>
    friend typename boost::enable_if<is_map_container<X> >::type
    DeserializeMap(X&, BondDataType, const T&, SimpleJsonReader<Buffer>&);

    SimpleJsonReader(const SimpleJsonReader& that, const char* name)
        : _stream(that._stream),
          _document(that._document),
          _value(&(*that._value)[name])
    {}


    Buffer _input;
    detail::RapidJsonInputStream<Buffer> _stream;
    boost::shared_ptr<rapidjson::Document> _document;
    const rapidjson::Value* _value;
};


template <typename Buffer>
BOND_CONSTEXPR_OR_CONST uint16_t SimpleJsonReader<Buffer>::magic;

// Disable fast pass-through optimization for Simple JSON
template <typename Input, typename Output> struct
is_protocol_same<SimpleJsonReader<Input>, SimpleJsonWriter<Output> >
    : std::false_type {};


} // namespace bond
