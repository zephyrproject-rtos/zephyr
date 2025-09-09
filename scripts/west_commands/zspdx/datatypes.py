# Copyright (c) 2021 The Linux Foundation
#
# SPDX-License-Identifier: Apache-2.0

from dataclasses import dataclass, field
from enum import Enum
from typing import Optional


# DocumentConfig contains settings used to configure how the SPDX Document
# should be built.
@dataclass(eq=True)
class DocumentConfig:
    # name of document
    name: str = ""

    # namespace for this document
    namespace: str = ""

    # standardized DocumentRef- (including that prefix) that the other
    # docs will use to refer to this one
    docRefID: str = ""


# Document contains the data assembled by the SBOM builder, to be used to
# create the actual SPDX Document.
class Document:
    # initialize with a DocumentConfig
    def __init__(self, cfg):
        super().__init__()

        # configuration - DocumentConfig
        self.cfg = cfg

        # dict of SPDX ID => Package
        self.pkgs = {}

        # relationships "owned" by this Document, _not_ those "owned" by its
        # Packages or Files; will likely be just DESCRIBES
        self.relationships = []

        # dict of filename (ignoring its directory) => number of times it has
        # been seen while adding files to this Document; used to calculate
        # useful SPDX IDs
        self.timesSeen = {}

        # dict of absolute path on disk => File
        self.fileLinks = {}

        # set of other Documents that our elements' Relationships refer to
        self.externalDocuments = set()

        # set of LicenseRef- custom licenses to be declared
        # may or may not include "LicenseRef-" license prefix
        self.customLicenseIDs = set()

        # this Document's SHA1 hash, filled in _after_ the Document has been
        # written to disk, so that others can refer to it
        self.myDocSHA1 = ""


# PackageConfig contains settings used to configure how an SPDX Package should
# be built.
@dataclass(eq=True)
class PackageConfig:
    # package name
    name: str = ""

    # SPDX ID, including "SPDXRef-"
    spdxID: str = ""

    # primary package purpose (ex. "LIBRARY", "APPLICATION", etc.)
    primaryPurpose: str = ""

    # package URL
    url: str = ""

    # package version
    version: str = ""

    # package revision
    revision: str = ""

    # package external references
    externalReferences: list = field(default_factory=list)

    # the Package's declared license
    declaredLicense: str = "NOASSERTION"

    # the Package's copyright text
    copyrightText: str = "NOASSERTION"

    # absolute path of the "root" directory on disk, to be used as the
    # base directory from which this Package's Files will calculate their
    # relative paths
    # may want to note this in a Package comment field
    relativeBaseDir: str = ""


# Package contains the data assembled by the SBOM builder, to be used to
# create the actual SPDX Package.
class Package:
    # initialize with:
    # 1) PackageConfig
    # 2) the Document that owns this Package
    def __init__(self, cfg, doc):
        super().__init__()

        # configuration - PackageConfig
        self.cfg = cfg

        # Document that owns this Package
        self.doc = doc

        # verification code, calculated per section 7.9 of SPDX spec v2.3
        self.verificationCode = ""

        # concluded license for this Package, if
        # cfg.shouldConcludePackageLicense == True; NOASSERTION otherwise
        self.concludedLicense = "NOASSERTION"

        # list of licenses found in this Package's Files
        self.licenseInfoFromFiles = []

        # Files in this Package
        # dict of SPDX ID => File
        self.files = {}

        # Relationships "owned" by this Package (e.g., this Package is left
        # side)
        self.rlns = []

        # If this Package was a target, which File was its main build product?
        self.targetBuildFile = None


# RelationshipDataElementType defines whether a RelationshipData element
# (e.g., the "owner" or the "other" element) is a File, a target Package,
# a Package's ID (as other only, and only where owner type is DOCUMENT),
# or the SPDX document itself (as owner only).
class RelationshipDataElementType(Enum):
    UNKNOWN = 0
    FILENAME = 1
    TARGETNAME = 2
    PACKAGEID = 3
    DOCUMENT = 4


# RelationshipData contains the pre-analysis data about a relationship between
# Files and/or Packages/targets. It is eventually parsed into a corresponding
# Relationship after we have organized the SPDX Package and File data.
@dataclass(eq=True)
class RelationshipData:
    # for the "owner" element (e.g., the left side of the Relationship),
    # is it a filename or a target name (e.g., a Package in the build doc)
    ownerType: RelationshipDataElementType = RelationshipDataElementType.UNKNOWN

    # owner file absolute path (if ownerType is FILENAME)
    ownerFileAbspath: str = ""

    # owner target name (if ownerType is TARGETNAME)
    ownerTargetName: str = ""

    # owner SPDX Document (if ownerType is DOCUMENT)
    ownerDocument: Optional['Document'] = None

    # for the "other" element (e.g., the right side of the Relationship),
    # is it a filename or a target name (e.g., a Package in the build doc)
    otherType: RelationshipDataElementType = RelationshipDataElementType.UNKNOWN

    # other file absolute path (if otherType is FILENAME)
    otherFileAbspath: str = ""

    # other target name (if otherType is TARGETNAME)
    otherTargetName: str = ""

    # other package ID (if ownerType is DOCUMENT and otherType is PACKAGEID)
    otherPackageID: str = ""

    # text string with Relationship type
    # from table 68 in section 11.1 of SPDX spec v2.3
    rlnType: str = ""


# Relationship contains the post-analysis, processed data about a relationship
# in a form suitable for creating the actual SPDX Relationship in a particular
# Document's context.
@dataclass(eq=True)
class Relationship:
    # SPDX ID for left side of relationship
    # including "SPDXRef-" as well as "DocumentRef-" if needed
    refA: str = ""

    # SPDX ID for right side of relationship
    # including "SPDXRef-" as well as "DocumentRef-" if needed
    refB: str = ""

    # text string with Relationship type
    # from table 68 in section 11.1 of SPDX spec v2.3
    rlnType: str = ""


# File contains the data needed to create a File element in the context of a
# particular SPDX Document and Package.
class File:
    # initialize with:
    # 1) Document containing this File
    # 2) Package containing this File
    def __init__(self, doc, pkg):
        super().__init__()

        # absolute path to this file on disk
        self.abspath = ""

        # relative path for this file, measured from the owning Package's
        # cfg.relativeBaseDir
        self.relpath = ""

        # SPDX ID for this file, including "SPDXRef-"
        self.spdxID = ""

        # SHA1 hash
        self.sha1 = ""

        # SHA256 hash, if pkg.cfg.doSHA256 == True; empty string otherwise
        self.sha256 = ""

        # MD5 hash, if pkg.cfg.doMD5 == True; empty string otherwise
        self.md5 = ""

        # concluded license, if pkg.cfg.shouldConcludeFileLicenses == True;
        # "NOASSERTION" otherwise
        self.concludedLicense = "NOASSERTION"

        # license info in file
        self.licenseInfoInFile = []

        # copyright text
        self.copyrightText = "NOASSERTION"

        # Relationships "owned" by this File (e.g., this File is left side)
        self.rlns = []

        # Package that owns this File
        self.pkg = pkg

        # Document that owns this File
        self.doc = doc
