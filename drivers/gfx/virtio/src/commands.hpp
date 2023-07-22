#pragma once

#include <async/result.hpp>

#include "src/virtio.hpp"

struct Cmd {
	static async::result<void> transferToHost2d(uint32_t width, uint32_t height, uint32_t resourceId, GfxDevice *device);
	static async::result<void> setScanout(uint32_t width, uint32_t height, uint32_t scanoutId, uint32_t resourceId, GfxDevice *device);
	static async::result<void> resourceFlush(uint32_t width, uint32_t height, uint32_t resourceId, GfxDevice *device);
	static async::result<spec::DisplayInfo> getDisplayInfo(GfxDevice *device);
	static async::result<void> create2d(uint32_t width, uint32_t height, uint32_t resourceId, GfxDevice *device);
	static async::result<void> attachBacking(uint32_t resourceId, void *ptr, size_t size, GfxDevice *device);
	static async::result<spec::CapsetInfo> getCapsetInfo(uint32_t capId, GfxDevice *device);
	static async::result<std::vector<uint8_t>> getCapset(uint32_t cap_id, uint32_t cap_version, uint32_t max_size, GfxDevice *device);
	static async::result<void> createContext(uint32_t context_id, uint32_t context_init, std::string debug_name, GfxDevice *device);
	static async::result<void> create3d(ObjectParams params, std::shared_ptr<GfxDevice::BufferObject> bo, GfxDevice *device);
};
