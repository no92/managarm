#include <async/basic.hpp>
#include <nic/i8254x/common.hpp>

Intel8254xNic::Intel8254xNic(protocols::hw::Device device)
	: nic::Link(1500, &_dmaPool), _device{std::move(device)} {
		async::run(this->init(), helix::currentDispatcher);
}

async::result<void> Intel8254xNic::init() {
	auto info = co_await _device.getPciInfo();
	_irq = co_await _device.accessIrq();
	co_await _device.enableBusmaster();

	auto &barInfo = info.barInfo[0];
	assert(barInfo.ioType == protocols::hw::IoType::kIoTypeMemory);
	auto bar0 = co_await _device.accessBar(0);

	_mmio_mapping = {bar0, barInfo.offset, barInfo.length};
	_mmio = _mmio_mapping.get();
}

namespace nic::intel8254x {

std::shared_ptr<nic::Link> makeShared(protocols::hw::Device device) {
	return std::make_shared<Intel8254xNic>(std::move(device));
}

} // namespace nic::intel8254x
