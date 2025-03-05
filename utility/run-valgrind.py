#!/usr/bin/env python
"""A simplified interface for valgrind utility

"""
import os
import argparse
import sys
import subprocess
import xml.etree.ElementTree as ET
from typing import List


class PIPE_Value:
    def __init__(self, stdout: str, stderr: str):
        self.stdout = stdout
        self.stderr = stderr


def run_process(executable: str, arguments, encoding='utf-8', throw_on_failure=True, live_print=True, live_print_errors=False):
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
                    stripped = line.encode(encoding).strip()
                    print(stripped)
                if e_line:
                    stderr += e_line
                    stripped = e_line.encode(encoding).strip()
                    print(stripped)

        else:
            process.wait()
            stdout = ''.join(process.stdout.readlines())
        stderr = ''.join(process.stderr.readlines())
        if throw_on_failure:
            return_code = process.returncode
            if return_code != 0:
                error_msg = 'Running command ' + \
                    ' '.join(command) + ' returned an error: ' + stderr
                raise OSError(return_code, ''.join(error_msg))
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


def print_log(logfile: str):
    with open(logfile, "r") as file:
        text = file.read()
        print(text)

def print_xml(xml_file: str):
    root = ET.parse(xml_file).getroot()
    errors = root.findall("error")
    if errors:
        for error in errors:
            print("Error:", error.find("kind").text)
            if error.find("xwhat") is not None:
                print(" ", error.find("xwhat/text").text)
            else:
                print(" ", error.find("what").text)
            print(" Call stack:")
            offset_count = 1
            for frame in error.findall("stack/frame"):
                print_offset = " " + " " * offset_count
                print(print_offset, "In object: ",  frame.find("obj").text)
                print(print_offset, "At address: ",  frame.find("ip").text)
                if frame.find("fn") is not None:
                    print(print_offset, "Function:", frame.find("fn").text)
                if frame.find("dir") is not None:
                    dir_path = frame.find("dir").text
                    file_path = dir_path + "/" + frame.find("file").text
                    line_nr = file_path + ":" + frame.find("line").text
                    print("  In file: ", line_nr)
                offset_count += 1
    else:
        "Valgrind found no errors"

def get_error_count(xml_file: str):
    root = ET.parse(xml_file).getroot()
    error_count = 0
    for error_counter in root.findall('./errorcounts/pair/count'):
        if error_counter is not None:
            error_count += int(error_counter.text)
    return error_count


def run_memory_analysis(analyzer: str, settings, target: str, arguments: list, log_file="", xml_file=""):
    file_exists(target)
    settings.append(target)
    if arguments:
        settings.extend(arguments)
    print("Running memory analysis with {} {}".format(
        analyzer, ' '.join(settings)))
    print('On target {} with args {}'.format(target, arguments))
    run_process(
        analyzer, settings, throw_on_failure=False)
    print_log(log_file)
    print_xml(xml_file)
    error_count = get_error_count(xml_file)
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
        run_memory_analysis("valgrind", ["--leak-check=full", "--show-leak-kinds=all", "--trace-children=yes", "--track-origins=yes", "--verbose", "--log-file=valgrind.log", "--xml=yes", "--xml-file=valgrind.xml", "--num-callers=500"],
                            args.target, args.arguments, log_file="valgrind.log", xml_file="valgrind.xml")
    except Exception as exception:
        exception_type = "{} exception occurred while trying to run memory analysis.".format(
            type(exception).__name__)
        exception_message = "Exception states: {}".format(exception.args)
        print(exception_type)
        print(exception_message)
        sys.exit(1)
