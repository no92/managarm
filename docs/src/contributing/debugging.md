# Debugging

The default setup is sufficient for running managarm, but doesn't ship with all the debugging enabled.

## QEMU

By default, `vm-util.py` will enable all the debugging options available when launching qemu. These include debugging via qemu's built-in gdb stub or for debugging userspace, non-server crashes. This happens automatically on crashes - no further action needs to be taken. On your host machine, launching gdb can be done via `vm-utils.py gdb`.
For qemu's gdb stub, use the `--qemu` flag; for kernel crashes, use the `--kernel` flag; for userspace crashes, use the `--posix` flag.
However, this approach has a limitation: crashes in servers like `posix-subsystem` or `netserver` will not spawn a gdb server. However, this can be achieved by using a dmalog-enabled QEMU build.

The `host-qemu` tool provided includes the dmalog patches. To build it, run `xbstrap install-tool host-qemu` and give it a few minutes. Once it is installed, `vm-util.py` will pick it up automatically and use the dmalog device for debugging.

When using a dmalog-enabled qemu, a gdb server is spawned even for servers crashing. A `gdb` instance can then be launched by running `vm-util.py gdb --kernel`.

## logging switches

Throughout the code you can find flags that toggle additional logging. Usually, they look something like this:

```cpp
constexpr bool logDrmRequests = false;
```

For instance, setting it to true enables, in this specific case, every `ioctl` to DRM devices to be logged. These are usually placed at the top of their respecive files, or in separate (`.hpp`) headers.

A brief, non-comprehensive list of such commonly used flags and where to find them:

- `logRequests` and others: [`posix/subsystem/src/debug-options.hpp`](https://github.com/managarm/managarm/blob/master/posix/subsystem/src/debug-options.hpp)
- `logSockets`: [`posix/subsystem/src/un-socket.cpp`](https://github.com/managarm/managarm/blob/master/posix/subsystem/src/un-socket.cpp)
- `logDrmRequests`: [`core/drm/include/core/drm/debug.hpp`](https://github.com/managarm/managarm/blob/master/core/drm/include/core/drm/debug.hpp)
