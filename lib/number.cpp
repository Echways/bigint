#include "number.h"

#include <cstdint>
#include <cstring>
#include <ostream>
#include <stdexcept>

constexpr int kNumBytes = 35;
constexpr int kDataBits = kNumBytes * 7;
constexpr int kSignificantBits = 239;
constexpr int kPaddingBits = kDataBits - kSignificantBits;
constexpr std::uint64_t kShiftMask35 = (1ULL << 35) - 1ULL;

std::uint64_t _getShift64(const uint239_t& value) {
	std::uint64_t shift = 0;
	for (int byte_index = 0; byte_index < kNumBytes; ++byte_index) {
		shift <<= 1;
		shift |= static_cast<std::uint64_t>((value.data[byte_index] >> 7) & 1U);
	}
	return shift & kShiftMask35;
}

bool _setShift64(uint239_t& value, std::uint64_t shift) {
	shift &= kShiftMask35;
	for (int byte_index = 0; byte_index < kNumBytes; ++byte_index) {
		value.data[byte_index] &= static_cast<std::uint8_t>(0x7F);
	}
	for (int byte_index = kNumBytes - 1; byte_index >= 0; --byte_index) {
		if (shift & 1ULL) {
			value.data[byte_index] |= 0x80;
		}
		shift >>= 1;
	}
	return 0;
}

bool _extractStoredDataBits(const uint239_t& value, std::uint8_t stored_bits[kDataBits]) {
	int bit_index = 0;
	for (int byte_index = 0; byte_index < kNumBytes; ++byte_index) {
		std::uint8_t data_byte = static_cast<std::uint8_t>(value.data[byte_index] & 0x7F);
		for (int bit = 6; bit >= 0; --bit) {
			stored_bits[bit_index++] = static_cast<std::uint8_t>((data_byte >> bit) & 1U);
		}
	}
	return 0;
}

bool _storeDataBits(uint239_t& value, const std::uint8_t stored_bits[kDataBits]) {
	int bit_index = 0;
	for (int byte_index = 0; byte_index < kNumBytes; ++byte_index) {
		std::uint8_t data_byte = 0;
		for (int bit = 6; bit >= 0; --bit) {
			data_byte |= static_cast<std::uint8_t>(stored_bits[bit_index++] << bit);
		}
		std::uint8_t shift_bit = static_cast<std::uint8_t>(value.data[byte_index] & 0x80);
		value.data[byte_index] = static_cast<std::uint8_t>(data_byte | shift_bit);
	}
	return 0;
}

bool _rotateLeftData(std::uint8_t bits[kDataBits], std::uint64_t shift) {
	if (kDataBits == 0) return 0;
	int shift_mod = static_cast<int>(shift % kDataBits);
	if (shift_mod == 0) return 0;
	std::uint8_t rotated[kDataBits];
	for (int i = 0; i < kDataBits; ++i) {
		int from_index = i + shift_mod;
		if (from_index >= kDataBits) {
			from_index -= kDataBits;
		}
		rotated[i] = bits[from_index];
	}
	std::memcpy(bits, rotated, sizeof(rotated));
	return 0;
}

bool _rotateRightData(std::uint8_t bits[kDataBits], std::uint64_t shift) {
	if (kDataBits == 0) return 0;
	int shift_mod = static_cast<int>(shift % kDataBits);
	if (shift_mod == 0) return 0;
	std::uint8_t rotated[kDataBits];
	for (int i = 0; i < kDataBits; ++i) {
		int from_index = i - shift_mod;
		if (from_index < 0) {
			from_index += kDataBits;
		}
		rotated[i] = bits[from_index];
	}
	std::memcpy(bits, rotated, sizeof(rotated));
	return 0;
}

bool _decodeToBitsLe(const uint239_t& value, std::uint8_t bits_le[kSignificantBits], std::uint64_t& out_shift) {
	std::uint64_t shift = _getShift64(value);
	out_shift = shift;
	std::uint8_t stored_bits[kDataBits];
	_extractStoredDataBits(value, stored_bits);
	std::uint8_t canonical_bits[kDataBits];
	std::memcpy(canonical_bits, stored_bits, sizeof(canonical_bits));
	_rotateRightData(canonical_bits, shift);
	for (int i = 0; i < kSignificantBits; ++i) {
		int bit_in_canonical = kPaddingBits + (kSignificantBits - 1 - i);
		bits_le[i] = canonical_bits[bit_in_canonical];
	}
	return 0;
}

uint239_t _encodeFromBitsLe(const std::uint8_t bits_le[kSignificantBits], std::uint64_t shift) {
	uint239_t result;
	std::memset(result.data, 0, sizeof(result.data));
	std::uint8_t canonical_bits[kDataBits] = {0};
	for (int i = 0; i < kSignificantBits; ++i) {
		int bit_in_canonical = kPaddingBits + (kSignificantBits - 1 - i);
		canonical_bits[bit_in_canonical] = bits_le[i];
	}
	std::uint8_t stored_bits[kDataBits];
	std::memcpy(stored_bits, canonical_bits, sizeof(stored_bits));
	_rotateLeftData(stored_bits, shift);
	int bit_index = 0;
	for (int byte_index = 0; byte_index < kNumBytes; ++byte_index) {
		std::uint8_t data_byte = 0;
		for (int bit = 6; bit >= 0; --bit) {
			data_byte |= static_cast<std::uint8_t>(stored_bits[bit_index++] << bit);
		}
		result.data[byte_index] = data_byte;
	}
	_setShift64(result, shift);
	return result;
}

bool _bitsIsZero(const std::uint8_t bits_le[kSignificantBits]) {
	for (int i = 0; i < kSignificantBits; ++i) {
		if (bits_le[i]) return false;
	}
	return true;
}

bool _bitsEqual(const std::uint8_t lhs_bits[kSignificantBits], const std::uint8_t rhs_bits[kSignificantBits]) {
	for (int i = 0; i < kSignificantBits; ++i) {
		if (lhs_bits[i] != rhs_bits[i]) return false;
	}
	return true;
}

bool _bitsLess(const std::uint8_t lhs_bits[kSignificantBits], const std::uint8_t rhs_bits[kSignificantBits]) {
	for (int i = kSignificantBits - 1; i >= 0; --i) {
		if (lhs_bits[i] != rhs_bits[i]) {
			return lhs_bits[i] < rhs_bits[i];
		}
	}
	return false;
}

bool _bitsAdd(const std::uint8_t lhs_bits[kSignificantBits], const std::uint8_t rhs_bits[kSignificantBits], std::uint8_t result_bits[kSignificantBits]) {
	std::uint8_t carry = 0;
	for (int i = 0; i < kSignificantBits; ++i) {
		std::uint8_t sum = static_cast<std::uint8_t>(lhs_bits[i] + rhs_bits[i] + carry);
		result_bits[i] = static_cast<std::uint8_t>(sum & 1U);
		carry = static_cast<std::uint8_t>((sum >> 1) & 1U);
	}
	return 0;
}

bool _bitsSub(const std::uint8_t lhs_bits[kSignificantBits], const std::uint8_t rhs_bits[kSignificantBits], std::uint8_t result_bits[kSignificantBits]) {
	std::uint8_t borrow = 0;
	for (int i = 0; i < kSignificantBits; ++i) {
		int diff = static_cast<int>(lhs_bits[i]) - static_cast<int>(rhs_bits[i]) - static_cast<int>(borrow);
		if (diff >= 0) {
			result_bits[i] = static_cast<std::uint8_t>(diff);
			borrow = 0;
		} else {
			result_bits[i] = static_cast<std::uint8_t>(diff + 2);
			borrow = 1;
		}
	}
	return 0;
}

bool _bitsMul(const std::uint8_t lhs_bits[kSignificantBits], const std::uint8_t rhs_bits[kSignificantBits], std::uint8_t result_bits[kSignificantBits]) {
	std::uint8_t tmp_bits[kSignificantBits];
	std::memset(tmp_bits, 0, sizeof(tmp_bits));
	for (int rhs_index = 0; rhs_index < kSignificantBits; ++rhs_index) {
		if (!rhs_bits[rhs_index]) continue;
		std::uint8_t carry = 0;
		for (int lhs_index = 0; lhs_index + rhs_index < kSignificantBits; ++lhs_index) {
			std::uint8_t sum = static_cast<std::uint8_t>(tmp_bits[lhs_index + rhs_index] + lhs_bits[lhs_index] + carry);
			tmp_bits[lhs_index + rhs_index] = static_cast<std::uint8_t>(sum & 1U);
			carry = static_cast<std::uint8_t>((sum >> 1) & 1U);
		}
	}
	std::memcpy(result_bits, tmp_bits, sizeof(tmp_bits));
	return 0;
}

bool _bitsDiv(const std::uint8_t dividend_bits[kSignificantBits], const std::uint8_t divisor_bits[kSignificantBits], std::uint8_t quotient_bits[kSignificantBits]) {
	if (_bitsIsZero(divisor_bits)) {
		throw std::runtime_error("uint239_t division by zero");
	}
	std::memset(quotient_bits, 0, kSignificantBits);
	std::uint8_t remainder_bits[kSignificantBits];
	std::memset(remainder_bits, 0, kSignificantBits);
	for (int bit_index = kSignificantBits - 1; bit_index >= 0; --bit_index) {
		for (int i = kSignificantBits - 1; i > 0; --i) {
			remainder_bits[i] = remainder_bits[i - 1];
		}
		remainder_bits[0] = dividend_bits[bit_index];
		if (!_bitsLess(remainder_bits, divisor_bits)) {
			_bitsSub(remainder_bits, divisor_bits, remainder_bits);
			quotient_bits[bit_index] = 1;
		}
	}
	return 0;
}

uint32_t GetShift(const uint239_t& value) {
	return static_cast<uint32_t>(_getShift64(value) & 0xFFFFFFFFULL);
}

uint239_t FromInt(uint32_t value, uint32_t shift) {
	std::uint8_t bits_le[kSignificantBits];
	std::memset(bits_le, 0, sizeof(bits_le));
	int bit_index = 0;
	while (value != 0 && bit_index < kSignificantBits) {
		bits_le[bit_index++] = static_cast<std::uint8_t>(value & 1U);
		value >>= 1U;
	}
	std::uint64_t shift_64 = static_cast<std::uint64_t>(shift);
	return _encodeFromBitsLe(bits_le, shift_64);
}

uint239_t FromString(const char* str, uint32_t shift) {
	std::uint64_t value = 0;
	for (const char* p = str; *p != '\0'; ++p) {
		if (*p < '0' || *p > '9') {
			continue;
		}
		value = value * 10ULL + static_cast<std::uint64_t>(*p - '0');
	}
	std::uint8_t bits_le[kSignificantBits];
	std::memset(bits_le, 0, sizeof(bits_le));
	int bit_index = 0;
	while (value != 0 && bit_index < kSignificantBits) {
		bits_le[bit_index++] = static_cast<std::uint8_t>(value & 1ULL);
		value >>= 1ULL;
	}
	std::uint64_t shift_64 = static_cast<std::uint64_t>(shift);
	return _encodeFromBitsLe(bits_le, shift_64);
}

uint239_t operator<<(const uint239_t& lhs, uint32_t shift) {
	std::uint8_t lhs_bits[kSignificantBits];
	std::uint64_t lhs_shift = 0;
	_decodeToBitsLe(lhs, lhs_bits, lhs_shift);
	std::uint64_t new_shift = (lhs_shift + static_cast<std::uint64_t>(shift)) & kShiftMask35;
	return _encodeFromBitsLe(lhs_bits, new_shift);
}

uint239_t operator>>(const uint239_t& lhs, uint32_t shift) {
	std::uint8_t lhs_bits[kSignificantBits];
	std::uint64_t lhs_shift = 0;
	_decodeToBitsLe(lhs, lhs_bits, lhs_shift);
	std::uint64_t modulus = (1ULL << 35);
	std::uint64_t shift_64 = static_cast<std::uint64_t>(shift) & kShiftMask35;
	std::uint64_t new_shift = (lhs_shift + modulus - (shift_64 % modulus)) & kShiftMask35;
	return _encodeFromBitsLe(lhs_bits, new_shift);
}

bool operator==(const uint239_t& lhs, const uint239_t& rhs) {
	std::uint8_t lhs_bits[kSignificantBits];
	std::uint8_t rhs_bits[kSignificantBits];
	std::uint64_t lhs_shift = 0;
	std::uint64_t rhs_shift = 0;
	_decodeToBitsLe(lhs, lhs_bits, lhs_shift);
	_decodeToBitsLe(rhs, rhs_bits, rhs_shift);
	return _bitsEqual(lhs_bits, rhs_bits);
}

bool operator!=(const uint239_t& lhs, const uint239_t& rhs) {
	return !(lhs == rhs);
}

bool operator<(const uint239_t& lhs, const uint239_t& rhs) {
	std::uint8_t lhs_bits[kSignificantBits];
	std::uint8_t rhs_bits[kSignificantBits];
	std::uint64_t lhs_shift = 0;
	std::uint64_t rhs_shift = 0;
	_decodeToBitsLe(lhs, lhs_bits, lhs_shift);
	_decodeToBitsLe(rhs, rhs_bits, rhs_shift);
	return _bitsLess(lhs_bits, rhs_bits);
}

bool operator>(const uint239_t& lhs, const uint239_t& rhs) {
	return rhs < lhs;
}

uint239_t operator+(const uint239_t& lhs, const uint239_t& rhs) {
	std::uint8_t lhs_bits[kSignificantBits];
	std::uint8_t rhs_bits[kSignificantBits];
	std::uint8_t result_bits[kSignificantBits];
	std::uint64_t lhs_shift = 0;
	std::uint64_t rhs_shift = 0;
	_decodeToBitsLe(lhs, lhs_bits, lhs_shift);
	_decodeToBitsLe(rhs, rhs_bits, rhs_shift);
	_bitsAdd(lhs_bits, rhs_bits, result_bits);
	std::uint64_t new_shift = (lhs_shift + rhs_shift) & kShiftMask35;
	return _encodeFromBitsLe(result_bits, new_shift);
}

uint239_t operator-(const uint239_t& lhs, const uint239_t& rhs) {
	std::uint8_t lhs_bits[kSignificantBits];
	std::uint8_t rhs_bits[kSignificantBits];
	std::uint8_t result_bits[kSignificantBits];
	std::uint64_t lhs_shift = 0;
	std::uint64_t rhs_shift = 0;
	_decodeToBitsLe(lhs, lhs_bits, lhs_shift);
	_decodeToBitsLe(rhs, rhs_bits, rhs_shift);
	_bitsSub(lhs_bits, rhs_bits, result_bits);
	std::uint64_t modulus = (1ULL << 35);
	std::uint64_t new_shift = (lhs_shift + modulus - (rhs_shift % modulus)) & kShiftMask35;
	return _encodeFromBitsLe(result_bits, new_shift);
}

uint239_t operator*(const uint239_t& lhs, const uint239_t& rhs) {
	std::uint8_t lhs_bits[kSignificantBits];
	std::uint8_t rhs_bits[kSignificantBits];
	std::uint8_t result_bits[kSignificantBits];
	std::uint64_t lhs_shift = 0;
	std::uint64_t rhs_shift = 0;
	_decodeToBitsLe(lhs, lhs_bits, lhs_shift);
	_decodeToBitsLe(rhs, rhs_bits, rhs_shift);
	_bitsMul(lhs_bits, rhs_bits, result_bits);
	std::uint64_t new_shift = (lhs_shift + rhs_shift) & kShiftMask35;
	return _encodeFromBitsLe(result_bits, new_shift);
}

uint239_t operator/(const uint239_t& lhs, const uint239_t& rhs) {
	std::uint8_t lhs_bits[kSignificantBits];
	std::uint8_t rhs_bits[kSignificantBits];
	std::uint8_t quotient_bits[kSignificantBits];
	std::uint64_t lhs_shift = 0;
	std::uint64_t rhs_shift = 0;
	_decodeToBitsLe(lhs, lhs_bits, lhs_shift);
	_decodeToBitsLe(rhs, rhs_bits, rhs_shift);
	_bitsDiv(lhs_bits, rhs_bits, quotient_bits);
	std::uint64_t modulus = (1ULL << 35);
	std::uint64_t new_shift = (lhs_shift + modulus - (rhs_shift % modulus)) & kShiftMask35;
	return _encodeFromBitsLe(quotient_bits, new_shift);
}

std::ostream& operator<<(std::ostream& stream, const uint239_t& value) {
	for (int byte_index = 0; byte_index < kNumBytes; ++byte_index) {
		for (int bit = 7; bit >= 0; --bit) {
			stream << ((value.data[byte_index] >> bit) & 1);
		}
		stream << ' ';
	}
	return stream;
}
