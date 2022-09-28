#!/usr/bin/env python
'''A simplified interface for clang-format utility

'''

import argparse
import sys
import os
import difflib
import re
from typing import List
import json
import subprocess

if sys.version_info < (3, 6):
    raise RuntimeError("This package requires Python 3.6 or later")

VERBOSE = False
LIST_FILES = False


class PIPE_Value:
    def __init__(self, stdout: [str], stderr: [str]):
        self.stdout = stdout
        self.stderr = stderr


def run_process(executable: str, arguments: [str] = [], encoding='utf-8', throw_on_failure=True, live_print=True):
    command = [executable]
    if arguments:
        command.extend(arguments)
    try:
        process = subprocess.Popen(command, stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE, encoding=encoding, universal_newlines=True)
        stdout = []
        if live_print:
            while True:
                line = process.stdout.readline()
                if line == '' and process.poll() is not None:
                    break
                if line:
                    stdout.append(line)
                    print(line.strip())
        else:
            process.wait()
            stdout = process.stdout.readlines()
        stderr = process.stderr.readlines()
        if throw_on_failure:
            if stderr:
                error_msg = 'Running command ' + \
                    ' '.join(command) + ' returned an error: ' + \
                    '\n'.join(stderr)
                raise OSError(process.returncode, ''.join(error_msg))
            else:
                return PIPE_Value(stdout, [])
        else:
            return PIPE_Value(stdout, stderr)
    except subprocess.CalledProcessError as exception:
        raise RuntimeError('An exception occurred while running command: ' +
                           ' '.join(command) + ' Exception is: ' + exception.output)


def print_verbose(message: str):
    if VERBOSE:
        print(message)


def get_files_from_dir(dir_path: str, recursive: bool = False):
    files = []
    if os.path.isdir(dir_path):
        print_verbose('Searching directory {}'.format(dir_path))
        dir_items = [os.path.join(dir_path, item_path)
                     for item_path in os.listdir(dir_path)]
        for item in dir_items:
            if recursive and os.path.isdir(item):
                files.extend(get_files_from_dir(item))
            if os.path.isfile(item):
                files.append(item)
        print_verbose('Found files {}'.format(files))
        return files
    else:
        print('Could not find directory {} in {}'.format(dir, os.getcwd()))
        sys.exit(False)


def get_files(directories: [str], recursive: bool = False):
    files = []

    for directory in directories:
        files.extend(get_files_from_dir(
            os.path.join(os.getcwd(), directory), recursive))

    return files


def get_ignored(ignored: str):
    if ignored:
        ignore_file = os.path.join(os.getcwd(), ignored)
        if os.path.isfile(ignore_file):
            with open(ignore_file) as file_stream:
                ignored_files = []
                for line in file_stream:
                    line = line.rstrip()
                    if line and not line.startswith('#'):
                        ignored_files.append(line)
                return ignored_files
        elif VERBOSE:
            print('{} not found'.format(ignore_file))
    elif VERBOSE:
        print('No ignore file specified')


def filter_ignored(files: [str], file_types: str, ignored: str, pattern: str):
    ignored_files = get_ignored(ignored)
    file_types = file_types.split(',')
    for file in list(files):
        if list(filter(file.endswith, file_types)) == []:
            print_verbose(
                'Ignoring file {} since it`s type is not supported'.format(file))
            files.remove(file)
        if ignored_files and file in ignored_files:
            print_verbose(
                'Ignoring file {}, due to it being listed in {}'.format(file, ignored))
        if pattern and re.match(pattern, file):
            print_verbose(
                'Ignoring file {}, due to it matching RegEx {}'.format(file, pattern))
            files.remove(file)
    return files


def format_file(executable: str, file: str, formatter_args: [str] = []):
    args = []
    for argument in formatter_args:
        args.append(argument)
    args.append(file)
    print_verbose('Running {} {}'.format(executable, ' '.join(args)))
    process = run_process(
        executable, args, throw_on_failure=False, live_print=VERBOSE)
    if process.stderr:
        raise RuntimeError('Running {} {} returned an unhandled error: {}'.format(
            executable, ' '.join(args), '\n'.join(process.stderr)))
    else:
        return process.stdout


def get_diff(original_file: str, formatted: [str]):
    if formatted:
        with open(original_file) as file_stream:
            original = file_stream.readlines()
        differences = difflib.unified_diff(
            formatted, original, fromfile='formatted', tofile='original')
        return ''.join(list(differences))
    else:
        return None


def print_diff(differences: [str], original_file: str):
    print('{} needs to be formatted'.format(original_file))
    print(differences)


def save_formatted(formatted: [str], formatted_filename: str, original_filename: str):
    if formatted:
        fixes_dir = 'clang-format-fixes'
        if not os.path.isdir(fixes_dir):
            os.mkdir(fixes_dir)
        fixes_path = os.path.join(fixes_dir, formatted_filename)
        with open(fixes_path, 'w') as fixes:
            print_verbose(
                'Saving formatter suggestions to {} file'.format(fixes_path))
            fixes.writelines(formatted)

        formatted_files_manifest_path = os.path.join(
            fixes_dir, 'formatted_files.json')
        formatted_files = {}
        if os.path.isfile(formatted_files_manifest_path):
            with open(formatted_files_manifest_path, 'r') as formatted_files_json:
                formatted_files = json.load(formatted_files_json)

        relative_original_path = os.path.relpath(original_filename)
        if not relative_original_path in formatted_files:
            print_verbose('Adding {} file to fixes manifest {}'.format(
                fixes_path, formatted_files_manifest_path))
            formatted_files[relative_original_path] = fixes_path
        with open(formatted_files_manifest_path, 'w') as formatted_files_json:
            json.dump(formatted_files, formatted_files_json)
    else:
        raise RuntimeError('Given file {} has no formatting suggestions to save for file {}'.format(
            formatted_filename, original_filename))


def do_formatting(clang_format_exe: str, save_as: str, directories: [str], recursive: bool, file_types: str, ignored: str, ignore_pattern: str):
    files = get_files(directories, recursive)
    files = filter_ignored(files, file_types, ignored, ignore_pattern)

    if not files:
        raise RuntimeError('No files found')

    if LIST_FILES:
        print('Parsed Files:')
        print('\n'.join(files))

    formatter_args = []
    if save_as == 'in_place':
        print_verbose(
            'Applying formatting changes in place, instead of generating separate fix files')
        formatter_args.append('-i')
    if os.path.isfile(os.path.join(os.getcwd(), '.clang-format')):
        print_verbose(
            'Using .clang-format configuration file in current working directory {}'.format(os.getcwd()))
        formatter_args.append('-style=file')

    fixed_files = []
    for original in files:
        formatted_file = format_file(
            clang_format_exe, original, formatter_args=formatter_args)
        diff = get_diff(original, formatted_file)
        if diff:
            print_diff(diff, original)
            if not save_as == 'in_place':
                output_filename = str()
                if save_as == 'diff_file':
                    output_filename = os.path.splitext(
                        os.path.basename(original))[0] + '.diff'
                    save_formatted(diff, output_filename, original)
                elif save_as == 'formatted_file':
                    filename_tuple = os.path.splitext(
                        os.path.basename(original))
                    output_filename = filename_tuple[0] + \
                        '_formatted' + filename_tuple[1]
                    save_formatted(formatted_file, output_filename, original)
                else:
                    raise NotImplementedError(
                        'Argument save_as value {} is not supported'.format(save_as))
                fixed_files.append(output_filename)
            else:
                fixed_files.append(original)
        else:
            print_verbose('File {} does not need formatting'.format(original))

    if len(fixed_files) == 0:
        print('No files were formatted')
    else:
        return_message = str(len(fixed_files))
        if not save_as == 'in_place':
            return_message += (' file needs ' if len(fixed_files)
                               == 1 else ' files need ')
            return_message += 'to be formatted. Formatting suggestions can be found at ' + \
                os.path.join(os.getcwd(), 'clang-format-fixes') + os.sep
            return return_message
        else:
            return_message += (' file ' if len(fixed_files)
                               == 1 else ' files  ')
            return_message += 'have been formatted'
            print(return_message)


def fix_files(fixes_dir: str):
    fixes_manifest_json = os.path.join(fixes_dir, 'formatted_files.json')
    if os.path.isfile(fixes_manifest_json):
        with open(fixes_manifest_json, 'r') as fixes_json:
            fixes_manifest = json.load(fixes_json)
        for fixed_file in fixes_manifest:
            if os.path.isfile(fixed_file):
                fixes = fixes_manifest[fixed_file]
                if os.path.splitext(os.path.basename(fixes))[1] != '.diff':
                    with open(fixes) as formatted:
                        changes = formatted.readlines()
                    with open(fixed_file, 'w') as original:
                        print_verbose(
                            'Applying changes to file {} from file {}'.format(fixed_file, fixes))
                        original.writelines(changes)
                else:
                    raise NotImplementedError(
                        'Could not apply fixes from file {}. Applying fixes from a diff file is not yet supported!'.format(fixes))
            else:
                raise FileNotFoundError(
                    'Could not find file {} in {}'.format(fixed_file, os.getcwd()))
    else:
        raise RuntimeError('Directory {} does not have fixes manifest file {}'.format(
            fixes_dir, fixes_manifest_json))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        '--clang-format-exe',
        metavar='executable',
        help='clang-format executable name or location',
        default='clang-format')
    DEFAULT_EXTENSIONS = 'c,h,C,H,cpp,hpp,cc,hh,c++,h++,cxx,hxx'
    parser.add_argument(
        '--file-types', metavar='file-types-list',
        help='comma separated list of formattable file types (default {})'.format(
            DEFAULT_EXTENSIONS),
        default=DEFAULT_EXTENSIONS)
    parser.add_argument('--dirs', metavar='directory-location',
                        help='list of directories, separated by space, that include formattable files', nargs='+', default=[])
    parser.add_argument(
        '-r',
        '--recursive',
        action='store_true',
        help='run recursively over given directories in --dir')
    parser.add_argument(
        '-v',
        '--verbose',
        action='store_true',
        help='verbose print all of the actions')
    parser.add_argument(
        '-l',
        '--list-files',
        action='store_true',
        help='lists all parsed files')
    parser.add_argument('--fix', metavar='clang-format-fixes',
                        help='uses specified fixes to apply externally generated formatter changes')
    parser.add_argument(
        '--save-as',
        default='formatted_file',
        const='formatted_file',
        nargs='?',
        choices=['in_place', 'formatted_file', 'diff_file'],
        help='Specifies how to save formatter changes.' +
        'Choosing in_place applies formatter fixes in formatted file.' +
        'Choosing formatted_file will save the entire formatted file as a new file appended with _formatted to it`s name.' +
        'Choosing diff_file will save differences between the original file and formatted file in a separate .diff file')
    parser.add_argument(
        '--ignore',
        metavar='.clang-format-ignore',
        default='',
        help='specifies a file containing a list of files to ignore during clang-format')
    parser.add_argument(
        '--ignore-pattern',
        metavar='RegEx',
        default='',
        help='exclude files with names that match a given RegEx pattern')
    args = parser.parse_args()
    global VERBOSE
    VERBOSE = args.verbose
    global LIST_FILES
    LIST_FILES = args.list_files

    print_verbose(args)
    if not args.fix:
        return do_formatting(
            args.clang_format_exe, args.save_as, args.dirs, args.recursive, args.file_types, args.ignore, args.ignore_pattern)
    else:
        fix_files(args.fix)


if __name__ == '__main__':
    sys.exit(main())
