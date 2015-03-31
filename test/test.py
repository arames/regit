#!/usr/bin/python

import argparse
import itertools
import os
from os.path import join, dirname
import subprocess
import sys

# Import regit utils.
dir_tests = dirname(os.path.realpath(__file__))
dir_regit = dir_tests
while 'SConstruct' not in os.listdir(dir_regit):
  dir_regit = os.path.realpath(join(dir_regit, '..'))
sys.path.insert(0, join(dir_regit, 'tools'))
import utils



script_description = \
'''
Run regit tests.
The tests are run for a all possible combinations of options that are required
to be tested.
'''



def optionify(name):
  return '--' + name



# The options that can be tested are abstracted to provide an easy way to add
# new ones.
# Build options are options passed to scons, with a syntax like `scons opt=val`
# Runtime options are options passed to the test program and control runtime
# flags in regit.
# See the definition of `test_options` below.

# 'all' is a special value for the options. If specified, all other values of
# the option are tested.
class TestOption(object):
  def __init__(self, option_type, name, help,
               val_test_choices, val_test_default = None):
    self.name = name
    self.option_type = option_type
    self.help = help
    self.val_test_choices = val_test_choices
    if val_test_default is not None:
      self.val_test_default = val_test_default
    else:
      self.val_test_default = val_test_choices[0]

  type_build = 'build'
  type_run = 'run'

  def is_build_option(self):
    return self.option_type == TestOption.type_build
  def is_run_option(self):
    return self.option_type == TestOption.type_run

class BuildOption(TestOption):
  option_type = TestOption.type_build
  def __init__(self, name, help,
               val_test_choices, val_test_default = None):
    super(BuildOption, self).__init__(BuildOption.option_type,
                                      name,
                                      help,
                                      val_test_choices,
                                      val_test_default)
  def args_list(self, to_test):
    res = []
    if to_test == 'all':
      for val in self.val_test_choices:
        if val != 'all':
          res.append(self.name + '=' + val)
    else:
      res.append(self.name + '=' + to_test)
    return res

class RunOption(TestOption):
  option_type = TestOption.type_run
  def __init__(self, name, help,
               val_test_choices, val_test_default = None):
    super(RunOption, self).__init__(RunOption.option_type,
                                    name,
                                    help,
                                    val_test_choices,
                                    val_test_default)
  def args_list(self, to_test):
    res = []
    if to_test == 'all':
      for val in self.val_test_choices:
        if val != 'all':
          res.append(optionify(self.name) + '=' + val)
    else:
      res.append(optionify(self.name) + '=' + to_test)
    return res



test_build_options = [
  BuildOption('mode', 'Test with the specified build modes.',
              val_test_choices=['all'] + utils.build_options_modes)
]

test_runtime_options = [
  RunOption('parser_opt',
            '''Test with the specified configurations for parser level
            optimizations.''',
            val_test_choices=['all', '1', '0'])
]

test_options = \
  test_build_options + test_runtime_options


parser = argparse.ArgumentParser(
  description=script_description,
  formatter_class=argparse.ArgumentDefaultsHelpFormatter)
# Automatically build the script options from the options to be tested.
test_arguments = parser.add_argument_group(
  'Test options',
  'These options indicate what should be tested')
for option in test_options:
  test_arguments.add_argument(optionify(option.name),
                              choices=option.val_test_choices,
                              default=option.val_test_default,
                              help=option.help,
                              action='store')
general_arguments = parser.add_argument_group('General options')
general_arguments.add_argument(
  '-j', '--jobs', type=int, action='store',
  default=1,
  help='''Number of jobs to run simultaneously for the *build*
  commands''')
args = parser.parse_args()



# List all combinations of options that will be tested.
def list_options_combinations(options):
  list_options = map(lambda opt : opt.args_list(args.__dict__[opt.name]), options)
  return list(itertools.product(*list_options))
test_build_args_combinations = list_options_combinations(test_build_options)
test_runtime_args_combinations = list_options_combinations(test_runtime_options)



for build_args in test_build_args_combinations:
  # Build the test executable for the current build options.
  scons_command = ["scons", "-C", dir_regit, 'test',
                   '-j', str(args.jobs), "benchtest=on", "modifiable_flags=on"]
  scons_command += build_args
  print ' '.join(scons_command)
  p_scons = subprocess.Popen(scons_command,
                             stdout=subprocess.PIPE,
                             stderr=subprocess.STDOUT)
  scons_ret = p_scons.wait()
  if scons_ret != 0:
    print 'Command:'
    print ' '.join(scons_command)
    print 'Output:'
    scons_output = p_scons.communicate()[0]
    print scons_output
    sys.exit(scons_ret)

  for run_args in test_runtime_args_combinations:
    # Use the realpath of the test executable so that the commands printed can
    # be copy-pasted and used.
    test_executable = \
      os.path.realpath(join(utils.dir_build_latest, 'test', 'test'))
    test_command = [test_executable]
    test_command += run_args
    print ' '.join(test_command),
    p_test = subprocess.Popen(test_command, stdout=subprocess.PIPE)
    test_output = ''
    test_ret = p_test.poll()
    while test_ret is None:
      test_output += p_test.communicate()[0]
      test_ret = p_test.poll()
    if test_ret != 0:
      print 'FAILED'
      print 'Output:'
      print test_output
    else:
      print '\t\tsuccess'
