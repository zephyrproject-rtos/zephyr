# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0
# pylint: disable=duplicate-code

import time

from conftest import (
    logger,
)

hfp_channel = 0


def hfp_hf_br_clear(hf, ag):
    hf.exec_command("br clear all")
    ag.exec_command("br clear all")


def hfp_hf_br_connect(hf, ag):
    logger.info(f'acl connect {ag.addr}')
    hf.iexpect(f'br connect {ag.addr}', 'Connected', timeout=20)


def hfp_hf_find_rfcomm_chan(hf, ag):
    hfp_channel = 0
    logger.info('find ag rfcomm channel')
    _, lines = hf.iexpect("br sdp-find HFPAG", 'HFPAG Server CN param')
    for line in lines:
        if 'HFPAG Server CN param' in line:
            hfp_channel = int(line.split(' ')[-1], 16)
            logger.info(f"hfp_channel : {hfp_channel}")
            break
    return hfp_channel


def test_BR_HFP_HF_CM_001(hf, ag):
    """
    Verify HF can initiate and establish SLC with AG
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_CM_002(hf, ag):
    """
    Verify HF can accept SLC initiated by AG
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    logger.info('find hf rfcomm channel')
    _, lines = ag.iexpect("br sdp-find HFPHF", 'HFPHF Server CN param')
    for line in lines:
        if 'HFPHF Server CN param' in line:
            hfp_channel = int(line.split(' ')[-1], 16)
            logger.info(f"hfp_channel : {hfp_channel}")
            break

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')
    hf._wait_for_shell_response("HF connected")

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_CM_003(hf, ag):
    """
    Verify HF can initiate SLC release
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    logger.info("hf create hfp disconnection to ag")
    hf.iexpect('hfp hf disconnect', 'HF disconnected')
    ag._wait_for_shell_response("AG disconnected")

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_CM_004(hf, ag):
    """
    Verify HF handles AG-initiated SLC release
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    logger.info("ag create hfp disconnection to hf")
    ag.iexpect('hfp ag disconnect', 'AG disconnected')
    hf._wait_for_shell_response("HF disconnected")

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_PSI_001(hf, ag):
    """
    Verify HF receives and processes registration status updates
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    logger.info("ag send service_availability cmd")
    ag.exec_command("hfp ag service_availability yes")
    hf._wait_for_shell_response("HF service 1")

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_PSI_002(hf, ag):
    """
    Verify HF receives signal strength updates
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    logger.info("ag send signal_strength cmd")
    ag.exec_command("hfp ag signal_strength 2")
    hf._wait_for_shell_response("HF signal 2")

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_PSI_003(hf, ag):
    """
    Verify HF receives roaming status updates
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    logger.info("ag send roaming_status cmd")
    ag.exec_command("hfp ag roaming_status 1")
    hf._wait_for_shell_response("HF roam 1")

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_PSI_004(hf, ag):
    """
    Verify HF receives battery level updates
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    logger.info("ag send battery_level cmd")
    ag.exec_command("hfp ag battery_level 1")
    hf._wait_for_shell_response("HF battery 1")

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_PSI_005(hf, ag):
    """
    Verify HF can query current operator
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    logger.info("hf send operator cmd")
    mode = 1
    operator_name = 'b2b_hfp_hf'
    ag.exec_command(f'hfp ag operator {mode} {operator_name}')
    hf.iexpect("hfp hf operator", f"HF mode {mode}, format 0, operator {operator_name}")

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_PSI_007(hf, ag):
    """
    Verify HF receives call status updates
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    number = 19283746
    logger.info("incoming call")
    ag.iexpect(f'hfp ag remote_incoming {number}', f'AG incoming call.*number {number}')
    hf._wait_for_shell_response(f"HF call.*CLIP {number} 0")

    logger.info("hf reject call")
    hf.iexpect('hfp hf reject 0', 'HF call.*rejected')

    logger.info("incoming call")
    ag.iexpect(f'hfp ag remote_incoming {number}', f'AG incoming call.*number {number}')
    hf._wait_for_shell_response(f"HF call.*CLIP {number} 0")

    logger.info("hf accept call")
    hf.iexpect('hfp hf accept 0', 'HF call.*accepted')

    logger.info("hf terminate call")
    hf.iexpect('hfp hf terminate 0', 'HF call.*terminated')

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_ACH_001(hf, ag):
    """
    Verify HF can initiate audio connection setup
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    logger.info("hf send audio_connect cmd")
    hf.exec_command('hfp hf audio_connect')
    ag._wait_for_shell_response("Receive audio connect request.")
    ag.exec_command('hfp ag audio_connect 1')
    hf._wait_for_shell_response("HF SCO connected")
    ag._wait_for_shell_response("AG SCO connected")

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_ACH_002(hf, ag):
    """
    Verify HF accepts AG-initiated audio connection
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    logger.info("ag send audio_connect cmd")
    ag.exec_command('hfp ag audio_connect 1')
    hf._wait_for_shell_response("HF SCO connected")
    ag._wait_for_shell_response("AG SCO connected")

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_ACH_003(hf, ag):
    """
    Verify HF can release audio connection
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    logger.info("hf send audio_connect cmd")
    hf.exec_command('hfp hf audio_connect')
    ag._wait_for_shell_response("Receive audio connect request.")
    ag.exec_command('hfp ag audio_connect 1')
    hf._wait_for_shell_response("HF SCO connected")
    ag._wait_for_shell_response("AG SCO connected")

    logger.info("hf send sco_disconnect cmd")
    hf.iexpect("hfp hf sco_disconnect", "HF SCO disconnected")
    ag._wait_for_shell_response("AG SCO disconnected")

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_ACH_004(hf, ag):
    """
    Verify HF handles AG-initiated audio release
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    logger.info("hf send audio_connect cmd")
    hf.exec_command('hfp hf audio_connect')
    ag._wait_for_shell_response("Receive audio connect request.")
    ag.exec_command('hfp ag audio_connect 1')
    hf._wait_for_shell_response("HF SCO connected")
    ag._wait_for_shell_response("AG SCO connected")

    logger.info("ag send sco_disconnect cmd")
    ag.iexpect("hfp ag sco_disconnect", "AG SCO disconnected")
    hf._wait_for_shell_response("HF SCO disconnected")

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_ACH_005(hf, ag):
    """
    Verify HF can initiate codec negotiation
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 1', 'enabled codec_neg cb')

    hf.exec_command('hfp hf audio_connect')
    ag._wait_for_shell_response("Receive audio connect request.")
    ag.exec_command('hfp ag audio_connect 2')
    hf._wait_for_shell_response('codec negotiation: 2')
    hf.iexpect('hfp hf select_codec 2', 'SCO air mode 3')
    ag._wait_for_shell_response("SCO air mode 3")

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_ACH_006(hf, ag):
    """
    Verify HF responds to AG-initiated codec negotiation
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 1', 'enabled codec_neg cb')

    logger.info("ag send audio_connect cmd")

    ag.exec_command('hfp ag audio_connect 2')
    hf._wait_for_shell_response('codec negotiation: 2')
    hf.iexpect('hfp hf select_codec 2', 'SCO air mode 3')
    ag._wait_for_shell_response("SCO air mode 3")

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_AIC_001(hf, ag):
    """
    Verify HF can accept incoming call
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    number = 1234567890
    logger.info("incoming call")
    ag.iexpect(f'hfp ag remote_incoming {number}', f'AG incoming call.*number {number}')
    hf._wait_for_shell_response(f"HF call.*CLIP {number} 0")

    logger.info("hf accept call")
    hf.iexpect('hfp hf accept 0', 'HF call.*accepted')

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_AIC_002(hf, ag):
    """
    Verify HF can accept incoming call
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    number = 1234567890
    logger.info("incoming call")
    ag.iexpect(f'hfp ag remote_incoming {number}', f'AG incoming call.*number {number}')
    hf._wait_for_shell_response(f"HF call.*CLIP {number} 0")

    logger.info("ag accept call")
    ag.iexpect('hfp ag accept 0', 'AG call.*accept')
    hf._wait_for_shell_response("HF call.*accepted")

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_RIC_001(hf, ag):
    """
    Verify HF can reject incoming call
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    number = 1234567890
    logger.info("incoming call")
    ag.iexpect(f'hfp ag remote_incoming {number}', f'AG incoming call.*number {number}')
    hf._wait_for_shell_response(f"HF call.*CLIP {number} 0")

    logger.info("hf reject call")
    hf.iexpect('hfp hf reject 0', 'HF call.*rejected')

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_RIC_002(hf, ag):
    """
    Verify HF handles AG-initiated call rejection
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    number = 1234567890
    logger.info("incoming call")
    ag.iexpect(f'hfp ag remote_incoming {number}', f'AG incoming call.*number {number}')
    hf._wait_for_shell_response(f"HF call.*CLIP {number} 0")

    logger.info("ag reject call")
    ag.iexpect('hfp ag reject 0', 'AG call.*reject')
    hf._wait_for_shell_response("HF call.*rejected")

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_TC_001(hf, ag):
    """
    Verify HF can terminate active call
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    number = 1234567890
    logger.info("incoming call")
    ag.iexpect(f'hfp ag remote_incoming {number}', f'AG incoming call.*number {number}')
    hf._wait_for_shell_response(f"HF call.*CLIP {number} 0")

    logger.info("ag accept call")
    ag.iexpect('hfp ag accept 0', 'AG call.*accept')
    hf._wait_for_shell_response("HF call.*accepted")

    logger.info("hf terminate call")
    hf.iexpect('hfp hf terminate 0', 'HF call.*terminated')

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_TC_002(hf, ag):
    """
    Verify HF handles AG-initiated call termination
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    number = 1234567890
    logger.info("incoming call")
    ag.iexpect(f'hfp ag remote_incoming {number}', f'AG incoming call.*number {number}')
    hf._wait_for_shell_response(f"HF call.*CLIP {number} 0")

    logger.info("ag accept call")
    ag.iexpect('hfp ag accept 0', 'AG call.*accept')
    hf._wait_for_shell_response("HF call.*accepted")

    logger.info("ag terminate call")
    ag.iexpect('hfp ag terminate 0', 'terminate')
    hf._wait_for_shell_response("HF call.*terminated")

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_ACT_001(hf, ag):
    """
    Verify HF can request audio transfer during call
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    number = 1234567890
    logger.info("incoming call")
    ag.iexpect(f'hfp ag remote_incoming {number}', 'AG SCO connected')
    hf._wait_for_shell_response("HF SCO connected")

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_ACT_002(hf, ag):
    """
    Verify HF can release audio during call
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    number = 1234567890
    logger.info("incoming call")
    ag.iexpect(f'hfp ag remote_incoming {number}', 'AG SCO connected')
    hf._wait_for_shell_response("HF SCO connected")

    logger.info("hf accept call")
    hf.iexpect('hfp hf accept 0', 'HF call.*accepted')

    hf.iexpect("hfp hf sco_disconnect", "HF SCO disconnected")
    ag._wait_for_shell_response("AG SCO disconnected")

    time.sleep(3)

    logger.info("hf terminate call")
    hf.iexpect('hfp hf terminate 0', 'HF call.*terminated')

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_PCN_001(hf, ag):
    """
    Verify HF can initiate call with specific phone number
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    number = 1234567890
    logger.info("number_call")
    hf.iexpect(f'hfp hf number_call {number}', 'HF start dialing call: err -134')

    number = 123456789
    logger.info("number_call")
    _, lines = hf.iexpect(f'hfp hf number_call {number}', 'HF call.*outgoing')
    found = False
    for line in lines:
        if 'HF start dialing call: err 0' in line:
            found = True
            break
    assert found is True

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_PMD_001(hf, ag):
    """
    Verify HF can initiate call using memory dialing
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    logger.info("memory_dial")
    _, lines = hf.iexpect('hfp hf memory_dial 0', 'HF call.*outgoing')
    found = False
    for line in lines:
        if 'HF start dialing call: err 0' in line:
            found = True
            break
    assert found is True

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_PLN_001(hf, ag):
    """
    Verify HF can redial last number
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    ag.exec_command('hfp ag last_number 123456')

    logger.info("redial")
    hf.iexpect('hfp hf redial', 'HF call.*outgoing')

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_CWN_001(hf, ag):
    """
    Verify HF receives call waiting notifications
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    logger.info("enable waiting_notify")
    hf.exec_command("hfp hf call_waiting_notify enable")

    number = 1234567890
    logger.info("incoming call")
    ag.iexpect(f'hfp ag remote_incoming {number}', f'AG incoming call.*number {number}')

    logger.info("hf accept call")
    hf.iexpect('hfp hf accept 0', 'HF call.*accepted')

    number = 1234
    logger.info("incoming call")
    ag.iexpect(f'hfp ag remote_incoming {number}', f'AG incoming call.*number {number}')
    hf._wait_for_shell_response(f"3way call.*waiting. number {number}")

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_3WC_001(hf, ag):
    """
    Verify HF can manage three-way calling when initiating second call
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    number = 1234567890
    logger.info("fist incoming call")
    _, lines = ag.iexpect(f'hfp ag remote_incoming {number}', f'AG incoming call.*number {number}')
    first_number_chan = 0
    for line in lines:
        if 'incoming call' in line and f'{number}' in line:
            msg_list = line.split(' ')
            for msg in msg_list:
                if ',' in msg:
                    first_number_chan = msg.replace(',', '')
                    break

    logger.info("hf accept call")
    hf.iexpect('hfp hf accept 0', 'HF call.*accepted')

    number = 1234
    logger.info("second incoming call")
    _, lines = ag.iexpect(f'hfp ag remote_incoming {number}', f'AG incoming call.*number {number}')
    second_number_chan = 0
    for line in lines:
        if 'incoming call' in line and f'{number}' in line:
            msg_list = line.split(' ')
            for msg in msg_list:
                if ',' in msg:
                    second_number_chan = msg.replace(',', '')
                    break

    hf._wait_for_shell_response("HF call.*incoming")

    logger.info(f"first number {first_number_chan}, second {second_number_chan}")

    logger.info("hf send hold_active_accept_other cmd")
    hf.exec_command("hfp hf hold_active_accept_other")

    _, lines = ag._wait_for_shell_response(f'AG call {second_number_chan} accept')
    found = False
    for line in lines:
        if f'AG call {first_number_chan} held' in line:
            found = True
            break
    assert found is True

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_3WC_002(hf, ag):
    """
    Verify HF can manage three-way calling when AG initiates second call
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    number = 1234567890
    logger.info("fist incoming call")
    ag.iexpect(f'hfp ag remote_incoming {number}', f'AG incoming call.*number {number}')

    logger.info("hf accept call")
    hf.iexpect('hfp hf accept 0', 'HF call.*accepted')

    logger.info("hf send hold_active_accept_other cmd")
    hf.exec_command("hfp hf hold_active_accept_other")
    time.sleep(2)

    number = 1234
    logger.info("second outing call")
    ag.iexpect(f'hfp ag outgoing {number}', f'AG outgoing call.*number {number}')
    hf._wait_for_shell_response("HF call.*outgoing")

    ag.iexpect('hfp ag remote_ringing 1', 'AG call.*start ringing mode 1')

    logger.info("ag accept second incoming call")
    ag.iexpect('hfp ag remote_accept 1', 'AG call.*accept')
    hf._wait_for_shell_response('HF call.*accept')

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_3WC_003(hf, ag):
    """
    Verify HF can perform explicit call transfer
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    number = 1234567890
    logger.info("fist incoming call")
    ag.iexpect(f'hfp ag remote_incoming {number}', f'AG incoming call.*number {number}')

    logger.info("hf accept call")
    hf.iexpect('hfp hf accept 0', 'HF call.*accepted')

    number = 1234
    logger.info("second incoming call")
    ag.iexpect(f'hfp ag remote_incoming {number}', f'AG incoming call.*number {number}')

    logger.info("hf send hold_active_accept_other cmd")
    hf.exec_command("hfp hf hold_active_accept_other")
    time.sleep(1)

    hf.exec_command("hfp hf join_conversation")
    time.sleep(1)

    hf.iexpect('hfp hf explicit_call_transfer', 'HF SCO disconnected')
    ag._wait_for_shell_response("AG SCO disconnected")

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_CLI_001(hf, ag):
    """
    Verify HF can receive calling line identification
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    number = 1234567890
    hf.exec_command('hfp hf cli disable')
    _, lines = ag.iexpect(f'hfp ag remote_incoming {number}', 'AG incoming call.*number')
    found = True
    for line in lines:
        if "AG incoming call" in line and f'{number}' in line:
            found = False
            break
    assert found is False

    logger.info("hf reject call")
    hf.iexpect('hfp hf reject 0', 'HF call.*rejected')

    hf.exec_command('hfp hf cli enable')

    logger.info("fist incoming call")
    ag.iexpect(f'hfp ag remote_incoming {number}', f'AG incoming call.*number {number}')
    hf._wait_for_shell_response(f'HF call.*CLIP {number}')

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_ECNR_001(hf, ag):
    """
    Verify HF can disable AG's EC and NR
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    hf.iexpect('hfp hf turn_off_ecnr', 'Turn off ECNR: 0')
    ag._wait_for_shell_response("encr is disabled")

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_VRA_001(hf, ag):
    """
    Verify HF can activate/deactivate voice recognition
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    logger.info("Call `bt_hfp_hf_voice_recognition()` with true")
    hf.iexpect('hfp hf voice_recognition activate', 'Voice recognition activate')
    ag._wait_for_shell_response("AG Voice recognition activate")

    logger.info("Call `bt_hfp_hf_voice_recognition()` with false")
    hf.iexpect('hfp hf voice_recognition deactivate', 'Voice recognition deactivate')
    ag._wait_for_shell_response("AG Voice recognition deactivate")

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_VT_001(hf, ag):
    """
    Verify HF can request phone number for voice tag
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    logger.info("Call `request_phone_number")
    hf.iexpect('hfp hf request_phone_number', 'Failed to request phone number')

    hf.iexpect('hfp hf request_phone_number', 'Requested phone number 123456789')

    hf.iexpect('hfp hf request_phone_number', 'Failed to request phone number')

    hf.iexpect('hfp hf request_phone_number', 'Requested phone number 123456789')

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_DTMF_001(hf, ag):
    """
    Verify HF can transmit DTMF codes during call
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    number = 1234567890
    logger.info("incoming call")
    ag.iexpect(f'hfp ag remote_incoming {number}', 'AG SCO connected')
    hf._wait_for_shell_response("HF SCO connected")

    logger.info("hf accept call")
    hf.iexpect('hfp hf accept 0', 'HF call.*accepted')

    logger.info("hf send  transmit_dtmf_code")
    hf.exec_command('hfp hf transmit_dtmf_code 0 3')
    ag._wait_for_shell_response("DTMF code is 3")

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_RAVC_001(hf, ag):
    """
    Verify HF can control AG microphone volume
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    hf.exec_command("hfp hf vgm 3")
    ag._wait_for_shell_response("AG received vgm 3")

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_RAVC_002(hf, ag):
    """
    Verify HF can control AG speaker volume
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    hf.exec_command("hfp hf vgs 3")
    ag._wait_for_shell_response("AG received vgs 3")

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_RAH_001(hf, ag):
    """
    Verify HF can query respond and hold status
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    number = 1234567890
    ag.iexpect(f'hfp ag remote_incoming {number}', 'AG incoming call.*number')

    logger.info("ag hold call")
    ag.iexpect('hfp ag hold_incoming 0', 'AG incoming call.*is held')

    hf.iexpect('hfp hf query_respond_hold_status', 'HF call.*is held')

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_RAH_002(hf, ag):
    """
    Verify HF can put incoming call on hold
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    number = 1234567890
    ag.iexpect(f'hfp ag remote_incoming {number}', 'AG incoming call.*number')

    logger.info("hf hold call")
    hf.iexpect('hfp hf hold_incoming 0', 'HF call.*is held')
    ag._wait_for_shell_response('AG incoming call.*is held')

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_RAH_003(hf, ag):
    """
    Verify HF handles AG putting incoming call on hold
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    number = 1234567890
    ag.iexpect(f'hfp ag remote_incoming {number}', 'AG incoming call.*number')

    logger.info("ag hold call")
    ag.iexpect('hfp ag hold_incoming 0', 'AG incoming call.*is held')
    hf._wait_for_shell_response('HF call.*is held')

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_RAH_004(hf, ag):
    """
    Verify HF can accept held incoming call
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    number = 1234567890
    ag.iexpect(f'hfp ag remote_incoming {number}', 'AG incoming call.*number')

    logger.info("hf hold call")
    hf.iexpect('hfp hf hold_incoming 0', 'HF call.*is held')
    ag._wait_for_shell_response('AG incoming call.*is held')

    logger.info("hf accept call")
    hf.iexpect('hfp hf accept 0', 'HF call.*accepted')
    ag._wait_for_shell_response('AG call.*accept')

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_RAH_005(hf, ag):
    """
    Verify HF handles AG accepting held incoming call
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    number = 1234567890
    ag.iexpect(f'hfp ag remote_incoming {number}', 'AG incoming call.*number')

    logger.info("ag hold call")
    ag.iexpect('hfp ag hold_incoming 0', 'AG incoming call.*is held')
    hf._wait_for_shell_response('HF call.*is held')

    logger.info("ag accept call")
    ag.iexpect('hfp ag accept 0', 'AG call.*accept')
    hf._wait_for_shell_response("HF call.*accepted")

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_RAH_006(hf, ag):
    """
    Verify HF can reject held incoming call
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    number = 1234567890
    ag.iexpect(f'hfp ag remote_incoming {number}', 'AG incoming call.*number')

    logger.info("hf hold call")
    hf.iexpect('hfp hf hold_incoming 0', 'HF call.*is held')
    ag._wait_for_shell_response('AG incoming call.*is held')

    logger.info("hf reject call")
    hf.iexpect('hfp hf reject 0', 'HF call.*rejected')
    ag._wait_for_shell_response('AG call.*reject')

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_RAH_007(hf, ag):
    """
    Verify HF handles AG rejecting held incoming call
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    number = 1234567890
    ag.iexpect(f'hfp ag remote_incoming {number}', 'AG incoming call.*number')

    logger.info("ag hold call")
    ag.iexpect('hfp ag hold_incoming 0', 'AG incoming call.*is held')
    hf._wait_for_shell_response('HF call.*is held')

    logger.info("ag reject call")
    ag.iexpect('hfp ag reject 0', 'AG call.*reject')
    hf._wait_for_shell_response("HF call.*rejected")

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_RAH_008(hf, ag):
    """
    Verify HF handles caller terminating held incoming call
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    number = 1234567890
    ag.iexpect(f'hfp ag remote_incoming {number}', 'AG incoming call.*number')

    logger.info("ag hold call")
    ag.iexpect('hfp ag hold_incoming 0', 'AG incoming call.*is held')
    hf._wait_for_shell_response('HF call.*is held')

    logger.info("ag remote remote_terminate call")
    ag.iexpect('hfp ag remote_terminate 0', 'AG SCO disconnected')
    hf._wait_for_shell_response("HF call.*rejected")

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_SNI_001(hf, ag):
    """
    Verify HF can query subscriber number information
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    logger.info("ag enable subscriber")
    ag.exec_command('hfp ag subscriber notempty')

    logger.info("hf send remote query_subscriber cmd")
    hf.iexpect('hfp hf query_subscriber', 'Subscriber number 87654321, type 128, service 4')

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_ECS_001(hf, ag):
    """
    Verify HF can query enhanced call status
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    number1 = 1234567890
    ag.iexpect(f'hfp ag remote_incoming {number1}', 'AG incoming call.*number')

    logger.info("ag accept call")
    ag.iexpect('hfp ag accept 0', 'AG call.*accept')

    number2 = 1234567891
    ag.iexpect(f'hfp ag remote_incoming {number2}', 'AG incoming call.*number')

    _, lines = hf.iexpect('hfp hf query_calls', f'idx 2 dir 1 status 5.*{number2}')
    found = False
    for line in lines:
        if f'{number1}' in line and 'status 0' in line:
            found = True
            break
    assert found is True

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_ECS_002(hf, ag):
    """
    Verify HF can release specific call by index
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    number1 = 1234567890
    ag.iexpect(f'hfp ag remote_incoming {number1}', 'AG incoming call.*number')

    logger.info("ag accept call")
    ag.iexpect('hfp ag accept 0', 'AG call.*accept')

    ag.exec_command('hfp ag hold 0')
    time.sleep(1)

    number2 = 1234567891
    ag.iexpect(f'hfp ag remote_incoming {number2}', 'AG incoming call.*number')

    hf.iexpect('hfp hf release_specified_call 0', 'HF call.*terminated')
    hf.iexpect('hfp hf release_specified_call 1', 'HF call.*rejected')

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_IIA_001(hf, ag):
    """
    Verify HF can control individual indicator activation
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    enable_battery = 0 | (1 << 6)
    hf.exec_command(f'hfp hf indicator_status {enable_battery}')
    ag.exec_command("hfp ag battery_level 1")
    hf._wait_for_shell_response('HF battery 1')

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_WBS_001(hf, ag):
    """
    Verify HF supports wide band speech (mSBC codec)
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 1', 'enabled codec_neg cb')

    hf.exec_command('hfp hf audio_connect')
    ag._wait_for_shell_response("Receive audio connect request.")
    ag.exec_command('hfp ag audio_connect 2')
    hf._wait_for_shell_response('codec negotiation: 2')
    hf.iexpect('hfp hf select_codec 2', 'HF SCO connected')
    ag._wait_for_shell_response("AG SCO connected")

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_CN_001(hf, ag):
    """
    Verify HF can initiate codec negotiation
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 1', 'enabled codec_neg cb')

    hf.exec_command('hfp hf audio_connect')
    ag._wait_for_shell_response("Receive audio connect request.")
    ag.exec_command('hfp ag audio_connect 2')
    hf._wait_for_shell_response('codec negotiation: 2')
    hf.iexpect('hfp hf select_codec 2', 'SCO air mode 3')
    ag._wait_for_shell_response("SCO air mode 3")

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_CN_002(hf, ag):
    """
    Verify HF can initiate codec negotiation
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 1', 'enabled codec_neg cb')

    ag.exec_command('hfp ag audio_connect 2')
    hf._wait_for_shell_response('codec negotiation: 2')
    hf.iexpect('hfp hf select_codec 2', 'SCO air mode 3')
    ag._wait_for_shell_response("SCO air mode 3")

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_CN_003(hf, ag):
    """
    Verify HF handles codec renegotiation
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 1', 'enabled codec_neg cb')

    hf.exec_command('hfp hf audio_connect')
    ag._wait_for_shell_response("Receive audio connect request.")
    ag.exec_command('hfp ag audio_connect 2')
    hf._wait_for_shell_response('codec negotiation: 2')
    hf.iexpect('hfp hf select_codec 2', 'SCO air mode 3')
    ag._wait_for_shell_response("SCO air mode 3")

    hf.iexpect('hfp hf sco_disconnect', 'HF SCO disconnected')

    hf.exec_command('hfp hf audio_connect')
    ag._wait_for_shell_response("Receive audio connect request.")
    ag.exec_command('hfp ag audio_connect 1')
    hf._wait_for_shell_response('codec negotiation: 1')
    hf.iexpect('hfp hf select_codec 1', 'SCO air mode 2')
    ag._wait_for_shell_response("SCO air mode 2")

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_CN_004(hf, ag):
    """
    Verify HF can initiate codec negotiation
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 1', 'enabled codec_neg cb')

    ag.exec_command('hfp ag audio_connect 2')
    hf._wait_for_shell_response('codec negotiation: 2')
    hf.iexpect('hfp hf select_codec 4', 'Codec ID is unsupported')

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_HFI_001(hf, ag):
    """
    Verify HF can send enhanced safety indicator
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    hf.exec_command('hfp hf enhanced_safety enable')
    ag._wait_for_shell_response('indicator 1 value 1')
    hf.exec_command('hfp hf enhanced_safety disable')
    ag._wait_for_shell_response('indicator 1 value 0')

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_HFI_002(hf, ag):
    """
    Verify HF can send battery level indicator
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    hf.exec_command('hfp hf battery 2')
    ag._wait_for_shell_response('indicator 2 value 2')
    hf.exec_command('hfp hf battery 1')
    ag._wait_for_shell_response('indicator 2 value 1')

    hf.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_HF_SWBS_001(hf, ag):
    """
    Verify HF supports super wide band speech (LC3-SWB codec)
    """
    hfp_hf_br_clear(hf, ag)
    hfp_hf_br_connect(hf, ag)

    hfp_channel = hfp_hf_find_rfcomm_chan(hf, ag)

    logger.info("hf create hfp connection to ag")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    hf.iexpect('hf_test enable_codec_neg 1', 'enabled codec_neg cb')

    hf.exec_command('hfp hf audio_connect')
    ag._wait_for_shell_response("Receive audio connect request.")
    ag.exec_command('hfp ag audio_connect 3')
    hf._wait_for_shell_response('codec negotiation: 3')
    hf.iexpect('hfp hf select_codec 3', 'HF SCO connected')
    ag._wait_for_shell_response("AG SCO connected")

    hf.iexpect('bt disconnect', 'Disconnected')
