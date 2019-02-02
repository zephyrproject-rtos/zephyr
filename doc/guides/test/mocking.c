#include <ztest.h>

static void expect_two_parameters(int a, int b)
{
	ztest_check_expected_value(a);
	ztest_check_expected_value(b);
}

static void parameter_tests(void)
{
	ztest_expect_value(expect_two_parameters, a, 2);
	ztest_expect_value(expect_two_parameters, b, 3);
	expect_two_parameters(2, 3);
}

static int returns_int(void)
{
	return ztest_get_return_value();
}

static void return_value_tests(void)
{
	ztest_returns_value(returns_int, 5);
	zassert_equal(returns_int(), 5, NULL);
}

void test_main(void)
{
	ztest_test_suite(mock_framework_tests,
		ztest_unit_test(parameter_test),
		ztest_unit_test(return_value_test)
	);

	ztest_run_test_suite(mock_framework_tests);
}
