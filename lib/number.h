#pragma once
#include <cinttypes>
#include <iostream>

struct uint239_t {
    uint8_t data[35];
};

static_assert(sizeof(uint239_t) == 35, "Size of uint239_t must be no higher than 35 bytes");

uint32_t GetShift(const uint239_t& value); // done

uint239_t FromInt(uint32_t value, uint32_t shift); // done

uint239_t FromString(const char* str, uint32_t shift); // done

uint239_t operator+(const uint239_t& lhs, const uint239_t& rhs); // done

uint239_t operator-(const uint239_t& lhs, const uint239_t& rhs); // done

uint239_t operator*(const uint239_t& lhs, const uint239_t& rhs); // done

uint239_t operator/(const uint239_t& lhs, const uint239_t& rhs);

uint239_t operator<<(const uint239_t& lhs, uint32_t shift); // done

uint239_t operator>>(const uint239_t& lhs, uint32_t shift); // done

bool operator==(const uint239_t& lhs, const uint239_t& rhs); // done

bool operator!=(const uint239_t& lhs, const uint239_t& rhs); // done

std::ostream& operator<<(std::ostream& stream, const uint239_t& value); // done

bool operator>(const uint239_t& lhs, const uint239_t& rhs); // done

bool operator<(const uint239_t& lhs, const uint239_t& rhs); // done
