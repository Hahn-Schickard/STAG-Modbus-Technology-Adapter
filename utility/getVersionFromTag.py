#!/usr/bin/env python
"""This utility returns a version string from a git tag

"""

import sys
import re
from utilities import run_process


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
