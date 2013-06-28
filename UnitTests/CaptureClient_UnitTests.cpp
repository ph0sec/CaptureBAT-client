#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>
#include <boost/test/execution_monitor.hpp>
using boost::unit_test_framework::test_suite;
using boost::execution_monitor;

#include "OptionsManager_UnitTests.h"
#include "ProcessManager_UnitTests.h"
#include "Logger_UnitTests.h"

test_suite* __cdecl init_unit_test_suite(int argc, char* argv[])
{
	test_suite*  top_test_suite = BOOST_TEST_SUITE( "Capture Client Test Suite" );

	top_test_suite->add(new OptionsManager_TestSuite());
	top_test_suite->add(new ProcessManager_TestSuite());
	top_test_suite->add(new Logger_TestSuite());

	return top_test_suite;
}
