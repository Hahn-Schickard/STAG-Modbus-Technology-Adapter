#!/usr/bin/env python
"""This utility generates a static page for project releases

"""
import os
import shutil
import markdown
import argparse
import fileinput
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


def getProjectName(dir_loc):
    cmake_file = os.path.join(dir_loc, 'CMakeLists.txt')
    if os.path.isfile(cmake_file):
        with open(cmake_file, 'r', encoding='utf-8') as file:
            content = file.read()
            return re.search(r'set\(THIS (.*)\)', content).group(1)
    else:
        raise FileNotFoundError(
            'No CMakeLists.txt file was found in directory {}'.format(dir_loc))


def generate_location(root_dir_loc, workspace_name, workspace_loc):
    if os.path.exists(workspace_name):
        shutil.rmtree(workspace_name, ignore_errors=True)

    amalgam_template_loc = os.path.join(root_dir_loc, 'docs')

    if os.path.exists(os.path.join(amalgam_template_loc, 'code_documentation', 'html', 'index.html')):
        shutil.copytree(amalgam_template_loc, workspace_loc)
        shutil.move(os.path.join(workspace_loc, 'code_documentation', 'html'),
                    os.path.join(workspace_loc, "documentation"))
        shutil.rmtree(os.path.join(
            workspace_loc, 'code_documentation'), ignore_errors=True)
    else:
        raise RuntimeError("Code documentation not found")

    code_coverage_loc = os.path.join(
        root_dir_loc, 'code_coverage_report')
    if os.path.exists(code_coverage_loc):
        shutil.copytree(code_coverage_loc, os.path.join(
            workspace_loc, 'code_coverage'))
    else:
        raise RuntimeError("Code test coverage report not found")


def create_about_html(root_dir_loc, workspace_loc):
    with open(os.path.join(root_dir_loc, 'README.md'), "r", encoding="utf-8") as input_file:
        text = input_file.read()
    html = markdown.markdown(text)
    with open(os.path.join(workspace_loc, 'about.html'), "w", encoding="utf-8") as output_file:
        output_file.writelines(html)


def create_header_html(workspace_loc, project_name, project_version, homepage):
    os.rename(os.path.join(workspace_loc, '.index.html'),
              os.path.join(workspace_loc, 'index.html'))
    os.rename(os.path.join(workspace_loc, '.w3.css'),
              os.path.join(workspace_loc, 'w3.css'))
    with fileinput.FileInput(os.path.join(workspace_loc, 'index.html'), inplace=True) as file:
        for line in file:
            print(line.replace("PROJECT_WEBSITE", homepage), end='')
    with fileinput.FileInput(os.path.join(workspace_loc, 'index.html'), inplace=True) as file:
        for line in file:
            print(line.replace("PROJECT_NAME", project_name), end='')
    with fileinput.FileInput(os.path.join(workspace_loc, 'index.html'), inplace=True) as file:
        for line in file:
            print(line.replace("PROJECT_VERSION", project_version), end='')


def generate_html(root_dir_loc, workspace_loc, project_name, project_version, homepage):
    if os.path.exists(os.path.join(root_dir_loc, 'README.md')):
        create_about_html(root_dir_loc, workspace_loc)
        create_header_html(workspace_loc, project_name,
                           project_version, homepage)
    else:
        raise RuntimeError("Project README.md not found.")


def get_code_coverage_percent(workspace_loc):
    with open(os.path.join(workspace_loc, 'code_coverage', 'index.html')) as fp:
        for i, line in enumerate(fp):
            if i == 44:
                coverage_percent = line.strip()
    coverage_percent = re.sub('<.*?>', '', coverage_percent)
    coverage_percent = re.sub(r'([.]+[\s\S]*)', '', coverage_percent)
    return coverage_percent


def print_code_coverage_percent(workspace_loc, coverage_percent):
    with fileinput.FileInput(os.path.join(workspace_loc, 'index.html'), inplace=True) as file:
        for line in file:
            print(line.replace("PERCENT", coverage_percent), end='')


def define_coverage_percent_colour(coverage_percent):
    coverage_colour = "red"
    if int(coverage_percent) > 40:
        coverage_colour = "orange"
    if int(coverage_percent) > 60:
        coverage_colour = "yellow"
    if int(coverage_percent) > 80:
        coverage_colour = "green"
    return coverage_colour


def set_coverage_percent_colour(workspace_loc, coverage_percent):
    coverage_colour = define_coverage_percent_colour(coverage_percent)
    with fileinput.FileInput(os.path.join(workspace_loc, 'index.html'), inplace=True) as file:
        for line in file:
            print(line.replace("COLOUR", coverage_colour), end='')


def generate_code_coverage_percent(workspace_loc):
    if os.path.exists(os.path.join(workspace_loc, 'code_coverage', 'index.html')):
        coverage_percent = get_code_coverage_percent(workspace_loc)
        print_code_coverage_percent(workspace_loc, coverage_percent)
        set_coverage_percent_colour(workspace_loc, coverage_percent)
    else:
        raise RuntimeError(
            "Code coverage report was not copied into the work directory")


def generate_license_html(workspace_loc, root_dir_loc):
    with open(os.path.join(workspace_loc, 'license.html'), 'w', encoding="utf-8") as output_file:
        license_loc = os.path.join(root_dir_loc, 'LICENSE')
        if os.path.exists(license_loc):
            license = markdown.markdown(
                open(license_loc, "r", encoding="utf-8").read())
            output_file.writelines(license)
        else:
            output_file.writelines(markdown.markdown("No license"))
        notice_loc = os.path.join(root_dir_loc, 'NOTICE')
        if os.path.exists(notice_loc):
            notice = markdown.markdown(
                open(notice_loc, "r", encoding="utf-8").read())
            output_file.writelines(notice)
        else:
            output_file.writelines(markdown.markdown("No notices"))


def create_amalgam_website(root_dir_loc, project_name, project_version, workspace_name, workspace_loc, homepage):
    generate_location(root_dir_loc, workspace_name, workspace_loc)
    generate_html(root_dir_loc, workspace_loc,
                  project_name, project_version, homepage)
    generate_code_coverage_percent(workspace_loc)
    generate_license_html(workspace_loc, root_dir_loc)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Arguments to use")
    parser.add_argument("homepage", type=str, help="project homepage")
    parser.add_argument("-dv", "--dummyVersion", type=str,
                        help="Test version of the project")
    homepage = parser.parse_args().homepage
    dummy_project_version = parser.parse_args().dummyVersion

    root_dir_loc = os.path.abspath(
        os.path.join(os.path.dirname(__file__), '..'))
    project_name = getProjectName(root_dir_loc)
    if dummy_project_version:
        project_version = dummy_project_version
    else:
        project_version = getVersionFromTag(True)
    workspace_name = "public"
    workspace_loc = os.path.join(root_dir_loc, workspace_name)

    create_amalgam_website(root_dir_loc, project_name, project_version,
                           workspace_name, workspace_loc, homepage)
