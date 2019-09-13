#!/usr/bin/env python
#
# Copyright 2019 The Nakama Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
from __future__ import print_function
import sys
import subprocess

filename = '../build_config.py'
if sys.version_info[0] <= 2:
    execfile(filename)
else:
    exec(compile(open(filename, "rb").read(), filename, 'exec'))

abi_list = ['armeabi-v7a', 'arm64-v8a', 'x86', 'x86_64']

def call(command):
    res = subprocess.call(command, shell=False)
    if res != 0:
        sys.exit(-1)

# static libs
if BUILD_NAKAMA_STATIC:
    for abi in abi_list:
        call(['python', 'build_android.py', abi])

# shared libs
if BUILD_NAKAMA_SHARED:
    for abi in abi_list:
        call(['python', 'build_android.py', abi, '--so'])
