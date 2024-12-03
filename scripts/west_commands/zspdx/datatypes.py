# Copyright (c) 2021 The Linux Foundation
#
# SPDX-License-Identifier: Apache-2.0

from enum import Enum


# DocumentConfig contains settings used to configure how the SPDX Document
# should be built.
class DocumentConfig:
    def __init__(self):
        super(DocumentConfig, self).__init__()

        # name of document
        self.name = ""

        # namespace for this document
        self.namespace = ""

        # standardized DocumentRef- (including that prefix) that the other
        # docs will use to refer to this one
        self.docRefID = ""


# Document contains the data assembled by the SBOM builder, to be used to
# create the actual SPDX Document.
class Document:
    # initialize with a DocumentConfig
    def __init__(self, cfg):
        super(Document, self).__init__()

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
class PackageConfig:
    def __init__(self):
        super(PackageConfig, self).__init__()

        # package name
        self.name = ""

        # SPDX ID, including "SPDXRef-"
        self.spdxID = ""

        # primary package purpose (ex. "LIBRARY", "APPLICATION", etc.)
        self.primaryPurpose = ""

        # package URL
        self.url = ""

        # package version
        self.version = ""

        # package revision
        self.revision = ""

        # package external references
        self.externalReferences = []

        # the Package's declared license
        self.declaredLicense = "NOASSERTION"

        # the Package's copyright text
        self.copyrightText = "NOASSERTION"

        # absolute path of the "root" directory on disk, to be used as the
        # base directory from which this Package's Files will calculate their
        # relative paths
        # may want to note this in a Package comment field
        self.relativeBaseDir = ""


# Package contains the data assembled by the SBOM builder, to be used to
# create the actual SPDX Package.
class Package:
    # initialize with:
    # 1) PackageConfig
    # 2) the Document that owns this Package
    def __init__(self, cfg, doc):
        super(Package, self).__init__()

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
class RelationshipData:
    def __init__(self):
        super(RelationshipData, self).__init__()

        # for the "owner" element (e.g., the left side of the Relationship),
        # is it a filename or a target name (e.g., a Package in the build doc)
        self.ownerType = RelationshipDataElementType.UNKNOWN

        # owner file absolute path (if ownerType is FILENAME)
        self.ownerFileAbspath = ""

        # owner target name (if ownerType is TARGETNAME)
        self.ownerTargetName = ""

        # owner SPDX Document (if ownerType is DOCUMENT)
        self.ownerDocument = None

        # for the "other" element (e.g., the right side of the Relationship),
        # is it a filename or a target name (e.g., a Package in the build doc)
        self.otherType = RelationshipDataElementType.UNKNOWN

        # other file absolute path (if otherType is FILENAME)
        self.otherFileAbspath = ""

        # other target name (if otherType is TARGETNAME)
        self.otherTargetName = ""

        # other package ID (if ownerType is DOCUMENT and otherType is PACKAGEID)
        self.otherPackageID = ""

        # text string with Relationship type
        # from table 68 in section 11.1 of SPDX spec v2.3
        self.rlnType = ""


# Relationship contains the post-analysis, processed data about a relationship
# in a form suitable for creating the actual SPDX Relationship in a particular
# Document's context.
class Relationship:
    def __init__(self):
        super(Relationship, self).__init__()

        # SPDX ID for left side of relationship
        # including "SPDXRef-" as well as "DocumentRef-" if needed
        self.refA = ""

        # SPDX ID for right side of relationship
        # including "SPDXRef-" as well as "DocumentRef-" if needed
        self.refB = ""

        # text string with Relationship type
        # from table 68 in section 11.1 of SPDX spec v2.3
        self.rlnType = ""


# File contains the data needed to create a File element in the context of a
# particular SPDX Document and Package.
class File:
    # initialize with:
    # 1) Document containing this File
    # 2) Package containing this File
    def __init__(self, doc, pkg):
        super(File, self).__init__()

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
