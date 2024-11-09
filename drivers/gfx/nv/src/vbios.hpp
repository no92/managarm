#pragma once

#include <cassert>
#include <cstring>
#include <span>

#include "spec.hpp"

namespace vbios {

template<typename T>
T biosRead(std::span<std::byte> buf, size_t offset) {
	assert(offset + sizeof(T) <= buf.size_bytes());

	T ret{};
	memcpy(reinterpret_cast<std::byte *>(&ret), buf.subspan(offset, sizeof(T)).data(), sizeof(T));
	return ret;
}

bool is_efi_compressed(pci::rom::header *hdr, pci::rom::pcir *pcir);
size_t locatePciHeader(std::span<std::byte> image);
std::pair<size_t, size_t> locateExpansionRoms(std::span<std::byte> image, size_t pci_offset);

} // namespace vbios
