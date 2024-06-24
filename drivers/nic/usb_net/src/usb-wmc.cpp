#include <fcntl.h>
#include <format>
#include <nic/usb_net/usb_net.hpp>
#include <protocols/mbus/client.hpp>
#include <protocols/fs/server.hpp>
#include <smarter.hpp>
#include <sys/epoll.h>

#include "usb-wmc.hpp"
#include "usb-net.hpp"

namespace nic::usb_wmc {

static async::result<void> setFileFlags(void *object, int flags) {
	auto self = static_cast<CdcWdmDevice *>(object);

	if(flags & ~O_NONBLOCK) {
		std::cout << std::format("netserver: setFileFlags with unknown flags 0x{:x}\n", flags);
		co_return;
	}

	if(flags & O_NONBLOCK)
		self->nonBlock_ = true;
	else
		self->nonBlock_ = false;
	co_return;
}

static async::result<int> getFileFlags(void *object) {
	auto self = static_cast<CdcWdmDevice *>(object);
	int flags = O_RDWR;

	if(self->nonBlock_)
		flags |= O_NONBLOCK;

	co_return flags;
}

static async::result<frg::expected<protocols::fs::Error, protocols::fs::PollWaitResult>>
pollWait(void *object, uint64_t pastSeq, int mask, async::cancellation_token cancellation) {
	(void)mask; // TODO: utilize mask.

	auto self = static_cast<CdcWdmDevice *>(object);

	if(cancellation.is_cancellation_requested())
		std::cout << "\e[33mnetserver: pollWait() cancellation is untested\e[39m" << std::endl;

	assert(pastSeq <= self->nic->_currentSeq);
	while(pastSeq == self->nic->_currentSeq && !cancellation.is_cancellation_requested())
		co_await self->nic->_statusBell.async_wait(cancellation);

	// For now making this always writable is sufficient.
	int edges = EPOLLOUT | EPOLLWRNORM;
	if(self->nic->_inSeq > pastSeq)
		edges |= EPOLLIN | EPOLLRDNORM;

	co_return protocols::fs::PollWaitResult(self->nic->_currentSeq, edges);
}

static async::result<frg::expected<protocols::fs::Error, protocols::fs::PollStatusResult>>
pollStatus(void *object) {
	auto self = static_cast<CdcWdmDevice *>(object);
	int events = EPOLLOUT | EPOLLWRNORM;
	if(!self->nic->queue_.empty())
		events |= EPOLLIN | EPOLLRDNORM;

	co_return protocols::fs::PollStatusResult(self->nic->_currentSeq, events);
}

static async::result<frg::expected<protocols::fs::Error, size_t>>
write(void *object, const char *credentials, const void *buffer, size_t length) {
	(void) credentials;

	auto self = static_cast<CdcWdmDevice *>(object);

	co_await self->nic->writeCommand(arch::dma_buffer_view{nullptr, const_cast<void *>(buffer), length});

	co_return length;
}

static async::result<protocols::fs::ReadResult> read(void *object, const char *credentials,
			void *buffer, size_t length) {
	auto self = static_cast<CdcWdmDevice *>(object);

	printf("KANKERKANKERKANKER: read unimplemented!\n");

	auto p = co_await self->nic->queue_.async_get();
	assert(p);

	printf("\tpacket popped\n");

	co_return protocols::fs::Error::endOfFile;
}

constexpr auto fileOperations = protocols::fs::FileOperations{
	.read = &read,
	.write = &write,
	.pollWait = &pollWait,
	.pollStatus = &pollStatus,
	.getFileFlags = &getFileFlags,
	.setFileFlags = &setFileFlags,
};

async::detached serveDevice(helix::UniqueLane lane, smarter::shared_ptr<CdcWdmDevice> cdc_wdm) {
	while(true) {
		auto [accept, recv_req] = co_await helix_ng::exchangeMsgs(lane,
			helix_ng::accept(
				helix_ng::recvInline())
		);
		HEL_CHECK(accept.error());
		HEL_CHECK(recv_req.error());

		auto conversation = accept.descriptor();

		managarm::fs::CntRequest req;
		req.ParseFromArray(recv_req.data(), recv_req.length());
		recv_req.reset();
		if(req.req_type() == managarm::fs::CntReqType::DEV_OPEN) {
			helix::UniqueLane local_lane, remote_lane;
			std::tie(local_lane, remote_lane) = helix::createStream();
			async::detach(protocols::fs::servePassthrough(
					std::move(local_lane), cdc_wdm, &fileOperations));

			managarm::fs::SvrResponse resp;
			resp.set_error(managarm::fs::Errors::SUCCESS);

			auto ser = resp.SerializeAsString();
			auto [send_resp, push_node] = co_await helix_ng::exchangeMsgs(conversation,
				helix_ng::sendBuffer(ser.data(), ser.size()),
				helix_ng::pushDescriptor(remote_lane)
			);
			HEL_CHECK(send_resp.error());
			HEL_CHECK(push_node.error());
		}else{
			throw std::runtime_error("Invalid serveDevice request!");
		}
	}
}

UsbWmcNic::UsbWmcNic(protocols::usb::Device hw_device, nic::MacAddress mac,
		protocols::usb::Interface ctrl_intf, protocols::usb::Endpoint ctrl_ep,
		protocols::usb::Interface data_intf, protocols::usb::Endpoint in, protocols::usb::Endpoint out,
		size_t config_index)
	: UsbNic{hw_device, mac, ctrl_intf, ctrl_ep, data_intf, in, out}, config_index_{config_index} {

}

async::result<void> UsbWmcNic::initialize() {
	mbus_ng::Properties descriptor{
		{"generic.devtype", mbus_ng::StringItem{"char"}},
		{"generic.devname", mbus_ng::StringItem{"cdc-wdm"}}
	};

	auto wwan_entity = (co_await mbus_ng::Instance::global().createEntity(
		"wwan", descriptor)).unwrap();

	auto cdc_wdm = smarter::make_shared<CdcWdmDevice>(this);

	[] (smarter::shared_ptr<CdcWdmDevice> cdc_wdm, mbus_ng::EntityManager entity) -> async::detached {
		while (true) {
			auto [localLane, remoteLane] = helix::createStream();

			// If this fails, too bad!
			(void)(co_await entity.serveRemoteLane(std::move(remoteLane)));

			serveDevice(std::move(localLane), cdc_wdm);
		}
	}(cdc_wdm, std::move(wwan_entity));

	cdc_wdm_devs_.push_back(cdc_wdm);

	receiveEncapsulated();
	listenForNotifications();

	co_return;
}

async::detached UsbWmcNic::receiveEncapsulated() {
	while(true) {
		co_await response_available_.async_wait();

		arch::dma_object<protocols::usb::SetupPacket> ctrl_msg{&dmaPool_};
		arch::dma_buffer data{&dmaPool_, 0x1000};
		ctrl_msg->type = protocols::usb::setup_type::byClass |
						protocols::usb::setup_type::toHost | protocols::usb::setup_type::targetInterface;
		ctrl_msg->request = uint8_t(nic::usb_net::RequestCode::GET_ENCAPSULATED_RESPONSE);
		ctrl_msg->value = 0;
		ctrl_msg->index = ctrl_intf_.num();
		ctrl_msg->length = data.size();

		std::cout << std::format("netserver: getting encapsulated command on interface {}\n", ctrl_intf_.num());

		auto control = protocols::usb::ControlTransfer{
			protocols::usb::kXferToHost, ctrl_msg, data
		};
		auto res = co_await device_.transfer(control);
		assert(res);

		std::cout << std::format("KANKER: received packet size=0x{:x}\n", res.value());

		// TODO: put the correct data size
		queue_.put({data.size(), std::move(data)});
		_inSeq = ++_currentSeq;
		_statusBell.raise();
	}
}

async::detached UsbWmcNic::listenForNotifications() {
	using NotificationHeader = protocols::usb::CdcNotificationHeader;

	while(true) {
		arch::dma_buffer report{device_.bufferPool(), 16};
		protocols::usb::InterruptTransfer transfer{protocols::usb::XferFlags::kXferToHost, report};
		transfer.allowShortPackets = true;
		auto length = (co_await ctrl_ep_.transfer(transfer)).unwrap();

		assert(length >= sizeof(NotificationHeader));

		auto notification = reinterpret_cast<NotificationHeader *>(report.data());

		switch(notification->bNotificationCode) {
			using Notification = NotificationHeader::Notification;

			case Notification::RESPONSE_AVAILABLE:
				response_available_.raise();
				break;
			case Notification::NETWORK_CONNECTION:
				l1_up_ = (notification->wValue == 1);
				break;
			case Notification::CONNECTION_SPEED_CHANGE: {
				auto change = reinterpret_cast<protocols::usb::CdcConnectionSpeedChange *>(report.subview(sizeof(protocols::usb::CdcNotificationHeader)).data());
				printf("netserver: connection speed %u MBit/s\n", change->DlBitRate / 1000 / 1000);
				break;
			}
			default: {
				printf("netserver: received notification 0x%x\n", uint8_t(notification->bNotificationCode));
				break;
			}
		}
	}

	co_return;
}

async::result<size_t> UsbWmcNic::receive(arch::dma_buffer_view) {
	arch::dma_buffer buf{&dmaPool_, mtu};

	auto res = co_await data_in_.transfer(protocols::usb::BulkTransfer{protocols::usb::kXferToHost, buf});
	assert(res);

	if(res.value() != 0) {
		assert(!"recv unimplemented");
	}

	assert(!"USB NIC receive failed!");
}

async::result<void> UsbWmcNic::send(const arch::dma_buffer_view) {
	co_return;
}

async::result<void> UsbWmcNic::writeCommand(arch::dma_buffer_view request) {
	arch::dma_object<protocols::usb::SetupPacket> ctrl_msg{&dmaPool_};
	ctrl_msg->type = protocols::usb::setup_type::byClass |
					protocols::usb::setup_type::toDevice | protocols::usb::setup_type::targetInterface;
	ctrl_msg->request = uint8_t(nic::usb_net::RequestCode::SEND_ENCAPSULATED_COMMAND);
	ctrl_msg->value = 0;
	ctrl_msg->index = ctrl_intf_.num();
	ctrl_msg->length = request.size();

	std::cout << std::format("netserver: sending encapsulated command of length {} on interface {}\n", request.size(), ctrl_intf_.num());

	auto res = co_await device_.transfer(protocols::usb::ControlTransfer{
		protocols::usb::kXferToDevice, ctrl_msg, request
	});
	assert(res);

	co_return;
}

} // namespace nic::usb_wmc
