# Copyright (c) 2020, 2021 The Linux Foundation
#
# SPDX-License-Identifier: Apache-2.0

from datetime import datetime

from west import log

from zspdx.util import getHashes

# Output tag-value SPDX 2.2 content for the given Relationship object.
# Arguments:
#   1) f: file handle for SPDX document
#   2) rln: Relationship object being described
def writeRelationshipSPDX(f, rln):
    f.write(f"Relationship: {rln.refA} {rln.rlnType} {rln.refB}\n")

# Output tag-value SPDX 2.2 content for the given File object.
# Arguments:
#   1) f: file handle for SPDX document
#   2) bf: File object being described
def writeFileSPDX(f, bf):
    f.write(f"""FileName: ./{bf.relpath}
SPDXID: {bf.spdxID}
FileChecksum: SHA1: {bf.sha1}
""")
    if bf.sha256 != "":
        f.write(f"FileChecksum: SHA256: {bf.sha256}\n")
    if bf.md5 != "":
        f.write(f"FileChecksum: MD5: {bf.md5}\n")
    f.write(f"LicenseConcluded: {bf.concludedLicense}\n")
    if len(bf.licenseInfoInFile) == 0:
        f.write(f"LicenseInfoInFile: NONE\n")
    else:
        for licInfoInFile in bf.licenseInfoInFile:
            f.write(f"LicenseInfoInFile: {licInfoInFile}\n")
    f.write(f"FileCopyrightText: {bf.copyrightText}\n\n")

    # write file relationships
    if len(bf.rlns) > 0:
        for rln in bf.rlns:
            writeRelationshipSPDX(f, rln)
        f.write("\n")

# Output tag-value SPDX 2.2 content for the given Package object.
# Arguments:
#   1) f: file handle for SPDX document
#   2) pkg: Package object being described
def writePackageSPDX(f, pkg):
    f.write(f"""##### Package: {pkg.cfg.name}

PackageName: {pkg.cfg.name}
SPDXID: {pkg.cfg.spdxID}
PackageDownloadLocation: NOASSERTION
PackageLicenseConcluded: {pkg.concludedLicense}
""")
    f.write(f"""PackageLicenseDeclared: {pkg.cfg.declaredLicense}
PackageCopyrightText: {pkg.cfg.copyrightText}
""")

    # flag whether files analyzed / any files present
    if len(pkg.files) > 0:
        if len(pkg.licenseInfoFromFiles) > 0:
            for licFromFiles in pkg.licenseInfoFromFiles:
                f.write(f"PackageLicenseInfoFromFiles: {licFromFiles}\n")
        else:
            f.write(f"PackageLicenseInfoFromFiles: NOASSERTION\n")
        f.write(f"FilesAnalyzed: true\nPackageVerificationCode: {pkg.verificationCode}\n\n")
    else:
        f.write(f"FilesAnalyzed: false\nPackageComment: Utility target; no files\n\n")

    # write package relationships
    if len(pkg.rlns) > 0:
        for rln in pkg.rlns:
            writeRelationshipSPDX(f, rln)
        f.write("\n")

    # write package files, if any
    if len(pkg.files) > 0:
        bfs = list(pkg.files.values())
        bfs.sort(key = lambda x: x.relpath)
        for bf in bfs:
            writeFileSPDX(f, bf)

# Output tag-value SPDX 2.2 content for a custom license.
# Arguments:
#   1) f: file handle for SPDX document
#   2) lic: custom license ID being described
def writeOtherLicenseSPDX(f, lic):
    f.write(f"""LicenseID: {lic}
ExtractedText: {lic}
LicenseName: {lic}
LicenseComment: Corresponds to the license ID `{lic}` detected in an SPDX-License-Identifier: tag.
""")

# Output tag-value SPDX 2.2 content for the given Document object.
# Arguments:
#   1) f: file handle for SPDX document
#   2) doc: Document object being described
def writeDocumentSPDX(f, doc):
    f.write(f"""SPDXVersion: SPDX-2.2
DataLicense: CC0-1.0
SPDXID: SPDXRef-DOCUMENT
DocumentName: {doc.cfg.name}
DocumentNamespace: {doc.cfg.namespace}
Creator: Tool: Zephyr SPDX builder
Created: {datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%SZ")}

""")

    # write any external document references
    if len(doc.externalDocuments) > 0:
        extDocs = list(doc.externalDocuments)
        extDocs.sort(key = lambda x: x.cfg.docRefID)
        for extDoc in extDocs:
            f.write(f"ExternalDocumentRef: {extDoc.cfg.docRefID} {extDoc.cfg.namespace} SHA1: {extDoc.myDocSHA1}\n")
        f.write(f"\n")

    # write relationships owned by this Document (not by its Packages, etc.), if any
    if len(doc.relationships) > 0:
        for rln in doc.relationships:
            writeRelationshipSPDX(f, rln)
        f.write(f"\n")

    # write packages
    for pkg in doc.pkgs.values():
        writePackageSPDX(f, pkg)

    # write other license info, if any
    if len(doc.customLicenseIDs) > 0:
        for lic in sorted(list(doc.customLicenseIDs)):
            writeOtherLicenseSPDX(f, lic)

# Open SPDX document file for writing, write the document, and calculate
# its hash for other referring documents to use.
# Arguments:
#   1) spdxPath: path to write SPDX document
#   2) doc: SPDX Document object to write
def writeSPDX(spdxPath, doc):
    # create and write document to disk
    try:
        log.inf(f"Writing SPDX document {doc.cfg.name} to {spdxPath}")
        with open(spdxPath, "w") as f:
            writeDocumentSPDX(f, doc)
    except OSError as e:
        log.err(f"Error: Unable to write to {spdxPath}: {str(e)}")
        return False

    # calculate hash of the document we just wrote
    hashes = getHashes(spdxPath)
    if not hashes:
        log.err(f"Error: created document but unable to calculate hash values")
        return False
    doc.myDocSHA1 = hashes[0]

    return True
