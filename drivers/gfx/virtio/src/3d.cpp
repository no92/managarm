#include <async/result.hpp>
#include <core/drm/debug.hpp>
#include <optional>

#include "src/commands.hpp"
#include "src/virtio.hpp"

async::result<uint32_t> GfxDevice::createContext(drm_core::File *f) {
	FileContext *fc = reinterpret_cast<FileContext *>(f->getDriverContext());

	if(!fc) {
		fc = new FileContext{};
		f->setDriverContext(fc);
	} else if(fc->contextId.has_value()) {
		co_return fc->contextId.value();
	}

	uint32_t chosen_context_id = _contextIdAllocator.allocate();
	auto name = "virtio-gpu-ctx-" + std::to_string(chosen_context_id);

	if(logDrmRequests)
		std::cout << "gfx/virtio: setting up context '" << name << "'" << std::endl;

	co_await Cmd::createContext(chosen_context_id, 0, std::move(name), this);

	fc->contextId.emplace(chosen_context_id);
	co_return chosen_context_id;
}

async::result<std::pair<std::shared_ptr<GfxDevice::BufferObject>, uint32_t>> GfxDevice::createObject(drm_core::File *file, ObjectParams params) {
	HelHandle handle;
	auto size = (params.size + (4096 - 1)) & ~(4096 - 1);
	HEL_CHECK(helAllocateMemory(size, 0, nullptr, &handle));

	auto bo = std::make_shared<BufferObject>(this, _resourceIdAllocator.allocate(), size,
			helix::UniqueDescriptor(handle), params.width, params.height, true);

	auto mapping = installMapping(bo.get());
	bo->setupMapping(mapping);

	assert(params.virgl);

	uint32_t context_id = co_await createContext(file);
	co_await bo->_initHw3d(std::move(params), context_id);

	auto drm_handle = file->createHandle(bo);

	co_return {bo, drm_handle};
}
