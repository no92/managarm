#pragma once

#include <array>
#include <cstdint>

namespace pci {

constexpr uint32_t IFR_SIGNATURE = 0x4947564E;
constexpr uint32_t RFRD_SIGNATURE = 0x44524652;

struct [[gnu::packed]] ifr_header {
	uint32_t signature;
	uint8_t unknown;
	uint8_t ifr_version;
	uint16_t fixed_data_size;
	uint32_t total_data_size: 20;
	uint32_t reserved: 12;
};

struct [[gnu::packed]] rfrd_header {
	uint32_t signature;
	uint32_t unknown;
	uint16_t rfrd_size;
};

namespace rom {

struct [[gnu::packed]] header {
	std::array<uint8_t, 2> signature;
	uint8_t reserved[0x16];
	uint16_t pcir_offset;
};

struct [[gnu::packed]] efi_header {
	std::array<uint8_t, 2> signature;
	// in units of 512 bytes
	uint16_t initialization_size;
	uint32_t efi_signature;
	uint16_t efi_subsystem;
	uint16_t efi_machine_type;
	uint16_t compression_type;
	uint8_t reserved[8];
	uint16_t efi_image_offset;
	uint16_t pcir_offset;
};

static_assert(sizeof(header) == sizeof(efi_header));

constexpr uint32_t PCIR_SIGNATURE = 0x52494350;
constexpr uint32_t NPDS_SIGNATURE = 0x5344504E;
constexpr uint32_t RGIS_SIGNATURE = 0x53494752;

struct [[gnu::packed]] pcir {
	enum code_type : uint8_t {
		PC_AT = 0x00,
		EFI = 0x03,
		NV_EXTENDED_VBIOS = 0xE0,
	};

	uint32_t signature;
	uint16_t vendor;
	uint16_t device;
	uint16_t reserved0;
	uint16_t pcir_length;
	uint8_t pcir_revision;
	uint8_t class_code[3];
	uint16_t image_length;
	uint16_t revision_level;
	code_type code_type;
	uint8_t indicator;
	uint16_t reserved1;
};

static_assert(sizeof(pcir) == 0x18);

constexpr uint32_t NPDE_SIGNATURE = 0x4544504E;

struct [[gnu::packed]] npde {
	uint32_t signature;
	uint16_t npde_revision;
	uint16_t npde_length;
	uint16_t subimage_len;
	uint8_t last;
	uint8_t flags;
};

} // namespace rom

} // namespace pci

namespace bit {

struct [[gnu::packed]] header {
	uint16_t id;
	uint32_t signature;
	uint16_t bcd_version;
	uint8_t header_size;
	uint8_t token_size;
	uint8_t token_entries;
	uint8_t bit_header_checksum;
};

struct [[gnu::packed]] token {
	uint8_t id;
	uint8_t data_version;
	uint16_t data_size;
	uint16_t data_offset;
};

namespace falcon {

struct [[gnu::packed]] header {
	uint8_t version;
	uint8_t header_size;
	uint8_t entry_size;
	uint8_t entry_count;
	uint8_t desc_version;
	uint8_t desc_size;
};

struct [[gnu::packed]] entry {
	uint8_t application_id;
	uint8_t target_id;
	uint32_t desc_offset;
};

struct [[gnu::packed]] desc_header {
	uint8_t version_available: 1;
	uint8_t reserved0: 1;
	uint8_t encrypted: 1;
	uint8_t reserved1: 5;
	uint8_t version;
	uint16_t size;
};

static_assert(sizeof(desc_header) == 4);

} // namespace falcon

} // namespace bit
