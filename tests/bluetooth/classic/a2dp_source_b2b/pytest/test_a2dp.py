# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0
# pylint: disable=duplicate-code

import time

from conftest import (
    logger,
)


def a2dp_source_preparation(source):
    logger.info("Preparation")
    logger.info("a2dp connect")
    source.iexpect('a2dp_source_test connect', 'a2dp connected', timeout=10)

    logger.info("a2dp source send discover cmd")
    source.iexpect('a2dp_source_test discover_peer_eps', 'Bitpool Range', timeout=10)

    logger.info("a2dp set correct configuration")
    source.iexpect('a2dp_source_test modify_sbc_config min_b 20', 'set configuration success')

    logger.info("a2dp source configure")
    source.iexpect('a2dp_source_test configure', 'success to configure')


def a2dp_source_br_clear(source, sink):
    source.exec_command("br clear all")
    sink.exec_command("br clear all")


def a2dp_source_br_connect(source, sink):
    retry = 3
    logger.info(f'acl connect {sink.addr}')
    while retry > 0:
        try:
            source.iexpect(f'br connect {sink.addr}', 'Connected', timeout=35)
            break
        except Exception:
            time.sleep(5)
            retry -= 1
            if retry > 0:
                logger.info('Retry BR connection')
            continue


def test_a2dp_source_set_configuration_cmd(source, sink):
    """
    Test A2DP set configuration command with incorrect and correct parameters.
    """

    a2dp_source_br_clear(source, sink)
    a2dp_source_br_connect(source, sink)
    time.sleep(1)

    def test_set_configuration(source, sink):
        logger.info("a2dp connect")
        source.iexpect('a2dp_source_test connect', 'a2dp connected', timeout=10)

        logger.info("a2dp source send discover cmd")
        source.iexpect('a2dp_source_test discover_peer_eps', 'Bitpool Range', timeout=10)

        logger.info("a2dp set incorrect configuration")
        source.iexpect('a2dp_source_test modify_sbc_config min_b 1', 'set configuration success')

        logger.info("a2dp configure")
        source.iexpect('a2dp_source_test configure', 'peer device fails to configure')

        logger.info("a2dp set correct configuration")
        source.iexpect('a2dp_source_test modify_sbc_config min_b 20', 'set configuration success')

        logger.info("a2dp configure")
        source.iexpect('a2dp_source_test configure', 'success to configure')
        found, _ = sink._wait_for_shell_response('receive requesting config and accept')
        assert found is True

    test_set_configuration(source, sink)
    source.iexpect('a2dp_source_test disconnect', 'a2dp disconnected')

    time.sleep(1)

    test_set_configuration(source, sink)
    source.iexpect('bt disconnect', 'Disconnected')


def test_a2dp_source_establish_cmd(source, sink):
    """
    Test A2DP stream establish command with incorrect and correct parameters.
    """
    a2dp_source_br_clear(source, sink)
    a2dp_source_br_connect(source, sink)
    time.sleep(1)

    def test_establish(source, sink):
        logger.info("a2dp set incorrect acp_seid")
        source.iexpect('a2dp_source_test modify_acp_seid 5', 'set remote endpoint id successfully')

        logger.info("a2dp send stream establish cmd")
        source.iexpect('a2dp_source_test establish', 'fail to establish')

        logger.info("a2dp set correct acp_seid")
        source.iexpect('a2dp_source_test modify_acp_seid', 'set remote endpoint id successfully')

        logger.info("a2dp send stream establish cmd")
        source.iexpect('a2dp_source_test establish', 'success to establish')
        found, _ = sink._wait_for_shell_response('receive requesting establishment and accept')
        assert found is True

    a2dp_source_preparation(source)
    test_establish(source, sink)

    logger.info("wait for iut ready")
    time.sleep(20)
    source.iexpect('a2dp_source_test disconnect', 'a2dp disconnected')

    time.sleep(1)

    a2dp_source_preparation(source)
    test_establish(source, sink)
    source.iexpect('bt disconnect', 'Disconnected')


def test_a2dp_source_release_cmd(source, sink):
    """
    Test A2DP stream release command with incorrect and correct parameters.
    """
    a2dp_source_br_clear(source, sink)
    a2dp_source_br_connect(source, sink)
    time.sleep(1)

    def test_release(source, sink):
        logger.info("a2dp source establish")
        source.iexpect('a2dp_source_test establish', 'success to establish')
        found, _ = sink._wait_for_shell_response('receive requesting establishment and accept')
        assert found is True

        logger.info("a2dp set incorrect acp_seid")
        source.iexpect('a2dp_source_test modify_acp_seid 5', 'set remote endpoint id successfully')

        logger.info("a2dp send stream release cmd")
        source.iexpect('a2dp_source_test release', 'fail to release')

        logger.info("a2dp set correct acp_seid")
        source.iexpect('a2dp_source_test modify_acp_seid', 'set remote endpoint id successfully')

        logger.info("a2dp send stream release cmd")
        source.iexpect('a2dp_source_test release', 'success to release')
        found, _ = sink._wait_for_shell_response('receive requesting release and accept')
        assert found is True

    a2dp_source_preparation(source)
    test_release(source, sink)
    source.iexpect('a2dp_source_test disconnect', 'a2dp disconnected')

    time.sleep(1)

    a2dp_source_preparation(source)
    test_release(source, sink)
    source.iexpect('bt disconnect', 'Disconnected')


def test_a2dp_source_start_cmd(source, sink):
    """
    Test A2DP stream start command with incorrect and correct parameters.
    """

    a2dp_source_br_clear(source, sink)
    a2dp_source_br_connect(source, sink)
    time.sleep(1)

    def test_start(source, sink):
        logger.info("a2dp source establish")
        source.iexpect('a2dp_source_test establish', 'success to establish')
        found, _ = sink._wait_for_shell_response('receive requesting establishment and accept')
        assert found is True

        logger.info("a2dp set incorrect acp_seid")
        source.iexpect('a2dp_source_test modify_acp_seid 5', 'set remote endpoint id successfully')

        logger.info("a2dp send stream start cmd")
        source.iexpect('a2dp_source_test start', 'fail to start')

        logger.info("a2dp set correct acp_seid")
        source.iexpect('a2dp_source_test modify_acp_seid', 'set remote endpoint id successfully')

        logger.info("a2dp send stream start cmd")
        source.iexpect('a2dp_source_test start', 'success to start')
        found, _ = sink._wait_for_shell_response('receive requesting start and accept')
        assert found is True

    a2dp_source_preparation(source)
    test_start(source, sink)
    source.iexpect('a2dp_source_test disconnect', 'a2dp disconnected')

    time.sleep(1)

    a2dp_source_preparation(source)
    test_start(source, sink)
    source.iexpect('bt disconnect', 'Disconnected')


def test_a2dp_source_suspend_cmd(source, sink):
    """
    Test A2DP stream suspend command with incorrect and correct parameters.
    """
    a2dp_source_br_clear(source, sink)
    a2dp_source_br_connect(source, sink)
    time.sleep(1)

    def test_suspend(source, sink):
        logger.info("a2dp source establish")
        source.iexpect('a2dp_source_test establish', 'success to establish')
        found, _ = sink._wait_for_shell_response('receive requesting establishment and accept')
        assert found is True

        logger.info("a2dp send stream start cmd")
        source.iexpect('a2dp_source_test start', 'success to start')

        logger.info("a2dp set incorrect acp_seid")
        source.iexpect('a2dp_source_test modify_acp_seid 5', 'set remote endpoint id successfully')

        logger.info("a2dp send stream suspend cmd")
        source.iexpect('a2dp_source_test suspend', 'fail to suspend')

        logger.info("a2dp set correct acp_seid")
        source.iexpect('a2dp_source_test modify_acp_seid', 'set remote endpoint id successfully')

        logger.info("a2dp send stream suspend cmd")
        source.iexpect('a2dp_source_test suspend', 'success to suspend')
        found, _ = sink._wait_for_shell_response("receive requesting suspend and accept")

    a2dp_source_preparation(source)
    test_suspend(source, sink)
    source.iexpect('a2dp_source_test disconnect', 'a2dp disconnected')

    time.sleep(1)
    a2dp_source_preparation(source)
    test_suspend(source, sink)

    source.iexpect('bt disconnect', 'Disconnected')


def test_a2dp_source_reconfigure_cmd(source, sink):
    """
    Test A2DP stream reconfigure command with incorrect and correct parameters.
    """
    a2dp_source_br_clear(source, sink)
    a2dp_source_br_connect(source, sink)
    time.sleep(1)

    def test_reconfigure(source, sink):
        logger.info("a2dp source establish")
        source.iexpect('a2dp_source_test establish', 'success to establish')
        found, _ = sink._wait_for_shell_response('receive requesting establishment and accept')
        assert found is True

        logger.info("a2dp send stream start cmd")
        source.iexpect('a2dp_source_test start', 'success to start')

        logger.info("a2dp send stream suspend cmd")
        source.iexpect('a2dp_source_test suspend', 'success to suspend')
        found, _ = sink._wait_for_shell_response("receive requesting suspend and accept")

        logger.info("a2dp set correct configuration")
        source.iexpect('a2dp_source_test modify_sbc_config min_b 13', 'set configuration success')

        logger.info("a2dp send stream reconfigure cmd")
        source.iexpect('a2dp_source_test reconfigure', 'peer device fails to configure')

        logger.info("a2dp set correct configuration")
        source.iexpect('a2dp_source_test modify_sbc_config min_b 22', 'set configuration success')

        logger.info("a2dp send stream reconfigure cmd")
        source.iexpect('a2dp_source_test reconfigure', 'success to configure')

    a2dp_source_preparation(source)
    test_reconfigure(source, sink)
    source.iexpect('a2dp_source_test disconnect', 'a2dp disconnected')

    time.sleep(1)

    a2dp_source_preparation(source)
    test_reconfigure(source, sink)
    source.iexpect('bt disconnect', 'Disconnected')


def test_a2dp_source_abort_cmd(source, sink):
    """
    Test A2DP stream abort command with incorrect and correct parameters.
    """
    a2dp_source_br_clear(source, sink)
    a2dp_source_br_connect(source, sink)
    time.sleep(1)

    def test_abort(source, sink):
        logger.info("a2dp source establish")
        source.iexpect('a2dp_source_test establish', 'success to establish')
        found, _ = sink._wait_for_shell_response('receive requesting establishment and accept')
        assert found is True

        logger.info("a2dp set incorrect acp_seid")
        source.iexpect('a2dp_source_test modify_acp_seid 5', 'set remote endpoint id successfully')

        logger.info("a2dp send stream abort cmd")
        source.iexpect('a2dp_source_test abort', 'fail to abort')

        logger.info("a2dp set correct acp_seid")
        source.iexpect('a2dp_source_test modify_acp_seid', 'set remote endpoint id successfully')

        logger.info("a2dp send stream abort cmd")
        source.iexpect('a2dp_source_test abort', 'success to abort')
        found, _ = sink._wait_for_shell_response('receive requesting abort and accept')
        assert found is True

    a2dp_source_preparation(source)
    test_abort(source, sink)
    source.iexpect('a2dp_source_test disconnect', 'a2dp disconnected')

    time.sleep(1)

    a2dp_source_preparation(source)
    test_abort(source, sink)
    source.iexpect('bt disconnect', 'Disconnected')


def test_a2dp_source_get_configuration_cmd(source, sink):
    """
    Test A2DP stream get configuration command with incorrect and correct parameters.
    """
    a2dp_source_br_clear(source, sink)
    a2dp_source_br_connect(source, sink)
    time.sleep(1)

    def test_get_configuration(source, sink):
        logger.info("a2dp set incorrect acp_seid")
        source.iexpect('a2dp_source_test modify_acp_seid 5', 'set remote endpoint id successfully')

        logger.info("a2dp send get configuration cmd")
        source.iexpect('a2dp_source_test get_config', 'fail to get config')

        logger.info("a2dp set correct acp_seid")
        source.iexpect('a2dp_source_test modify_acp_seid', 'set remote endpoint id successfully')

        logger.info("a2dp send stream get configuration cmd")
        source.iexpect('a2dp_source_test get_config', 'success to get config')
        found, _ = sink._wait_for_shell_response('receive get config request and accept')
        assert found is True

    a2dp_source_preparation(source)
    test_get_configuration(source, sink)
    source.iexpect('a2dp_source_test disconnect', 'a2dp disconnected')

    time.sleep(1)

    a2dp_source_preparation(source)
    test_get_configuration(source, sink)
    source.iexpect('bt disconnect', 'Disconnected')


def test_a2dp_source_get_mtu(source, sink):
    """
    Test A2DP get mtu function.
    """
    a2dp_source_br_clear(source, sink)
    a2dp_source_br_connect(source, sink)
    time.sleep(1)

    def test_get_mtu(source, sink):
        logger.info("a2dp source establish")
        source.iexpect('a2dp_source_test establish', 'success to establish')

        source.iexpect('a2dp_source_test get_mtu', 'mtu: 196', timeout=10)

    a2dp_source_preparation(source)
    test_get_mtu(source, sink)
    source.iexpect('a2dp_source_test disconnect', 'a2dp disconnected')
    time.sleep(1)

    a2dp_source_preparation(source)
    test_get_mtu(source, sink)
    source.iexpect('bt disconnect', 'Disconnected')


def test_a2dp_sink_fragmentation(source, sink):
    """
    Test A2DP Fragmentation function.
    """
    a2dp_source_br_clear(source, sink)
    a2dp_source_br_connect(source, sink)
    time.sleep(1)

    logger.info("a2dp connect")
    source.iexpect('a2dp_source_test connect', 'a2dp connected', timeout=10)

    source.iexpect('a2dp_source_test modify_mtu 4', 'set mtu successfully')
    sink.iexpect('a2dp_source_test modify_mtu 4', 'set mtu successfully')

    logger.info("a2dp sink send discover cmd")
    source.iexpect('a2dp_source_test discover_peer_eps', 'Bitpool Range', timeout=10)

    logger.info("a2dp sink configure")
    source.iexpect('a2dp_source_test configure', 'success to configure')

    source.iexpect('a2dp_source_test modify_mtu', 'set mtu successfully')
    source.iexpect('bt disconnect', 'Disconnected')


def test_a2dp_source_random_procedure(source, sink):
    import random

    test_count = 50
    cmd_list = [
        'a2dp_source_test configure',
        'a2dp_source_test establish',
        'a2dp_source_test reconfigure',
        'a2dp_source_test release',
        'a2dp_source_test start',
        'a2dp_source_test suspend',
        'a2dp_source_test abort',
        'a2dp_source_test get_config',
        'a2dp_source_test get_mtu',
        'a2dp_source_test send_delay_report 1',
    ]

    source.exec_command("br clear all")
    sink.exec_command("br clear all")

    time.sleep(1)

    logger.info(f'acl connect {sink.addr}')
    source.iexpect(f'br connect {sink.addr}', 'Connected', timeout=10)

    logger.info("a2dp connect")
    source.iexpect('a2dp_source_test connect', 'a2dp connected', timeout=10)

    logger.info("Preparation")

    logger.info("a2dp source send discover cmd")
    source.iexpect('a2dp_source_test discover_peer_eps', 'Bitpool Range', timeout=10)

    for _ in range(test_count):
        source.exec_command(random.choice(cmd_list))
        time.sleep(2)

    source.iexpect('bt disconnect', 'Disconnected')
