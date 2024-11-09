#pragma once

#include <arch/mem_space.hpp>
#include <protocols/hw/client.hpp>

struct GfxDevice {
	GfxDevice(protocols::hw::Device device);

	async::result<void> initialize();

private:
	protocols::hw::Device hwDevice_;
	arch::mem_space regs_;
	std::vector<uint32_t> vbiosData_;
};
