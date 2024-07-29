#pragma once

#include "src/drvcore.hpp"

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
