#include "Url.h"

Url::Url(wstring u, wstring app, int vTime)
{
	url = u;
	visitTime = vTime;
	applicationName = app;
}

Url::~Url(void)
{
}