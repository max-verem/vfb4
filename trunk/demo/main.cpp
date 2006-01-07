#include "../include/vfb4-data.h"
#include <windows.h>
#include <stdio.h>

static vfb4_data vfb4_info;

static char* str = "Hellow World!";

int main(void)
{
	unsigned long ret;
	HANDLE vdf_dev;
	
	// open device
	if (!(vdf_dev = CreateFile(VFB4_API_DEV, GENERIC_READ | GENERIC_WRITE | FILE_FLAG_NO_BUFFERING, 0, NULL, OPEN_EXISTING, 0, NULL)))
	{
		printf("Error opening '%s'. Please see vfb4.txt\n",VFB4_API_DEV);
		return -1;
	};

	// read data
	if(!ReadFile(vdf_dev, &vfb4_info, sizeof(vfb4_data), &ret, NULL))
	{
		printf("Error reading '%s'. Please see vfb4.txt\n",VFB4_API_DEV,GetLastError());
		return -1;
	};

	// smoke test !!!
	memcpy((void*)vfb4_info.user,str,strlen(str) + 1);

	return 0;
};