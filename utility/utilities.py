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


def run_process(executable: str, arguments: List[str] = [], encoding='utf-8', throw_on_failure=True, live_print=True, live_print_errors=False):
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
