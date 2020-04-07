project('raft_sobjects', 'cpp',
	default_options: ['cpp_std=c++17', 'warning_level=3'])

thread_dep = dependency('threads')
src = ['src/main.cpp']

inc_dir = include_directories(['include',
								get_option('sobjectizer_include_dir')])

lib_sobj = meson.get_compiler('cpp').find_library('so.5.7.0',
    dirs : ['/usr/lib'])

executable('raft_consensus', sources: src, include_directories: inc_dir,
dependencies : [thread_dep, lib_sobj])