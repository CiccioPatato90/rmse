project('sched_with_batsim', 'cpp',
  version: '0.1.0',
  license: 'LGPL-3.0',
  default_options: ['cpp_std=c++17'],
  meson_version: '>=0.40.0'
)

batprotocol_cpp_dep = dependency('batprotocol-cpp')
intervalset_dep = dependency('intervalset')
nlohmann_json_dep = dependency('nlohmann_json')
deps = [
  batprotocol_cpp_dep
, intervalset_dep
, nlohmann_json_dep
]

common = ['src/batsim_edc.h']

exec1by1 = shared_library('exec1by1', common + ['src/exec1by1.cpp'],
  dependencies: deps,
  install: true,
)

fcfs = shared_library('fcfs', common + ['src/fcfs.cpp'],
  dependencies: deps,
  install: true,
)

easy_backfill = shared_library('easy_backfill', common + ['src/easy_backfill.cpp'],
  dependencies: deps,
  install: true,
)

basic = shared_library('basic', common + ['src/basic.cpp'],
  dependencies: deps,
  install: true,
)

best_cont = shared_library('best_cont', common + ['src/best_cont.cpp'],
  dependencies: deps,
  install: true,
)

force_cont = shared_library('force_cont', common + ['src/force_cont.cpp'],
  dependencies: deps,
  install: true,
)
