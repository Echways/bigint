#include "number.h"
#include <stdexcept>

constexpr int NBITS = 35 * 8;

std::string _toBinaryShift(uint32_t number) {
    std::string result = "000";
    for (int i = 31; i >= 0; --i) {
        result += (number & (1 << i)) ? '1' : '0';
    }
    return result;
}

uint8_t _getBit(const uint239_t& x, int index) {
    int byte = index / 8;
    int bit  = 7 - (index % 8);
    return static_cast<uint8_t>((x.data[byte] >> bit) & 0x01);
}

bool _setBit(uint239_t& x, int index) {
    int byte = index / 8;
    int bit  = 7 - (index % 8);
    x.data[byte] |= static_cast<uint8_t>(1u << bit);
    return 0;
}

bool _shiftLeftWithBit(uint239_t& x, uint8_t in_bit) {
    uint8_t carry = in_bit & 1;
    for (int i = 34; i >= 0; --i) {
        uint16_t v = static_cast<uint16_t>(x.data[i]) << 1;
        v |= carry;
        x.data[i] = static_cast<uint8_t>(v & 0xFF);
        carry = static_cast<uint8_t>((v >> 8) & 0x01);
    }
    return 0;
}

bool _checkIfZero(const uint239_t& x) {
    for (int i = 0; i < 35; ++i) {
        if (x.data[i] != 0) return false;
    }
    return true;
}

uint239_t FromString(const char* str, uint32_t shift) {
    uint239_t result{};
    for (size_t i = 0; str[i] != '\0'; ++i) {
        uint16_t carry = str[i] - '0';

        for (int j = 34; j >= 0; --j) {
            uint16_t temp = (result.data[j] & 0x7F) * 10 + carry;
            result.data[j] = static_cast<uint8_t>(temp & 0x7F);
            carry = temp >> 7;
        }
    }
    return result<<shift;
}

bool operator==(const uint239_t& lhs, const uint239_t& rhs) {
    for (size_t i = 0; i < 35; ++i) {
        if (lhs.data[i] != rhs.data[i]) {return false;}
    }
    return true;
}

bool operator!=(const uint239_t& lhs, const uint239_t& rhs) {
    for (size_t i = 0; i < 35; ++i) {
        if (lhs.data[i] != rhs.data[i]) {return true;}
    }
    return false;
}

uint239_t operator<<(const uint239_t& lhs, uint32_t shift) {
    uint239_t result{};
    uint239_t temp{};

    if (shift == 0) {
        return lhs;
    }

    int byteShift = static_cast<int>(shift / 8 % 35);
    int bitShift = static_cast<int>(shift % 8);

    uint8_t carry = 0;
    std::string shiftString = _toBinaryShift(shift);
    for (int i = 34; i >= 0; i--) {
        temp.data[i] = ((lhs.data[i] & 0x7F) << bitShift) | carry;
        carry = (lhs.data[i] & 0x7F) >> (7 - bitShift);

        if (shiftString[i] == '1') {
            temp.data[i] |= 0x80;
        }else {
            temp.data[i] &= 0x7F;
        }

    }

    for (int i = 34; i - byteShift >= 0; i--) {
        result.data[i - byteShift] = temp.data[i];
    }

    return result;
}

uint239_t operator>>(const uint239_t& lhs, uint32_t shift) {
    uint239_t result{};
    uint239_t temp{};

    if (shift == 0) {
        return lhs;
    }

    int byteShift = static_cast<int>(shift / 8 % 35);
    int bitShift = static_cast<int>(shift % 8);

    uint8_t carry = 0;
    for (int i = 0; i < 35; i++) {
        temp.data[i] = ((lhs.data[i] & 0x7F) >> bitShift) | carry;
        carry = (lhs.data[i] & 0x7F) << (7 - bitShift);
        temp.data[i] &= 0x7F;

    }

    for (int i = 0; i + byteShift < 35; i++) {
        result.data[i + byteShift] = temp.data[i];
    }

    return result;
}

std::ostream& operator<<(std::ostream& stream, const uint239_t& value) {
    for (size_t i = 0; i < 35; ++i) {
        for (int j = 7; j >= 0; --j) {
            stream << (value.data[i] >> j & 1);
        }
        stream << " ";
    }
    return stream;
}

uint239_t FromInt(uint32_t value, uint32_t shift) {
    uint239_t result{};

    for (int i = 34; i >= 0; --i) {
        result.data[i] = static_cast<uint8_t>(value & 0x7F);
        value >>= 7;
    }
    return result<<shift;
}

uint32_t GetShift(const uint239_t &value) {
    std::string string="";
    for (size_t i = 3; i < 35; ++i) {
        string+=value.data[i]&0x80?'1':'0';
    }
    return std::stoi(string, nullptr, 2);
}

uint239_t operator+(const uint239_t& lhs, const uint239_t& rhs) {
    uint239_t lhsShifter = lhs>>GetShift(lhs);
    uint239_t rhsShifted = rhs>>GetShift(rhs);
    uint239_t result{};
    uint16_t carry = 0;
    uint32_t shift = GetShift(lhs)+GetShift(rhs);

    for (int i = 34; i >= 0; --i) {
        uint16_t sum = lhsShifter.data[i] + rhsShifted.data[i] + carry;
        result.data[i] = static_cast<uint8_t>(sum & 0xFF);
        carry = sum >> 8;
    }
    return result<<shift;
}

uint239_t operator-(const uint239_t& lhs, const uint239_t& rhs) {
    uint239_t lhsShifter = lhs>>GetShift(lhs);
    uint239_t rhsShifted = rhs>>GetShift(rhs);
    uint239_t result{};
    uint16_t carry = 0;
    uint32_t shift = GetShift(lhs)-GetShift(rhs);

    for (int i = 34; i >= 0; --i) {
        uint16_t diff = lhsShifter.data[i] - rhsShifted.data[i] - carry;
        result.data[i] = static_cast<uint8_t>(diff & 0xFF);
        carry = diff >> 8;
    }
    return result<<shift;
}

bool operator>(const uint239_t& lhs, const uint239_t& rhs) {
    for (int i = 0; i < 35; ++i) {
        if (lhs.data[i] != rhs.data[i]) {
            if (lhs.data[i] > rhs.data[i]) {
                return true;
            }
        }
    }
    return false;
}

bool operator<(const uint239_t& lhs, const uint239_t& rhs) {
    for (int i = 0; i < 35; ++i) {
        if (lhs.data[i] != rhs.data[i]) {
            if (lhs.data[i] > rhs.data[i]) {
                return false;
            }
        }
    }
    return true;
}

uint239_t operator*(const uint239_t& lhs, const uint239_t& rhs) {
    uint239_t lhsShifted = lhs >> GetShift(lhs);
    uint239_t rhsShifted = rhs >> GetShift(rhs);
    uint239_t result{};
    uint32_t shift = GetShift(lhs) + GetShift(rhs);

    uint64_t temp[35]{};

    for (int i = 34; i >= 0; --i) {
        for (int j = 34; j >= 0; --j) {
            int pos = i + j - 34;
            if (pos >= 0 && pos <= 34) {
                temp[pos] += static_cast<uint64_t>(lhsShifted.data[i] * rhsShifted.data[j]);
            }
        }
    }

    uint16_t carry = 0;
    for (int i = 34; i >= 0; --i) {
        temp[i] += carry;
        result.data[i] = static_cast<uint8_t>(temp[i] & 0xFF);
        carry = static_cast<uint16_t>(temp[i] >> 8);
    }

    return result << shift;
}

uint239_t operator/(const uint239_t& lhs, const uint239_t& rhs) {
    uint32_t shiftL = GetShift(lhs);
    uint32_t shiftR = GetShift(rhs);

    uint239_t dividend = lhs >> shiftL;
    uint239_t divisor  = rhs >> shiftR;

    if (_checkIfZero(divisor)) {
        throw std::runtime_error("uint239_t division by zero");
    }

    if (dividend < divisor) {
        return uint239_t{};
    }

    uint239_t quotient{};
    uint239_t remainder{};

    for (int i = 0; i < NBITS; ++i) {
        uint8_t bit = _getBit(dividend, i);

        _shiftLeftWithBit(remainder, bit);

        if (!(remainder < divisor)) {
            remainder = remainder - divisor;
            _setBit(quotient, i);
        }
    }

    uint32_t resultShift = (shiftL > shiftR) ? (shiftL - shiftR) : 0u;

    return quotient << resultShift;
}