project('raft_sobjects', 'cpp',
	default_options: ['cpp_std=c++17', 'warning_level=3'])

thread_dep = dependency('threads')
src = ['src/main.cpp']

inc_dir = include_directories(['include', '/usr/lib/so_5'])

lib_sobj = meson.get_compiler('cpp').find_library('so.5.7.0',
    dirs : ['/usr/lib'], required : true, has_headers : 'so_5/all.hpp',
    header_include_directories : include_directories('/usr/lib/so_5'))

executable('raft_consensus', sources: src, include_directories: inc_dir,
dependencies : [thread_dep, lib_sobj])