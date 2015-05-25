import platform
import sys
import os
import subprocess

from os.path import join

# Path helpers -----------------------------------------------------------------
dir_tools = os.path.dirname(os.path.realpath(__file__))
dir_root  = os.path.abspath(join(dir_tools, '..'))
dir_regit  = os.path.realpath(dir_root)
dir_include                = join(dir_root, 'include')
dir_src                    = join(dir_root, 'src')
dir_test                   = join(dir_root, 'test')
dir_build                  = join(dir_root, 'build')
dir_build_latest           = join(dir_build, 'latest')


def ensure_dir(path_name):
  if not os.path.exists(path_name):
    os.makedirs(path_name)

def check_command_available(command, help=None):
  rc = subprocess.call(['which', command])
  if rc != 0:
    print('ERROR: %s command not found.' % command)
    if help:
      print help
    sys.exit(1)


# Build helpers ----------------------------------------------------------------
build_options_modes = ['release', 'debug']

build_options_oses = ['macos', 'linux']
def GuessOS():
  id = platform.system()
  if id == 'Linux':
    return 'linux'
  elif id == 'Darwin':
    return 'macos'
  else:
    return None

