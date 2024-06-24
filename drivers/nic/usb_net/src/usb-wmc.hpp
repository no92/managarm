#pragma once

#include <async/recurring-event.hpp>
#include <async/queue.hpp>
#include <vector>
#include <smarter.hpp>

#include "usb-net.hpp"

namespace nic::usb_wmc {

struct UsbWmcNic;

struct CdcWdmDevice {
	UsbWmcNic *nic;

	bool nonBlock_ = false;
};

struct UsbWmcNic : UsbNic {
	friend CdcWdmDevice;

	UsbWmcNic(protocols::usb::Device hw_device, nic::MacAddress mac,
		protocols::usb::Interface ctrl_intf, protocols::usb::Endpoint ctrl_ep,
		protocols::usb::Interface intf, protocols::usb::Endpoint in, protocols::usb::Endpoint out,
		size_t config_index);

	async::result<void> initialize() override;
	async::detached receiveEncapsulated();
	async::detached listenForNotifications() override;

	async::result<size_t> receive(arch::dma_buffer_view) override;
	async::result<void> send(const arch::dma_buffer_view) override;

	async::result<void> writeCommand(arch::dma_buffer_view request);
private:
	async::recurring_event response_available_;
	size_t config_index_;

	struct PacketInfo {
		size_t len;
		arch::dma_buffer view;
	};


	std::vector<smarter::shared_ptr<CdcWdmDevice>> cdc_wdm_devs_;

public:
	async::queue<PacketInfo, frg::stl_allocator> queue_;

	async::recurring_event _statusBell;
	uint64_t _currentSeq;
	uint64_t _inSeq;
};

} // namespace nic::usb_wmc
