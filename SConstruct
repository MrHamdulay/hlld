import platform
plat = platform.system()

envmurmur = Environment(CC='clang', CXX='clang++', CPPPATH = ['deps/murmurhash/'], CPPFLAGS="-fno-exceptions -O0")
murmur = envmurmur.Library('murmur', Glob("deps/murmurhash/*.cpp"))


envinih = Environment(CC='gcc-4.9', CXX='g++-4.9', CPATH = ['deps/inih/'], CFLAGS="-O0")
inih = envinih.Library('inih', Glob("deps/inih/*.c"))

env_with_err = Environment(CC='g++-4.9', CXX='g++-4.9', CFLAGS='', CXXFLAGS='-std=c++11', CCFLAGS = '-g -D_GNU_SOURCE -Wall -Wextra -Werror -O0 -pthread -Isrc/ -Ideps/inih/ -Ideps/libev/ -Igen-cpp/')
env_without_unused_err = Environment(CC='g++-4.9', CXX='g++-4.9', CFLAGS='', CXXFLAGS='-std=c++11', CCFLAGS = '-g -D_GNU_SOURCE -Wall -Wextra -Wno-unused-function -Wno-unused-result -Werror -O0 -pthread -Isrc/ -Ideps/inih/ -Ideps/libev/ -Igen-cpp/')
env_without_err = Environment(CC='g++-4.9', CXX='g++-4.9', CFLAGS='', CXXFLAGS='-std=c++11', CCFLAGS = '-g -D_GNU_SOURCE -O0 -pthread -Isrc/ -Ideps/inih/ -Ideps/libev/ -Igen-cpp/')

objs =  env_with_err.Object('src/config', 'src/config.c') + \
        env_with_err.Object('src/convert', 'src/convert.c') + \
        env_with_err.Object('src/barrier', 'src/barrier.c') + \
        env_with_err.Object('src/hll', 'src/hll.c') + \
        env_with_err.Object('src/hll_constants', 'src/hll_constants.c') + \
        env_with_err.Object('src/bitmap', 'src/bitmap.c') + \
        env_with_err.Object('src/set', 'src/set.c') + \
        env_with_err.Object('src/set_manager', 'src/set_manager.c') + \
        env_with_err.Object('src/serialize', 'src/serialize.cpp') + \
        env_without_err.Object('src/networking', 'src/networking.c') + \
        env_with_err.Object('src/conn_handler', 'src/conn_handler.c') + \
        env_with_err.Object('src/art', 'src/art.c') + \
        env_with_err.Object('src/background', 'src/background.c') + \
        env_without_err.Object('src/thrift_server', 'src/thrift_server.cpp') + \
        env_without_err.Object('gen-cpp/sliding-hyper_types', 'gen-cpp/sliding-hyper_types.cpp') + \
        env_without_err.Object('gen-cpp/SlidingHyperService', 'gen-cpp/SlidingHyperService.cpp')
        #env_without_err.Object('deps/libev/ev', 'deps/libev/ev.c')


libs = ["pthread", murmur, inih, "m", "thrift", "rocksdb"]
if plat == 'Linux':
   libs.append("rt")

hlld = env_with_err.Program('hlld', objs + ["src/hlld.c"], LIBS=libs)

if plat == "Darwin":
    test = env_without_err.Program('test_runner', objs + Glob("tests/runner.c"), LIBS=libs + ["check"])
else:
    test = env_without_unused_err.Program('test_runner', objs + Glob("tests/runner.c"), LIBS=libs + ["check"])

bench_obj = Object("bench", "bench.c", CXXFLAGS='-std=c++11', CCFLAGS=" -O0")
Program('bench', bench_obj, LIBS=["pthread"])

# By default, only compile hlld
Default(hlld)
