#include "netlink.hpp"

#include <linux/rtnetlink.h>
#include <memory>

extern std::unordered_map<int64_t, std::shared_ptr<nic::Link>> baseDeviceMap;

namespace {
	constexpr bool logSocket = false;
}

namespace nl {

NetlinkSocket::NetlinkSocket(int flags)
: flags(flags)
{ }

async::result<size_t> NetlinkSocket::sockname(void *, void *addr_ptr, size_t max_addr_length) {
	// TODO: Fill in nl_groups.
	struct sockaddr_nl sa;
	memset(&sa, 0, sizeof(struct sockaddr_nl));
	sa.nl_family = AF_NETLINK;
	sa.nl_pid = 0;
	memcpy(addr_ptr, &sa, std::min(sizeof(struct sockaddr_nl), max_addr_length));

	co_return sizeof(struct sockaddr_nl);
};

async::result<protocols::fs::RecvResult> NetlinkSocket::recvMsg(void *obj,
		const char *creds, uint32_t flags, void *data,
		size_t len, void *addr_buf, size_t addr_size, size_t max_ctrl_len) {
	auto *self = static_cast<NetlinkSocket *>(obj);
	if(logSocket)
		std::cout << "netserver: Recv from netlink socket" << std::endl;

	while(self->_recvQueue.empty())
		co_await self->_statusBell.async_wait();

	auto packet = &self->_recvQueue.front();

	const auto size = packet->buffer.size();
	auto truncated_size = (size < len) ? size : len;

	if(size && data != nullptr)
		memcpy(data, packet->buffer.data(), truncated_size);

	if(addr_size >= sizeof(struct sockaddr_nl) && addr_buf != nullptr) {
		struct sockaddr_nl sa;
		memset(&sa, 0, sizeof(struct sockaddr_nl));
		sa.nl_family = AF_NETLINK;
		sa.nl_pid = 0;
		sa.nl_groups = packet->group ? (1 << (packet->group - 1)) : 0;
		memcpy(addr_buf, &sa, sizeof(struct sockaddr_nl));
	}

	if(!(flags & MSG_PEEK))
		self->_recvQueue.pop_front();

	uint32_t reply_flags = 0;

	if(!(flags & MSG_TRUNC)) {
		if(truncated_size < size) {
			reply_flags |= MSG_TRUNC;
		}
	}

	co_return protocols::fs::RecvData{{}, size, sizeof(struct sockaddr_nl), reply_flags};
}

async::result<frg::expected<protocols::fs::Error, size_t>> NetlinkSocket::sendMsg(void *obj,
			const char *creds, uint32_t flags, void *data, size_t len,
			void *addr_ptr, size_t addr_size, std::vector<uint32_t> fds) {
	if(logSocket)
		std::cout << "netserver: sendMsg on netlink socket!" << std::endl;
	const auto orig_len = len;
	auto self = static_cast<NetlinkSocket *>(obj);
	assert(!flags);
	assert(fds.empty());

	auto hdr = static_cast<struct nlmsghdr *>(data);
	for(; NLMSG_OK(hdr, len); hdr = NLMSG_NEXT(hdr, len)) {
		if(hdr->nlmsg_type == NLMSG_DONE)
			co_return orig_len;

		// TODO: maybe send an error packet back instead of erroring here?
		if(hdr->nlmsg_type == NLMSG_ERROR)
			co_return protocols::fs::Error::illegalArguments;

		if(hdr->nlmsg_type == RTM_NEWROUTE) {
			self->newRoute(hdr);
		} else if(hdr->nlmsg_type == RTM_GETROUTE) {
			self->getRoute(hdr);
		} else if(hdr->nlmsg_type == RTM_DELROUTE) {
			self->deleteRoute(hdr);
		} else if(hdr->nlmsg_type == RTM_NEWLINK) {
			self->sendError(hdr, EPERM);
		} else if(hdr->nlmsg_type == RTM_GETLINK) {
			self->getLink(hdr);
		} else if(hdr->nlmsg_type == RTM_DELLINK) {
			self->sendError(hdr, EPERM);
		} else if(hdr->nlmsg_type == RTM_NEWADDR) {
			self->newAddr(hdr);
		} else if(hdr->nlmsg_type == RTM_GETADDR) {
			self->getAddr(hdr);
		} else {
			std::cout << "netlink: unknown nlmsg_type " << hdr->nlmsg_type << std::endl;
			co_return protocols::fs::Error::illegalArguments;
		}
	}

	co_return orig_len;
}

} // namespace nl
