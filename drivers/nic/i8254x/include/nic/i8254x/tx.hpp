#pragma once

#include <arch/variable.hpp>
#include <stdint.h>

namespace flags::tx {

namespace cmd {

constexpr arch::field<uint8_t, bool> end_of_packet{0, 1};
constexpr arch::field<uint8_t, bool> insert_fcs{1, 1};
constexpr arch::field<uint8_t, bool> report_status{3, 1};

}

namespace status {

constexpr arch::field<uint8_t, bool> done{0, 1};
constexpr arch::field<uint8_t, bool> end_of_packet{1, 1};

}

} // namespace flags::tx

struct TxDescriptor {
	volatile uint64_t address;
	volatile uint16_t length;
	volatile uint8_t cso;
	arch::bit_value<uint8_t> cmd;
	arch::bit_value<uint8_t> status;
	volatile uint8_t css;
	volatile uint16_t special;
};

static_assert(sizeof(TxDescriptor) == 16, "TxDescriptor should be 16 bytes");
