#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Texas Instruments Incorporated

"""Script to add x509 certificate TI ROM bootable images."""

import argparse
import os
import subprocess
import sys
from abc import ABC, abstractmethod
from re import sub
from typing import ClassVar


class SHA:
    """SHA algorithm constants and utilities."""

    SHA256 = "sha256"
    SHA384 = "sha384"
    SHA512 = "sha512"
    SHA224 = "sha224"

    OIDS: ClassVar[dict[str, str]] = {
        SHA256: "2.16.840.1.101.3.4.2.1",
        SHA384: "2.16.840.1.101.3.4.2.2",
        SHA512: "2.16.840.1.101.3.4.2.3",
        SHA224: "2.16.840.1.101.3.4.2.4",
    }

    @staticmethod
    def calculate(file_path: str, sha_type: str = SHA512) -> str:
        """Calculate SHA hash of a file."""
        # Using shell=False for security - pass command as list
        sha_val = subprocess.check_output(
            ["openssl", "dgst", f"-{sha_type}", "-hex", file_path],
            stderr=subprocess.STDOUT,
        ).decode()
        return sub(r"^.*= ", "", sha_val).strip()


class DebugType:
    """Debug type constants."""

    DBG_PERM_DISABLE = "0"
    DBG_SOC_DEFAULT = "1"
    DBG_PUBLIC_ENABLE = "2"
    DBG_FULL_ENABLE = "4"

    VALID_TYPES: ClassVar[dict[str, str]] = {
        "DBG_PERM_DISABLE": DBG_PERM_DISABLE,
        "DBG_SOC_DEFAULT": DBG_SOC_DEFAULT,
        "DBG_PUBLIC_ENABLE": DBG_PUBLIC_ENABLE,
        "DBG_FULL_ENABLE": DBG_FULL_ENABLE,
    }


class X509Component(ABC):
    """Base class for X509 certificate components."""

    # Component type constants
    COMP_TYPE_SPL = 1
    COMP_TYPE_SYSFW = 2
    COMP_TYPE_INNER_CERT = 3
    COMP_TYPE_APPLICATION = 17  # Also used for spldata in split architecture
    COMP_TYPE_BOARDCFG = 18

    def __init__(
        self,
        comp_type: int,
        boot_core: int,
        binary_path: str,
        load_addr: str,
        comp_opts: int = 0,
        section_name: str | None = None,
    ) -> None:
        """Initialize X509 component.

        Args:
            comp_type: Component type identifier
            boot_core: Boot core ID
            binary_path: Path to binary file
            load_addr: Load address (hex string with 0x prefix)
            comp_opts: Component options
            section_name: Certificate section name (derived from class name if None)
        """
        self.comp_type = comp_type
        self.boot_core = boot_core
        self.binary_path = binary_path
        self.load_addr = load_addr
        self.comp_opts = comp_opts
        self.section_name = (
            section_name
            or self.__class__.__name__.replace(
                "Component",
                "",
            ).lower()
        )

    def validate(self) -> None:
        """Validate that the binary file exists."""
        if not os.path.exists(self.binary_path):
            msg = f"Binary file not found: {self.binary_path}"
            raise FileNotFoundError(msg)

    def get_size(self) -> int:
        """Get size of the binary file."""
        return os.path.getsize(self.binary_path)

    def get_sha(self, sha_type: str = SHA.SHA512) -> str:
        """Calculate SHA hash of the binary."""
        return SHA.calculate(self.binary_path, sha_type)

    def get_binary_data(self) -> bytes:
        """Read binary data."""
        with open(self.binary_path, "rb") as f:
            return f.read()

    @abstractmethod
    def get_cert_extension_ref(self) -> str:
        """Return the extension reference line for ext_boot_info."""

    def get_cert_section(self, sha_type: str = SHA.SHA512) -> str:
        """Generate the certificate section."""
        return f"""
[ {self.section_name} ]
compType = INTEGER:{self.comp_type}
bootCore = INTEGER:{self.boot_core}
compOpts = INTEGER:{self.comp_opts}
destAddr = FORMAT:HEX,OCT:{int(self.load_addr, 16):08X}
compSize = INTEGER:{self.get_size()}
shaType  = OID:{SHA.OIDS[sha_type]}
shaValue = FORMAT:HEX,OCT:{self.get_sha(sha_type)}
"""


class SPLComponent(X509Component):
    """SPL/ATF component (compType=1, bootCore=16)."""

    def __init__(
        self,
        binary_path: str,
        load_addr: str,
        dual_stage_boot: bool = False,
    ) -> None:
        """Initialize SPL component.

        Args:
            binary_path: Path to SPL/ATF binary
            load_addr: Load address
            dual_stage_boot: Enable dual stage boot (sets comp_opts=160)
        """
        comp_opts = 160 if dual_stage_boot else 0
        super().__init__(
            comp_type=self.COMP_TYPE_SPL,
            boot_core=16,
            binary_path=binary_path,
            load_addr=load_addr,
            comp_opts=comp_opts,
            section_name="spl",
        )

    def get_cert_extension_ref(self) -> str:
        """Return extension reference."""
        return "spl = SEQUENCE:spl"


class SYSFWComponent(X509Component):
    """SYSFW component (compType=2, bootCore=0)."""

    def __init__(self, binary_path: str, load_addr: str) -> None:
        """Initialize SYSFW component."""
        super().__init__(
            comp_type=self.COMP_TYPE_SYSFW,
            boot_core=0,
            binary_path=binary_path,
            load_addr=load_addr,
            section_name="sysfw",
        )

    def get_cert_extension_ref(self) -> str:
        """Return extension reference."""
        return "fw = SEQUENCE:sysfw"


class BoardConfigComponent(X509Component):
    """Board Config component (compType=18, bootCore=0)."""

    def __init__(self, binary_path: str, load_addr: str) -> None:
        """Initialize BoardConfig component."""
        super().__init__(
            comp_type=self.COMP_TYPE_BOARDCFG,
            boot_core=0,
            binary_path=binary_path,
            load_addr=load_addr,
            section_name="boardcfg",
        )

    def get_cert_extension_ref(self) -> str:
        """Return extension reference."""
        return "bd2 = SEQUENCE:boardcfg"


class InnerCertComponent(X509Component):
    """SYSFW Inner Certificate component (compType=3, bootCore=0)."""

    def __init__(self, binary_path: str) -> None:
        """Initialize InnerCert component."""
        super().__init__(
            comp_type=self.COMP_TYPE_INNER_CERT,
            boot_core=0,
            binary_path=binary_path,
            load_addr="0x00000000",
            section_name="sysfw_inner_cert",
        )

    def get_cert_extension_ref(self) -> str:
        """Return extension reference."""
        return "bd1 = SEQUENCE:sysfw_inner_cert"


class ApplicationComponent(X509Component):
    """Application/SPLData component (compType=17, bootCore=16).

    This component can be used for:
    - Application binaries in standard architecture
    - SPL data in split architecture

    The section name can be customized (e.g., 'application' or 'spl data').
    """

    def __init__(
        self,
        binary_path: str,
        load_addr: str,
        section_name: str = "application",
    ) -> None:
        """Initialize Application component."""
        super().__init__(
            comp_type=self.COMP_TYPE_APPLICATION,
            boot_core=16,
            binary_path=binary_path,
            load_addr=load_addr,
            section_name=section_name,
        )

    def get_cert_extension_ref(self) -> str:
        """Return extension reference."""
        return f"bd3 = SEQUENCE:{self.section_name}"


class DebugExtension:
    """Debug extension for X509 certificate."""

    def __init__(self, debug_type: str, device_uid: str | None = None) -> None:
        """Initialize debug extension.

        Args:
            debug_type: Debug type from DebugType constants
            device_uid: Device UID (64 hex chars, defaults to all zeros)
        """
        if debug_type not in DebugType.VALID_TYPES:
            msg = f"Invalid debug type: {debug_type}"
            raise ValueError(msg)
        self.debug_type = DebugType.VALID_TYPES[debug_type]
        self.device_uid = device_uid or "0" * 64

    def get_extension_ref(self) -> str:
        """Return extension reference."""
        return "1.3.6.1.4.1.294.1.8 = ASN1:SEQUENCE:debug"

    def get_section(self) -> str:
        """Return debug section."""
        return f"""
[ debug ]
debugUID     = FORMAT:HEX,OCT:{self.device_uid}
debugType    = INTEGER:{self.debug_type}
coreDbgEn    = INTEGER:0
coreDbgSecEn = INTEGER:0
"""


class CertificateBuilder:
    """Build X509 certificate template from components.

    Follows U-Boot's approach of dynamically building the template
    from registered components.
    """

    # Distinguished name defaults
    DEFAULT_DN: ClassVar[dict[str, str]] = {
        "C": "US",
        "ST": "SC",
        "L": "New York",
        "O": "Texas Instruments., Inc.",
        "OU": "DSP",
        "CN": "Albert",
        "emailAddress": "Albert@gt.ti.com",
    }

    def __init__(self, sha_type: str = SHA.SHA512, swrv: int = 1) -> None:
        """Initialize certificate builder.

        Args:
            sha_type: SHA algorithm to use
            swrv: Software revision number
        """
        self.sha_type = sha_type
        self.swrv = swrv
        self.components: list[X509Component] = []
        self.debug_ext: DebugExtension | None = None

    def add_component(self, component: X509Component) -> "CertificateBuilder":
        """Add a component to the certificate (builder pattern)."""
        component.validate()
        self.components.append(component)
        return self

    def set_debug(self, debug_type: str) -> "CertificateBuilder":
        """Set debug extension."""
        self.debug_ext = DebugExtension(debug_type)
        return self

    def get_total_size(self) -> int:
        """Calculate total size of all components."""
        return sum(comp.get_size() for comp in self.components)

    def get_num_components(self) -> int:
        """Get the number of components."""
        return len(self.components)

    def build_template(self) -> str:
        """Build the complete X509 certificate template."""
        # Build extension references
        ext_refs = [comp.get_cert_extension_ref() for comp in self.components]

        # Build component sections
        sections = [comp.get_cert_section(self.sha_type) for comp in self.components]

        # Build debug extension if present
        debug_ext_ref = self.debug_ext.get_extension_ref() if self.debug_ext else ""
        debug_section = self.debug_ext.get_section() if self.debug_ext else ""

        # Build the template
        template = f"""
[ req ]
distinguished_name     = req_distinguished_name
x509_extensions        = v3_ca
prompt                 = no
dirstring_type         = nobmp

[ req_distinguished_name ]
C                      = {self.DEFAULT_DN["C"]}
ST                     = {self.DEFAULT_DN["ST"]}
L                      = {self.DEFAULT_DN["L"]}
O                      = {self.DEFAULT_DN["O"]}
OU                     = {self.DEFAULT_DN["OU"]}
CN                     = {self.DEFAULT_DN["CN"]}
emailAddress           = {self.DEFAULT_DN["emailAddress"]}

[ v3_ca ]
basicConstraints = CA:true
1.3.6.1.4.1.294.1.3 = ASN1:SEQUENCE:swrv
1.3.6.1.4.1.294.1.9 = ASN1:SEQUENCE:ext_boot_info
{debug_ext_ref}

[ swrv ]
swrv = INTEGER:{self.swrv}

[ ext_boot_info ]
extImgSize = INTEGER:{self.get_total_size()}
numComp = INTEGER:{self.get_num_components()}
{chr(10).join(ext_refs)}
"""

        # Add all component sections
        for section in sections:
            template += section

        # Add debug section if present
        if debug_section:
            template += debug_section

        return template


class ROMImageGenerator:
    """Generate ROM bootable combined image."""

    def __init__(self, builder: CertificateBuilder) -> None:
        """Initialize ROM image generator."""
        self.builder = builder

    def generate(self, key_file: str, output_file: str) -> None:
        """Generate the signed ROM image.

        Args:
            key_file: Path to signing key (.pem)
            output_file: Path to output ROM image

        Build artifacts created (not deleted):
            - <output_file>.x509.conf - Certificate configuration template
            - <output_file>.x509.der - Signed X.509 certificate in DER format
        """
        # Build certificate template
        cert_template = self.builder.build_template()

        # Create certificate artifacts in build directory (same dir as output)
        # These are kept as build artifacts, not deleted
        cert_config = f"{output_file}.x509.conf"
        cert_file = f"{output_file}.x509.der"

        # Write certificate configuration template
        with open(cert_config, "w") as f:
            f.write(cert_template)

        try:
            # Generate certificate using OpenSSL
            cmd = [
                "openssl",
                "req",
                "-new",
                "-x509",
                "-key",
                key_file,
                "-nodes",
                "-outform",
                "DER",
                "-out",
                cert_file,
                "-config",
                cert_config,
                f"-{self.builder.sha_type}",
            ]

            subprocess.check_output(cmd, stderr=subprocess.STDOUT)

            # Concatenate certificate and all component binaries
            with open(output_file, "wb") as out_fh:
                # Write certificate
                with open(cert_file, "rb") as cert_fh:
                    out_fh.write(cert_fh.read())

                # Write all components in order
                for component in self.builder.components:
                    out_fh.write(component.get_binary_data())

            print(f"ROM image generated successfully: {output_file}")
            print(f"Certificate template: {cert_config}")
            print(f"Certificate (DER): {cert_file}")
            print(f"Total components: {self.builder.get_num_components()}")
            print(f"Total size: {self.builder.get_total_size()} bytes")

        except subprocess.CalledProcessError as e:
            print(f"Error generating certificate: {e.output.decode()}")
            raise


def main() -> int:
    """Main entry point."""
    parser = argparse.ArgumentParser(
        description="Creates a ROM-bootable combined image (SPL+SYSFW+BoardCfg)",
        allow_abbrev=False,
    )

    # Required arguments
    parser.add_argument(
        "--spl-bin",
        type=str,
        required=True,
        help="Path to the SPL/BL1 binary or the ATF binary",
    )
    parser.add_argument(
        "--spl-loadaddr",
        type=str,
        required=True,
        help="Load address at which SPL/BL1 needs to be loaded",
    )

    parser.add_argument(
        "--sysfw-bin",
        type=str,
        required=True,
        help="Path to the sysfw binary",
    )
    parser.add_argument(
        "--sysfw-loadaddr",
        type=str,
        required=True,
        help="Load address at which SYSFW needs to be loaded",
    )

    parser.add_argument(
        "--boardcfg-blob",
        type=str,
        required=True,
        help="Path to the boardcfg blob",
    )
    parser.add_argument(
        "--bcfg-loadaddr",
        type=str,
        required=True,
        help="Load address at which BOARDCFG needs to be loaded",
    )

    parser.add_argument(
        "--key",
        type=str,
        required=True,
        help="Path to the signing key (.pem file)",
    )
    parser.add_argument(
        "--rom-image",
        type=str,
        required=True,
        help="Output file combined ROM image",
    )

    # Optional arguments
    parser.add_argument(
        "--sysfw-inner-cert",
        type=str,
        help="Path to the sysfw inner certificate",
    )

    parser.add_argument(
        "--application-bin",
        type=str,
        help="Path to application binary or spldata (for split architecture)",
    )
    parser.add_argument(
        "--application-loadaddr",
        type=str,
        help="Load address for application binary or spldata",
    )
    parser.add_argument(
        "--application-name",
        type=str,
        default="application",
        help=(
            'Section name for application component '
            '(default: "application", use "spl data" for split arch)'
        ),
    )

    parser.add_argument(
        "--swrv",
        type=int,
        default=1,
        help="Software revision number (default: 1)",
    )
    parser.add_argument(
        "--debug",
        type=str,
        choices=list(DebugType.VALID_TYPES.keys()),
        help="Debug options for the image",
    )
    parser.add_argument(
        "--dual-stage-boot",
        action="store_true",
        help="Enable dual stage ATF integrated ROM boot",
    )
    parser.add_argument(
        "--sha",
        type=str,
        default=SHA.SHA512,
        choices=[SHA.SHA256, SHA.SHA384, SHA.SHA512, SHA.SHA224],
        help="SHA algorithm to use (default: sha512)",
    )

    args = parser.parse_args()

    # Validate application arguments
    if (args.application_bin is None) != (args.application_loadaddr is None):
        parser.error(
            "--application-bin and --application-loadaddr must be used together",
        )

    try:
        # Build certificate
        builder = CertificateBuilder(sha_type=args.sha, swrv=args.swrv)

        # Add required components (order matters!)
        builder.add_component(
            SPLComponent(
                binary_path=args.spl_bin,
                load_addr=args.spl_loadaddr,
                dual_stage_boot=args.dual_stage_boot,
            ),
        )

        builder.add_component(
            SYSFWComponent(
                binary_path=args.sysfw_bin,
                load_addr=args.sysfw_loadaddr,
            ),
        )

        # Add inner cert if present (must come before boardcfg per ROM requirements)
        if args.sysfw_inner_cert:
            builder.add_component(
                InnerCertComponent(
                    binary_path=args.sysfw_inner_cert,
                ),
            )

        builder.add_component(
            BoardConfigComponent(
                binary_path=args.boardcfg_blob,
                load_addr=args.bcfg_loadaddr,
            ),
        )

        # Add application/spldata if present
        if args.application_bin:
            builder.add_component(
                ApplicationComponent(
                    binary_path=args.application_bin,
                    load_addr=args.application_loadaddr,
                    section_name=args.application_name,
                ),
            )

        # Add debug extension if requested
        if args.debug:
            builder.set_debug(args.debug)

        # Generate ROM image
        generator = ROMImageGenerator(builder)
        generator.generate(args.key, args.rom_image)

    except Exception as e:  # noqa: BLE001
        print(f"Error: {e}")
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
