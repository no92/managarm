#pragma once

#include <arch/mem_space.hpp>

namespace regs {
	constexpr arch::bit_register<uint32_t> ctrl{0x00};
	constexpr arch::bit_register<uint32_t> status{0x08};
	constexpr arch::scalar_register<uint32_t> fcal{0x28};
	constexpr arch::scalar_register<uint32_t> fcah{0x2C};
	constexpr arch::scalar_register<uint32_t> fct{0x30};
	constexpr arch::scalar_register<uint32_t> fcttv{0x170};
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
} // namespace flags
