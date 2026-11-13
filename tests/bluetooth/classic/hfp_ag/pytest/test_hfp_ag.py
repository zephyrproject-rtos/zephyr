# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0


import time

from conftest import (
    logger,
)

hfp_channel = 0


def hfp_ag_br_clear(ag, hf):
    ag.exec_command("br clear all")
    hf.exec_command("br clear all")


def hfp_ag_br_connect(ag, hf):
    logger.info(f'acl connect {hf.addr}')
    ag.iexpect(f'br connect {hf.addr}', 'Connected', timeout=20)


def hfp_ag_find_rfcomm_chan(ag, hf):
    hfp_channel = 0
    logger.info('find hf rfcomm channel')
    _, lines = ag.iexpect("br sdp-find HFPHF", 'HFPHF Server CN param')
    for line in lines:
        if 'HFPHF Server CN param' in line:
            hfp_channel = int(line.split(' ')[-1], 16)
            logger.info(f"hfp_channel : {hfp_channel}")
            break
    return hfp_channel


def test_BR_HFP_AG_CM_001(ag, hf):
    """
    Verify AG can initiate and establish SLC with HF
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')
    hf._wait_for_shell_response("HF connected")

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_CM_002(ag, hf):
    """
    Verify AG can accept SLC initiated by HF
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    logger.info('find ag rfcomm channel')
    _, lines = hf.iexpect("br sdp-find HFPAG", 'HFPAG Server CN param')
    for line in lines:
        if 'HFPAG Server CN param' in line:
            hfp_channel = int(line.split(' ')[-1], 16)
            logger.info(f"hfp_channel : {hfp_channel}")
            break

    logger.info("ag create hfp connection to hf")
    hf.iexpect(f"hfp hf connect {hfp_channel}", 'HF connected')
    ag._wait_for_shell_response("AG connected")

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_CM_003(ag, hf):
    """
    Verify AG can initiate SLC release
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    logger.info("ag create hfp disconnection to hf")
    ag.iexpect('hfp ag disconnect', 'AG disconnected')
    hf._wait_for_shell_response("HF disconnected")

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_CM_004(ag, hf):
    """
    Verify AG handles HF-initiated SLC release
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    logger.info("hf create hfp disconnection to ag")
    hf.iexpect('hfp hf disconnect', 'HF disconnected')
    ag._wait_for_shell_response("AG disconnected")

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_PSI_001(ag, hf):
    """
    Verify AG can send registration status updates
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    logger.info("ag send service_availability cmd")
    ag.exec_command("hfp ag service_availability yes")
    hf._wait_for_shell_response("HF service 1")

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_PSI_002(ag, hf):
    """
    Verify AG can send signal strength updates
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    logger.info("ag send signal_strength cmd")
    ag.exec_command("hfp ag signal_strength 2")
    hf._wait_for_shell_response("HF signal 2")

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_PSI_003(ag, hf):
    """
    Verify AG can send roaming status updates
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    logger.info("ag send roaming_status cmd")
    ag.exec_command("hfp ag roaming_status 1")
    hf._wait_for_shell_response("HF roam 1")

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_PSI_004(ag, hf):
    """
    Verify AG can send battery level updates
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    logger.info("ag send battery_level cmd")
    ag.exec_command("hfp ag battery_level 1")
    hf._wait_for_shell_response("HF battery 1")

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_PSI_005(ag, hf):
    """
    Verify AG responds to operator query
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    logger.info("hf send operator cmd")
    mode = 1
    operator_name = 'b2b_hfp_hf'
    ag.exec_command(f'hfp ag operator {mode} {operator_name}')
    hf.iexpect("hfp hf operator", f"HF mode {mode}, format 0, operator {operator_name}")

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_PSI_007(ag, hf):
    """
    Verify HF receives call status updates
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    number = 1234567890
    logger.info("incoming call")
    ag.iexpect(f'hfp ag remote_incoming {number}', f'AG incoming call.*number {number}')
    hf._wait_for_shell_response(f"HF call.*CLIP {number} 0")

    logger.info("ag reject call")
    ag.iexpect('hfp ag reject 0', 'AG call.*reject')

    logger.info("incoming call")
    ag.iexpect(f'hfp ag remote_incoming {number}', f'AG incoming call.*number {number}')
    hf._wait_for_shell_response(f"HF call.*CLIP {number} 0")

    logger.info("ag accept call")
    ag.iexpect('hfp ag accept 0', 'AG call.*accept')

    logger.info("ag terminate call")
    ag.iexpect('hfp ag terminate 0', 'AG SCO disconnected')

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_ACH_001(ag, hf):
    """
    Verify AG can initiate audio connection setup
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    logger.info("ag send audio_connect cmd")
    ag.exec_command('hfp ag audio_connect 1')
    hf._wait_for_shell_response("HF SCO connected")
    ag._wait_for_shell_response("AG SCO connected")

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_ACH_002(ag, hf):
    """
    Verify AG accepts HF-initiated audio connection
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    logger.info("hf send audio_connect cmd")
    hf.exec_command('hfp hf audio_connect')
    ag._wait_for_shell_response("Receive audio connect request.")
    ag.exec_command('hfp ag audio_connect 1')
    hf._wait_for_shell_response("HF SCO connected")
    ag._wait_for_shell_response("AG SCO connected")

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_ACH_003(ag, hf):
    """
    Verify AG can release audio connection
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    logger.info("ag send audio_connect cmd")
    ag.exec_command('hfp ag audio_connect 1')
    hf._wait_for_shell_response("HF SCO connected")
    ag._wait_for_shell_response("AG SCO connected")

    logger.info("ag send sco_disconnect cmd")
    ag.iexpect("hfp ag sco_disconnect", "AG SCO disconnected")
    hf._wait_for_shell_response("HF SCO disconnected")

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_ACH_004(ag, hf):
    """
    Verify AG handles HF-initiated audio release
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    logger.info("ag send audio_connect cmd")
    ag.exec_command('hfp ag audio_connect 1')
    hf._wait_for_shell_response("HF SCO connected")
    ag._wait_for_shell_response("AG SCO connected")

    logger.info("hf send sco_disconnect cmd")
    hf.iexpect("hfp hf sco_disconnect", "HF SCO disconnected")
    ag._wait_for_shell_response("AG SCO disconnected")

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_ACH_005(ag, hf):
    """
    Verify AG can initiate codec negotiation
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    hf.iexpect('hf_test enable_codec_neg 1', 'enabled codec_neg cb')

    logger.info("ag send audio_connect cmd")
    codec_id = 1
    ag.exec_command(f'hfp ag audio_connect {codec_id}')
    time.sleep(0.5)
    codec_id = 1  # when enabe code2 will change it to 2
    hf.exec_command(f'hfp hf select_codec {codec_id}')

    hf._wait_for_shell_response("HF SCO connected")
    ag._wait_for_shell_response("AG SCO connected")

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_ACH_006(ag, hf):
    """
    Verify HF responds to AG-initiated codec negotiation
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    hf.exec_command("hfp hf set_codecs 2")
    ag._wait_for_shell_response("AG received codec id bit map 2")

    # when enable codec 2, will create codec connection based 2.

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_AIC_001(ag, hf):
    """
    Verify AG processes HF call acceptance
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to ag")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    number = 1234567890
    logger.info("incoming call")
    ag.iexpect(f'hfp ag remote_incoming {number}', f'AG incoming call.*number {number}')
    hf._wait_for_shell_response(f"HF call.*CLIP {number} 0")

    logger.info("hf accept call")
    hf.iexpect('hfp hf accept 0', 'HF call.*accepted')
    ag._wait_for_shell_response("AG call.*accept")

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_AIC_002(ag, hf):
    """
    Verify AG can accept incoming call directly
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    number = 1234567890
    logger.info("incoming call")
    ag.iexpect(f'hfp ag remote_incoming {number}', f'AG incoming call.*number {number}')
    hf._wait_for_shell_response(f"HF call.*CLIP {number} 0")

    logger.info("ag accept call")
    ag.iexpect('hfp ag accept 0', 'AG call.*accept')
    hf._wait_for_shell_response("HF call.*accepted")

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_RIC_001(ag, hf):
    """
    Verify AG processes HF call rejection
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to ag")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    number = 1234567890
    logger.info("incoming call")
    ag.iexpect(f'hfp ag remote_incoming {number}', f'AG incoming call.*number {number}')
    hf._wait_for_shell_response(f"HF call.*CLIP {number} 0")

    logger.info("hf reject call")
    hf.iexpect('hfp hf reject 0', 'HF call.*rejected')
    ag._wait_for_shell_response('AG call.*reject')

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_RIC_002(ag, hf):
    """
    Verify AG can reject incoming call directly
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    number = 1234567890
    logger.info("incoming call")
    ag.iexpect(f'hfp ag remote_incoming {number}', f'AG incoming call.*number {number}')
    hf._wait_for_shell_response(f"HF call.*CLIP {number} 0")

    logger.info("ag reject call")
    ag.iexpect('hfp ag reject 0', 'AG call.*reject')
    hf._wait_for_shell_response("HF call.*rejected")

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_TC_001(ag, hf):
    """
    Verify AG processes HF call termination
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

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
    ag._wait_for_shell_response("AG call.*terminate")

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_TC_002(ag, hf):
    """
    Verify AG can terminate active call directly
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

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
    hf._wait_for_shell_response("HF call.*terminate")

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_ACT_001(ag, hf):
    """
    Verify AG can transfer audio to HF during call
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    number = 1234567890
    logger.info("incoming call")
    ag.iexpect(f'hfp ag remote_incoming {number}', 'AG SCO connected')
    hf._wait_for_shell_response("HF SCO connected")

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_ACT_002(ag, hf):
    """
    Verify HF can release audio during call
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    number = 1234567890
    logger.info("incoming call")
    ag.iexpect(f'hfp ag remote_incoming {number}', 'AG SCO connected')
    hf._wait_for_shell_response("HF SCO connected")

    logger.info("ag accept call")
    ag.iexpect('hfp ag accept 0', 'AG call.*accept')

    ag.iexpect("hfp ag sco_disconnect", "AG SCO disconnected")
    hf._wait_for_shell_response("HF SCO disconnected")

    time.sleep(3)

    logger.info("ag terminate call")
    ag.exec_command('hfp ag terminate 0')
    hf._wait_for_shell_response('HF call.*terminated')

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_PCN_001(ag, hf):
    """
    Verify AG processes HF phone number call request
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    number = 123456789
    logger.info("number_call")
    hf.iexpect(f'hfp hf number_call {number}', 'HF call.*outgoing')
    ag._wait_for_shell_response('AG outgoing call')

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_PMD_001(ag, hf):
    """
    Verify AG processes HF memory dialing request
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    logger.info("memory_dial")
    hf.iexpect('hfp hf memory_dial 0', 'HF call.*outgoing')
    ag._wait_for_shell_response('AG outgoing call')

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_PLN_001(ag, hf):
    """
    Verify AG processes HF last number redial
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    ag.exec_command('hfp ag last_number 123456')

    logger.info("redial")
    hf.iexpect('hfp hf redial', 'HF call.*outgoing')
    ag._wait_for_shell_response('AG outgoing call')

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_CWN_001(ag, hf):
    """
    Verify AG sends call waiting notifications
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

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


def test_BR_HFP_AG_3WC_001(ag, hf):
    """
    Verify AG supports three-way calling when HF initiates second call
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

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

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_3WC_002(ag, hf):
    """
    Verify AG can initiate second call for three-way calling
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

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

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_3WC_003(ag, hf):
    """
    Verify AG processes explicit call transfer
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

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

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_CLI_001(ag, hf):
    """
    Verify AG sends calling line identification
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

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

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_ECNR_001(ag, hf):
    """
    Verify AG processes EC/NR disable request
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    hf.iexpect('hfp hf turn_off_ecnr', 'Turn off ECNR: 0')
    ag._wait_for_shell_response("encr is disabled")

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_VRA_001(ag, hf):
    """
    Verify AG processes voice recognition requests
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    logger.info("Call `bt_hfp_hf_voice_recognition()` with true")
    hf.iexpect('hfp hf voice_recognition activate', 'Voice recognition activate')
    ag._wait_for_shell_response("AG Voice recognition activate")

    logger.info("Call `bt_hfp_hf_voice_recognition()` with false")
    hf.iexpect('hfp hf voice_recognition deactivate', 'Voice recognition deactivate')
    ag._wait_for_shell_response("AG Voice recognition deactivate")

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_VT_001(ag, hf):
    """
    Verify AG provides phone number for voice tag
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    logger.info("Call `request_phone_number")
    hf.iexpect('hfp hf request_phone_number', 'Failed to request phone number')

    hf.iexpect('hfp hf request_phone_number', 'Requested phone number 123456789')

    hf.iexpect('hfp hf request_phone_number', 'Failed to request phone number')

    hf.iexpect('hfp hf request_phone_number', 'Requested phone number 123456789')

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_DTMF_001(ag, hf):
    """
    Verify AG processes DTMF codes from HF
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    number = 1234567890
    logger.info("incoming call")
    ag.iexpect(f'hfp ag remote_incoming {number}', 'AG SCO connected')
    hf._wait_for_shell_response("HF SCO connected")

    logger.info("ag accept call")
    ag.iexpect('hfp ag accept 0', 'AG call.*accept')

    logger.info("hf send  transmit_dtmf_code")
    hf.exec_command('hfp hf transmit_dtmf_code 0 3')
    ag._wait_for_shell_response("DTMF code is 3")

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_RAVC_001(ag, hf):
    """
    Verify AG processes HF microphone volume changes
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    hf.exec_command("hfp hf vgm 3")
    ag._wait_for_shell_response("AG received vgm 3")

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_RAVC_002(ag, hf):
    """
    Verify AG processes HF speaker volume changes
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    hf.exec_command("hfp hf vgs 3")
    ag._wait_for_shell_response("AG received vgs 3")

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_RAH_001(ag, hf):
    """
    Verify AG responds to respond and hold status query
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    number = 1234567890
    ag.iexpect(f'hfp ag remote_incoming {number}', 'AG incoming call.*number')

    logger.info("ag hold call")
    ag.iexpect('hfp ag hold_incoming 0', 'AG incoming call.*is held')

    hf.iexpect('hfp hf query_respond_hold_status', 'HF call.*is held')

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_RAH_002(ag, hf):
    """
    Verify AG processes HF putting incoming call on hold
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    number = 1234567890
    ag.iexpect(f'hfp ag remote_incoming {number}', 'AG incoming call.*number')

    logger.info("hf hold call")
    hf.iexpect('hfp hf hold_incoming 0', 'HF call.*is held')
    ag._wait_for_shell_response('AG incoming call.*is held')

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_RAH_003(ag, hf):
    """
    Verify AG can put incoming call on hold
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    number = 1234567890
    ag.iexpect(f'hfp ag remote_incoming {number}', 'AG incoming call.*number')

    logger.info("ag hold call")
    ag.iexpect('hfp ag hold_incoming 0', 'AG incoming call.*is held')
    hf._wait_for_shell_response('HF call.*is held')

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_RAH_004(ag, hf):
    """
    Verify AG processes HF accepting held incoming call
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    number = 1234567890
    ag.iexpect(f'hfp ag remote_incoming {number}', 'AG incoming call.*number')

    logger.info("hf hold call")
    hf.iexpect('hfp hf hold_incoming 0', 'HF call.*is held')
    ag._wait_for_shell_response('AG incoming call.*is held')

    logger.info("hf accept call")
    hf.iexpect('hfp hf accept 0', 'HF call.*accepted')
    ag._wait_for_shell_response('AG call.*accept')

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_RAH_005(ag, hf):
    """
    Verify AG can accept held incoming call
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    number = 1234567890
    ag.iexpect(f'hfp ag remote_incoming {number}', 'AG incoming call.*number')

    logger.info("ag hold call")
    ag.iexpect('hfp ag hold_incoming 0', 'AG incoming call.*is held')
    hf._wait_for_shell_response('HF call.*is held')

    logger.info("ag accept call")
    ag.iexpect('hfp ag accept 0', 'AG call.*accept')
    hf._wait_for_shell_response("HF call.*accepted")

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_RAH_006(ag, hf):
    """
    Verify AG processes HF rejecting held incoming call
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    number = 1234567890
    ag.iexpect(f'hfp ag remote_incoming {number}', 'AG incoming call.*number')

    logger.info("hf hold call")
    hf.iexpect('hfp hf hold_incoming 0', 'HF call.*is held')
    ag._wait_for_shell_response('AG incoming call.*is held')

    logger.info("hf reject call")
    hf.iexpect('hfp hf reject 0', 'HF call.*rejected')
    ag._wait_for_shell_response('AG call.*reject')

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_RAH_007(ag, hf):
    """
    Verify AG can reject held incoming call
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    number = 1234567890
    ag.iexpect(f'hfp ag remote_incoming {number}', 'AG incoming call.*number')

    logger.info("ag hold call")
    ag.iexpect('hfp ag hold_incoming 0', 'AG incoming call.*is held')
    hf._wait_for_shell_response('HF call.*is held')

    logger.info("ag reject call")
    ag.iexpect('hfp ag reject 0', 'AG call.*reject')
    hf._wait_for_shell_response("HF call.*rejected")

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_RAH_008(ag, hf):
    """
    Verify AG handles caller terminating held incoming call
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    number = 1234567890
    ag.iexpect(f'hfp ag remote_incoming {number}', 'AG incoming call.*number')

    logger.info("ag hold call")
    ag.iexpect('hfp ag hold_incoming 0', 'AG incoming call.*is held')
    hf._wait_for_shell_response('HF call.*is held')

    logger.info("ag remote remote_terminate call")
    ag.iexpect('hfp ag remote_terminate 0', 'AG SCO disconnected')
    hf._wait_for_shell_response("HF call.*rejected")

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_SNI_001(ag, hf):
    """
    Verify AG provides subscriber number
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    logger.info("ag enable subscriber")
    ag.exec_command('hfp ag subscriber notempty')

    logger.info("hf send remote query_subscriber cmd")
    hf.iexpect('hfp hf query_subscriber', 'Subscriber number 87654321, type 128, service 4')

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_ECS_001(ag, hf):
    """
    Verify AG provides enhanced call status information
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    ag.exec_command("hfp ag ongoing_calls yes")

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'Please set ongoing calls')

    ag.iexpect("hfp ag set_ongoing_calls 9876789 0 0 0 all", "AG call.*accept")
    hf._wait_for_shell_response("HF call.*accepted")

    ag.exec_command("hfp ag ongoing_calls no")

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_IIA_001(ag, hf):
    """
    Verify AG processes individual indicator activation requests
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    enable_battery = 0 | (1 << 6)
    hf.exec_command(f'hfp hf indicator_status {enable_battery}')
    ag.exec_command("hfp ag battery_level 1")
    hf._wait_for_shell_response('HF battery 1')

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_WBS_001(ag, hf):
    """
    Verify AG supports wide band speech (mSBC codec)
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    hf.iexpect('hf_test enable_codec_neg 1', 'enabled codec_neg cb')

    ag.exec_command('hfp ag audio_connect 2')
    hf._wait_for_shell_response('codec negotiation: 2')
    hf.iexpect('hfp hf select_codec 2', 'HF SCO connected')
    ag._wait_for_shell_response("AG SCO connected")

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_CN_001(ag, hf):
    """
    Verify AG can initiate codec negotiation
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    hf.iexpect('hf_test enable_codec_neg 1', 'enabled codec_neg cb')

    ag.exec_command('hfp ag audio_connect 2')
    hf._wait_for_shell_response('codec negotiation: 2')
    hf.iexpect('hfp hf select_codec 2', 'SCO air mode 3')
    ag._wait_for_shell_response("SCO air mode 3")

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_CN_002(ag, hf):
    """
    Verify AG responds to HF-initiated codec negotiation
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    hf.iexpect('hf_test enable_codec_neg 1', 'enabled codec_neg cb')

    hf.exec_command('hfp hf audio_connect')
    ag._wait_for_shell_response("Receive audio connect request.")
    ag.exec_command('hfp ag audio_connect 2')
    hf._wait_for_shell_response('codec negotiation: 2')
    hf.iexpect('hfp hf select_codec 2', 'SCO air mode 3')
    ag._wait_for_shell_response("SCO air mode 3")

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_CN_003(ag, hf):
    """
    Verify AG handles codec renegotiation
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    hf.iexpect('hf_test enable_codec_neg 1', 'enabled codec_neg cb')

    ag.exec_command('hfp ag audio_connect 2')
    hf._wait_for_shell_response('codec negotiation: 2')
    hf.iexpect('hfp hf select_codec 2', 'SCO air mode 3')
    ag._wait_for_shell_response("SCO air mode 3")

    ag.iexpect('hfp ag sco_disconnect', 'AG SCO disconnected')

    ag.exec_command('hfp ag audio_connect 1')
    hf._wait_for_shell_response('codec negotiation: 1')
    hf.iexpect('hfp hf select_codec 1', 'SCO air mode 2')
    ag._wait_for_shell_response("SCO air mode 2")

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_CN_004(ag, hf):
    """
    Verify AG handles codec negotiation failure
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    hf.iexpect('hf_test enable_codec_neg 1', 'enabled codec_neg cb')

    ag.exec_command('hfp ag audio_connect 2')
    hf._wait_for_shell_response('codec negotiation: 2')
    hf.iexpect('hfp hf select_codec 4', 'Codec ID is unsupported')

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_ECS_002(ag, hf):
    """
    Verify AG processes enhanced call control commands
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

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

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_HFI_001(ag, hf):
    """
    Verify AG processes HF enhanced safety indicator
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    ag.exec_command('hfp ag hf_indicator 1 disable')
    hf.iexpect('hfp hf enhanced_safety enable', 'The indicator is disabled')

    ag.exec_command('hfp ag hf_indicator 1 enable')

    hf.exec_command('hfp hf enhanced_safety enable')
    ag._wait_for_shell_response("indicator 1 value 1")
    hf.exec_command('hfp hf enhanced_safety disable')
    ag._wait_for_shell_response("indicator 1 value 0")

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_HFI_002(ag, hf):
    """
    Verify AG processes HF battery level indicator
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    hf.iexpect('hf_test enable_codec_neg 0', 'disabled codec_neg cb')

    ag.exec_command('hfp ag hf_indicator 2 disable')
    hf.iexpect('hfp hf battery 2', 'Failed to transfer battery level')

    ag.exec_command('hfp ag hf_indicator 2 enable')

    hf.exec_command('hfp hf battery 2')
    ag._wait_for_shell_response("indicator 2 value 2")
    hf.exec_command('hfp hf battery 1')
    ag._wait_for_shell_response("indicator 2 value 1")

    ag.iexpect('bt disconnect', 'Disconnected')


def test_BR_HFP_AG_SWBS_001(ag, hf):
    """
    Verify AG supports super wide band speech (LC3-SWB codec)
    """
    hfp_ag_br_clear(ag, hf)
    hfp_ag_br_connect(ag, hf)

    hfp_channel = hfp_ag_find_rfcomm_chan(ag, hf)

    logger.info("ag create hfp connection to hf")
    ag.iexpect(f"hfp ag connect {hfp_channel}", 'AG connected')

    hf.iexpect('hf_test enable_codec_neg 1', 'enabled codec_neg cb')

    ag.exec_command('hfp ag audio_connect 3')
    hf._wait_for_shell_response('codec negotiation: 3')
    hf.iexpect('hfp hf select_codec 3', 'HF SCO connected')
    ag._wait_for_shell_response("AG SCO connected")

    ag.iexpect('bt disconnect', 'Disconnected')
