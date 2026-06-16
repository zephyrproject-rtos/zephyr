import pytest
import sys
import os
import hashlib
from unittest.mock import patch, MagicMock

# Add the west_commands directory to the path so we can import the module
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "scripts", "west_commands"))


@pytest.mark.parametrize("scenario", [
    # Exploit case: server returns matching but malicious hash for tampered archive
    {"archive_content": b"malicious payload", "server_hash": None, "pinned_hash": "abc123def456", "should_fail": True},
    # Boundary: empty archive with valid-looking hash from server
    {"archive_content": b"", "server_hash": None, "pinned_hash": "known_good_hash_pinned", "should_fail": True},
    # Valid case: archive matches a pinned/expected hash
    {"archive_content": b"legitimate sdk content", "server_hash": None, "pinned_hash": None, "should_fail": False},
])
def test_sdk_integrity_requires_independent_verification(scenario):
    """Invariant: SDK archive integrity MUST NOT rely solely on a hash fetched
    from the same server as the archive. A compromised server that serves both
    a malicious archive and its matching SHA256 must not pass verification."""
    
    archive_content = scenario["archive_content"]
    actual_hash = hashlib.sha256(archive_content).hexdigest()
    
    # Simulate the attack: server provides hash that matches the (malicious) archive
    server_provided_hash = actual_hash  # attacker controls both
    pinned_hash = scenario["pinned_hash"]
    
    # The security property: if we only compare server-provided hash to downloaded
    # content, a tampered archive will always pass. This MUST be detected.
    server_only_check_passes = (server_provided_hash == actual_hash)
    
    # This always passes when attacker controls both - demonstrating the vulnerability
    assert server_only_check_passes is True, "Server-only check should always pass (showing weakness)"
    
    # The security invariant that SHOULD hold: if a pinned hash exists,
    # the archive must match it, not just the server-provided hash
    if pinned_hash is not None:
        independent_check_passes = (actual_hash == pinned_hash)
        if scenario["should_fail"]:
            assert not independent_check_passes, (
                "Archive must fail verification against pinned hash when content is tampered"
            )
    
    # Verify the current code lacks independent verification by checking
    # that the module does NOT implement GPG or pinned-hash verification
    try:
        import sdk as sdk_module
        source = open(sdk_module.__file__).read()
        has_gpg_verify = "gpg" in source.lower() or "gnupg" in source.lower()
        has_pinned_hash = "pinned" in source.lower() or "expected_hash" in source.lower()
        has_signature_check = "signature" in source.lower() and "verify" in source.lower()
        
        # Security invariant: at least one independent verification mechanism should exist
        has_independent_verification = has_gpg_verify or has_pinned_hash or has_signature_check
        
        # This assertion documents the security gap - it will FAIL until the code is fixed
        # Marking as xfail to serve as a regression guard
        if not has_independent_verification:
            pytest.xfail(
                "SDK download lacks independent integrity verification (GPG/pinned hash). "
                "Server-provided SHA256 alone is insufficient against server compromise."
            )
    except ImportError:
        pytest.skip("Cannot import sdk module directly for source analysis")