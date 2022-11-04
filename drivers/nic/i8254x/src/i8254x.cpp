#include <async/basic.hpp>
#include <nic/i8254x/common.hpp>
#include <nic/i8254x/regs.hpp>
#include <unistd.h>

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

	_mmio.store(regs::ctrl, flags::ctrl::reset(true));
	/* p. 228, Table 13-3: To ensure that global device reset has fully completed and that the Ethernet controller
	 * responds to subsequent access, wait approximately 1 µs after setting and before attempting to check to see
	 * if the bit has cleared or to access any other device register. */
	usleep(1);
	while(_mmio.load(regs::ctrl) & flags::ctrl::reset)
		usleep(1);

	_mmio.store(regs::ctrl, _mmio.load(regs::ctrl) / flags::ctrl::asde(true) / flags::ctrl::set_link_up(true) / flags::ctrl::lrst(false) / flags::ctrl::phy_reset(false) / flags::ctrl::ilos(false));

	_mmio.store(regs::fcal, 0);
	_mmio.store(regs::fcah, 0);
	_mmio.store(regs::fct, 0);
	_mmio.store(regs::fcttv, 0);

	_mmio.store(regs::ctrl, _mmio.load(regs::ctrl) / flags::ctrl::vme(false));
}

namespace nic::intel8254x {

std::shared_ptr<nic::Link> makeShared(protocols::hw::Device device) {
	return std::make_shared<Intel8254xNic>(std::move(device));
}

} // namespace nic::intel8254x
