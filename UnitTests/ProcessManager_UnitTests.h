#include <boost/test/unit_test.hpp>
using boost::unit_test_framework::test_suite;
using boost::unit_test_framework::test_case;
using boost::unit_test::test_suite;
#include "..\ProcessManager.h"

bool ProcessManager::instanceCreated = false;
ProcessManager* ProcessManager::processManager = NULL;

class ProcessManager_Tests
{
public:
	void getProcessPath()
	{
		wstring pathToCapture = ProcessManager::getInstance()->getProcessPath(GetCurrentProcessId());
		wchar_t szTemp[1024];
		GetCurrentDirectory(1024, szTemp);
		wstring realPathToCapture = szTemp;
		realPathToCapture += L"\\CaptureClient_UnitTests.exe";
		BOOST_CHECK(pathToCapture == realPathToCapture);
		delete ProcessManager::getInstance();
		
	}

	void getProcessModuleName()
	{
		wstring captureModuleName = ProcessManager::getInstance()->getProcessModuleName(GetCurrentProcessId());
		wstring realCaptureModuleName = L"CaptureClient_UnitTests.exe";
		BOOST_CHECK(captureModuleName == realCaptureModuleName);
		delete ProcessManager::getInstance();
	}

	void onProcessEvent()
	{
		wstring fakeProcessPath = L"c:\hello.exe";
		ProcessManager::getInstance()->onProcessEvent(true, L"<UNKNOWN>", 0, L"<UNKNOWN>", 123222, L"\\Device\\HarddiskVolume0\\hello.exe");
		wstring processPath = ProcessManager::getInstance()->getProcessPath(123222);
		BOOST_CHECK(fakeProcessPath == processPath);
		delete ProcessManager::getInstance();
	}
};

class ProcessManager_TestSuite : public test_suite
{
   public:

	ProcessManager_TestSuite() : test_suite("ProcessManager Test Suite")
	{
		boost::shared_ptr<ProcessManager_Tests> instance(new ProcessManager_Tests());

		test_case* getProcessPath = BOOST_CLASS_TEST_CASE(&ProcessManager_Tests::getProcessPath, instance);
		test_case* getProcessModuleName = BOOST_CLASS_TEST_CASE(&ProcessManager_Tests::getProcessModuleName, instance);
		test_case* onProcessEvent = BOOST_CLASS_TEST_CASE(&ProcessManager_Tests::onProcessEvent, instance);
 
		add(getProcessPath);
		add(getProcessModuleName);
		add(onProcessEvent);
   }
};