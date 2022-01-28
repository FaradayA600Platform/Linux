#ifndef __PLAT_SD_H
#define __PLAT_SD_H

#if defined(CONFIG_MACH_LEO)
extern void platform_sdhci_wait_dll_lock(void);
#else
void platform_sdhci_wait_dll_lock(void) {}
#endif

#endif
