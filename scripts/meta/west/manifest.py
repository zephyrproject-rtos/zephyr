# Copyright (c) 2018, Nordic Semiconductor ASA
# Copyright 2018, Foundries.io Ltd
#
# SPDX-License-Identifier: Apache-2.0

'''Parser and abstract data types for west manifests.

The main class is Manifest. The recommended method for creating a
Manifest instance is via its from_file() or from_data() helper
methods.

There are additionally Defaults, Remote, and Project types defined,
which represent the values by the same names in a west
manifest. (I.e. "Remote" represents one of the elements in the
"remote" sequence in the manifest, and so on.) Some Default values,
such as the default project revision, may be supplied by this module
if they are not present in the manifest data.'''

import os

import pykwalify.core
import yaml

from west import util, log

# Todo: take from _bootstrap?
# Default west repository URL.
WEST_URL_DEFAULT = 'https://github.com/zephyrproject-rtos/west'
# Default revision to check out of the west repository.
WEST_REV_DEFAULT = 'master'

META_NAMES = ['west', 'manifest']
'''Names of the special "meta-projects", which are reserved and cannot
be used to name a project in the manifest file.'''

MANIFEST_SECTIONS = ['manifest', 'west']
'''Sections in the manifest file'''


def default_path():
    '''Return the path to the default manifest in the west directory.

    Raises WestNotFound if called from outside of a west working directory.'''
    return os.path.join(util.west_dir(), 'manifest', 'default.yml')


class Manifest:
    '''Represents the contents of a West manifest file.

    The most convenient way to construct an instance is using the
    from_file and from_data helper methods.'''

    @staticmethod
    def from_file(source_file=None, sections=MANIFEST_SECTIONS):
        '''Create and return a new Manifest object given a source YAML file.

        :param source_file: Path to a YAML file containing the manifest.
        :param sections: Only parse specified sections from YAML file,
                         default: all sections are parsed.

        If source_file is None, the value returned by default_path()
        is used.

        Raises MalformedManifest in case of validation errors.'''
        if source_file is None:
            source_file = default_path()
        return Manifest(source_file=source_file, sections=sections)

    @staticmethod
    def from_data(source_data, sections=MANIFEST_SECTIONS):
        '''Create and return a new Manifest object given parsed YAML data.

        :param source_data: Parsed YAML data as a Python object.
        :param sections: Only parse specified sections from YAML data,
                         default: all sections are parsed.

        Raises MalformedManifest in case of validation errors.'''
        return Manifest(source_data=source_data, sections=sections)

    def __init__(self, source_file=None, source_data=None,
                 sections=MANIFEST_SECTIONS):
        '''Create a new Manifest object.

        :param source_file: Path to a YAML file containing the manifest.
        :param source_data: Parsed YAML data as a Python object.
        :param sections: Only parse specified sections from YAML file,
                         default: all sections are parsed.

        Normally, it is more convenient to use the `from_file` and
        `from_data` convenience factories than calling the constructor
        directly.

        Exactly one of the source_file and source_data parameters must
        be given.

        Raises MalformedManifest in case of validation errors.'''
        if source_file and source_data:
            raise ValueError('both source_file and source_data were given')

        if source_file:
            with open(source_file, 'r') as f:
                self._data = yaml.safe_load(f.read())
            path = source_file
        else:
            self._data = source_data
            path = None

        self.path = path
        '''Path to the file containing the manifest, or None if created
        from data rather than the file system.'''

        if not self._data:
            self._malformed('manifest contains no data')

        if 'manifest' not in self._data:
            self._malformed('manifest contains no manifest element')

        for key in self._data:
            if key in sections:
                try:
                    pykwalify.core.Core(
                        source_data=self._data[key],
                        schema_files=[_SCHEMA_PATH[key]]
                    ).validate()
                except pykwalify.errors.SchemaError as e:
                    self._malformed(e, key)

        self.defaults = None
        '''west.manifest.Defaults object representing default values
        in the manifest, either as specified by the user or west itself.'''

        self.remotes = None
        '''Sequence of west.manifest.Remote objects representing manifest
        remotes.'''

        self.projects = None
        '''Sequence of west.manifest.Project objects representing manifest
        projects.

        Each element's values are fully initialized; there is no need
        to consult the defaults field to supply missing values.'''

        self.west_project = None
        '''west.manifest.SpecialProject object representing the west meta
        project.'''

        # Set up the public attributes documented above, as well as
        # any internal attributes needed to implement the public API.
        self._load(self._data, sections)

    def get_remote(self, name):
        '''Get a manifest Remote, given its name.'''
        return self._remotes_dict[name]

    def _malformed(self, complaint, section='manifest'):
        context = (' file {} '.format(self.path) if self.path
                   else ' data:\n{}\n'.format(self._data))
        raise MalformedManifest('Malformed manifest{}(schema: {}):\n{}'
                                .format(context, _SCHEMA_PATH[section],
                                        complaint))

    def _load(self, data, sections):
        # Initialize this instance's fields from values given in the
        # manifest data, which must be validated according to the schema.
        if 'west' in sections:
            west = data.get('west', {})

            url = west.get('url') or WEST_URL_DEFAULT
            revision = west.get('revision') or WEST_REV_DEFAULT

            self.west_project = SpecialProject('west',
                                               url=url,
                                               revision=revision,
                                               path=os.path.join('west',
                                                                 'west'))

        # Next is the manifest section
        if 'manifest' not in sections:
            return

        projects = []
        project_abspaths = set()

        manifest = data.get('manifest')

        # Map from each remote's name onto that remote's data in the manifest.
        remotes = tuple(Remote(r['name'], r['url-base']) for r in
                        manifest['remotes'])
        remotes_dict = {r.name: r for r in remotes}

        # Get any defaults out of the manifest.
        #
        # md = manifest defaults (dictionary with values parsed from
        # the manifest)
        md = manifest.get('defaults', dict())
        mdrem = md.get('remote')
        if mdrem:
            # The default remote name, if provided, must refer to a
            # well-defined remote.
            if mdrem not in remotes_dict:
                self._malformed('default remote {} is not defined'.
                                format(mdrem))
            default_remote = remotes_dict[mdrem]
            default_remote_name = mdrem
        else:
            default_remote = None
            default_remote_name = None
        defaults = Defaults(remote=default_remote, revision=md.get('revision'))

        # mp = manifest project (dictionary with values parsed from
        # the manifest)
        for mp in manifest['projects']:
            # Validate the project name.
            name = mp['name']
            if name in META_NAMES:
                self._malformed('the name "{}" is reserved and cannot '.
                                format(name) +
                                'be used to name a manifest project')

            # Validate the project remote.
            remote_name = mp.get('remote', default_remote_name)
            if remote_name is None:
                self._malformed('project {} does not specify a remote'.
                                format(name))
            if remote_name not in remotes_dict:
                self._malformed('project {} remote {} is not defined'.
                                format(name, remote_name))
            project = Project(name,
                              remotes_dict[remote_name],
                              defaults,
                              path=mp.get('path'),
                              clone_depth=mp.get('clone-depth'),
                              revision=mp.get('revision'))

            # Two projects cannot have the same path. We use absolute
            # paths to check for collisions to ensure paths are
            # normalized (e.g. for case-insensitive file systems or
            # in cases like on Windows where / or \ may serve as a
            # path component separator).
            if project.abspath in project_abspaths:
                self._malformed('project {} path {} is already in use'.
                                format(project.name, project.path))

            project_abspaths.add(project.abspath)
            projects.append(project)

        self.defaults = defaults
        self.remotes = remotes
        self._remotes_dict = remotes_dict
        self.projects = tuple(projects)


class MalformedManifest(Exception):
    '''Exception indicating that west manifest parsing failed due to a
    malformed value.'''


# Definitions for Manifest attribute types.

class Defaults:
    '''Represents default values in a manifest, either specified by the
    user or by west itself.

    Defaults are neither comparable nor hashable.'''

    __slots__ = 'remote revision'.split()

    def __init__(self, remote=None, revision=None):
        '''Initialize a defaults value from manifest data.

        :param remote: Remote instance corresponding to the default remote,
                       or None (an actual Remote object, not the name of
                       a remote as a string).
        :param revision: Default Git revision; 'master' if not given.'''
        if remote is not None:
            _wrn_if_not_remote(remote)
        if revision is None:
            revision = 'master'

        self.remote = remote
        self.revision = revision

    def __eq__(self, other):
        return NotImplemented

    def __repr__(self):
        return 'Defaults(remote={}, revision={})'.format(repr(self.remote),
                                                         repr(self.revision))


class Remote:
    '''Represents a remote defined in a west manifest.

    Remotes may be compared for equality, but are not hashable.'''

    __slots__ = 'name url_base'.split()

    def __init__(self, name, url_base):
        '''Initialize a remote from manifest data.

        :param name: remote's name
        :param url_base: remote's URL base.'''
        if url_base.endswith('/'):
            log.wrn('Remote', name, 'URL base', url_base,
                    'ends with a slash ("/"); these are automatically',
                    'appended by West')

        self.name = name
        self.url_base = url_base

    def __eq__(self, other):
        return self.name == other.name and self.url_base == other.url_base

    def __repr__(self):
        return 'Remote(name={}, url_base={})'.format(repr(self.name),
                                                     repr(self.url_base))


class Project:
    '''Represents a project defined in a west manifest.

    Projects are neither comparable nor hashable.'''

    __slots__ = 'name remote url path abspath clone_depth revision'.split()

    def __init__(self, name, remote, defaults, path=None, clone_depth=None,
                 revision=None):
        '''Specify a Project by name, Remote, and optional information.

        :param name: Project's user-defined name in the manifest.
        :param remote: Remote instance corresponding to this Project as
                       specified in the manifest. This is used to build
                       the project's URL, and is also stored as an attribute.
        :param defaults: If the revision parameter is not given, the project's
                         revision is set to defaults.revision.
        :param path: Relative path to the project in the west
                     installation, if present in the manifest. If not given,
                     the project's ``name`` is used.
        :param clone_depth: Nonnegative integer clone depth if present in
                            the manifest.
        :param revision: Project revision as given in the manifest, if present.
                         If not given, defaults.revision is used instead.
        '''
        _wrn_if_not_remote(remote)

        self.name = name
        self.remote = remote
        self.url = remote.url_base + '/' + name
        self.path = os.path.normpath(path or name)
        self.abspath = os.path.realpath(os.path.join(util.west_topdir(),
                                                     self.path))
        self.clone_depth = clone_depth
        self.revision = revision or defaults.revision

    def __eq__(self, other):
        return NotImplemented

    def __repr__(self):
        reprs = [repr(x) for x in
                 (self.name, self.remote, self.url, self.path,
                  self.abspath, self.clone_depth, self.revision)]
        return ('Project(name={}, remote={}, url={}, path={}, abspath={}, '
                'clone_depth={}, revision={})').format(*reprs)


class SpecialProject(Project):
    '''Represents a special project, e.g. the west or manifest project.

    Projects are neither comparable nor hashable.'''

    def __init__(self, name, path=None, revision=None, url=None):
        '''Specify a Special Project by name, and url, and optional information.

        :param name: Special Project's user-defined name in the manifest
        :param path: Relative path to the project in the west
                     installation, if present in the manifest. If None,
                     the project's ``name`` is used.
        :param revision: Project revision as given in the manifest, if present.
        :param url: Complete URL for special project.
        '''
        self.name = name
        self.url = url
        self.path = path or name
        self.abspath = os.path.realpath(os.path.join(util.west_topdir(),
                                                     self.path))
        self.revision = revision
        self.remote = None
        self.clone_depth = None


def _wrn_if_not_remote(remote):
    if not isinstance(remote, Remote):
        log.wrn('Remote', remote, 'is not a Remote instance')


_SCHEMA_PATH = {'manifest': os.path.join(os.path.dirname(__file__),
                                         "manifest-schema.yml"),
                'west': os.path.join(os.path.dirname(__file__),
                                     "_bootstrap",
                                     "west-schema.yml")}
