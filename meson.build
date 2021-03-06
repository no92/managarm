project('managarm')

build_code = get_option('build_kernel') \
	or get_option('build_drivers') \
	or get_option('build_tools')

if build_code
	add_languages('c', 'cpp')
	c_compiler = meson.get_compiler('cpp')
	cpp_compiler = meson.get_compiler('cpp')

	protoc = find_program('protoc')
	bragi = find_program('bragi')

	add_project_arguments('--std=c++2a', language: 'cpp')

	# clang needs an additional flag to enable sized allocation/deallocation.
	if cpp_compiler.get_id() == 'clang'
		add_project_arguments('-fsized-deallocation', language: 'cpp')
	endif

	bragi_dep = declare_dependency(
		include_directories: include_directories('subprojects/bragi/include')
	)
endif

if get_option('build_kernel')
	pb2frigg = find_program('pb2frigg')

	if host_machine.cpu_family() == 'x86_64'
		add_global_arguments(
			[
				'-mno-80387',
				'-mno-mmx',
				'-mno-sse',
				'-mno-sse2'
			],
			language: ['c', 'cpp'])
	endif


	add_global_arguments(
		[
			'-ffreestanding',
		],
		language: ['c', 'cpp'])

	add_global_arguments(['-fno-exceptions', '-fno-rtti'], language: 'cpp')

	add_project_arguments('-fcoroutines-ts', language: 'cpp')

	cralgo = subproject('cralgo')
	cxxshim = subproject('cxxshim')
	frigg = subproject('frigg')
	libarch = subproject('libarch', default_options: ['install_headers=false', 'header_only=true'])
	libarch_dep = libarch.get_variable('libarch_dep')
	lai = subproject('lai')
	libsmarter = subproject('libsmarter')
	cralgo_sources = cralgo.get_variable('sources')
	cralgo_includes = cralgo.get_variable('includes')
	clang_coroutine_dep = cxxshim.get_variable('clang_coroutine_dep')
	cxxshim_dep = cxxshim.get_variable('cxxshim_dep')
	frigg_dep = frigg.get_variable('frigg_dep')
	lai_sources = lai.get_variable('sources')
	lai_includes = lai.get_variable('includes')
	libsmarter_dep = libsmarter.get_variable('libsmarter_dep')

	subdir('kernel/eir')
	subdir('kernel/thor')
endif

if get_option('build_drivers')
	bakesvr = find_program('bakesvr')

	add_project_arguments('-fcoroutines-ts', '-DLIBASYNC_FORCE_USE_EXPERIMENTAL', language: 'cpp')

	cxxshim = subproject('cxxshim')
	clang_coroutine_dep = cxxshim.get_variable('clang_coroutine_dep')

	proto_lite_dep = dependency('protobuf-lite')
	libudev_dep = dependency('libudev')

	libarch = subproject('libarch', default_options: 'install_headers=true')
	libarch_dep = libarch.get_variable('libarch_dep')
endif

subdir('hel')

if get_option('build_drivers')
	subdir('protocols/clock/')
	subdir('protocols/fs/')
	subdir('protocols/hw/')
	subdir('protocols/mbus/')
	subdir('protocols/usb/')
	subdir('protocols/svrctl/')
	subdir('protocols/kernlet/') # Depends on mbus.
	subdir('core/drm/')
	subdir('core/virtio/')
	subdir('mbus/')
	subdir('posix/subsystem/')
	subdir('posix/init/')
	subdir('drivers/libblockfs/')
	subdir('drivers/libevbackend/')
	subdir('drivers/block/ata')
	subdir('drivers/block/virtio-blk/')
	subdir('drivers/gfx/bochs/')
	subdir('drivers/gfx/intel/')
	subdir('drivers/gfx/virtio/')
	subdir('drivers/gfx/plainfb/')
	subdir('drivers/gfx/vmware/')
	subdir('drivers/kbd/')
	subdir('drivers/tty/virtio-console')
	subdir('drivers/uart/')
	subdir('drivers/usb/hcds/uhci/')
	subdir('drivers/usb/hcds/ehci/')
	subdir('drivers/usb/hcds/xhci/')
	subdir('drivers/usb/devices/hid/')
	subdir('drivers/usb/devices/storage/')
	subdir('drivers/kernletcc')
	subdir('utils/runsvr/')
	subdir('utils/lsmbus/')
	subdir('testsuites/kernel-tests/')
	subdir('testsuites/posix-torture/')
	subdir('testsuites/posix-tests/')

	subdir('drivers/nic/virtio/')
	subdir('servers/netserver/')

	subdir('drivers/clocktracker')

	install_data(
		[
			'drivers/gfx/90-managarm-vga.rules',
			'drivers/nic/90-managarm-nic.rules',
		],
		install_dir: 'lib/udev/rules.d')
endif

if get_option('build_tools')
	subdir('tools/bakesvr')
	subdir('tools/pb2frigg')
endif

subdir('docs')
