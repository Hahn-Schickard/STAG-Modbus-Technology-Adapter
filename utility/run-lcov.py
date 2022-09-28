#!/usr/bin/env python
"""A simplified interface for lcov utility

"""
import os
import shutil
import argparse
import subprocess
from typing import List


def is_installed(executable: str, encoding='utf-8', throw_on_failure=True, live_print=True):
    try:
        print('Checking if {} is installed'.format(executable))
        process = subprocess.Popen([executable, '--version'],
                                   stdout=subprocess.PIPE, stderr=subprocess.PIPE, encoding=encoding, universal_newlines=True)
        if live_print:
            while True:
                line = process.stdout.readline().strip()
                if process.poll() is not None:
                    break
                if line:
                    print(line)
        process.wait()
        stderr = process.stderr.readlines()
        if stderr:
            raise RuntimeError(
                'Calling {} returned and error message {}'.format(executable, stderr))
    except subprocess.CalledProcessError:
        error_msg = 'Program ' + executable + ' is not installed.'
        if throw_on_failure:
            raise FileNotFoundError(error_msg)
        else:
            return error_msg


class PIPE_Value:
    def __init__(self, stdout: str, stderr: str):
        self.stdout = stdout
        self.stderr = stderr


def run_process(executable: str, arguments: [str] = [], encoding='utf-8', throw_on_failure=True, live_print=True, live_print_errors=False):
    command = [executable]
    if arguments:
        command.extend(arguments)
    try:
        process = subprocess.Popen(command, stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE, encoding=encoding, universal_newlines=True)
        stdout = str()
        stderr = str()
        if live_print:
            while True:
                line = process.stdout.readline()
                e_line = str()
                if live_print_errors:
                    e_line = process.stderr.readline()
                if process.poll() is not None:
                    break
                if line:
                    stdout += line
                    print(line.strip())
                if e_line:
                    stderr += e_line
                    print(e_line.strip())

        else:
            process.wait()
            stdout = ''.join(process.stdout.readlines())
        stderr = ''.join(process.stderr.readlines())
        if throw_on_failure:
            if stderr:
                error_msg = 'Running command ' + \
                    ' '.join(command) + ' returned an error: ' + stderr
                raise OSError(process.returncode, ''.join(error_msg))
            else:
                return PIPE_Value(stdout, str())
        else:
            return PIPE_Value(stdout, stderr)
    except subprocess.CalledProcessError as exception:
        raise RuntimeError('An exception occurred while running command: ' +
                           ' '.join(command) + ' Exception is: ' + exception.output)


def read_ignore_list(filename: str):
    if os.path.isfile(filename):
        print('Reading ignored files from: {}'.format(filename))
        with open(filename, 'r', encoding='utf-8') as file:
            ignore_list = file.readlines()
            ignore_list = [ignore_pattern.strip()
                           for ignore_pattern in ignore_list]
        return ignore_list
    else:
        raise FileNotFoundError(filename)


def clean_directory(directory: str):
    if os.path.exists(directory):
        print('Cleaning workdirectory at {}'.format(directory))
        shutil.rmtree(directory, ignore_errors=True)
    os.mkdir(directory)


def remove_ignore_pattern(executable: str, output_file: str, ignore_list: [str]):
    for ignore_pattern in ignore_list:
        print('Ignoring filenames that fit: {} pattern'.format(ignore_pattern))
        process = run_process(executable, ["--remove", output_file,
                                           ignore_pattern, "-o", output_file], throw_on_failure=False)
        if process.stderr:
            print(process.stderr)
        if process.stdout:
            print(process.stdout)


def generate_coverage_report(executable: str, build_directory: str):
    cwd = os.path.dirname(os.path.realpath(__file__))
    workspace = os.path.join(cwd, "..", "code_coverage_report")
    output_file = os.path.join(workspace, "code_coverage.info")
    clean_directory(workspace)
    print('Running {} at {}'.format(executable, build_directory))
    run_process(executable, ["--directory", build_directory, "--capture", "--output-file",
                             output_file, "-rc", "lcov_branch_coverage=1"], throw_on_failure=False)
    if os.path.isfile(output_file):
        ignore_list = read_ignore_list(
            os.path.join(cwd, '.lcov-ignores'))
        remove_ignore_pattern(executable, output_file, ignore_list)
        coverage_report = run_process(
            executable, ["--list", output_file], throw_on_failure=False).stdout
        if coverage_report:
            print(coverage_report)
            run_process("genhtml", [
                output_file, "--branch-coverage", "--output-directory", workspace + "/"])
        else:
            raise RuntimeError('No ')
    else:
        raise RuntimeError('{} did not generate a {}'.format(
            executable, output_file))


def to_bool(value):
    valid = {'true': True, 't': True, '1': True,
             'false': False, 'f': False, '0': False}

    if isinstance(value, bool):
        return value

    if not isinstance(value, str):
        raise ValueError('invalid letter for boolean. Not a string.')

    lower_value = value.lower()
    if lower_value in valid:
        return valid[lower_value]
    else:
        raise ValueError('invalid letter for boolean: "%s"' % value)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build_directory", help="the build directory for lcov",
                        type=str)
    parser.add_argument("runnable", help="set to True if a given target is runnable",
                        type=str)
    parser.add_argument("target", help="full path to the binary target that will be analyzed",
                        type=str)
    parser.add_argument('arguments', nargs='?', default=[],
                        help="add an argument to the list of arguments, that are used by the target binary")
    args = parser.parse_args()

    if to_bool(args.runnable):
        print('Running target {}{}'.format(args.target, args.arguments))
        run_process(args.target, args.arguments)
    print('Generating code coverage report based on build directory at {}'.format(
        args.build_directory))
    is_installed('lcov')
    generate_coverage_report('lcov', args.build_directory)
