	ztest_test_suite(test_api,
			 ztest_1cpu_unit_test(test_a), /* comment! */
			 ztest_1cpu_unit_test(test_b),
			 ztest_1cpu_unit_test(test_c),
			 ztest_unit_test(test_unit_a),
			 ztest_unit_test(test_unit_b),
			 /* random comment */
			 ztest_1cpu_unit_test(
				test_newline),
			 ztest_1cpu_unit_test(test_test_test_aa),
			 ztest_user_unit_test(test_user),
			 ztest_1cpu_unit_test(test_last));
	ztest_run_test_suite(test_api);
