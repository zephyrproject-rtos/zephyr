# Copyright (c) 2020 The Linux Foundation
#
# SPDX-License-Identifier: Apache-2.0

from datetime import datetime
import hashlib
import os
import re

from west import log

class BuilderDocumentConfig:
    def __init__(self):
        super(BuilderDocumentConfig, self).__init__()

        #####
        ##### Document info
        #####

        # name of document
        self.documentName = ""

        # namespace for this document
        self.documentNamespace = ""

        # external document refs that this document uses
        # list of tuples of external doc refs, in format:
        #    [("DocumentRef-<docID>", "<namespaceURI>", "<hashAlg>", "<hashValue>"), ...]
        self.extRefs = []

        # configs for packages: package root dir => BuilderPackageConfig
        self.packageConfigs = {}

class BuilderPackageConfig:
    def __init__(self):
        super(BuilderPackageConfig, self).__init__()

        #####
        ##### Package-specific config info
        #####

        # name of package
        self.packageName = ""

        # SPDX ID for package, must begin with "SPDXRef-"
        self.spdxID = ""

        # download location for package, defaults to "NOASSERTION"
        self.packageDownloadLocation = "NOASSERTION"

        # should conclude package license based on detected licenses,
        # AND'd together?
        self.shouldConcludeLicense = True

        # declared license, defaults to "NOASSERTION"
        self.declaredLicense = "NOASSERTION"

        # copyright text, defaults to "NOASSERTION"
        self.copyrightText = "NOASSERTION"

        # should include SHA256 hashes? (will also include SHA1 regardless)
        self.doSHA256 = False

        # should include MD5 hashes? (will also include MD5 regardless)
        self.doMD5 = False

        # root directory to be scanned
        self.scandir = ""

        # directories whose files should not be included
        self.excludeDirs = [".git/"]

        # directories whose files should be included, but not scanned
        # FIXME not yet enabled
        self.skipScanDirs = []

        # number of lines to scan for SPDX-License-Identifier (0 = all)
        # defaults to 20
        self.numLinesScanned = 20

class BuilderDocument:
    def __init__(self, docCfg):
        super(BuilderDocument, self).__init__()

        # corresponding document configuration
        self.config = docCfg

        # packages in this document: package root dir => BuilderPackage
        self.packages = {}
        for rootPath, pkgCfg in docCfg.packageConfigs.items():
            self.packages[rootPath] = BuilderPackage(pkgCfg)

class BuilderPackage:
    def __init__(self, pkgCfg):
        super(BuilderPackage, self).__init__()

        self.config = pkgCfg

        self.name = pkgCfg.packageName
        self.spdxID = pkgCfg.spdxID
        self.downloadLocation = pkgCfg.packageDownloadLocation
        self.verificationCode = ""
        self.licenseConcluded = "NOASSERTION"
        self.licenseInfoFromFiles = []
        self.licenseDeclared = pkgCfg.declaredLicense
        self.copyrightText = pkgCfg.copyrightText
        self.files = []

class BuilderFile:
    def __init__(self):
        super(BuilderFile, self).__init__()

        self.name = ""
        self.spdxID = ""
        # FIXME not yet implementing FileType
        self.type = ""
        self.sha1 = ""
        self.sha256 = ""
        self.md5 = ""
        self.licenseConcluded = "NOASSERTION"
        self.licenseInfoInFile = []
        self.copyrightText = "NOASSERTION"

def shouldExcludeFile(filename, excludes):
    """
    Determines whether a file is in an excluded directory.

    Arguments:
        - filename: filename being tested
        - excludes: array of excluded directory names
    Returns: True if should exclude, False if not.
    """
    for exc in excludes:
        if exc in filename:
            return True
    return False

def getAllPaths(topDir, excludes):
    """
    Gathers a list of all paths for all files within topDir or its children.

    Arguments:
        - topDir: root directory of files being collected
        - excludes: array of excluded directory names
    Returns: array of paths
    """
    paths = []
    # ignoring second item in tuple, which lists immediate subdirectories
    for (currentDir, _, filenames) in os.walk(topDir):
        for filename in filenames:
            p = os.path.join(currentDir, filename)
            if not shouldExcludeFile(p, excludes):
                paths.append(p)
    return sorted(paths)

def parseLineForExpression(line):
    """Return parsed SPDX expression if tag found in line, or None otherwise."""
    p = line.partition("SPDX-License-Identifier:")
    if p[2] == "":
        return None
    # strip away trailing comment marks and whitespace, if any
    expression = p[2].strip()
    expression = expression.rstrip("/*")
    expression = expression.strip()
    return expression

def getExpressionData(filePath, numLines):
    """
    Scans the specified file for the first SPDX-License-Identifier:
    tag in the file.

    Arguments:
        - filePath: path to file to scan.
        - numLines: number of lines to scan for an expression before
                    giving up. If 0, will scan the entire file.
    Returns: parsed expression if found; None if not found.
    """
    with open(filePath, "r") as f:
        try:
            lineno = 0
            for line in f:
                lineno += 1
                if lineno > numLines > 0:
                    break
                expression = parseLineForExpression(line)
                if expression is not None:
                    return expression
        except UnicodeDecodeError:
            # invalid UTF-8 content
            return None

    # if we get here, we didn't find an expression
    return None

def splitExpression(expression):
    """
    Parse a license expression into its constituent identifiers.

    Arguments:
        - expression: SPDX license expression
    Returns: array of split identifiers
    """
    # remove parens and plus sign
    e2 = re.sub(r'\(|\)|\+', "", expression, flags=re.IGNORECASE)

    # remove word operators, ignoring case, leaving a blank space
    e3 = re.sub(r' AND | OR | WITH ', " ", e2, flags=re.IGNORECASE)

    # and split on space
    e4 = e3.split(" ")

    return sorted(e4)

def getHashes(filePath):
    """
    Scan for and return hashes.

    Arguments:
        - filePath: path to file to scan.
    Returns: tuple of (SHA1, SHA256, MD5) hashes for filePath.
    """
    hSHA1 = hashlib.sha1()
    hSHA256 = hashlib.sha256()
    hMD5 = hashlib.md5()

    with open(filePath, 'rb') as f:
        buf = f.read()
        hSHA1.update(buf)
        hSHA256.update(buf)
        hMD5.update(buf)

    return (hSHA1.hexdigest(), hSHA256.hexdigest(), hMD5.hexdigest())

def calculateVerificationCode(bfs):
    """
    Calculate the SPDX Package Verification Code for all files in the package.

    Arguments:
        - bfs: array of BuilderFiles
    Returns: verification code as string
    """
    hashes = []
    for bf in bfs:
        hashes.append(bf.sha1)
    hashes.sort()
    filelist = "".join(hashes)

    hSHA1 = hashlib.sha1()
    hSHA1.update(filelist.encode('utf-8'))
    return hSHA1.hexdigest()

def getSPDXIDSafeCharacter(c):
    """
    Converts a character to an SPDX-ID-safe character.

    Arguments:
        - c: character to test
    Returns: c if it is SPDX-ID-safe (letter, number, '-' or '.');
             '-' otherwise
    """
    if c.isalpha() or c.isdigit() or c == "-" or c == ".":
        return c
    return "-"

def convertToSPDXIDSafe(filenameOnly):
    """
    Converts a filename to only SPDX-ID-safe characters.
    Note that a separate check (such as in getUniqueID, below) will need
    to be used to confirm that this is still a unique identifier, after
    conversion.

    Arguments:
        - filenameOnly: filename only (directories omitted) seeking ID.
    Returns: filename with all non-safe characters replaced with dashes.
    """
    return "".join([getSPDXIDSafeCharacter(c) for c in filenameOnly])

def getUniqueID(filenameOnly, timesSeen):
    """
    Find an SPDX ID that is unique among others seen so far.

    Arguments:
        - filenameOnly: filename only (directories omitted) seeking ID.
        - timesSeen: dict of all filename-only to number of times seen.
    Returns: unique SPDX ID; updates timesSeen to include it.
    """

    converted = convertToSPDXIDSafe(filenameOnly)
    spdxID = f"SPDXRef-File-{converted}"

    # determine whether spdxID is unique so far, or not
    filenameTimesSeen = timesSeen.get(converted, 0) + 1
    if filenameTimesSeen > 1:
        # we'll append the # of times seen to the end
        spdxID += f"-{filenameTimesSeen}"
    else:
        # first time seeing this filename
        # edge case: if the filename itself ends in "-{number}", then we
        # need to add a "-1" to it, so that we don't end up overlapping
        # with an appended number from a similarly-named file.
        p = re.compile(r"-\d+$")
        if p.search(converted):
            spdxID += "-1"

    timesSeen[converted] = filenameTimesSeen
    return spdxID

def makeFileData(filePath, pkgCfg, timesSeen):
    """
    Scan for expression, get hashes, and fill in data.

    Arguments:
        - filePath: path to file to scan.
        - pkgCfg: BuilderPackageConfig for this scan.
        - timesSeen: dict of all filename-only (converted to SPDX-ID-safe)
                     to number of times seen.
    Returns: BuilderFile
    """
    bf = BuilderFile()
    bf.name = os.path.join(".", os.path.relpath(filePath, pkgCfg.scandir))

    filenameOnly = os.path.basename(filePath)
    bf.spdxID = getUniqueID(filenameOnly, timesSeen)

    (sha1, sha256, md5) = getHashes(filePath)
    bf.sha1 = sha1
    if pkgCfg.doSHA256:
        bf.sha256 = sha256
    if pkgCfg.doMD5:
        bf.md5 = md5

    expression = getExpressionData(filePath, pkgCfg.numLinesScanned)
    if expression is not None:
        bf.licenseConcluded = expression
        bf.licenseInfoInFile = splitExpression(expression)

    return bf

def makeAllFileData(filePaths, pkgCfg, timesSeen):
    """
    Scan all files for expressions and hashes, and fill in data.

    Arguments:
        - filePaths: sorted array of paths to files to scan.
        - pkgCfg: BuilderPackageConfig for this scan.
        - timesSeen: dict of all filename-only (converted to SPDX-ID-safe)
                     to number of times seen.
    Returns: array of BuilderFiles
    """
    bfs = []
    for filePath in filePaths:
        bf = makeFileData(filePath, pkgCfg, timesSeen)
        bfs.append(bf)

    return bfs

def getPackageLicenses(bfs):
    """
    Extract lists of all concluded and infoInFile licenses seen.

    Arguments:
        - bfs: array of BuilderFiles
    Returns: tuple(sorted list of concluded license exprs,
                   sorted list of infoInFile ID's)
    """
    licsConcluded = set()
    licsFromFiles = set()
    for bf in bfs:
        licsConcluded.add(bf.licenseConcluded)
        for licInfo in bf.licenseInfoInFile:
            licsFromFiles.add(licInfo)
    return (sorted(list(licsConcluded)), sorted(list(licsFromFiles)))

def normalizeExpression(licsConcluded):
    """
    Combine array of license expressions into one AND'd expression,
    adding parens where needed.

    Arguments:
        - licsConcluded: array of license expressions
    Returns: string with single AND'd expression.
    """
    # return appropriate for simple cases
    if len(licsConcluded) == 0:
        return "NOASSERTION"
    if len(licsConcluded) == 1:
        return licsConcluded[0]

    # more than one, so we'll need to combine them
    # iff an expression has spaces, it needs parens
    revised = []
    for lic in licsConcluded:
        if lic in ["NONE", "NOASSERTION"]:
            continue
        if " " in lic:
            revised.append(f"({lic})")
        else:
            revised.append(lic)
    return " AND ".join(revised)

def makePackageData(pkg, timesSeen):
    """
    Create package and call sub-functions to scan and create file data.

    Arguments:
        - pkg: BuilderPackage (already stored in BuilderDocument.packages)
        - timesSeen: dict of all filename-only (converted to SPDX-ID-safe)
                     to number of times seen.
    Returns: None; fills in Package data in-place
    """
    filePaths = getAllPaths(pkg.config.scandir, pkg.config.excludeDirs)
    bfs = makeAllFileData(filePaths, pkg.config, timesSeen)
    (licsConcluded, licsFromFiles) = getPackageLicenses(bfs)

    if pkg.config.shouldConcludeLicense:
        pkg.licenseConcluded = normalizeExpression(licsConcluded)
    pkg.licenseInfoFromFiles = licsFromFiles
    pkg.files = bfs
    pkg.verificationCode = calculateVerificationCode(bfs)

def makeDocument(docCfg):
    """
    Create BuilderDocument (and its sub-Packages) from BuilderDocumentConfig.

    Arguments:
        - cfg: BuilderDocumentConfig
    Returns: BuilderDocument
    """
    doc = BuilderDocument(docCfg)
    # dict of filename-only (converted to SPDX-ID-safe) to number of times seen
    # for use in making unique identifiers
    timesSeen = {}
    for pkg in doc.packages.values():
        makePackageData(pkg, timesSeen)

    return doc

def outputSPDX(doc, spdxPath):
    """
    Write SPDX doc, package and files content to disk.

    Arguments:
        - doc: BuilderDocument
        - spdxPath: path to write SPDX content
    Returns: True on success, False on error.
    """
    try:
        with open(spdxPath, 'w') as f:
            # write document creation info section
            f.write(f"""SPDXVersion: SPDX-2.2
DataLicense: CC0-1.0
SPDXID: SPDXRef-DOCUMENT
DocumentName: {doc.config.documentName}
DocumentNamespace: {doc.config.documentNamespace}
Creator: Tool: cmake-spdx
Created: {datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%SZ")}
""")
            # write any external document references
            for extRef in doc.config.extRefs:
                f.write(f"ExternalDocumentRef: {extRef[0]} {extRef[1]} {extRef[2]}:{extRef[3]}\n")
            f.write(f"\n")

            # write package sections
            for pkg in doc.packages.values():
                f.write(f"""##### Package: {pkg.name}

PackageName: {pkg.name}
SPDXID: {pkg.spdxID}
PackageDownloadLocation: {pkg.downloadLocation}
FilesAnalyzed: true
PackageVerificationCode: {pkg.verificationCode}
PackageLicenseConcluded: {pkg.licenseConcluded}
""")
                for licFromFiles in pkg.licenseInfoFromFiles:
                    f.write(f"PackageLicenseInfoFromFiles: {licFromFiles}\n")
                f.write(f"""PackageLicenseDeclared: {pkg.licenseDeclared}
PackageCopyrightText: NOASSERTION

Relationship: SPDXRef-DOCUMENT DESCRIBES {pkg.spdxID}

""")

                # write file sections
                for bf in pkg.files:
                    f.write(f"""FileName: {bf.name}
SPDXID: {bf.spdxID}
FileChecksum: SHA1: {bf.sha1}
""")
                    if bf.sha256 != "":
                        f.write(f"FileChecksum: SHA256: {bf.sha256}\n")
                    if bf.md5 != "":
                        f.write(f"FileChecksum: MD5: {bf.md5}\n")
                    f.write(f"LicenseConcluded: {bf.licenseConcluded}\n")
                    if len(bf.licenseInfoInFile) == 0:
                        f.write(f"LicenseInfoInFile: NONE\n")
                    else:
                        for licInfoInFile in bf.licenseInfoInFile:
                            f.write(f"LicenseInfoInFile: {licInfoInFile}\n")
                    f.write(f"FileCopyrightText: {bf.copyrightText}\n\n")

            # we're done for now; will do other relationships later
            return True

    except OSError as e:
        log.err(f"Error: Unable to write to {spdxPath}: {str(e)}")
        return False

def makeSPDX(docCfg, spdxPath):
    """
    Scan, create and write SPDX details to disk.

    Arguments:
        - docCfg: BuilderDocumentConfig
        - spdxPath: path to write SPDX content
    Returns: BuilderDocument on success, None on failure.
    """
    doc = makeDocument(docCfg)
    if outputSPDX(doc, spdxPath):
        return doc
    else:
        return None
