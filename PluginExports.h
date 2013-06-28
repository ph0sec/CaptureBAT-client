#ifdef APPLICATION_PLUGIN_NAME
extern "C" {
	__declspec(dllexport) void New(void** clsPtr)
	{ 
		*clsPtr = new APPLICATION_PLUGIN_NAME(); 
	}
	__declspec(dllexport) void Delete(void** clsPtr)
	{ 
		delete (APPLICATION_PLUGIN_NAME*)*clsPtr; 
	}
}
#endif