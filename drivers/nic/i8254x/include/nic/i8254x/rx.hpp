#pragma once

#include <arch/variable.hpp>
#include <stdint.h>

namespace flags::rx::status {

constexpr arch::field<uint8_t, bool> done{0, 1};
constexpr arch::field<uint8_t, bool> end_of_packet{1, 1};

}

struct RxDescriptor {
	volatile uint64_t address;
	volatile uint16_t length;
	volatile uint16_t checksum;
	arch::bit_value<uint8_t> status;
	volatile uint8_t errors;
	volatile uint16_t special;
} __attribute__((packed));

static_assert(sizeof(RxDescriptor) == 16, "RxDescriptor should be 16 bytes");
