#ifndef ____OMX_MALLOC_H____
#define ____OMX_MALLOC_H____
#include "al_libc.h"
void *omx_malloc_phy(int len, unsigned long *vir_addr);
void omx_free_phy(void *phy_addr);
void* omx_mmap_ion_fd(int ion_fd, int length);
int omx_munmap_ion_fd(void * vir_addr, int length);
int omx_getphy_from_vir(void *vir_addr);
void* omx_getvir_from_phy(void *phy_addr);

#endif
