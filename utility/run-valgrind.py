#!/usr/bin/env python
"""A simplified interface for valgrind utility

"""
import os
import argparse
import sys
import subprocess
from typing import List


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


def file_exists(name: str):
    if os.path.isfile(name):
        return True
    else:
        raise FileNotFoundError(name)


def count_errors(error_msg: str, error_beggining_marker: str, error_end_marker: str):
    if error_msg:
        error_marker_beginning = error_msg.find(
            error_beggining_marker) + len(error_beggining_marker)
        error_marker_end = error_msg.find(error_end_marker)
        return int(error_msg[error_marker_beginning:error_marker_end])
    else:
        return 0


def read_log(logfile: str):
    with open(logfile, "r") as file:
        return file.read()


def run_memory_analysis(analyzer: str, settings: [str], error_beggining_marker: str, error_end_marker: str, target: str, arguments: list, output_file=""):
    file_exists(target)
    settings.append(target)
    if arguments:
        settings.extend(arguments)
    print("Runing memory analysis with {} {}".format(
        analyzer, ' '.join(settings)))
    print('On target {} with args {}'.format(target, arguments))
    run_process(
        analyzer, settings, throw_on_failure=False)
    result = read_log(output_file)
    print(result)
    error_count = count_errors(
        result, error_beggining_marker, error_end_marker)
    if error_count > 0:
        error_msg = '{} found {} errors'.format(analyzer, error_count)
        raise RuntimeError(error_msg)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "target", help="full path to the binary target that will be analyzed", type=str)
    parser.add_argument('arguments', nargs='*', default=[],
                        help="add an argument to the list of arguments, that are used by the target binary")
    args = parser.parse_args()
    try:
        run_memory_analysis("valgrind", ["--leak-check=full", "--show-leak-kinds=all", "--trace-children=yes", "--track-origins=yes", "--verbose", "--log-file=valgrind.log"],
                            "ERROR SUMMARY: ", "errors", args.target, args.arguments, output_file="valgrind.log")
    except Exception as exception:
        exception_type = "{} exception occurred while trying to run memory analysis.".format(
            type(exception).__name__)
        exception_message = "Exception states: {}".format(exception.args)
        print(exception_type)
        print(exception_message)
        sys.exit(1)
