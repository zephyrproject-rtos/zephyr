#!/usr/bin/env python3
# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

import logging
import subprocess
import sys
import os
import yaml

logger = logging.getLogger('twister')
logger.setLevel(logging.DEBUG)

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")


class Image:
    """Store information about an image and statuses of its stages"""
    def __init__(self, image, main_build, main_source):
        self.name = image['name']
        self.path = image['path']
        self.extra_args = image.get('extra_args', None)
        if self.extra_args:
            self.extra_args = [self.extra_args]
        else:
            self.extra_args = []
        # Each image has two stages, as regular images:
        # 'cmake' and 'build'. Needed for piping in ProjectBuilder.
        self.cmake_done = False
        self.build_done = False
        self.build_dir = os.path.normpath(os.path.join(main_build, self.name))
        self.source_dir = os.path.normpath(os.path.join(main_source, self.path))

    def get_args_for_pb(self):
        """Return args to switch the active image in ProjectBuilder"""
        args_for_pb = [
            self.source_dir,
            self.build_dir,
            self.extra_args
        ]
        return args_for_pb


class MultiImage:
    """Handle tests with multiple images"""
    def __init__(self, build_dir, source_dir, images):
        self.main_source = source_dir
        self.main_build = build_dir
        self.images = {}
        for image in images:
            self.images[image['name']] = Image(image, self.main_build, self.main_source)

    def image_toggler(self, message):
        """Toggle between images and stages until all images have cmake and build stages done"""
        # TODO: This can be improved so the loop does not always start from the beginning
        for image in self.images:
            args_for_pb = self.images[image].get_args_for_pb()
            if not self.images[image].cmake_done:
                op = "cmake"
                active_image = image
                break
            elif not self.images[image].build_done:
                op = "build"
                active_image = image
                break
        else:
            # If we got to this place it means all images have all their stages
            # done and the original op can be proceed
            active_image = 'main'
            op = message.get('op')
        return active_image, op, args_for_pb


class ExecutionStage:
    """Abstract class for test execution stages."""
    def __init__(self, description=None, proj_builder=None):
        """Abstract constructor"""
        self.description = description
        if proj_builder:
            self.pb = proj_builder

    def run(self, hardware_fixed=None):
        """Define procedure to be executed at a given stage.

        Must be implemented in sub classes. Defines actions taken during a given
        stage.
        """
        raise NotImplementedError(f"{self.__class__.__name__}.run()")

    def report_error(self, message):
        """ Report error and set the handler status"""
        logger.error(message)
        self.pb.instance.handler.state = "failed"
        self.pb.instance.reason = message


class CallScriptsStage(ExecutionStage):
    """Stage for running a user-defined script."""
    def __init__(self, description=None, proj_builder=None):
        ExecutionStage.__init__(self, description, proj_builder)

    def run(self, hardware_fixed=None):
        """Execute user-defined script"""
        for script in self.description:
            logger.debug(f"Calling: {script}")
            s = script.split()
            try:
                run_custom_script(script=s, timeout=15)
            except Exception as ex:
                logger.error(ex)
                self.report_error("Script error")


class WestSignStage(ExecutionStage):
    """ Stage for signing an image using west sign and imgtool."""
    def __init__(self, description=None, proj_builder=None):
        ExecutionStage.__init__(self, description, proj_builder)

    def run(self, hardware_fixed=None):
        """Run west sign with arguments that can be defined in description"""
        # 'image' tells twister which image to sign
        image = self.description.get('image', 'main')
        imgtool_path = os.path.join(ZEPHYR_BASE,
                                    "../bootloader/mcuboot/scripts/imgtool.py")

        # Load args for signing an image
        img_arg_path = (os.path.join(self.pb.source_dir, "imgtool_conf.yaml"))

        with open(img_arg_path, 'r') as file:
            try:
                imgtool_args_file = yaml.safe_load(file)
            except yaml.YAMLError as exc:
                logger.error(exc)

        # Load default imgtool args from file
        imgtool_args = imgtool_args_file['default']

        # Load platform-specific args if needed
        if self.pb.instance.platform.name in imgtool_args_file:
            imgtool_args.update(imgtool_args_file[self.pb.instance.platform.name])

        # Update imgtool args if they are given in a stage description
        if 'imgtool_args' in self.description:
            imgtool_args.update(self.description['imgtool_args'])

        if imgtool_args['key'] == 'default':
            imgtool_args['key'] = os.path.join(ZEPHYR_BASE,
                                               "../bootloader/mcuboot/root-rsa-2048.pem")
        else:
            imgtool_args['key'] = os.path.join(ZEPHYR_BASE,
                                               imgtool_args['key'])

        if self.pb.instance.multi_build:
            try:
                img_path = self.pb.instance.multi_build.images[image].build_dir
            except KeyError:
                self.report_error(f"Image {image} not defined in multi_build")
        elif image == 'main':
            img_path = self.pb.instance.build_dir
        else:
            self.report_error(f"Unknown image to sign: {image}")
            raise Exception("Stage error")

        # Create command for west sign and add extra arguments based on used configuration
        command = ["west", "sign", "-d", img_path, "--shex",
                   f"{img_path}/zephyr/zephyr.hex", "-t",
                   "imgtool", "-p",
                   imgtool_path, "--"
                   ]
        for k, v in imgtool_args.items():
            arg = f'--{k.replace("_", "-")}'
            if k == "hex_addr":
                # --hex-addr is used only with "--pad"
                continue
            if v is None:
                continue
            elif v is True:
                command.extend([arg])
                # Add hex-addr arg only when "pad" is True.
                if k == "pad":
                    command.extend(["--hex-addr", imgtool_args['hex_addr']])
            elif v is False:
                continue
            else:
                command.extend([arg, f"{v}"])
        # command.append("--")

        logger.debug(" ".join(command))
        try:
            run_custom_script(command, timeout=15)
        except Exception as ex:
            logger.error(ex)
            self.report_error("Script error")


class OnTargetStage(ExecutionStage):
    def __init__(self, description=None, proj_builder=None):
        ExecutionStage.__init__(self, description, proj_builder)

    def run(self, hardware_fixed=None):
        """Execute the the test on target.

        As in ProjectBuilder.run() but with MultiImage support
        """
        instance = self.pb.instance
        suite = self.pb.suite
        image = self.description.get('image', None)
        command = self.description.get('command', None)

        # instance.testcase attributes have to be updated with stage specific data
        for k, v in self.description.items():
            if k in ["harness", "harness_config"]:
                setattr(instance.testcase, k, v)

        # As in ProjectBuilder.run()
        if instance.handler:
            if instance.handler.type_str == "device":
                instance.handler.suite = suite
            # Point to image's build dir
            if image:
                instance.handler.build_dir = instance.multi_build.images[image].build_dir
            instance.handler.handle(command=command, hardware_fixed=hardware_fixed)

        sys.stdout.flush()


class StageContainer:
    def __init__(self, proj_builder):
        self.pb = proj_builder
        self.stages = self.get_stages()
        self.total_time = 0

    def __iter__(self):
        for stage in self.stages:
            yield stage

    def get_stages(self):
        stages = []
        for stage in self.pb.instance.testcase.stages:
            (name, description), = stage.items()
            # This will create a stage object of a type given in the name
            # e.g. for name=CallScript: CallScriptStage(description) object
            # will be created
            ev = f"{name}Stage({description}, self.pb)"
            stages.append(eval(ev))
        return stages


def run_custom_script(script, timeout):
    with subprocess.Popen(script, stderr=subprocess.PIPE,
                          stdout=subprocess.PIPE) as proc:
        try:
            stdout, stderr = proc.communicate(timeout=timeout)
            logger.debug(stdout.decode())
            if stderr:
                raise Exception(stderr.decode())

        except subprocess.TimeoutExpired:
            proc.kill()
            proc.communicate()
            logger.error("{} timed out".format(script))
            raise TimeoutError
