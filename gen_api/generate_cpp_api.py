import os
import sys
import subprocess
import argparse

def getEnvVar(name):
    if name in os.environ:
        return os.environ[name]
    return ''

parser = argparse.ArgumentParser(description='Nakama C++ API generator')
parser.add_argument('-n', '--nakama', help='Nakama server sources')
parser.add_argument('-g', '--gateway', help='grpc-gateway sources')

args = parser.parse_args()

def getArgOrEnvVar(env_var_name, arg_value):
    if arg_value:
        value = arg_value
    else:
        value = getEnvVar(env_var_name)
        if not value:
            print 'Error: missing "' + env_var_name + '" env variable'
            sys.exit(-1)

    return value

# https://github.com/heroiclabs/nakama
NAKAMA = getArgOrEnvVar('NAKAMA', args.nakama)

# https://github.com/grpc-ecosystem/grpc-gateway
GRPC_GATEWAY = getArgOrEnvVar('GRPC_GATEWAY', args.gateway)

def path(p):
    return os.path.normpath(p)

def find_grpc_cpp_plugin():
    grpc_cpp_plugin = path(NAKAMA_CPP + '/build/win32/build/third_party/grpc/Debug/grpc_cpp_plugin.exe')
    if not os.path.exists(grpc_cpp_plugin):
        grpc_cpp_plugin = path(NAKAMA_CPP + '/build/win32/build/third_party/grpc/Release/grpc_cpp_plugin.exe')

    if not os.path.exists(grpc_cpp_plugin):
        print 'Please build for desktop OS first'
        sys.exit(-1)
    
    return grpc_cpp_plugin

def find_protoc():
    protoc = path(NAKAMA_CPP + '/build/win32/build/third_party/grpc/third_party/protobuf/Debug/protoc.exe')
    if not os.path.exists(protoc):
        protoc = path(NAKAMA_CPP + '/build/win32/build/third_party/grpc/third_party/protobuf/Release/protoc.exe')

    if not os.path.exists(protoc):
        print 'Please build for desktop OS first'
        sys.exit(-1)
    
    return protoc

NAKAMA_CPP      = os.path.abspath('./..')
GRPC            = path(NAKAMA_CPP + '/third_party/grpc')
GOOGLEAPIS      = path(GRPC_GATEWAY + '/third_party/googleapis')
GRPC_CPP_PLUGIN = find_grpc_cpp_plugin()
PROTOC          = find_protoc()
PROTOBUF_SRC    = path(GRPC + '/third_party/protobuf/src')
OUT             = os.path.abspath('cppout')

def call(commands, shell=False):
    #print 'call', str(commands)
    res = subprocess.call(commands, shell=shell)
    if res != 0:
        sys.exit(-1)

def check_required_folder(folder):
    if not os.path.exists(folder):
        print 'ERROR: not exist', folder
        sys.exit(-1)

def makedirs(path):
    if not os.path.exists(path):
        os.makedirs(path)

def mklink(link, target):
    if not os.path.exists(target):
        call(['mklink', link, target], shell=True)

check_required_folder(NAKAMA)
check_required_folder(GRPC)
check_required_folder(GRPC_GATEWAY)
check_required_folder(GOOGLEAPIS)
check_required_folder(GRPC_CPP_PLUGIN)
check_required_folder(PROTOC)
check_required_folder(PROTOBUF_SRC)

CUR_DIR = os.path.abspath('.')

makedirs(OUT)
makedirs(path(OUT + '/google/api'))
makedirs(path(OUT + '/google/rpc'))

makedirs(path('github.com/heroiclabs/nakama/api'))
makedirs(path('github.com/heroiclabs/nakama/apigrpc'))
makedirs(path('github.com/heroiclabs/nakama/rtapi'))
mklink(path('github.com/heroiclabs/nakama/api/api.proto'), path(NAKAMA + '/api/api.proto'))
mklink(path('github.com/heroiclabs/nakama/apigrpc/apigrpc.proto'), path(NAKAMA + '/apigrpc/apigrpc.proto'))
mklink(path('github.com/heroiclabs/nakama/rtapi/realtime.proto'), path(NAKAMA + '/rtapi/realtime.proto'))

print 'generating apigrpc'

call([PROTOC, '-I.', '-I' + GRPC_GATEWAY, '-I' + GOOGLEAPIS, '-I' + PROTOBUF_SRC, '--grpc_out=' + OUT, '--plugin=protoc-gen-grpc=' + GRPC_CPP_PLUGIN, path('github.com/heroiclabs/nakama/apigrpc/apigrpc.proto')])
call([PROTOC, '-I.', '-I' + GRPC_GATEWAY, '-I' + GOOGLEAPIS, '-I' + PROTOBUF_SRC, '--cpp_out=' + OUT, path('github.com/heroiclabs/nakama/apigrpc/apigrpc.proto')])
call([PROTOC, '-I.', '-I' + GRPC_GATEWAY, '-I' + GOOGLEAPIS, '-I' + PROTOBUF_SRC, '--cpp_out=' + OUT, path('github.com/heroiclabs/nakama/api/api.proto')])

os.chdir(path(GOOGLEAPIS + '/google/rpc'))

call([PROTOC, '-I.', '-I' + GRPC_GATEWAY, '-I' + GOOGLEAPIS, '-I' + PROTOBUF_SRC, '--cpp_out=' + path(OUT + '/google/rpc'), 'status.proto'])

os.chdir(path(GOOGLEAPIS + '/google/api'))

call([PROTOC, '-I.', '-I' + GRPC_GATEWAY, '-I' + GOOGLEAPIS, '-I' + PROTOBUF_SRC, '--cpp_out=' + path(OUT + '/google/api'), 'annotations.proto'])
call([PROTOC, '-I.', '-I' + GRPC_GATEWAY, '-I' + GOOGLEAPIS, '-I' + PROTOBUF_SRC, '--cpp_out=' + path(OUT + '/google/api'), 'http.proto'])

os.chdir(CUR_DIR)

call([PROTOC, '-I.', '-I' + GRPC_GATEWAY, '-I' + GOOGLEAPIS, '-I' + PROTOBUF_SRC, '--cpp_out=' + OUT, path(GRPC_GATEWAY + '/protoc-gen-swagger/options/annotations.proto')])
call([PROTOC, '-I.', '-I' + GRPC_GATEWAY, '-I' + GOOGLEAPIS, '-I' + PROTOBUF_SRC, '--cpp_out=' + OUT, path(GRPC_GATEWAY + '/protoc-gen-swagger/options/openapiv2.proto')])

print 'generating rtapi'

call([PROTOC, '-I.', '-I' + PROTOBUF_SRC, '--cpp_out=' + OUT, path('github.com/heroiclabs/nakama/rtapi/realtime.proto')])

print 'done.'
