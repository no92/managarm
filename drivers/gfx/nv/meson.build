src = files(
	'src/main.cpp',
	'src/vbios.cpp',
)

executable('gfx_nv', src,
	dependencies : [drm_core_dep, svrctl_proto_dep],
	install : true
)

custom_target('gfx-nv-server',
	command : [bakesvr, '-o', '@OUTPUT@', '@INPUT@'],
	output : 'gfx-nv.bin',
	input : 'gfx-nv.yml',
	install : true,
	install_dir : server
)
