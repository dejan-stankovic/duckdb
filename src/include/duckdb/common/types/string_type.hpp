//===----------------------------------------------------------------------===//
//                         DuckDB
//
// duckdb/common/types/string_type.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb/common/constants.hpp"
#include <cstring>
#include <cassert>

namespace duckdb {

struct string_t {
	friend struct StringComparisonOperators;
	friend class StringSegment;

public:
	static constexpr idx_t PREFIX_LENGTH = 4 * sizeof(char);
	static constexpr idx_t INLINE_LENGTH = 12;

	string_t() = default;
	string_t(uint32_t len) {
		value.inlined.length = len;
		memset(value.inlined.inlined, 0, INLINE_LENGTH);
	}
	string_t(const char *data, uint32_t len) {
		value.inlined.length = len;
		assert(data || GetSize() == 0);
		if (IsInlined()) {
			// zero initialize the prefix first
			// this makes sure that strings with length smaller than 4 still have an equal prefix
			memset(value.inlined.inlined, 0, PREFIX_LENGTH);
			if (GetSize() == 0) {
				return;
			}
			// small string: inlined
			/* Note: this appears to write out-of bounds on `prefix` if `length` > `PREFIX_LENGTH`
			 but this is not the case because the `value_` union `inlined` char array directly
			 follows it with 8 more chars to use for the string value.
			 */
			memcpy(value.inlined.inlined, data, GetSize());
			value.inlined.inlined[GetSize()] = '\0';
		} else {
			// large string: store pointer
			memcpy(value.pointer.prefix, data, PREFIX_LENGTH);
			value.pointer.ptr = (char *)data;
		}
	}
	string_t(const char *data) : string_t(data, strlen(data)) {
	}
	string_t(const string &value) : string_t(value.c_str(), value.size()) {
	}

	bool IsInlined() const {
		return GetSize() < INLINE_LENGTH;
	}

	char *GetData() {
		return IsInlined() ? (char *)value.inlined.inlined : value.pointer.ptr;
	}

	const char *GetData() const {
		return IsInlined() ? (const char *)value.inlined.inlined : value.pointer.ptr;
	}

	const char *GetPrefix() const {
		return value.pointer.prefix;
	}

	idx_t GetSize() const {
		return value.inlined.length;
	}

	string GetString() const {
		return string(GetData(), GetSize());
	}

	void Finalize() {
		// set trailing NULL byte
		auto dataptr = (char *)GetData();
		dataptr[GetSize()] = '\0';
		if (GetSize() < INLINE_LENGTH) {
			// fill prefix with zeros if the length is smaller than the prefix length
			for (idx_t i = GetSize(); i < PREFIX_LENGTH; i++) {
				value.inlined.inlined[i] = '\0';
			}
		} else {
			// copy the data into the prefix
			memcpy(value.pointer.prefix, dataptr, PREFIX_LENGTH);
		}
	}

	void Verify();

private:
	union {
		struct {
			uint32_t length;
			char prefix[4];
			char *ptr;
		} pointer;
		struct {
			uint32_t length;
			char inlined[12];
		} inlined;
	} value;
};

} // namespace duckdb
