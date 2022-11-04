#pragma once

#include <arch/mem_space.hpp>

namespace regs {
	constexpr arch::bit_register<uint32_t> ctrl{0x00};
	constexpr arch::bit_register<uint32_t> status{0x08};
	constexpr arch::bit_register<uint32_t> eecd{0x10};
	constexpr arch::bit_register<uint32_t> eerd{0x14};
	constexpr arch::scalar_register<uint32_t> fcal{0x28};
	constexpr arch::scalar_register<uint32_t> fcah{0x2C};
	constexpr arch::scalar_register<uint32_t> fct{0x30};
	constexpr arch::scalar_register<uint32_t> fcttv{0x170};
	constexpr arch::scalar_register<uint32_t> ral_0{0x5400};
	constexpr arch::scalar_register<uint32_t> rah_0{0x5404};
} // namespace regs

namespace flags {
	namespace ctrl {
		constexpr arch::field<uint32_t, bool> set_link_up{6, 1};
		constexpr arch::field<uint32_t, bool> lrst{3, 1};
		constexpr arch::field<uint32_t, bool> asde{5, 1};
		constexpr arch::field<uint32_t, bool> ilos{7, 1};
		constexpr arch::field<uint32_t, bool> reset{26, 1};
		constexpr arch::field<uint32_t, bool> vme{30, 1};
		constexpr arch::field<uint32_t, bool> phy_reset{31, 1};
	}

	namespace eecd {
		constexpr arch::field<uint32_t, bool> present{8, 1};
	}

	namespace eerd {
		constexpr arch::field<uint32_t, bool> start{0, 1};
		constexpr arch::field<uint32_t, bool> done{1, 1};
		constexpr arch::field<uint32_t, uint8_t> addr{2, 14};
		constexpr arch::field<uint32_t, uint16_t> data{16, 16};
	}
} // namespace flags
