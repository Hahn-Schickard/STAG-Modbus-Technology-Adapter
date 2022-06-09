#!/usr/bin/env python
"""This utility looks for a CMake project name within a specified directory

"""

import re
import os


def getProjectName(dir_loc):
    cmake_file = os.path.join(dir_loc, 'CMakeLists.txt')
    if os.path.isfile(cmake_file):
        with open(cmake_file, 'r', encoding='utf-8') as file:
            content = file.read()
            return re.search(r'set\(THIS (.*)\)', content).group(1)
    else:
        raise FileNotFoundError(
            'No CMakeLists.txt file was found in directory {}'.format(dir_loc))


if __name__ == '__main__':
    current_file_loc = os.path.dirname(os.path.realpath(__file__))
    root_dir = os.path.join(current_file_loc, '..')
    print(getProjectName(root_dir))
