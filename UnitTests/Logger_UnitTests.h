#include <boost/test/unit_test.hpp>
using boost::unit_test_framework::test_suite;
using boost::unit_test_framework::test_case;
using boost::unit_test::test_suite;
#include "..\Logger.h"

bool Logger::instanceCreated = false;
Logger* Logger::logger = NULL;

class Logger_Tests
{
public:
	void openLogFile()
	{
		Logger::getInstance()->openLogFile(L"test.txt");
		BOOST_CHECK(Logger::getInstance()->isFileOpen() == true);
		Logger::getInstance()->closeLogFile();
		delete Logger::getInstance();
		DeleteFile(L"test.txt");
	}

	void writeToLog()
	{
		BOOST_CHECK(true == false);
	}

	void closeLogFile()
	{
		Logger::getInstance()->openLogFile(L"test.txt");	
		Logger::getInstance()->closeLogFile();
		BOOST_CHECK(Logger::getInstance()->isFileOpen() == false);
		delete Logger::getInstance();
		DeleteFile(L"test.txt");
	}

	void getLogFileName()
	{
		Logger::getInstance()->openLogFile(L"test.txt");	
		Logger::getInstance()->closeLogFile();
		wchar_t currentDirectory[1024];
		GetCurrentDirectory(1024, currentDirectory);
		wstring actualLogFileName = currentDirectory;
		actualLogFileName += L"\\logs\\test.txt";
		transform(actualLogFileName.begin(), actualLogFileName.end(), actualLogFileName.begin(), tolower);
		BOOST_CHECK( Logger::getInstance()->getLogFileName() == actualLogFileName);
		delete Logger::getInstance();
		DeleteFile(L"test.txt");
	}
};

class Logger_TestSuite : public test_suite
{
   public:

	Logger_TestSuite() : test_suite("Logger Test Suite")
	{
		boost::shared_ptr<Logger_Tests> instance(new Logger_Tests());

		test_case* openLogFile = BOOST_CLASS_TEST_CASE(&Logger_Tests::openLogFile, instance);
		test_case* writeToLog = BOOST_CLASS_TEST_CASE(&Logger_Tests::writeToLog, instance);
		test_case* closeLogFile = BOOST_CLASS_TEST_CASE(&Logger_Tests::closeLogFile, instance);
		test_case* getLogFileName = BOOST_CLASS_TEST_CASE(&Logger_Tests::getLogFileName, instance);

		add(openLogFile);
		add(writeToLog);
		add(closeLogFile);
		add(getLogFileName);
   }
};