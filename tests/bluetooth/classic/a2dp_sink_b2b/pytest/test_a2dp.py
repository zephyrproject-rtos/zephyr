# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0
# pylint: disable=duplicate-code

import time

from conftest import (
    logger,
)


def a2dp_sink_preparation(sink):
    logger.info("Preparation")
    logger.info("a2dp connect")
    sink.iexpect('a2dp_sink_test connect', 'a2dp connected', timeout=10)

    logger.info("a2dp sink send discover cmd")
    sink.iexpect('a2dp_sink_test discover_peer_eps', 'Bitpool Range', timeout=10)

    logger.info("a2dp set correct configuration")
    sink.iexpect('a2dp_sink_test modify_sbc_config min_b 20', 'set configuration success')

    logger.info("a2dp sink configure")
    sink.iexpect('a2dp_sink_test configure', 'success to configure')


def a2dp_sink_br_clear(sink, source):
    sink.exec_command("br clear all")
    source.exec_command("br clear all")


def a2dp_sink_br_connect(sink, source):
    retry = 3
    logger.info(f'acl connect {source.addr}')
    while retry > 0:
        try:
            sink.iexpect(f'br connect {source.addr}', 'Connected', timeout=35)
            break
        except Exception:
            time.sleep(5)
            retry -= 1
            if retry > 0:
                logger.info('Retry BR connection')
            continue


def test_a2dp_sink_set_configuration_cmd(sink, source):
    """
    Test A2DP set configuration command with incorrect and correct parameters.
    """

    a2dp_sink_br_clear(sink, source)
    a2dp_sink_br_connect(sink, source)
    time.sleep(1)

    def test_set_configuration(sink, source):
        logger.info("a2dp connect")
        sink.iexpect('a2dp_sink_test connect', 'a2dp connected', timeout=10)

        logger.info("a2dp sink send discover cmd")
        sink.iexpect('a2dp_sink_test discover_peer_eps', 'Bitpool Range', timeout=10)

        logger.info("a2dp set incorrect configuration")
        sink.iexpect('a2dp_sink_test modify_sbc_config min_b 1', 'set configuration success')

        logger.info("a2dp configure")
        sink.iexpect('a2dp_sink_test configure', 'peer device fails to configure')

        logger.info("a2dp set correct configuration")
        sink.iexpect('a2dp_sink_test modify_sbc_config min_b 20', 'set configuration success')

        logger.info("a2dp configure")
        sink.iexpect('a2dp_sink_test configure', 'success to configure')
        found, _ = source._wait_for_shell_response('receive requesting config and accept')
        assert found is True

    test_set_configuration(sink, source)
    sink.iexpect('a2dp_sink_test disconnect', 'a2dp disconnected')

    time.sleep(1)

    test_set_configuration(sink, source)
    sink.iexpect('bt disconnect', 'Disconnected')


def test_a2dp_sink_establish_cmd(sink, source):
    """
    Test A2DP stream establish command with incorrect and correct parameters.
    """
    a2dp_sink_br_clear(sink, source)
    a2dp_sink_br_connect(sink, source)
    time.sleep(1)

    def test_establish(sink, source):
        logger.info("a2dp set incorrect acp_seid")
        sink.iexpect('a2dp_sink_test modify_acp_seid 5', 'set remote endpoint id successfully')

        logger.info("a2dp send stream establish cmd")
        sink.iexpect('a2dp_sink_test establish', 'fail to establish')

        logger.info("a2dp set correct acp_seid")
        sink.iexpect('a2dp_sink_test modify_acp_seid', 'set remote endpoint id successfully')

        logger.info("a2dp send stream establish cmd")
        sink.iexpect('a2dp_sink_test establish', 'success to establish')
        found, _ = source._wait_for_shell_response('receive requesting establishment and accept')
        assert found is True

    a2dp_sink_preparation(sink)
    test_establish(sink, source)

    sink.iexpect('a2dp_sink_test disconnect', 'a2dp disconnected')

    time.sleep(1)

    a2dp_sink_preparation(sink)
    test_establish(sink, source)
    sink.iexpect('bt disconnect', 'Disconnected')


def test_a2dp_sink_release_cmd(sink, source):
    """
    Test A2DP stream release command with incorrect and correct parameters.
    """
    a2dp_sink_br_clear(sink, source)
    a2dp_sink_br_connect(sink, source)
    time.sleep(1)

    def test_release(sink, source):
        logger.info("a2dp sink establish")
        sink.iexpect('a2dp_sink_test establish', 'success to establish')
        found, _ = source._wait_for_shell_response('receive requesting establishment and accept')
        assert found is True

        logger.info("a2dp set incorrect acp_seid")
        sink.iexpect('a2dp_sink_test modify_acp_seid 5', 'set remote endpoint id successfully')

        logger.info("a2dp send stream release cmd")
        sink.iexpect('a2dp_sink_test release', 'fail to release')

        logger.info("a2dp set correct acp_seid")
        sink.iexpect('a2dp_sink_test modify_acp_seid', 'set remote endpoint id successfully')

        logger.info("a2dp send stream release cmd")
        sink.iexpect('a2dp_sink_test release', 'success to release')
        found, _ = source._wait_for_shell_response('receive requesting release and accept')
        assert found is True

    a2dp_sink_preparation(sink)
    test_release(sink, source)
    sink.iexpect('a2dp_sink_test disconnect', 'a2dp disconnected')

    time.sleep(1)

    a2dp_sink_preparation(sink)
    test_release(sink, source)
    sink.iexpect('bt disconnect', 'Disconnected')


def test_a2dp_sink_start_cmd(sink, source):
    """
    Test A2DP stream start command with incorrect and correct parameters.
    """

    a2dp_sink_br_clear(sink, source)
    a2dp_sink_br_connect(sink, source)
    time.sleep(1)

    def test_start(sink, source):
        logger.info("a2dp sink establish")
        sink.iexpect('a2dp_sink_test establish', 'success to establish')
        found, _ = source._wait_for_shell_response('receive requesting establishment and accept')
        assert found is True

        logger.info("a2dp set incorrect acp_seid")
        sink.iexpect('a2dp_sink_test modify_acp_seid 5', 'set remote endpoint id successfully')

        logger.info("a2dp send stream start cmd")
        sink.iexpect('a2dp_sink_test start', 'fail to start')

        logger.info("a2dp set correct acp_seid")
        sink.iexpect('a2dp_sink_test modify_acp_seid', 'set remote endpoint id successfully')

        logger.info("a2dp send stream start cmd")
        sink.iexpect('a2dp_sink_test start', 'success to start')
        found, _ = source._wait_for_shell_response('receive requesting start and accept')
        assert found is True

    a2dp_sink_preparation(sink)
    test_start(sink, source)
    sink.iexpect('a2dp_sink_test disconnect', 'a2dp disconnected')

    time.sleep(1)

    a2dp_sink_preparation(sink)
    test_start(sink, source)
    sink.iexpect('bt disconnect', 'Disconnected')


def test_a2dp_sink_suspend_cmd(sink, source):
    """
    Test A2DP stream suspend command with incorrect and correct parameters.
    """
    a2dp_sink_br_clear(sink, source)
    a2dp_sink_br_connect(sink, source)
    time.sleep(1)

    def test_suspend(sink, source):
        logger.info("a2dp sink establish")
        sink.iexpect('a2dp_sink_test establish', 'success to establish')
        found, _ = source._wait_for_shell_response('receive requesting establishment and accept')
        assert found is True

        logger.info("a2dp send stream start cmd")
        sink.iexpect('a2dp_sink_test start', 'success to start')

        logger.info("a2dp set incorrect acp_seid")
        sink.iexpect('a2dp_sink_test modify_acp_seid 5', 'set remote endpoint id successfully')

        logger.info("a2dp send stream suspend cmd")
        sink.iexpect('a2dp_sink_test suspend', 'fail to suspend')

        logger.info("a2dp set correct acp_seid")
        sink.iexpect('a2dp_sink_test modify_acp_seid', 'set remote endpoint id successfully')

        logger.info("a2dp send stream suspend cmd")
        sink.iexpect('a2dp_sink_test suspend', 'success to suspend')
        found, _ = source._wait_for_shell_response("receive requesting suspend and accept")

    a2dp_sink_preparation(sink)
    test_suspend(sink, source)
    sink.iexpect('a2dp_sink_test disconnect', 'a2dp disconnected')

    time.sleep(1)
    a2dp_sink_preparation(sink)
    test_suspend(sink, source)

    sink.iexpect('bt disconnect', 'Disconnected')


def test_a2dp_sink_reconfigure_cmd(sink, source):
    """
    Test A2DP stream reconfigure command with incorrect and correct parameters.
    """
    a2dp_sink_br_clear(sink, source)
    a2dp_sink_br_connect(sink, source)
    time.sleep(1)

    def test_reconfigure(sink, source):
        logger.info("a2dp sink establish")
        sink.iexpect('a2dp_sink_test establish', 'success to establish')
        found, _ = source._wait_for_shell_response('receive requesting establishment and accept')
        assert found is True

        logger.info("a2dp send stream start cmd")
        sink.iexpect('a2dp_sink_test start', 'success to start')

        logger.info("a2dp send stream suspend cmd")
        sink.iexpect('a2dp_sink_test suspend', 'success to suspend')
        found, _ = source._wait_for_shell_response("receive requesting suspend and accept")

        logger.info("a2dp set correct configuration")
        sink.iexpect('a2dp_sink_test modify_sbc_config min_b 13', 'set configuration success')

        logger.info("a2dp send stream reconfigure cmd")
        sink.iexpect('a2dp_sink_test reconfigure', 'peer device fails to configure')

        logger.info("a2dp set correct configuration")
        sink.iexpect('a2dp_sink_test modify_sbc_config min_b 22', 'set configuration success')

        logger.info("a2dp send stream reconfigure cmd")
        sink.iexpect('a2dp_sink_test reconfigure', 'success to configure')

    a2dp_sink_preparation(sink)
    test_reconfigure(sink, source)
    sink.iexpect('a2dp_sink_test disconnect', 'a2dp disconnected')

    time.sleep(1)

    a2dp_sink_preparation(sink)
    test_reconfigure(sink, source)
    sink.iexpect('bt disconnect', 'Disconnected')


def test_a2dp_sink_abort_cmd(sink, source):
    """
    Test A2DP stream abort command with incorrect and correct parameters.
    """
    a2dp_sink_br_clear(sink, source)
    a2dp_sink_br_connect(sink, source)
    time.sleep(1)

    def test_abort(sink, source):
        logger.info("a2dp sink establish")
        sink.iexpect('a2dp_sink_test establish', 'success to establish')
        found, _ = source._wait_for_shell_response('receive requesting establishment and accept')
        assert found is True

        logger.info("a2dp set incorrect acp_seid")
        sink.iexpect('a2dp_sink_test modify_acp_seid 5', 'set remote endpoint id successfully')

        logger.info("a2dp send stream abort cmd")
        sink.iexpect('a2dp_sink_test abort', 'fail to abort')

        logger.info("a2dp set correct acp_seid")
        sink.iexpect('a2dp_sink_test modify_acp_seid', 'set remote endpoint id successfully')

        logger.info("a2dp send stream abort cmd")
        sink.iexpect('a2dp_sink_test abort', 'success to abort')
        found, _ = source._wait_for_shell_response('receive requesting abort and accept')
        assert found is True

    a2dp_sink_preparation(sink)
    test_abort(sink, source)
    sink.iexpect('a2dp_sink_test disconnect', 'a2dp disconnected')

    time.sleep(1)

    a2dp_sink_preparation(sink)
    test_abort(sink, source)
    sink.iexpect('bt disconnect', 'Disconnected')


def test_a2dp_sink_get_configuration_cmd(sink, source):
    """
    Test A2DP stream get configuration command with incorrect and correct parameters.
    """
    a2dp_sink_br_clear(sink, source)
    a2dp_sink_br_connect(sink, source)
    time.sleep(1)

    def test_get_configuration(sink, source):
        logger.info("a2dp set incorrect acp_seid")
        sink.iexpect('a2dp_sink_test modify_acp_seid 5', 'set remote endpoint id successfully')

        logger.info("a2dp send get configuration cmd")
        sink.iexpect('a2dp_sink_test get_config', 'fail to get config')

        logger.info("a2dp set correct acp_seid")
        sink.iexpect('a2dp_sink_test modify_acp_seid', 'set remote endpoint id successfully')

        logger.info("a2dp send stream get configuration cmd")
        sink.iexpect('a2dp_sink_test get_config', 'success to get config')
        found, _ = source._wait_for_shell_response('receive get config request and accept')
        assert found is True

    a2dp_sink_preparation(sink)
    test_get_configuration(sink, source)
    sink.iexpect('a2dp_sink_test disconnect', 'a2dp disconnected')

    time.sleep(1)

    a2dp_sink_preparation(sink)
    test_get_configuration(sink, source)
    sink.iexpect('bt disconnect', 'Disconnected')


def test_a2dp_sink_get_mtu(sink, source):
    """
    Test A2DP get mtu function.
    """
    a2dp_sink_br_clear(sink, source)
    a2dp_sink_br_connect(sink, source)
    time.sleep(1)

    def test_get_mtu(sink, source):
        logger.info("a2dp sink establish")
        sink.iexpect('a2dp_sink_test establish', 'success to establish')

        sink.iexpect('a2dp_sink_test get_mtu', 'mtu: 196', timeout=10)

    a2dp_sink_preparation(sink)
    test_get_mtu(sink, source)
    sink.iexpect('a2dp_sink_test disconnect', 'a2dp disconnected')
    time.sleep(1)

    a2dp_sink_preparation(sink)
    test_get_mtu(sink, source)
    sink.iexpect('bt disconnect', 'Disconnected')


def test_a2dp_sink_delay_report_cmd(sink, source):
    """
    Test A2DP send delay report function.
    """

    a2dp_sink_br_clear(sink, source)
    a2dp_sink_br_connect(sink, source)
    time.sleep(1)

    def test_delay_report(sink, source):
        logger.info("a2dp set incorrect acp_seid")
        sink.iexpect('a2dp_sink_test modify_acp_seid 5', 'set remote endpoint id successfully')

        logger.info("a2dp send send delay_report cmd")
        sink.iexpect('a2dp_sink_test send_delay_report 1', 'fail to send report delay')

        logger.info("a2dp set correct acp_seid")
        sink.iexpect('a2dp_sink_test modify_acp_seid', 'set remote endpoint id successfully')

        sink.iexpect(
            'a2dp_sink_test send_delay_report 1', 'success to send report delay', timeout=10
        )
        found, _ = source._wait_for_shell_response("received delay report: 1 1/10ms")
        assert found is True

    a2dp_sink_preparation(sink)
    test_delay_report(sink, source)

    sink.iexpect('a2dp_sink_test disconnect', 'a2dp disconnected')
    time.sleep(1)

    a2dp_sink_preparation(sink)
    test_delay_report(sink, source)
    sink.iexpect('bt disconnect', 'Disconnected')


def test_a2dp_sink_fragmentation(sink, source):
    """
    Test A2DP Fragmentation function.
    """
    a2dp_sink_br_clear(sink, source)
    a2dp_sink_br_connect(sink, source)
    time.sleep(1)

    logger.info("a2dp connect")
    sink.iexpect('a2dp_sink_test connect', 'a2dp connected', timeout=10)

    sink.iexpect('a2dp_sink_test modify_mtu 4', 'set mtu successfully')
    source.iexpect('a2dp_sink_test modify_mtu 4', 'set mtu successfully')

    logger.info("a2dp sink send discover cmd")
    sink.iexpect('a2dp_sink_test discover_peer_eps', 'Bitpool Range', timeout=10)

    logger.info("a2dp sink configure")
    sink.iexpect('a2dp_sink_test configure', 'success to configure')

    sink.iexpect('a2dp_sink_test modify_mtu', 'set mtu successfully')
    sink.iexpect('bt disconnect', 'Disconnected')


def test_a2dp_sink_random_procedure(sink, source):
    import random

    test_count = 50
    cmd_list = [
        'a2dp_sink_test configure',
        'a2dp_sink_test establish',
        'a2dp_sink_test reconfigure',
        'a2dp_sink_test release',
        'a2dp_sink_test start',
        'a2dp_sink_test suspend',
        'a2dp_sink_test abort',
        'a2dp_sink_test get_config',
        'a2dp_sink_test get_mtu',
        'a2dp_sink_test send_delay_report 1',
    ]

    sink.exec_command("br clear all")
    source.exec_command("br clear all")

    time.sleep(1)

    logger.info(f'acl connect {source.addr}')
    sink.iexpect(f'br connect {source.addr}', 'Connected', timeout=10)

    logger.info("a2dp connect")
    sink.iexpect('a2dp_sink_test connect', 'a2dp connected', timeout=10)

    logger.info("Preparation")

    logger.info("a2dp sink send discover cmd")
    sink.iexpect('a2dp_sink_test discover_peer_eps', 'Bitpool Range', timeout=10)

    for _ in range(test_count):
        sink.exec_command(random.choice(cmd_list))
        time.sleep(2)

    sink.iexpect('bt disconnect', 'Disconnected')
