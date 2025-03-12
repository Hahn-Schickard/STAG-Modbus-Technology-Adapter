#!/usr/bin/env python
'''A simplified interface for clang-tidy utility

'''

import subprocess
import argparse
import sys
import os
import json
import queue as queue
from multiprocessing import Pool, Lock, cpu_count

if sys.version_info < (3, 6):
    raise RuntimeError("This package requires Python 3.6 or later")

VERBOSE = False


class PIPE_Value:
    def __init__(self, stdout: [str], stderr: [str]):
        self.stdout = stdout
        self.stderr = stderr


def run_process(executable: str, arguments=[str], encoding='utf-8'):
    command = [executable]
    if arguments:
        command.extend(arguments)
    try:
        process = subprocess.Popen(command, stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE, encoding=encoding, universal_newlines=True)
        stdout = process.stdout.readlines()
        stderr = process.stderr.readlines()
        return PIPE_Value(stdout, stderr)
    except subprocess.CalledProcessError as exception:
        raise RuntimeError('An exception occurred while running command: ' +
                           ' '.join(command) + ' Exception is: ' + exception.output)


def print_verbose(message: str):
    if VERBOSE:
        print(message)


def threadsafe_list_print(lines: [str]):
    CONSOLE_LOCK.acquire()
    for line in lines:
        print_verbose(line)
    CONSOLE_LOCK.release()


def count_warnings(stream: [str], warning_markers: [str]):
    result = 0
    for line in stream:
        for marker in warning_markers:
            result += line.count(marker)
    return result


def lint_file(executable: str,
              build_loc: str,
              file: str,
              export_fixes: bool,
              fixes_dir: str):
    args = []
    if build_loc:
        args.append('-p='+build_loc)
    fixes_saved = ''
    if export_fixes:
        filename = os.path.basename(file)
        filename = filename.replace('.', '_')
        fix_loc = os.path.join(fixes_dir, filename + '.yml')
        fixes_saved = '. File fixes will be saved in {}'.format(fix_loc)
        args.append('-export-fixes={}'.format(fix_loc))
    if file:
        args.append(file)
    process = run_process(executable, args)
    if VERBOSE:
        console_log = ['Calling {} {}'.format(executable, ' '.join(args))]
        console_log.extend(process.stdout)
        threadsafe_list_print(console_log)

    warning_count = count_warnings(process.stdout, ['warning:'])
    if warning_count > 0:
        return 'Linter found {} warnings for file {}{}'.format(warning_count, file, fixes_saved)


def make_absolute(f, directory):
    if os.path.isabs(f):
        return f
    return os.path.normpath(os.path.join(directory, f))


def not_none(value_type):
    def not_none_checker(arg):
        '''Argparse none type checker function'''
        try:
            value = value_type(arg)
        except ValueError:
            raise argparse.ArgumentTypeError(f'must be a valid {value_type}')
        if value is None:
            raise argparse.ArgumentTypeError('can not be null')
        return value

    return not_none_checker


def in_range_of(value_type, min_value=None, max_value=None):
    '''
    Return ArgumentParser function handle to check if ArgumentParser arg is in a given range:
        min_value <= arg <= max_value

    If min_value is left to None, the check will only check the upper bound (arg <= max_value)
    If max_value is left to None, the check will only check the lower bound (min_value <= arg)
    '''
    def in_range_of_checker(arg):
        '''Argparse in-range type checker function'''
        try:
            value = value_type(arg)
        except ValueError:
            raise argparse.ArgumentTypeError(f'must be a valid {value_type}')

        if min_value is not None and value < min_value:
            raise argparse.ArgumentTypeError(f'must be more thant {min_value}')
        elif max_value is not None and value > max_value:
            raise argparse.ArgumentTypeError(f'must be less thant {max_value}')

        return value

    return in_range_of_checker


def initialize_pool(console_lock, executable: str, build_dir: str, export_fixes: bool, fixes_dir: str):
    global CONSOLE_LOCK
    CONSOLE_LOCK = console_lock

    global WORKER_EXE
    WORKER_EXE = executable

    global WORKER_BUILD_DIR
    WORKER_BUILD_DIR = build_dir

    global WORKER_EXPORT_FIXES
    WORKER_EXPORT_FIXES = export_fixes

    global WORKER_FIXES_DIR
    WORKER_FIXES_DIR = fixes_dir


def lint_worker(file: str):
    return lint_file(executable=WORKER_EXE,
                     build_loc=WORKER_BUILD_DIR,
                     file=file,
                     export_fixes=WORKER_EXPORT_FIXES,
                     fixes_dir=WORKER_FIXES_DIR)


def read_ignored(ignored: str):
    ignored_files = []
    if ignored:
        ignore_file = os.path.join(os.getcwd(), ignored)
        if os.path.isfile(ignore_file):
            with open(ignore_file) as file_stream:
                for line in file_stream:
                    line = line.rstrip()
                    if line and not line.startswith('#'):
                        ignored_files.append(line)
        elif VERBOSE:
            print('{} not found'.format(ignore_file))
    elif VERBOSE:
        print('No ignore file specified')
    return ignored_files


def ignored(filepath: str, blacklist: [str]):
    striped_path = os.path.relpath(filepath, os.getcwd())
    for ignored in blacklist:
        if ignored in striped_path:
            return True
    return False


def read_files(filepath: str, ignores: str):
    database = json.load(open(filepath))
    blacklist = read_ignored(ignores)
    print_verbose(
        str('Ignored file patterns: \n ' + '\n '.join(blacklist)))
    files = [make_absolute(entry['file'], entry['directory'])
             for entry in database
             if not ignored(make_absolute(entry['file'], entry['directory']), blacklist)]
    print_verbose(str('Formatted files: \n ' + '\n '.join(files)))
    return files


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        '--clang-tidy-exe',
        help='clang-tidy executable name or location',
        default='clang-tidy')
    parser.add_argument('-build', dest='build', type=not_none(str),
                        help='Path used to read a compile command database.')
    parser.add_argument('--threads', dest='threads', default=1, type=in_range_of(int, 1),
                        help='Number of threads, used when processing. '
                        'More threads will lead to faster execution. '
                        'If more threads are allocated, than available, '
                        'the maximum supported thread count will be used.')
    parser.add_argument(
        '-at',
        '--all-threads',
        action='store_true',
        help='Uses all of the available threads to execute clang-tidy')
    parser.add_argument(
        '-v',
        '--verbose',
        action='store_true',
        help='verbose print all of the actions')
    parser.add_argument(
        '-x',
        '--export-fixes',
        action='store_true',
        help='export fixes to a file')
    parser.add_argument(
        '--export-loc',
        default='clang-tidy-fixes',
        dest='fixes_dir',
        help='The location exported fixes will be saved at')
    parser.add_argument(
        '--ignore-file',
        metavar='.clang-tidy-ignore',
        default='.clang-ignore',
        help='lists ignore files and patterns that clang-tidy will not process'
    )
    args = parser.parse_args()
    global VERBOSE
    VERBOSE = args.verbose

    thread_count = cpu_count()
    if args.all_threads is False:
        if args.threads > cpu_count():
            print('Current system only supports {} threads, but {} where specified on call. '
                  'Defaulting to {} threads'.format(cpu_count(), thread_count, cpu_count()))
        else:
            thread_count = args.threads
            print('Using {} threads to run clang-tidy'.format(thread_count))
    else:
        print('Using all available {} threads to run clang-tidy'.format(thread_count))

    if args.export_fixes:
        if not os.path.isdir(args.fixes_dir):
            os.mkdir(args.fixes_dir)

    print_verbose(args)
    compile_database = os.path.join(args.build, 'compile_commands.json')
    try:
        files = read_files(compile_database, args.ignore_file)
    except:
        print('{} does not exist. Linting aborted'.format(compile_database))
        return 1
    total_warnings = []
    console_lock = Lock()
    with Pool(
        processes=thread_count,
        initializer=initialize_pool,
        initargs=(
            console_lock,
            args.clang_tidy_exe,
            args.build,
            args.export_fixes,
            args.fixes_dir)) as pool:
        file_warnings = pool.map(lint_worker, files, chunksize=1)
        total_warnings = [
            warning for warning in file_warnings if warning is not None]

    if total_warnings:
        print('{} files need to be formatted, please review them'.format(
            len(total_warnings)))
        for warning in total_warnings:
            print(warning)
        if args.export_fixes is True:
            print('File fixes have been saved in clang-tidy-fixes/ as yml '
                  'files and can be applied with clang-apply-replacements')
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
