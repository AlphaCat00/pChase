project('pChase','cpp')

if get_option('buildtype').startswith('debug')
  add_project_arguments('-DDEBUG', language : 'cpp')
  message('DEBUG')
endif

add_project_arguments('-DGTOD', language : 'cpp')

dependencies = []

numa_dep = dependency('numa', required : false)
if numa_dep.found()
    add_project_arguments('-DNUMA', language : 'cpp')
    dependencies += [numa_dep]
endif

dependencies += [dependency('threads')]

cmake = import('cmake')
opt_var = cmake.subproject_options()
opt_var.add_cmake_defines({'ASMJIT_STATIC': true})
sub_prj = cmake.subproject('asmjit', options: opt_var)
dependencies += [sub_prj.dependency('asmjit')]

utils_lib = static_library('utils', 'src/spinbarrier.cpp', 'src/lock.cpp', 'src/thread.cpp', 'src/timer.cpp', 'src/output.cpp')

executable('chase', 'src/experiment.cpp', 'src/run.cpp', 'src/main.cpp', link_with: utils_lib, dependencies: dependencies)