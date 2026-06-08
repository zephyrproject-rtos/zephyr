# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0

import hashlib
import logging
import os
from datetime import UTC, datetime

from zspdx.model import (
    ComponentPurpose,
    ExternalReferenceType,
    SBOMComponent,
    SBOMDocument,
    SBOMElement,
    SBOMFile,
)
from zspdx.serializers.helpers import (
    generate_download_url,
    get_standard_licenses,
    normalize_spdx_name,
)
from zspdx.util import getHashes
from zspdx.version import SPDX_VERSION_2_3

_logger = logging.getLogger(__name__)


class SPDX2Serializer:
    """Serializer that converts SBOMGraph to SPDX 2.x tag-value format."""

    def __init__(self, sbom_graph, spdx_version=SPDX_VERSION_2_3):
        self.sbom_graph = sbom_graph
        self.spdx_version = spdx_version

        # Track generated IDs
        self.component_ids = {}  # component_name -> SPDX ID
        self.file_ids = {}  # file_path -> SPDX ID
        self.document_refs = {}  # document_name -> DocumentRef ID
        self.document_hashes = {}  # document_name -> SHA1 hash
        self.document_created_timestamps = {}  # document_name -> SPDX Created timestamp

    def serialize(self, output_dir):
        """Serialize SBOMGraph to SPDX 2.x format files."""
        # Generate IDs for all components and files
        self._generate_ids()

        # First pass: write all documents to calculate hashes
        written_docs = {}
        for doc in self.sbom_graph.documents.values():
            if not doc.components:
                continue

            output_path = os.path.join(output_dir, f"{doc.name}.spdx")
            if self._write_document_first_pass(doc, output_path):
                written_docs[doc.name] = output_path
            else:
                _logger.error(f"Failed to write document {doc.name}")
                return False

        # Second pass: rewrite documents with external document references
        for doc in self.sbom_graph.documents.values():
            if not doc.components or doc.name not in written_docs:
                continue

            output_path = written_docs[doc.name]
            if not self._write_document_second_pass(doc, output_path):
                _logger.error(f"Failed to rewrite document {doc.name} with external references")
                return False

        return True

    def _generate_ids(self):
        """Generate SPDX IDs for all components and files."""
        # Generate component IDs
        for component in self.sbom_graph.components.values():
            spdx_id = f"SPDXRef-{normalize_spdx_name(component.name)}"
            self.component_ids[component.name] = spdx_id

        # Generate file IDs with a global collision check.
        # Track every assigned ID and keep incrementing the suffix until an unused one is found,
        # regardless of the order files are processed.
        used_ids = set(self.component_ids.values())
        for file_path in self.sbom_graph.files:
            safe_name = normalize_spdx_name(os.path.basename(file_path))
            spdx_id = f"SPDXRef-File-{safe_name}"

            count = 1
            while spdx_id in used_ids:
                count += 1
                spdx_id = f"SPDXRef-File-{safe_name}-{count}"

            used_ids.add(spdx_id)
            self.file_ids[file_path] = spdx_id

        # Generate document reference IDs
        for doc_name in self.sbom_graph.documents:
            self.document_refs[doc_name] = f"DocumentRef-{doc_name}"

    def _write_document_first_pass(self, doc: SBOMDocument, output_path):
        """Write a single SPDX 2.x document (first pass, without external refs)."""
        try:
            _logger.info(f"Writing SPDX {self.spdx_version} document {doc.name} to {output_path}")
            with open(output_path, "w") as f:
                self._write_document_header(f, doc)
                # Skip external document refs in first pass
                self._write_document_relationships(f, doc)
                self._write_packages(f, doc)
                self._write_custom_licenses(f, doc)

            # Calculate document hash and store it locally for external document refs
            hashes = getHashes(output_path)
            if not hashes:
                _logger.error(
                    f"Error: created document but unable to calculate hash values for {output_path}"
                )
                return False
            self.document_hashes[doc.name] = hashes[0]

            return True
        except OSError:
            _logger.exception(f"Error: Unable to write to {output_path}")
            return False

    def _write_document_second_pass(self, doc: SBOMDocument, output_path):
        """Rewrite document with external document references (second pass)."""
        try:
            # Write the updated document
            with open(output_path, "w") as f:
                # Write header
                self._write_document_header(f, doc)
                # Write external document refs (now we have all hashes)
                self._write_external_document_refs(f, doc)
                # Write the rest (relationships, packages, licenses)
                self._write_document_relationships(f, doc)
                self._write_packages(f, doc)
                self._write_custom_licenses(f, doc)

            # Recalculate hash after adding external refs
            hashes = getHashes(output_path)
            if not hashes:
                _logger.error(f"Error: unable to recalculate hash for {output_path}")
                return False
            self.document_hashes[doc.name] = hashes[0]

            return True
        except OSError:
            _logger.exception(f"Error: Unable to rewrite {output_path}")
            return False

    def _write_document_header(self, f, doc: SBOMDocument):
        """Write SPDX document header."""
        # Use document's namespace if set, otherwise generate from prefix
        namespace = doc.namespace or f"{self.sbom_graph.namespace_prefix}/{doc.name}"
        normalized_name = normalize_spdx_name(doc.title or doc.name)
        created = self.document_created_timestamps.get(doc.name)
        if created is None:
            created = datetime.now(UTC).strftime("%Y-%m-%dT%H:%M:%SZ")
            self.document_created_timestamps[doc.name] = created

        f.write(f"""SPDXVersion: SPDX-{self.spdx_version}
DataLicense: CC0-1.0
SPDXID: SPDXRef-DOCUMENT
DocumentName: {normalized_name}
DocumentNamespace: {namespace}
Creator: Tool: Zephyr SPDX builder
Created: {created}

""")

    def _write_external_document_refs(self, f, doc: SBOMDocument):
        """Write external document references."""
        ext_refs = []
        # Use document's external_documents which were populated during walking
        for ext_doc in doc.external_documents.values():
            doc_hash = self.document_hashes.get(ext_doc.name)
            if doc_hash:
                ext_refs.append(
                    (
                        ext_doc,
                        self.document_refs.get(ext_doc.name, f"DocumentRef-{ext_doc.name}"),
                        doc_hash,
                    )
                )

        if ext_refs:
            ext_refs.sort(key=lambda x: x[1])  # Sort by DocumentRef ID
            for ext_doc, doc_ref_id, doc_hash in ext_refs:
                namespace = (
                    ext_doc.namespace or f"{self.sbom_graph.namespace_prefix}/{ext_doc.name}"
                )
                f.write(f"ExternalDocumentRef: {doc_ref_id} {namespace} SHA1: {doc_hash}\n")
            f.write("\n")

    def _write_document_relationships(self, f, doc: SBOMDocument):
        """Write document-level relationships (DESCRIBES) for the document's primary subjects."""
        wrote_any = False
        for component_name in doc.described_components:
            if component_name not in doc.components:
                continue
            component_id = self.component_ids.get(component_name)
            if not component_id:
                continue
            f.write(f"Relationship: SPDXRef-DOCUMENT DESCRIBES {component_id}\n")
            wrote_any = True
        if wrote_any:
            f.write("\n")

    def _write_packages(self, f, doc: SBOMDocument):
        """Write all packages in this document."""
        for component in doc.components.values():
            self._write_package(f, component, doc)

    def _resolve_package_id(self, component):
        """Get the SPDX ID for a component, generating one as a fallback if missing."""
        spdx_id = self.component_ids.get(component.name)
        if spdx_id:
            return spdx_id

        _logger.error(f"Component {component.name} not found in component_ids")
        spdx_id = f"SPDXRef-{normalize_spdx_name(component.name)}"
        self.component_ids[component.name] = spdx_id
        return spdx_id

    def _resolve_cpe_metadata(self, component):
        """Derive (name, supplier, version), filling gaps from a CPE 2.3 reference.

        Returns local copies so component.name is left untouched (it drives ID lookup).
        """
        package_name = component.name
        supplier = component.supplier
        package_version = component.version
        for ref in component.external_references:
            if ref.reference_type != ExternalReferenceType.CPE23:
                continue
            # metadata: [cpe, 2.3, a, arm, mbed_tls, 3.5.1, *:*:*:*:*:*]
            metadata = ref.locator.split(':', 6)
            if len(metadata) > 5:
                supplier = supplier or metadata[3]
                package_name = metadata[4]
                package_version = package_version or metadata[5]
        return package_name, supplier, package_version

    def _write_files_analyzed(self, f, component):
        """Write the FilesAnalyzed section and verification code for a component."""
        if not component.files:
            f.write("FilesAnalyzed: false\nPackageComment: Utility target; no files\n\n")
            return

        if component.license_info_from_files:
            for lic in component.license_info_from_files:
                f.write(f"PackageLicenseInfoFromFiles: {lic}\n")
        else:
            f.write("PackageLicenseInfoFromFiles: NOASSERTION\n")
        f.write(
            f"FilesAnalyzed: true\n"
            f"PackageVerificationCode: {self._verification_code(component)}\n\n"
        )

    def _write_package(self, f, component, doc: SBOMDocument):
        """Write a single package."""
        spdx_id = self._resolve_package_id(component)
        package_name, supplier, package_version = self._resolve_cpe_metadata(component)
        normalized_name = normalize_spdx_name(package_name)

        f.write(f"""##### Package: {normalized_name}

PackageName: {package_name}
SPDXID: {spdx_id}
PackageLicenseConcluded: {component.concluded_license}
PackageLicenseDeclared: {component.declared_license}
PackageCopyrightText: {component.copyright_text}
""")

        # PrimaryPackagePurpose is only available in SPDX 2.3 and later
        if self.spdx_version >= SPDX_VERSION_2_3:
            purpose_str = self._purpose_to_spdx_string(component.purpose)
            if purpose_str:
                f.write(f"PrimaryPackagePurpose: {purpose_str}\n")

        # Download location
        if component.url:
            download_url = generate_download_url(component.url, component.revision)
            f.write(f"PackageDownloadLocation: {download_url}\n")
        else:
            f.write("PackageDownloadLocation: NOASSERTION\n")

        # Version
        if package_version:
            f.write(f"PackageVersion: {package_version}\n")
        elif component.revision:
            f.write(f"PackageVersion: {component.revision}\n")

        # Supplier
        if supplier:
            f.write(f"PackageSupplier: Organization: {supplier}\n")

        # External references
        for ref in component.external_references:
            if ref.reference_type == ExternalReferenceType.CPE23:
                f.write(f"ExternalRef: SECURITY cpe23Type {ref.locator}\n")
            elif ref.reference_type == ExternalReferenceType.PURL:
                f.write(f"ExternalRef: PACKAGE-MANAGER purl {ref.locator}\n")
            else:
                _logger.warning(f"Unknown external reference ({ref.locator})")

        # Files analyzed and verification code
        self._write_files_analyzed(f, component)

        # Package relationships
        for rel in component.relationships:
            self._write_relationship(f, rel, doc)

        # Package files
        if component.files:
            files_list = sorted(component.files.values(), key=lambda x: x.relative_path)
            for file_obj in files_list:
                self._write_file(f, file_obj, doc)

    def _write_file(self, f, file_obj, doc: SBOMDocument):
        """Write a single file."""
        spdx_id = self.file_ids[file_obj.path]

        f.write(f"""FileName: ./{file_obj.relative_path}
SPDXID: {spdx_id}
FileChecksum: SHA1: {file_obj.hashes.get('SHA1', '')}
""")

        if 'SHA256' in file_obj.hashes and file_obj.hashes['SHA256']:
            f.write(f"FileChecksum: SHA256: {file_obj.hashes['SHA256']}\n")
        if 'MD5' in file_obj.hashes and file_obj.hashes['MD5']:
            f.write(f"FileChecksum: MD5: {file_obj.hashes['MD5']}\n")

        f.write(f"LicenseConcluded: {file_obj.concluded_license}\n")

        if not file_obj.license_info_in_file:
            f.write("LicenseInfoInFile: NONE\n")
        else:
            for lic in file_obj.license_info_in_file:
                f.write(f"LicenseInfoInFile: {lic}\n")

        f.write(f"FileCopyrightText: {file_obj.copyright_text}\n\n")

        # File relationships
        for rel in file_obj.relationships:
            self._write_relationship(f, rel, doc)

    def _write_relationship(self, f, rel, current_doc: SBOMDocument):
        """Write a relationship."""
        from_id = self._get_id_for_element(rel.from_element)
        if not from_id:
            return

        # Get to ID(s)
        for to_elem in rel.to_elements:
            to_id = self._get_id_for_element(to_elem)
            doc_ref = None
            to_doc = self.sbom_graph.get_document_for_element(to_elem)
            if to_doc and to_doc.name != current_doc.name:
                doc_ref = self.document_refs.get(to_doc.name, "")

            if to_id:
                if doc_ref:
                    to_id = f"{doc_ref}:{to_id}"
                f.write(f"Relationship: {from_id} {rel.relationship_type} {to_id}\n")

    def _get_id_for_element(self, element: SBOMElement) -> str:
        """Get the SPDX ID generated for an SBOM element."""
        if isinstance(element, SBOMComponent):
            return self.component_ids.get(element.name, "")
        if isinstance(element, SBOMFile):
            return self.file_ids.get(element.path, "")
        return ""

    def _write_custom_licenses(self, f, doc: SBOMDocument):
        """Write custom license declarations."""
        # Get custom licenses from components in this document
        custom_licenses = set()
        standard_licenses = get_standard_licenses()
        for component in doc.components.values():
            for file_obj in component.files.values():
                for lic in file_obj.license_info_in_file:
                    if lic not in standard_licenses:
                        custom_licenses.add(lic)

        # Also include any custom licenses stored in the document
        custom_licenses.update(doc.custom_license_ids)

        if custom_licenses:
            for lic in sorted(custom_licenses):
                # REUSE-IgnoreStart
                f.write(f"""LicenseID: {lic}
ExtractedText: {lic}
LicenseName: {lic}
LicenseComment: Corresponds to the license ID `{lic}` detected in an SPDX-License-Identifier: tag.
""")
                # REUSE-IgnoreEnd

    def _purpose_to_spdx_string(self, purpose):
        """Convert ComponentPurpose enum to SPDX 2.x string."""
        if isinstance(purpose, ComponentPurpose):
            return purpose
        return ""

    def _verification_code(self, component):
        """Calculate an SPDX package verification code for a component."""
        hashes = []
        for file_obj in component.files.values():
            if "SHA1" in file_obj.hashes:
                hashes.append(file_obj.hashes["SHA1"])
        hashes.sort()
        file_list = "".join(hashes)

        sha1 = hashlib.sha1(usedforsecurity=False)
        sha1.update(file_list.encode('utf-8'))
        return sha1.hexdigest()
