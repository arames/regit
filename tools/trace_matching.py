#!/usr/bin/python

import argparse
import os
import subprocess
import sys

import trace_to_graphs

script_description = 'Trace the matching of `regexp` in `text`.'



def CreateParser():
  parser = argparse.ArgumentParser(
    description=script_description,
    formatter_class=argparse.ArgumentDefaultsHelpFormatter)
  parser.add_argument('regexp')
  parser.add_argument('text')
  parser.add_argument(
    '-m', '--match_type', action='store', default='full',
    help="Directory where the image files will be produced.")
  parser.add_argument(
    '-d', '--target_dir', action='store', default='trace',
    help="Directory where the image files will be produced.")
  return parser



if __name__ == "__main__":
  parser = CreateParser()
  args = parser.parse_args()
  target_dir = args.target_dir

  # Compile the `compinfo` executable if it is not available.
  compinfo = os.path.join(trace_to_graphs.dir_tools, 'compinfo')
  if not os.access(compinfo, os.X_OK):
    subprocess.check_call(['scons', '-C', trace_to_graphs.dir_regit, 'tools/compinfo'])
    if not os.access(compinfo, os.X_OK):
      print('Error. Failed to use `tools/compinfo`.')
      sys.exit(1)

  # Match the regexp in the text to produce the trace.
  compinfo_command = [compinfo,
                      '--match_type=%s' % args.match_type,
                      '--trace_matching',
                      args.regexp, args.text]
  trace = subprocess.check_output(compinfo_command)

  trace_to_graphs.TraceToGraphs(trace, args.target_dir,
                                trace_to_graphs.default_format)
