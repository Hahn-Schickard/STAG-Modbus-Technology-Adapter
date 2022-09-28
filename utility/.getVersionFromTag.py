#!/usr/bin/env python
"""This utility returns a version string from a git tag

"""

import sys
import re
import subprocess


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


def getVersionFromTag(exact_match=True):
    args = ['describe', '--abbrev=0', '--all']
    if exact_match:
        args.append('--exact-match')
    process = run_process('git', args, live_print=False)
    tag = process.stdout
    return re.sub('([A-Z]*?[^0-9.])', '', tag)


if __name__ == "__main__":
    try:
        print(getVersionFromTag(False))
    except OSError as error:
        print('Return Code: {}\nError message: {}'.format(
            error.errno, error.strerror.rstrip()))
        sys.exit(error.errno)
    except RuntimeError as error:
        print(error.args)
        sys.exit(1)
