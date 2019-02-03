#!/usr/bin/env python
import sys
import subprocess
import os
import platform

if len(sys.argv) < 2:
    print "Pass ABI parameter."
    print "e.g. armeabi-v7a, arm64-v8a or x86"
    sys.exit(-1)

ABI = sys.argv[1]
BUILD_MODE = 'Release'
'''
if platform.system() == 'Windows':
    # check in Release
    grpc_cpp_plugin_path = os.path.abspath(r'..\win32\build\third_party\grpc\Release\grpc_cpp_plugin.exe')
    protoc_path = os.path.abspath(r'..\win32\build\third_party\grpc\third_party\protobuf\Release\protoc.exe')

    if not os.path.isfile(grpc_cpp_plugin_path) or not os.path.isfile(protoc_path):
        # check in Debug
        grpc_cpp_plugin_path = os.path.abspath(r'..\win32\build\third_party\grpc\Debug\grpc_cpp_plugin.exe')
        protoc_path = os.path.abspath(r'..\win32\build\third_party\grpc\third_party\protobuf\Debug\protoc.exe')
        
        if not os.path.isfile(grpc_cpp_plugin_path) or not os.path.isfile(protoc_path):
            print 'grpc_cpp_plugin_path =', grpc_cpp_plugin_path
            print 'protoc_path =', protoc_path
            print "please build for Windows first"
            sys.exit(-1)

    print 'grpc_cpp_plugin_path =', grpc_cpp_plugin_path
    print 'protoc_path =', protoc_path
'''
def getEnvVar(name):
    if name in os.environ:
        return os.environ[name]
    return ''

ANDROID_NDK = getEnvVar('ANDROID_NDK')
if not ANDROID_NDK:
    ANDROID_NDK = getEnvVar('NDK_ROOT')
    if not ANDROID_NDK:
        print "Error: no ANDROID_NDK or NDK_ROOT environment variable"
        sys.exit(-1)

def call(command):
    res = subprocess.call(command, shell=False)
    if res != 0:
        sys.exit(-1)

build_dir = './build/' + ABI + '/' + BUILD_MODE

if not os.path.isdir(build_dir):
    os.makedirs(build_dir)

print 'ANDROID_NDK=' + ANDROID_NDK

cmake_args = [
              'cmake',
              '-DANDROID_ABI=' + ABI,
              '-DCMAKE_TOOLCHAIN_FILE=' + ANDROID_NDK + '/build/cmake/android.toolchain.cmake',
              '-DBUILD_DEFAULT_WEBSOCKETS=OFF',
              '-DCMAKE_BUILD_TYPE=' + BUILD_MODE,
              '-DANDROID_NATIVE_API_LEVEL=16',
              '-B',
              build_dir,
              '-GNinja',
              '../..'
              ]

#if platform.system() == 'Windows':
#    cmake_args.append('-DPROTOBUF_PROTOC_EXECUTABLE=' + protoc_path)
#    cmake_args.append('-DGRPC_CPP_PLUGIN_EXECUTABLE=' + grpc_cpp_plugin_path)

# generate projects
call(cmake_args)

# build
call(['ninja', '-C', build_dir, 'nakama-cpp'])
