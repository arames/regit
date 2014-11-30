# Vim YouCompleteMe completion configuration.

import os
import platform

repo_root = os.path.dirname(os.path.abspath(__file__))

# Paths in the compilation flags must be absolute to allow ycm to find them from
# any working directory.
def AbsolutePath(path):
  return os.path.join(repo_root, path)

flags = [
  '-I', AbsolutePath('include'),
  '-I', AbsolutePath('src'),
  '-Wall',
  '-Werror',
  '-Wextra',
  '-pedantic',
  '-Wwrite-strings',
  '-std=c++11',
  '-x', 'c++',
  '-DDEBUG',
  '-DMOD_FLAGS',
  '-isystem',
    '/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include/c++/v1',
  '-isystem', '/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.10.sdk/usr/include/'
]

def FlagsForFile(filename, **kwargs):
  return {
    'flags': flags,
    'do_cache': True
  }

