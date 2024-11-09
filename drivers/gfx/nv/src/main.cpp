#include <format>
#include <protocols/hw/client.hpp>
#include <protocols/mbus/client.hpp>
#include <protocols/svrctl/server.hpp>
#include <helix/memory.hpp>

#include "gfx.hpp"
#include "spec.hpp"
#include "vbios.hpp"

std::unordered_map<int64_t, std::shared_ptr<GfxDevice>> baseDeviceMap;

namespace {

using namespace vbios;

void parseFalcon(std::span<std::byte> expansion_rom, bit::token *token, uint32_t ucode_table_offset) {
	std::cout << std::format("Falcon version={} table_offset={:#x}\n", token->data_version, ucode_table_offset);

	auto hdr = biosRead<bit::falcon::header>(expansion_rom, ucode_table_offset);

	std::cout << std::format("Falcon header version={:#x} header_size={:#x} entries={}\n", hdr.version, hdr.header_size, hdr.entry_count);

	for(size_t i = 0; i < hdr.entry_count; i++) {
		auto entry = biosRead<bit::falcon::entry>(expansion_rom, ucode_table_offset + hdr.header_size + (i * hdr.entry_size));
		if(entry.application_id != 0x05 && entry.application_id != 0x45 && entry.application_id != 0x85)
			continue;

		auto desc_hdr = biosRead<bit::falcon::desc_header>(expansion_rom, entry.desc_offset);
		assert(desc_hdr.version_available);

		auto desc_size = desc_hdr.size;

		if(desc_hdr.version == 2) {
			assert(desc_size >= 60);
		} else if(desc_hdr.version == 3) {
			assert(desc_size >= 44);
		} else {
			assert(!"unsupported Falcon ucode table version");
		}

		std::cout << std::format("Falcon entry type={:#x} desc_version={} size={}\n",
			entry.application_id, desc_hdr.version, desc_size);
	}
}

void processImage(std::span<std::byte> rom, std::span<std::byte>expansion_rom) {
	std::array<std::byte, 5> bit_signature = { {
		std::byte(0xFF), std::byte(0xB8), std::byte('B'), std::byte('I'), std::byte('T')
	}};

	auto res = std::ranges::search(rom, bit_signature);
	assert(!res.empty());
	auto bit_header_offset = std::distance(rom.begin(), res.begin());

	auto bit_header = biosRead<bit::header>(rom, bit_header_offset);

	auto entries = bit_header.token_entries;
	auto signature = bit_header.signature;
	std::cout << std::format("BIT header signature={:x} entries={}\n", signature, entries);

	auto token = biosRead<bit::token>(rom, bit_header_offset + sizeof(bit::header));
	for(size_t i = 0; i < entries; i++) {
		auto data_offset = token.data_offset;
		std::cout << std::format("Token ID={:#x} ({:c}) version={} data_offset={:#x}\n", token.id, token.id, token.data_version, data_offset);

		switch(token.id) {
			case 'p': {
				assert(token.data_version == 2);
				assert(token.data_size >= 4);
				auto ucode_table_offset = biosRead<uint32_t>(rom, token.data_offset);
				parseFalcon(expansion_rom, &token, ucode_table_offset);
				break;
			}
			default:
				break;
		}

		token = biosRead<bit::token>(rom, bit_header_offset + sizeof(bit::header) + (i * bit_header.token_size));
	}
}

}

GfxDevice::GfxDevice(protocols::hw::Device hw_device)
: hwDevice_{std::move(hw_device)} {

}

async::result<void> GfxDevice::initialize() {
	auto info = co_await hwDevice_.getPciInfo();
	auto bar0 = co_await hwDevice_.accessBar(0);

	helix::Mapping mapping{bar0, info.barInfo[0].offset, info.barInfo[0].length};
	regs_ = {mapping.get()};

	auto val = regs_.load(arch::scalar_register<uint32_t>(0x88050));

	if(val & 1)
		regs_.store(arch::scalar_register<uint32_t>(0x88050), val & ~1);

	for(size_t i = 0; i < 0x100000; i += 4) {
		vbiosData_.push_back(regs_.load(arch::scalar_register<uint32_t>(0x300000 + i)));
	}

	std::span<std::byte> image{reinterpret_cast<std::byte *>(vbiosData_.data()), 0x100000};

	auto pci_rom_offset = locatePciHeader(image);
	auto [bios_size, expansion_rom_offset] = locateExpansionRoms(image, pci_rom_offset);
	image = image.subspan(0, bios_size);

	std::cout << std::format("BIOS total size {:#x} bytes, Expansion ROM offset {:#x}\n", bios_size, expansion_rom_offset);

	auto expansion_rom = image.subspan(pci_rom_offset + expansion_rom_offset);
	auto hdr = biosRead<pci::rom::header>(image, pci_rom_offset);
	auto pcir = biosRead<pci::rom::pcir>(image, pci_rom_offset + hdr.pcir_offset);
	assert(pcir.signature == pci::rom::PCIR_SIGNATURE);

	std::cout << std::format("Image offset={:#x} CodeType={} length={:#x}\n",
		pci_rom_offset, uint8_t(pcir.code_type), pcir.image_length * 512);

	auto rom = image.subspan(pci_rom_offset, image.size_bytes() - pci_rom_offset);
	auto npde_offset = (hdr.pcir_offset + pcir.pcir_length + 0x0F) & ~0x0F;

	auto npde = biosRead<pci::rom::npde>(rom, npde_offset);
	assert(npde.signature == pci::rom::NPDE_SIGNATURE);

	assert(!is_efi_compressed(&hdr, &pcir));
	processImage(rom, expansion_rom);

	co_return;
}

// ----------------------------------------------------------------
//
// ----------------------------------------------------------------

async::result<void> bindController(mbus_ng::Entity hwEntity) {
	protocols::hw::Device hwDevice((co_await hwEntity.getRemoteLane()).unwrap());
	auto gfxDevice = std::make_shared<GfxDevice>(std::move(hwDevice));
	co_await gfxDevice->initialize();

	std::cout << "gfx/nv: setup complete!" << std::endl;
}

async::result<protocols::svrctl::Error> bindDevice(int64_t base_id) {
	std::cout << "gfx/nv: Binding to device " << base_id << std::endl;
	auto baseEntity = co_await mbus_ng::Instance::global().getEntity(base_id);

	// Do not bind to devices that are already bound to this driver.
	if(baseDeviceMap.find(baseEntity.id()) != baseDeviceMap.end())
		co_return protocols::svrctl::Error::success;

	// Make sure that we only bind to supported devices.
	auto properties = (co_await baseEntity.getProperties()).unwrap();
	if(auto vendor_str = std::get_if<mbus_ng::StringItem>(&properties["pci-vendor"]);
			!vendor_str || vendor_str->value != "10de")
		co_return protocols::svrctl::Error::deviceNotSupported;
	if(auto device_str = std::get_if<mbus_ng::StringItem>(&properties["pci-class"]);
			!device_str || device_str->value != "03")
		co_return protocols::svrctl::Error::deviceNotSupported;
	if(auto device_str = std::get_if<mbus_ng::StringItem>(&properties["pci-subclass"]);
			!device_str || device_str->value != "00")
		co_return protocols::svrctl::Error::deviceNotSupported;
	if(auto device_str = std::get_if<mbus_ng::StringItem>(&properties["pci-interface"]);
			!device_str || device_str->value != "00")
		co_return protocols::svrctl::Error::deviceNotSupported;

	co_await bindController(std::move(baseEntity));
	co_return protocols::svrctl::Error::success;
}

static constexpr protocols::svrctl::ControlOperations controlOps = {
	.bind = bindDevice
};

int main() {
	std::cout << "gfx/nv: Starting driver" << std::endl;

	async::detach(protocols::svrctl::serveControl(&controlOps));
	async::run_forever(helix::currentDispatcher);
}
