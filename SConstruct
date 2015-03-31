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
    max_target_str_len = len('scons ') + max(map(len, self.targets))
    max_messages_str_len = len(' : ') + max(map(len, self.help_messages))
    help_format = \
      "\t{:<%d}{:<%d}\n" % (max_target_str_len, max_messages_str_len)
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
      'CXXFLAGS' : ['-std=c++11'],
      'CCFLAGS' : ['-Werror',
                   '-Wall',
                   '-Wextra',
                   '-Wredundant-decls',
                   '-Wwrite-strings',
                   '-fdiagnostics-show-option',
                   '-pedantic',
                   '-pthread'],
      'CPPPATH' : [utils.dir_src, utils.dir_include]
      },
#   'build_option:value' : {
#     'environment_key' : 'values to append'
#     },
    'mc_max_one_char:on' : {
      'CCFLAGS' : ['-DMC_MAX_ONE_CHAR']
      },
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
    'os:linux' : {
      'CCFLAGS' : ['-pthread'],
      },
    'symbols:on' : {
      'CCFLAGS' : ['-g'],
      'LINKFLAGS' : ['-g']
      },
    }


# DefaultVariable have a default value that depends on elements not known when
# variables are first evaluated.
def modifiable_flags_handler(env):
  env['modifiable_flags'] = \
      'on' if 'mode' in env and env['mode'] == 'debug' else 'off'
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

# TODO: Allow default build options to be overridden in a configuration file.
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
                 utils.GuessOS(), allowed_values=utils.build_options_oses),
    EnumVariable('mc_max_one_char',
                 'Limit the MultipleChar to contain one character.',
                 'off', ['on', 'off'])
    )

# Abort the build if any command line option is unknown or invalid.
unknown_build_options = vars.UnknownVariables()
if unknown_build_options:
  print 'Unknown build options:',  unknown_build_options.keys()
  Exit(1)

# We use 'variant directories' to avoid recompiling multiple times when build
# options are changed, different build paths are used depending on the options
# set. These are the options that should be reflected in the build directory
# path.
options_influencing_build_path = ['mode', 'symbols', 'modifiable_flags']



# Build helpers ----------------------------------------------------------------

def RetrieveEnvironmentVariables(env):
  for key in ['CC', 'CXX', 'CCFLAGS', 'CXXFLAGS', 'AR', 'RANLIB', 'LD']:
    if os.getenv(key): env[key] = os.getenv(key)
  if os.getenv('LD_LIBRARY_PATH'): env['LIBPATH'] = os.getenv('LD_LIBRARY_PATH')
  if os.getenv('CPPFLAGS'):
    env.Append(CPPFLAGS = os.getenv('CPPFLAGS').split())
  if os.getenv('LINKFLAGS'):
    env.Append(LINKFLAGS = os.getenv('LINKFLAGS').split())
  # This allows colors to be displayed when using with clang.
  env['ENV']['TERM'] = os.getenv('TERM')

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
    if key in vars_default_handlers and \
            dict[key] == vars_default_handlers[key][0]:
      vars_default_handlers[key][1](dict)
    # Then update the environment according to the value of the variable.
    key_val_couple = key + ':%s' % dict[key]
    if key_val_couple in options:
      for var in options[key_val_couple]:
        env[var] += options[key_val_couple][var]

def ConfigureEnvironment(env):
  RetrieveEnvironmentVariables(env)
  ProcessBuildOptions(env)

def TargetBuildDir(env):
  # Build-time option values are embedded in the build path to avoid requiring a
  # full build when an option changes.
  build_dir = utils.dir_build
  for option in options_influencing_build_path:
    build_dir = join(build_dir, option + '_'+ env[option])
  return build_dir

def PrepareVariantDir(location, build_dir):
  location_build_dir = join(build_dir, location)
  VariantDir(location_build_dir, location)
  return location_build_dir

def RegitLibraryTarget(env):
  build_dir = TargetBuildDir(env)
  # Create a link to the latest build directory.
  subprocess.check_call(["rm", "-f", utils.dir_build_latest])
  utils.ensure_dir(build_dir)
  subprocess.check_call(["ln", "-s", build_dir, utils.dir_build_latest])
  # All source files are in `src/`.
  variant_dir_src = PrepareVariantDir('src', build_dir)
  sources = [Glob(join(variant_dir_src, '*.cc'))]
  return env.StaticLibrary(join(build_dir, 'regit'), sources)


# Build ------------------------------------------------------------------------

# The regit library, built by default.
env = Environment(variables = vars)
ConfigureEnvironment(env)
Help(vars.GenerateHelpText(env))
libregit = RegitLibraryTarget(env)
Default(libregit)
top_level_targets.Add('', 'Build the regit library.')


# Build the samples.
sample_build_dir = PrepareVariantDir('sample', TargetBuildDir(env))
basic = env.Program('sample/basic',
                    join(sample_build_dir, 'basic.cc'),
                    LIBS=[libregit])
top_level_targets.Add('sample/basic', 'Build the basic sample.')


# Some tools require the flags to be modifiable. So we create an environment
# with a forced setting.
env_mod_flags = Environment(variables = vars)
libregit_mod_flags = libregit
if env['modifiable_flags'] == 'on':
  env_mod_flags = env
else:
  env_mod_flags['modifiable_flags'] = 'on'
  ConfigureEnvironment(env_mod_flags)
  libregit_mod_flags = RegitLibraryTarget(env_mod_flags)


# The tools/compinfo tool.
compinfo_libs = [libregit, libregit_mod_flags]
if env['os'] == 'macos':
  compinfo_libs += ['libargp']
tools_build_dir = PrepareVariantDir('tools', TargetBuildDir(env))
compinfo = env_mod_flags.Program('tools/compinfo',
                                 join(tools_build_dir, 'compinfo.cc'),
                                 LIBS=compinfo_libs)
top_level_targets.Add('tools/compinfo', 'Build the compilation info utility.')

# The tools/run_matching tool.
run_matching_libs = [libregit, libregit_mod_flags]
if env['os'] == 'macos':
  run_matching_libs += ['libargp']
tools_build_dir = PrepareVariantDir('tools', TargetBuildDir(env))
run_matching = env_mod_flags.Program('tools/run_matching',
                                     join(tools_build_dir, 'run_matching.cc'),
                                     LIBS=run_matching_libs)
top_level_targets.Add('tools/run_matching', 'Build the regexp matching test utility.')

# The tests.
test_libs = [libregit_mod_flags]
if env['os'] == 'macos':
  test_libs += ['libargp']
test_build_dir = PrepareVariantDir('test', TargetBuildDir(env))
test = env_mod_flags.Program(join(test_build_dir, 'test'),
                             join(test_build_dir, 'test.cc'),
                             LIBS=test_libs)
env_mod_flags.Alias('test', test)
# Don't document the test target. It should normally be built by the
# `test/test.py` utility.




Help('\n\nAvailable top level targets:\n' + top_level_targets.Print())
