#ifndef _vfb4_data_h
#define _vfb4_data_h

#define VFB4_API_DEV "\\\\.\\vfb4"

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct _vfb4_data
	{
		unsigned long physical_high;
		unsigned long physical_low;
		unsigned long user;
		unsigned long kernel;
	} vfb4_data;

#ifdef __cplusplus
}
#endif


#endif // _vfb4_data_h
