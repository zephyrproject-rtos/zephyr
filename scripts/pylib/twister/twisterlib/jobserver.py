# Copyright 2022 Google LLC.
# SPDX-License-Identifier: Apache-2.0
"""Module for job counters, limiting the amount of concurrent executions."""

import fcntl
import functools
import logging
import multiprocessing
import os
import re
import select
import selectors
import subprocess
import sys

logger = logging.getLogger('twister')


class JobHandle:
    """Small object to handle claim of a job."""

    def __init__(self, release_func, *args, **kwargs):
        self.release_func = release_func
        self.args = args
        self.kwargs = kwargs

    def __enter__(self):
        pass

    def __exit__(self, exc_type, exc_value, traceback):
        if self.release_func:
            self.release_func(*self.args, **self.kwargs)


class JobClient:
    """Abstract base class for all job clients."""

    def get_job(self):
        """Claim a job."""
        return JobHandle(None)

    @staticmethod
    def env():
        """Get the environment variables necessary to share the job server."""
        return {}

    @staticmethod
    def pass_fds():
        """Returns the file descriptors that should be passed to subprocesses."""
        return []

    def popen(self, argv, **kwargs):
        """Start a process using subprocess.Popen

        All other arguments are passed to subprocess.Popen.

        Returns:
            A Popen object.
        """
        kwargs.setdefault("env", os.environ)
        kwargs.setdefault("pass_fds", [])
        kwargs["env"].update(self.env())
        kwargs["pass_fds"] += self.pass_fds()

        return subprocess.Popen(argv, **kwargs)


class GNUMakeJobClient(JobClient):
    """A job client for GNU make.

    A client of jobserver is allowed to run 1 job without contacting the
    jobserver, so maintain an optional self._internal_pipe to hold that
    job.
    """

    def __init__(self, inheritable_pipe, jobs, internal_jobs=0, makeflags=None):
        self._makeflags = makeflags
        self._inheritable_pipe = inheritable_pipe
        self.jobs = jobs
        self._selector = selectors.DefaultSelector()
        if internal_jobs:
            self._internal_pipe = os.pipe()
            os.write(self._internal_pipe[1], b"+" * internal_jobs)
            os.set_blocking(self._internal_pipe[0], False)
            self._selector.register(
                self._internal_pipe[0],
                selectors.EVENT_READ,
                self._internal_pipe[1],
            )
        else:
            self._internal_pipe = None
        if self._inheritable_pipe is not None:
            os.set_blocking(self._inheritable_pipe[0], False)
            self._selector.register(
                self._inheritable_pipe[0],
                selectors.EVENT_READ,
                self._inheritable_pipe[1],
            )

    def __del__(self):
        if self._inheritable_pipe:
            os.close(self._inheritable_pipe[0])
            os.close(self._inheritable_pipe[1])
        if self._internal_pipe:
            os.close(self._internal_pipe[0])
            os.close(self._internal_pipe[1])

    @classmethod
    def from_environ(cls, env=None, jobs=0):
        """Create a job client from an environment with the MAKEFLAGS variable.

        If we are started under a GNU Make Job Server, we can search
        the environment for a string "--jobserver-auth=R,W", where R
        and W will be the read and write file descriptors to the pipe
        respectively.  If we don't find this environment variable (or
        the string inside of it), this will raise an OSError.

        The specification for MAKEFLAGS is:
          * If the first char is "n", this is a dry run, just exit.
          * If the flags contains -j1, go to sequential mode.
          * If the flags contains --jobserver-auth=R,W AND those file
            descriptors are valid, use the jobserver. Otherwise output a
            warning.

        Args:
            env: Optionally, the environment to search.
            jobs: The number of jobs set by the user on the command line.

        Returns:
            A GNUMakeJobClient configured appropriately or None if there is
            no MAKEFLAGS environment variable.
        """
        if env is None:
            env = os.environ
        makeflags = env.get("MAKEFLAGS")
        if not makeflags:
            return None
        match = re.search(r"--jobserver-auth=(\d+),(\d+)", makeflags)
        if match:
            pipe = [int(x) for x in match.groups()]
            if jobs:
                pipe = None
                logger.warning(
                    "-jN forced on command line; ignoring GNU make jobserver"
                )
            else:
                try:
                    # Use F_GETFL to see if file descriptors are valid
                    if pipe:
                        rc = fcntl.fcntl(pipe[0], fcntl.F_GETFL)
                        if rc & os.O_ACCMODE != os.O_RDONLY:
                            logger.warning(
                                f"FD {pipe[0]} is not readable (flags={rc:x});"
                                " ignoring GNU make jobserver"
                            )
                            pipe = None
                    if pipe:
                        rc = fcntl.fcntl(pipe[1], fcntl.F_GETFL)
                        if rc & os.O_ACCMODE != os.O_WRONLY:
                            logger.warning(
                                f"FD {pipe[1]} is not writable (flags={rc:x});"
                                " ignoring GNU make jobserver"
                            )
                            pipe = None
                    if pipe:
                        logger.info("using GNU make jobserver")
                except OSError:
                    pipe = None
                    logger.warning(
                        "No file descriptors; ignoring GNU make jobserver"
                    )
        else:
            pipe = None
        if not jobs:
            match = re.search(r"-j(\d+)", makeflags)
            if match:
                jobs = int(match.group(1))
                if jobs == 1:
                    logger.info("Running in sequential mode (-j1)")
        if makeflags[0] == "n":
            logger.info("MAKEFLAGS contained dry-run flag")
            sys.exit(0)
        return cls(pipe, jobs, internal_jobs=1, makeflags=makeflags)

    def get_job(self):
        """Claim a job.

        Returns:
            A JobHandle object.
        """
        while True:
            ready_items = self._selector.select()
            if len(ready_items) > 0:
                read_fd = ready_items[0][0].fd
                write_fd = ready_items[0][0].data
                try:
                    byte = os.read(read_fd, 1)
                    return JobHandle(
                        functools.partial(os.write, write_fd, byte)
                    )
                except BlockingIOError:
                    pass

    def env(self):
        """Get the environment variables necessary to share the job server."""
        if self._makeflags:
            return {"MAKEFLAGS": self._makeflags}
        flag = ""
        if self.jobs:
            flag += f" -j{self.jobs}"
        if self.jobs != 1 and self._inheritable_pipe is not None:
            flag += f" --jobserver-auth={self._inheritable_pipe[0]},{self._inheritable_pipe[1]}"
        return {"MAKEFLAGS": flag}

    def pass_fds(self):
        """Returns the file descriptors that should be passed to subprocesses."""
        if self.jobs != 1 and self._inheritable_pipe is not None:
            return self._inheritable_pipe
        return []


class GNUMakeJobServer(GNUMakeJobClient):
    """Implements a GNU Make POSIX Job Server.

    See https://www.gnu.org/software/make/manual/html_node/POSIX-Jobserver.html
    for specification.
    """

    def __init__(self, jobs=0):
        if not jobs:
            jobs = multiprocessing.cpu_count()
        elif jobs > select.PIPE_BUF:
            jobs = select.PIPE_BUF
        super().__init__(os.pipe(), jobs)

        os.write(self._inheritable_pipe[1], b"+" * jobs)
