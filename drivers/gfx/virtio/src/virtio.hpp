#pragma once

#include <queue>
#include <map>
#include <unordered_map>

#include <arch/mem_space.hpp>
#include <async/recurring-event.hpp>
#include <async/oneshot-event.hpp>
#include <async/mutex.hpp>
#include <async/result.hpp>
#include <core/drm/core.hpp>
#include <core/virtio/core.hpp>
#include <helix/ipc.hpp>

#include "spec.hpp"

struct ObjectParams {
	bool virgl;

	uint32_t size;

	uint32_t format;
	uint32_t width;
	uint32_t height;
	uint32_t target;
	uint32_t bind;
	uint32_t depth;
	uint32_t array_size;
	uint32_t last_level;
	uint32_t nr_samples;
	uint32_t flags;
};

struct Cmd;

struct GfxDevice final : drm_core::Device, std::enable_shared_from_this<GfxDevice> {
	friend struct Cmd;

	struct FrameBuffer;

	struct ScanoutState {
		ScanoutState()
		: width(0), height(0) { };

		GfxDevice::FrameBuffer *fb;
		std::shared_ptr<drm_core::Blob> mode;
		int width;
		int height;
	};

	struct Configuration : drm_core::Configuration {
		Configuration(GfxDevice *device)
		: _device(device) { };

		bool capture(std::vector<drm_core::Assignment> assignment, std::unique_ptr<drm_core::AtomicState> &state) override;
		void dispose() override;
		void commit(std::unique_ptr<drm_core::AtomicState> state) override;

	private:
		async::detached _dispatch(std::unique_ptr<drm_core::AtomicState> state);

		GfxDevice *_device;
	};

	struct Plane : drm_core::Plane {
		Plane(GfxDevice *device, int id, PlaneType type);

		int scanoutId();

	private:
		int _scanoutId;
	};

	struct BufferObject final : drm_core::BufferObject, std::enable_shared_from_this<BufferObject> {
		BufferObject(GfxDevice *device, uint32_t id, size_t size, helix::UniqueDescriptor memory,
			uint32_t width, uint32_t height)
		: drm_core::BufferObject{width, height}, _device{device}, _resourceId{id}, _size{size}, _memory{std::move(memory)}, _mapping{_memory, 0, _size} {
		};

		~BufferObject();

		std::shared_ptr<drm_core::BufferObject> sharedBufferObject() override;
		size_t getSize() override;
		std::pair<helix::BorrowedDescriptor, uint64_t> getMemory() override;
		async::detached _initHw();
		async::result<void> _initHw3d(ObjectParams params, uint32_t context_id);
		async::result<void> wait();
		uint32_t resourceId();

		bool is3D() const {
			return _is3D;
		}

	private:
		GfxDevice *_device;
		uint32_t _resourceId;
		size_t _size;
		helix::UniqueDescriptor _memory;
		helix::Mapping _mapping;
		async::oneshot_event _jump;
		const bool _is3D = false;
	};

	struct Connector : drm_core::Connector {
		Connector(GfxDevice *device);
	};

	struct Encoder : drm_core::Encoder {
		Encoder(GfxDevice *device);
	};

	struct Crtc final : drm_core::Crtc {
		Crtc(GfxDevice *device, int id, std::shared_ptr<Plane> plane);

		drm_core::Plane *primaryPlane() override;
		int scanoutId();
		// cursorPlane

	private:
		GfxDevice *_device;
		int _scanoutId;
		std::shared_ptr<Plane> _primaryPlane;
	};

	struct FrameBuffer final : drm_core::FrameBuffer {
		FrameBuffer(GfxDevice *device, std::shared_ptr<GfxDevice::BufferObject> bo);

		GfxDevice::BufferObject *getBufferObject();
		void notifyDirty() override;
		uint32_t getWidth() override;
		uint32_t getHeight() override;
		async::detached _xferAndFlush();

	private:
		std::shared_ptr<GfxDevice::BufferObject> _bo;
		GfxDevice *_device;
	};

	struct FileContext {
		std::optional<uint32_t> contextId;
	};

	GfxDevice(std::unique_ptr<virtio_core::Transport> transport);

	async::result<std::unique_ptr<drm_core::Configuration>> initialize();
	std::unique_ptr<drm_core::Configuration> createConfiguration() override;
	std::pair<std::shared_ptr<drm_core::BufferObject>, uint32_t> createDumb(uint32_t width,
			uint32_t height, uint32_t bpp) override;
	std::shared_ptr<drm_core::FrameBuffer>
			createFrameBuffer(std::shared_ptr<drm_core::BufferObject> bo,
			uint32_t width, uint32_t height, uint32_t format, uint32_t pitch) override;

	//returns major, minor, patchlvl
	std::tuple<int, int, int> driverVersion() override;
	//returns name, desc, date
	std::tuple<std::string, std::string, std::string> driverInfo() override;
	async::result<void> ioctl(void *object, uint32_t id, helix_ng::RecvInlineResult, helix::UniqueLane conversation) override;

private:
	async::result<uint32_t> createContext(drm_core::File *f);
	async::result<std::pair<std::shared_ptr<GfxDevice::BufferObject>, uint32_t>>
		createObject(drm_core::File *file, ObjectParams params);

	std::shared_ptr<Crtc> _theCrtcs[16];
	std::shared_ptr<Encoder> _theEncoders[16];
	std::shared_ptr<Plane> _thePlanes[16];
	std::shared_ptr<Connector> _activeConnectors[16];

	std::unique_ptr<virtio_core::Transport> _transport;
	virtio_core::Queue *_controlQ;
	virtio_core::Queue *_cursorQ;
	bool _claimedDevice;
	id_allocator<uint32_t> _resourceIdAllocator;
	id_allocator<uint32_t> _contextIdAllocator;

	bool _virgl3D = false;
};
