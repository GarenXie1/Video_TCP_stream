#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <sys/mman.h>
#include "omx_malloc.h"

void *omx_malloc_phy(int len, unsigned long *vir_addr)
{
	unsigned long phy_addr = 0;
	int newlen = (len + 4095) & (~4095);

	*vir_addr = (unsigned long) actal_malloc_uncache(newlen, (int *) (&phy_addr));
	if (*vir_addr == 0)
	{
		printf("malloc ion addr err %lx\n", phy_addr);
		return NULL;
	}
	memset((void*) (*vir_addr), 0, newlen);

	printf("malloc ion addr! phy:%lx,vir:%lx,len:%x\n", phy_addr, *vir_addr, len);
	return (void*) phy_addr;
}

void omx_free_phy(void *phy_addr)
{
	void*vir_addr = (void*) actal_get_virtaddr((long) phy_addr);
	if (vir_addr != 0)
		actal_free_uncache(vir_addr);
	printf("free ion phyaddr!phy:%p vir:%p\n", phy_addr, vir_addr);
}

void* omx_mmap_ion_fd(int ion_fd, int length)
{
	void* vir_addr = NULL;
	vir_addr = mmap(0, length, PROT_READ | PROT_WRITE, MAP_SHARED, ion_fd, 0);
	printf("get vir from ion fd!fd:%x vir:%p,len:%x\n", ion_fd, vir_addr, length);
	return vir_addr;
}

int omx_munmap_ion_fd(void * vir_addr, int length)
{
	int ret = 0;
	printf("munmap ion fd!vir:%p,len:%x\n", vir_addr, length);
	ret = munmap(vir_addr, length);
	if(ret != 0)
		printf("err!munmap fail,ret:%d!%s,%d\n", ret, __FILE__, __LINE__);
	return ret;
}

int omx_getphy_from_vir(void *vir_addr)
{
	printf("test,get phy from vir! %lx\n", (unsigned long) vir_addr);
	return actal_get_phyaddr(vir_addr);
}

void* omx_getvir_from_phy(void *phy_addr)
{
	void *vir_addr = (void*) actal_get_virtaddr((long) phy_addr);
	printf("test,get vir from phy!phy:%lx vir:%lx\n", (long) phy_addr, (unsigned long) vir_addr);
	return vir_addr;
}
