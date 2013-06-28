#include <boost/test/unit_test.hpp>
using boost::unit_test_framework::test_suite;
using boost::unit_test_framework::test_case;
using boost::unit_test::test_suite;
#include "..\OptionsManager.h"


bool OptionsManager::instanceCreated = false;
OptionsManager* OptionsManager::optionsManager = NULL;

class OptionsManager_Tests
{
   public:
	   void addOption()
	   {
		   OptionsManager::getInstance()->addOption(L"hello", L"world");
		   wstring value = OptionsManager::getInstance()->getOption(L"hello");

		   BOOST_CHECK(value == L"world");
		   delete OptionsManager::getInstance();
	   }
};

class OptionsManager_TestSuite : public test_suite
{
   public:

   OptionsManager_TestSuite() : test_suite("OptionsManager Test Suite")
   {
      boost::shared_ptr<OptionsManager_Tests> instance(new OptionsManager_Tests());

	  test_case* addOption_TestCase = BOOST_CLASS_TEST_CASE(&OptionsManager_Tests::addOption, instance);
	  add(addOption_TestCase);
   }
};

