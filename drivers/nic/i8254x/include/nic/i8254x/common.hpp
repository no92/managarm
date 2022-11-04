#pragma once

#include <async/basic.hpp>
#include <arch/mem_space.hpp>
#include <helix/memory.hpp>
#include <netserver/nic.hpp>
#include <nic/i8254x/queue.hpp>
#include <protocols/hw/client.hpp>

constexpr bool logDebug = true;

struct Intel8254xNic : nic::Link {
public:
	Intel8254xNic(protocols::hw::Device device);

	async::result<void> init();

private:
	async::result<uint16_t> eepromRead(uint8_t address);

	helix::Mapping _mmio_mapping;
	arch::mem_space _mmio;

	arch::contiguous_pool _dmaPool;
	protocols::hw::Device _device;

	helix::UniqueDescriptor _irq;
};
