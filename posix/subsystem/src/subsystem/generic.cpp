#include <string.h>
#include <iostream>

#include <core/id-allocator.hpp>
#include <protocols/mbus/client.hpp>

#include "../common.hpp"
#include "../device.hpp"
#include "../vfs.hpp"

namespace generic_subsystem {

namespace {

id_allocator<uint32_t> minorAllocator{0};

struct Device final : UnixDevice {
	Device(VfsType type, std::string name, helix::UniqueLane lane)
	: UnixDevice{type},
			_name{std::move(name)}, _lane{std::move(lane)} { }

	std::string nodePath() override {
		return _name;
	}
	
	async::result<frg::expected<Error, smarter::shared_ptr<File, FileHandle>>>
	open(std::shared_ptr<MountView> mount, std::shared_ptr<FsLink> link,
			SemanticFlags semantic_flags) override {
		return openExternalDevice(_lane, std::move(mount), std::move(link), semantic_flags);
	}

	FutureMaybe<std::shared_ptr<FsLink>> mount() override {
		return mountExternalDevice(_lane);
	}

private:
	std::string _name;
	helix::UniqueLane _lane;
};

id_allocator<uint64_t> ttyUsbAllocator{0};
id_allocator<uint64_t> ttySAllocator{0};
id_allocator<uint64_t> driCardAllocator{0};

uint64_t allocateDeviceIds(std::string type) {
	if(type == "ttyS") {
		return ttySAllocator.allocate();
	} else if(type == "ttyUSB") {
		return ttyUsbAllocator.allocate();
	} else if(type == "dri/card") {
		return driCardAllocator.allocate();
	} else {
		std::cout << "unhandled device type '" << type << "'" << std::endl;
		assert(!"unhandled device type");
		__builtin_unreachable();
	}
}

async::detached observeDevices(VfsType devType, auto &registry, int major) {
	const char *typeStr = devType == VfsType::blockDevice ? "block" : "char";

	auto filter = mbus_ng::Conjunction({
		mbus_ng::EqualsFilter {"generic.devtype", typeStr}
	});

	auto enumerator = mbus_ng::Instance::global().enumerate(filter);
	while (true) {
		auto [_, events] = (co_await enumerator.nextEvents()).unwrap();

		for (auto &event : events) {
			if (event.type != mbus_ng::EnumerationEvent::Type::created)
				continue;

			auto entity = co_await mbus_ng::Instance::global().getEntity(event.id);

			auto name = std::get<mbus_ng::StringItem>(event.properties.at("generic.devname"));
			auto id = allocateDeviceIds(name.value);

			auto sysfs_name = name.value + std::to_string(id);

			std::cout << "POSIX: Installing " << typeStr << " device "
					<< sysfs_name << std::endl;

			if(event.properties.contains("drvcore.mbus-parent")) {
				auto parent_device_id = std::get<mbus_ng::StringItem>(event.properties.at("drvcore.mbus-parent")).value;
				auto parent_entity = co_await mbus_ng::Instance::global().getEntity(std::stoll(parent_device_id));

				co_await parent_entity.updateProperties({
					{"drvcore.devname", mbus_ng::StringItem{sysfs_name}},
				});
			}

			auto lane = (co_await entity.getRemoteLane()).unwrap();
			auto device = std::make_shared<Device>(devType,
					sysfs_name, std::move(lane));

			// We use a fixed major here, and allocate minors sequentially.
			device->assignId({major, minorAllocator.allocate()});
			registry.install(device);
		}
	}
}

} // anonymous namepsace

void run() {
	observeDevices(VfsType::blockDevice, blockRegistry, 240);
	observeDevices(VfsType::charDevice, charRegistry, 234);
}

} // namespace generic_subsystem
