#!/usr/bin/python

import argparse
import os
import subprocess
import sys

# Import utils.
dir_tools = os.path.dirname(os.path.realpath(__file__))
dir_regit = dir_tools
while 'SConstruct' not in os.listdir(dir_regit):
  dir_regit = os.path.realpath(os.path.join(dir_regit, '..'))
sys.path.insert(0, os.path.join(dir_regit, 'tools'))
import utils



script_description = \
'''
Transform a trace of a regular expression matching run into a series of image
files.
The trace can be obtained from the Regit library via the "trace_matching" flag,
that output dot graphs.
See http://en.wikipedia.org/wiki/DOT_(graph_description_language).
'''

def CreateParser():
    parser = argparse.ArgumentParser(
        description=script_description,
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument(
        'dot_trace',
        help='The trace file.')
    parser.add_argument(
        '--format', action='store', default='png',
        help='''The output image format to use. See the list of format available
        for `dot` on your system.''')
    parser.add_argument(
        '-d', '--target_dir', action='store', default='trace',
        help="Directory where the image files will be produced.")
    return parser



def image_filename(dir, index_pos, index_tick, format):
    return os.path.join(
        dir, str(index_pos) + '_' + str(index_tick) + '.' + format)


if __name__ == "__main__":
    # Handle the command-line arguments.
    parser = CreateParser()
    args = parser.parse_args()
    target_dir = args.target_dir
    split_dir = os.path.join(args.target_dir, 'split')
    utils.ensure_dir(target_dir)
    utils.ensure_dir(split_dir)

    # Open the specified data file.
    trace_file = open(args.dot_trace, 'r')
    trace_data = trace_file.read()

    # The trace is split by position and tick. For each position, there is one
    # graph per valid tick.
    # TODO: The patterns on which the trace is split should be abstracted.
    positions = trace_data.split('// End of index')
    while positions and positions[-1].rstrip() == '':
        positions.pop()
    index_pos = 0
    for index in positions:
        # Split each graph from the output. `split` deletes the pattern used, so
        # we need to append it again.
        graphs = index.split('}')
        while graphs and graphs[-1].rstrip() == '':
            graphs.pop()
        graphs = map(lambda s : s + '}', graphs)
        # Create an image file for each graph.
        index_tick = 0
        image_files = []
        for graph in graphs:
            f_index_pos = image_filename(split_dir,
                                         index_pos,
                                         index_tick,
                                         args.format)
            image_files += [f_index_pos]
            # TODO: We should check that `dot` is available.
            p = subprocess.Popen(['dot', '-T' + args.format, '-o', f_index_pos],
                                 stdin=subprocess.PIPE,
                                 stdout=subprocess.PIPE,
                                 stderr=subprocess.STDOUT)
            p_out = p.communicate(input=graph)[0]
            index_tick += 1
        # Now stitch image files for the index into one.
        f_index = image_filename(target_dir, index_pos, 0, args.format)
        # TODO: We should check that `convert` from `imagemagick` is available.
        subprocess.check_call(['convert'] + image_files + ['-append', f_index])
        index_pos += 1

