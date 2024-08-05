#pragma once

#include "src/drvcore.hpp"
#include "usb/drivers.hpp"

struct DrmDevice final : drvcore::ClassDevice {
	DrmDevice(drvcore::ClassSubsystem *subsystem, std::string name,
		std::shared_ptr<Device> parent, UnixDevice *unix_device)
		: drvcore::ClassDevice{subsystem, std::move(parent), std::move(name), unix_device} {
		skipInstallingUnixDevice = true;
	}

	void composeUevent(drvcore::UeventProperties &ue) override {
		ue.set("SUBSYSTEM", "drm");
	}

	std::optional<std::string> getClassPath() override {
		return "drm";
	};
};

struct InputDevice final : drvcore::ClassDevice {
	InputDevice(drvcore::ClassSubsystem *subsystem, std::string name,
		std::shared_ptr<Device> parent, UnixDevice *unix_device)
		: drvcore::ClassDevice{subsystem, std::move(parent), std::move(name), unix_device} {
		skipInstallingUnixDevice = true;
	}

	void composeUevent(drvcore::UeventProperties &ue) override {
		ue.set("SUBSYSTEM", "input");
	}

	std::optional<std::string> getClassPath() override {
		return "input";
	};
};

struct UsbMiscDevice final : drvcore::ClassDevice {
	UsbMiscDevice(drvcore::ClassSubsystem *subsystem, std::string name,
		std::shared_ptr<Device> parent)
		: drvcore::ClassDevice{subsystem, parent, std::move(name), nullptr} {
	}

	void composeUevent(drvcore::UeventProperties &ue) override {
		ue.set("DEVNAME", "/dev/cdc-wdm0");
		ue.set("SUBSYSTEM", "usbmisc");
	}

	std::optional<std::string> getClassPath() override {
		return "usbmisc";
	};

private:
	std::shared_ptr<CdcMbimDriver> _mbimDriver;
};

struct NetDevice final : drvcore::ClassDevice {
	NetDevice(drvcore::ClassSubsystem *subsystem, std::string name, int ifindex, std::shared_ptr<Device> parent)
		: drvcore::ClassDevice{subsystem, parent, name, nullptr},
		ifindex_{ifindex} {

	}

	void composeUevent(drvcore::UeventProperties &ue) override {
		ue.set("INTERFACE", name());
		ue.set("IFINDEX", std::to_string(ifindex_));
		ue.set("DEVTYPE", "wwan");
		ue.set("SUBSYSTEM", "net");
	}

	std::optional<std::string> getClassPath() override {
		return "net";
	};
private:
	int ifindex_;
};
