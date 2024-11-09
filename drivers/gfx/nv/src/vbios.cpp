#include <ranges>
#include <optional>

#include "spec.hpp"
#include "vbios.hpp"

namespace vbios {

bool is_efi_compressed(pci::rom::header *hdr, pci::rom::pcir *pcir) {
	if(pcir->code_type != 3)
		return false;

	auto efi_hdr = reinterpret_cast<pci::rom::efi_header *>(hdr);

	return efi_hdr->compression_type != 0;
}

size_t locatePciHeader(std::span<std::byte> image) {
	auto ifr = biosRead<pci::ifr_header>(image, 0);
	size_t ifr_size = 0;

	if(ifr.signature == pci::IFR_SIGNATURE) {
		switch(ifr.ifr_version) {
			case 0x01:
			case 0x02:
				ifr_size = biosRead<uint32_t>(image, ifr.fixed_data_size + 4);
				break;
			case 0x03: {
				auto flash_status_offset = biosRead<uint32_t>(image, ifr.total_data_size);
				auto rom_directory_signature = biosRead<uint32_t>(image, flash_status_offset + 0x1000);
				if(rom_directory_signature == pci::RFRD_SIGNATURE)
					ifr_size = biosRead<uint32_t>(image, flash_status_offset + 0x1008);
				else
					assert(!"unknown ifr version");
				break;
			}
			default:
				assert(!"unknown ifr version");
		}
	}

	auto pci_rom_signature = biosRead<std::array<uint8_t, 2>>(image, ifr_size);
	std::array<uint8_t, 2> expected_pci_rom_signature = {{ 0x55, 0xAA }};
	if(pci_rom_signature != expected_pci_rom_signature) {
		assert(!"invalid PCI ROM header signature");
	}

	return ifr_size;
}

std::pair<size_t, size_t> locateExpansionRoms(std::span<std::byte> image, size_t pci_offset) {
	size_t image_offset = pci_offset;
	bool is_last_image = false;
	std::optional<uint32_t> base_rom_size = std::nullopt;
	std::optional<uint32_t> extended_rom_offset = std::nullopt;

	while(!is_last_image) {
		auto hdr = biosRead<pci::rom::header>(image, image_offset);
		auto pcir = biosRead<pci::rom::pcir>(image, image_offset + hdr.pcir_offset);

		std::array<uint32_t, 3> accepted_pcir_signatures = {{
			pci::rom::PCIR_SIGNATURE,
			pci::rom::NPDS_SIGNATURE,
			pci::rom::RGIS_SIGNATURE,
		}};

		assert(std::ranges::find(accepted_pcir_signatures, pcir.signature) != accepted_pcir_signatures.end());

		is_last_image = pcir.indicator & (1 << 7);
		size_t image_length = pcir.image_length;

		auto npde_offset = (image_offset + hdr.pcir_offset + pcir.pcir_length + 0x0F) & ~0x0F;
		auto npde = biosRead<pci::rom::npde>(image, npde_offset);

		if(npde.signature == pci::rom::NPDE_SIGNATURE && (npde.npde_revision == 0x100 || npde.npde_revision == 0x101)) {
			image_length = npde.subimage_len;

			if(offsetof(pci::rom::npde, last) + sizeof(npde.last) <= npde.npde_length)
				is_last_image = npde.last & (1 << 7);
			else if(npde.subimage_len < pcir.image_length)
				is_last_image = false;
		}

		if(pcir.code_type == pci::rom::pcir::code_type::PC_AT && !base_rom_size)
			base_rom_size = image_length * 512U;
		else if(pcir.code_type == pci::rom::pcir::code_type::NV_EXTENDED_VBIOS && !extended_rom_offset)
			extended_rom_offset = image_offset - pci_offset;

		image_offset += image_length * 512U;
	}

	uint32_t expansion_rom = 0;

	if(base_rom_size && extended_rom_offset)
		expansion_rom = *extended_rom_offset - *base_rom_size;

	return {image_offset, expansion_rom};
}

} // namespace vbios
