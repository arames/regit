import sys
import os
from os.path import join
import subprocess

dir_root = os.path.dirname(File('SConstruct').rfile().abspath)
sys.path.insert(0, join(dir_root, 'tools'))
import utils

Help("""
Build system for the Regit project.
""")

class TopLevelTargets:
  def __init__(self):
    self.targets = []
    self.help_messages = []
  def Add(self, target, help_message):
    self.targets += [target]
    self.help_messages += [help_message]
  def Print(self):
    res = ""
    for i in range(0, len(self.targets)):
      # TODO: Ideally the help messages should be aligned.
      res += '\tscons '
      res += self.targets[i]
      res += '\t:\t'
      res += self.help_messages[i]
      res += '\n'
    return res

top_level_targets = TopLevelTargets()
    

# Build options ----------------------------------------------------------------

# Store all the options in a dictionary.
# The SConstruct will check the build variables and construct the build
# environment as appropriate.
options = {
    'all' : { # Unconditionally processed.
      'CCFLAGS' : ['-std=c++11', '-Wall', '-Werror', '-pedantic'],
      'CCFLAGS' : ['-Wall', '-pedantic'],
      'CXXFLAGS' : ['-std=c++11'],
      'CPPPATH' : map(lambda p: join(utils.dir_regit, p), ['src/', 'include/'])
      },
#   'build_option:value' : {
#     'environment_key' : 'values to append'
#     },
    'mode:debug' : {
      'CCFLAGS' : ['-DDEBUG', '-O0']
      },
    'mode:release' : {
      'CCFLAGS' : ['-O3']
      },
    'modifiable_flags:on' : {
      'CCFLAGS' : ['-DMOD_FLAGS']
      },
    'os:macos' : {
      # argp is usually installed there by `brew`.
      'CCFLAGS' : ['-I/usr/local/include'],
      'LINKFLAGS' : ['-L/usr/local/lib']
      },
    'symbols:on' : {
      'CCFLAGS' : ['-g'],
      'LINKFLAGS' : ['-g']
      },
    }


# DefaultVariable have a default value that depends on elements not known when
# variables are first evaluated.
def modifiable_flags_handler(env):
  env['modifiable_flags'] = 'on' if 'mode' in env and env['mode'] == 'debug' else 'off'
def symbols_handler(env):
  env['symbols'] = 'on' if 'mode' in env and env['mode'] == 'debug' else 'off'

vars_default_handlers = {
    # variable_name    : [ 'default val', 'handler'                ]
    'symbols'          : [ 'mode==debug', symbols_handler          ],
    'modifiable_flags' : [ 'mode==debug', modifiable_flags_handler ]
    }

def DefaultVariable(name, help, allowed):
  default_value = vars_default_handlers[name][0]
  allowed.append(default_value)
  return EnumVariable(name, help, default_value, allowed)

# TODO: Allow build options to be specified in a file.
vars = Variables()
# Define command line build options.
vars.AddVariables(
    EnumVariable('mode', 'Build mode',
                 'release', allowed_values=utils.build_options_modes),
    DefaultVariable('symbols', 'Include debugging symbols in the binaries',
                    ['on', 'off']),
    DefaultVariable('modifiable_flags', 'Allow modifying flags at runtime.',
                    ['on', 'off']),
    EnumVariable('os', 'Target os',
                 utils.GuessOS(), allowed_values=utils.build_options_oses)
    )

# Abort build if any command line option is invalid.
unknown_build_options = vars.UnknownVariables()
if unknown_build_options:
  print 'Unknown build options:',  unknown_build_options.keys()
  Exit(1)

# To avoid recompiling multiple times when build options are changed, different
# build paths are used depending on the options set.
# This lists the options that should be taken into account to create the build
# path.
options_influencing_build_path = [
#   ('option_name', include_option_name_in_path),
    ('mode', False),
    ('symbols', True),
    ('modifiable_flags', True)
    ]


# Build helpers ----------------------------------------------------------------

def RetrieveEnvironmentVariables(env):
  # Grab compilation environment variables.
  env['CC'] = os.getenv('CC') or env['CC']
  env['CXX'] = os.getenv('CXX') or env['CXX']
  env['CXXFLAGS'] = os.getenv('CXXFLAGS') or env['CXXFLAGS']
  env['CCFLAGS'] = os.getenv('CCFLAGS') or env['CCFLAGS']
  if os.getenv('LD_LIBRARY_PATH'):
    env['LIBPATH'] = os.getenv('LD_LIBRARY_PATH')
  elif 'LIBPATH' not in env:
    env['LIBPATH'] = ''
  # This allows colors to be displayed when using with clang.
  env['ENV']['TERM'] = os.environ['TERM']

def ProcessBuildOptions(env):
  # 'all' is unconditionally processed.
  if 'all' in options:
    for var in options['all']:
      if var in env and env[var]:
        env[var] += options['all'][var]
      else:
        env[var] = options['all'][var]
  # Other build options must match 'option:value'
  dict = env.Dictionary()
  keys = dict.keys()
  keys.sort()
  for key in keys:
    # First apply the default variables handlers.
    if key in vars_default_handlers and dict[key] == vars_default_handlers[key][0]:
      vars_default_handlers[key][1](dict)
    # Then update the environment according to the value of the variable.
    key_val_couple = key + ':%s' % dict[key]
    if key_val_couple in options:
      for var in options[key_val_couple]:
        env[var] += options[key_val_couple][var]

def TargetBuildDir(env):
  # Build-time option values are embedded in the build path to avoid requiring a
  # full build when an option changes.
  build_dir = utils.dir_build
  utils.ensure_dir(build_dir)
  for option in options_influencing_build_path:
    if option[1]:
      build_dir = join(build_dir, option[0] + '_'+ env[option[0]])
    else:
      build_dir = join(build_dir, env[option[0]])
  return build_dir

def PrepareVariantDir(location, build_dir):
  location_build_dir = join(build_dir, location)
  VariantDir(location_build_dir, location)
  return location_build_dir

def RegitLibraryTarget(env):
  build_dir = TargetBuildDir(env)
  # All source files are in src/.
  variant_dir_src = PrepareVariantDir('src', build_dir)
  sources = [Glob(join(variant_dir_src, '*.cc'))]
  return env.StaticLibrary(join(build_dir, 'regit'), sources)


# Build ------------------------------------------------------------------------

# The regit library, built by default.
env = Environment(variables = vars)
RetrieveEnvironmentVariables(env)
ProcessBuildOptions(env)
Help(vars.GenerateHelpText(env))
libregit = RegitLibraryTarget(env)
Default(libregit)
top_level_targets.Add('', 'Build the regit library.')


# Build the compilation information tool.
# It requires a library built with 'modifiable_flags=on'.
if env['modifiable_flags'] != 'on':
  env = Environment(variables = vars)
  env['modifiable_flags'] = 'on'
  RetrieveEnvironmentVariables(env)
  ProcessBuildOptions(env)
  libregit = RegitLibraryTarget(env)
# TODO: This program requires libargp. We should check for it using CheckLib().
compinfo_libs = [libregit]
if env['os'] == 'macos':
  compinfo_libs += ['libargp']
compinfo = env.Program('tools/compinfo', 'tools/compinfo.cc',
                       LIBS=compinfo_libs)

top_level_targets.Add('tools/compinfo', 'Build the compilation info utility.')


Help('\n\nAvailable top level targets:\n' + top_level_targets.Print())
