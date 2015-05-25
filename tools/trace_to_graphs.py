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



default_format = 'png'


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
        '--format', action='store', default=default_format,
        help='''The output image format to use. See the list of format available
        for `dot` on your system.''')
    parser.add_argument(
        '-d', '--target_dir', action='store', default='trace',
        help="Directory where the image files will be produced.")
    return parser



def image_filename(dir, index_pos, index_tick, format):
    return os.path.join(
        dir, str(index_pos) + '_' + str(index_tick) + '.' + format)



def TraceToGraphs(trace, target_dir, format):
    split_dir = os.path.join(target_dir, 'split')
    split_graph_dir = os.path.join(split_dir, 'graphs')
    utils.ensure_dir(target_dir)
    utils.ensure_dir(split_dir)
    utils.ensure_dir(split_graph_dir)

    # The trace is split by position and tick. For each position, there is one
    # graph per valid tick.
    # TODO: The patterns on which the trace is split should be abstracted.
    positions = trace.split('// End of offset')
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
            f_index_pos_graph = image_filename(split_graph_dir,
                                               index_pos,
                                               index_tick,
                                               'dot')
            f_graph = open(f_index_pos_graph, 'w')
            f_graph.write(graph)
            f_graph.close()
            f_index_pos = image_filename(split_dir,
                                         index_pos,
                                         index_tick,
                                         format)
            image_files += [f_index_pos]
            dot_command = ['dot', '-T' + format, '-o', f_index_pos, f_index_pos_graph]
            # TODO: We should check that `dot` is available.
            subprocess.check_call(dot_command)
            index_tick += 1
        # Now stitch image files for the index into one.
        f_index = image_filename(target_dir, index_pos, 0, format)
        # TODO: We should check that `convert` from `imagemagick` is available.
        subprocess.check_call(['convert'] + image_files + ['-append', f_index])
        index_pos += 1



if __name__ == "__main__":
    parser = CreateParser()
    args = parser.parse_args()

    trace_file = open(args.dot_trace, 'r')
    trace = trace_file.read()

    TraceToGraphs(trace, args.target_dir, args.format)
