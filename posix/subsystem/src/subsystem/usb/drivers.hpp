#pragma once

#include "src/drvcore.hpp"

struct CdcNcmDriver final : drvcore::BusDriver {
	CdcNcmDriver(std::shared_ptr<drvcore::BusSubsystem> parent, std::string name)
	: drvcore::BusDriver(parent, name) {}
};
