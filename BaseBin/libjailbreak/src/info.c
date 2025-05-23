#include "info.h"
#include "kernel.h"
#include "machine_info.h"
#include "primitives.h"
#include <sys/utsname.h>
#include <xpc/xpc.h>
#include <sys/types.h>
#include <sys/sysctl.h>

struct system_info gSystemInfo = { 0 };

void jbinfo_initialize_dynamic_offsets(xpc_object_t xoffsetDict)
{
	SYSTEM_INFO_DESERIALIZE(xoffsetDict);
}

void jbinfo_initialize_hardcoded_offsets(void)
{
	struct utsname name;
	uname(&name);
	char *xnuVersion = name.release;

	cpu_subtype_t cpuFamily = 0;
	size_t cpuFamilySize = sizeof(cpuFamily);
	sysctlbyname("hw.cpufamily", &cpuFamily, &cpuFamilySize, NULL, 0);

	bool hasJitbox = (cpuFamily == CPUFAMILY_ARM_BLIZZARD_AVALANCHE || // A15
					  cpuFamily == CPUFAMILY_ARM_EVEREST_SAWTOOTH || // A16
					  cpuFamily == CPUFAMILY_ARM_COLL); // A17

	uint32_t taskJitboxAdjust = 0x0;
	if (hasJitbox) {
		taskJitboxAdjust = 0x10;
		if (strcmp(xnuVersion, "22.0.0") >= 0) {
			// In iOS 16, there is a new jitbox related attribute
			taskJitboxAdjust = 0x18;
		}
	}

	uint32_t pmapEl2Adjust = ((kconstant(kernel_el) == 2) ? 8 : 0);

#ifndef __arm64e__
	uint32_t pmapA11Adjust = 0;
	if (cpuFamily == CPUFAMILY_ARM_MONSOON_MISTRAL) {
		if (strcmp(xnuVersion, "21.0.0") >= 0) { // iOS 15+
			pmapA11Adjust = 1;
			if (strcmp(xnuVersion, "22.0.0") >= 0) { // iOS 16+
				pmapA11Adjust = 2;
			}
		}
	}
#endif

	// proc
	gSystemInfo.kernelStruct.proc.list_next =  0x0;
	gSystemInfo.kernelStruct.proc.list_prev =  0x8;
	gSystemInfo.kernelStruct.proc.task      = 0x10;
	gSystemInfo.kernelStruct.proc.pptr      = 0x18;
	gSystemInfo.kernelStruct.proc.pid       = 0x68;

	// filedesc
	gSystemInfo.kernelStruct.filedesc.ofiles_start = 0x20;

    // file
    gSystemInfo.kernelStruct.fileproc.fileglob = 0x10;
    gSystemInfo.kernelStruct.fileglob.vnode = 0x38;

    // file
    gSystemInfo.kernelStruct.fileproc.fileglob = 0x10;
    gSystemInfo.kernelStruct.fileglob.vnode = 0x38;

    // vnode
    gSystemInfo.kernelStruct.vnode.id = 0x74;
    gSystemInfo.kernelStruct.vnode.usecount = 0x60;
    gSystemInfo.kernelStruct.vnode.ncchildren.tqh_first = 0x30;
    gSystemInfo.kernelStruct.vnode.ncchildren.tqh_last = 0x38;
    gSystemInfo.kernelStruct.vnode.parent = 0xc0;
    gSystemInfo.kernelStruct.vnode.nclinks.lh_first = 0x40;

    // namecache
    gSystemInfo.kernelStruct.namecache.smr = false;
    gSystemInfo.kernelStruct.namecache.child.tqe_next = 0x10;
    gSystemInfo.kernelStruct.namecache.child.tqe_prev = 0x18;
    gSystemInfo.kernelStruct.namecache.hash.le_next = 0x30;
    gSystemInfo.kernelStruct.namecache.hash.le_prev = 0x38;
    gSystemInfo.kernelStruct.namecache.dvp = 0x40;
    gSystemInfo.kernelStruct.namecache.vp = 0x48;
    gSystemInfo.kernelStruct.namecache.hashval = 0x50;
    gSystemInfo.kernelStruct.namecache.name = 0x58;

	// task
	gSystemInfo.kernelStruct.task.map     = 0x28;
	gSystemInfo.kernelStruct.task.threads = 0x60;

	// ipc_space
	gSystemInfo.kernelStruct.ipc_space.table          = 0x20;
	gSystemInfo.kernelStruct.ipc_space.table_uses_smr = false;

	// ipc_entry
	gSystemInfo.kernelStruct.ipc_entry.object      = 0x0;
	gSystemInfo.kernelStruct.ipc_entry.struct_size = 0x18;

	// vm_map
	gSystemInfo.kernelStruct.vm_map.hdr = 0x10;

	// pmap
	gSystemInfo.kernelStruct.pmap.tte        = 0x0;
	gSystemInfo.kernelStruct.pmap.ttep       = 0x8;
#ifdef __arm64e__
	gSystemInfo.kernelStruct.pmap.pmap_cs_main = 0x90;
	gSystemInfo.kernelStruct.pmap.sw_asid      = 0xBE + pmapEl2Adjust;
	gSystemInfo.kernelStruct.pmap.wx_allowed   = 0xC2 + pmapEl2Adjust;
	gSystemInfo.kernelStruct.pmap.type         = 0xC8 + pmapEl2Adjust;
#else
	gSystemInfo.kernelStruct.pmap.sw_asid    = 0x96;
	gSystemInfo.kernelStruct.pmap.wx_allowed = 0;
	gSystemInfo.kernelStruct.pmap.type       = 0x9c + pmapA11Adjust;
#endif

#ifdef __arm64e__
	// pmap_cs_region
	gSystemInfo.kernelStruct.pmap_cs_region.pmap_cs_region_next = 0x0;
	gSystemInfo.kernelStruct.pmap_cs_region.cd_entry            = 0x28;

	// pmap_cs_code_directory
	gSystemInfo.kernelStruct.pmap_cs_code_directory.pmap_cs_code_directory_next = 0x0;
	gSystemInfo.kernelStruct.pmap_cs_code_directory.main_binary                 = 0x50;
	gSystemInfo.kernelStruct.pmap_cs_code_directory.trust                       = 0x9C;
#endif

	// pt_desc
	gSystemInfo.kernelStruct.pt_desc.pmap     = 0x10;
	gSystemInfo.kernelStruct.pt_desc.va       = 0x18;
	gSystemInfo.kernelStruct.pt_desc.ptd_info = koffsetof(pt_desc, va) + (kconstant(PT_INDEX_MAX) * sizeof(uint64_t));

	// vm_map_header
	gSystemInfo.kernelStruct.vm_map_header.links    =  0x0;

	// vm_map_entry
	gSystemInfo.kernelStruct.vm_map_entry.links = 0x0;
	gSystemInfo.kernelStruct.vm_map_entry.flags = 0x48;

	// vm_map_links
	gSystemInfo.kernelStruct.vm_map_links.prev =  0x0;
	gSystemInfo.kernelStruct.vm_map_links.next =  0x8;
	gSystemInfo.kernelStruct.vm_map_links.min  = 0x10;
	gSystemInfo.kernelStruct.vm_map_links.max  = 0x18;

	// ucred
	uint32_t ucred_cr_posix = 0x18;
	gSystemInfo.kernelStruct.ucred.uid    = ucred_cr_posix +  0x0;
	gSystemInfo.kernelStruct.ucred.ruid   = ucred_cr_posix +  0x4;
	gSystemInfo.kernelStruct.ucred.svuid  = ucred_cr_posix +  0x8;
	gSystemInfo.kernelStruct.ucred.groups = ucred_cr_posix + 0x10;
	gSystemInfo.kernelStruct.ucred.rgid   = ucred_cr_posix + 0x50;
	gSystemInfo.kernelStruct.ucred.svgid  = ucred_cr_posix + 0x54;
	gSystemInfo.kernelStruct.ucred.label  = 0x78;

	if (strcmp(xnuVersion, "21.0.0") >= 0) { // iOS 15+
		// proc
		gSystemInfo.kernelStruct.proc.svuid   =  0x3C;
		gSystemInfo.kernelStruct.proc.svgid   =  0x40;
		gSystemInfo.kernelStruct.proc.ucred   =  0xD8;
		gSystemInfo.kernelStruct.proc.fd      =  0xE0;
		gSystemInfo.kernelStruct.proc.flag    = 0x1BC;
		gSystemInfo.kernelStruct.proc.textvp  = 0x2A8;
		gSystemInfo.kernelStruct.proc.csflags = 0x300;

		// task
#ifdef __arm64e__
		gSystemInfo.kernelStruct.task.task_can_transfer_memory_ownership = 0x5B0 + taskJitboxAdjust;
		gSystemInfo.kernelStruct.task.extmod_statistics = 0x5B0 - 0x128 + taskJitboxAdjust;
#else
		gSystemInfo.kernelStruct.task.task_can_transfer_memory_ownership = 0x590;
		gSystemInfo.kernelStruct.task.extmod_statistics = 0x590 - 0x128;
#endif

		// ipc_port
		gSystemInfo.kernelStruct.ipc_port.kobject = 0x58;

		// vm_map
		gSystemInfo.kernelStruct.vm_map.flags = 0x11C;

		// trustcache
		gSystemInfo.kernelStruct.trustcache.nextptr        =  0x0;
		gSystemInfo.kernelStruct.trustcache.fileptr        =  0x8;
		gSystemInfo.kernelStruct.trustcache.struct_size = 0x10;

		if (strcmp(xnuVersion, "21.2.0") >= 0) { // iOS 15.2+
			// proc
			gSystemInfo.kernelStruct.proc.ucred   =   0x0; // Moved to proc_ro
			gSystemInfo.kernelStruct.proc.csflags =   0x0; // Moved to proc_ro
			gSystemInfo.kernelStruct.proc.proc_ro =  0x20;
			gSystemInfo.kernelStruct.proc.svuid   =  0x44;
			gSystemInfo.kernelStruct.proc.svgid   =  0x48;
			gSystemInfo.kernelStruct.proc.fd      =  0xD8;
			gSystemInfo.kernelStruct.proc.flag    = 0x264;
			gSystemInfo.kernelStruct.proc.textvp  = 0x358;

			// proc_ro
			gSystemInfo.kernelStruct.proc_ro.exists  = true;
			gSystemInfo.kernelStruct.proc_ro.csflags = 0x1C;
			gSystemInfo.kernelStruct.proc_ro.ucred   = 0x20;

			// task
#ifdef __arm64e__
			gSystemInfo.kernelStruct.task.task_can_transfer_memory_ownership = 0x580 + taskJitboxAdjust;
			gSystemInfo.kernelStruct.task.extmod_statistics = 0x580 - 0x128 + taskJitboxAdjust;
#else
			gSystemInfo.kernelStruct.task.task_can_transfer_memory_ownership = 0x560;
			gSystemInfo.kernelStruct.task.extmod_statistics = 0x560 - 0x128;
#endif
			if (strcmp(xnuVersion, "21.4.0") >= 0) { // iOS 15.4+
				// proc
				gSystemInfo.kernelStruct.proc.textvp = 0x350;

				// vm_map
				gSystemInfo.kernelStruct.vm_map.flags = 0x94;

				// ipc_port
				gSystemInfo.kernelStruct.ipc_port.kobject = 0x48;

				if (strcmp(xnuVersion, "22.0.0") >= 0) { // iOS 16+
					gSystemInfo.kernelConstant.smrBase = 3;

					// proc
					gSystemInfo.kernelStruct.proc.task    =   0x0; // Removed, task is now at (proc + sizeof(proc))
					gSystemInfo.kernelStruct.proc.pptr    =  0x10;
					gSystemInfo.kernelStruct.proc.proc_ro =  0x18;
					gSystemInfo.kernelStruct.proc.svuid   =  0x3C;
					gSystemInfo.kernelStruct.proc.svgid   =  0x40;
					gSystemInfo.kernelStruct.proc.pid     =  0x60;
					gSystemInfo.kernelStruct.proc.fd      =  0xD8;
					gSystemInfo.kernelStruct.proc.flag    = 0x25C;
					gSystemInfo.kernelStruct.proc.textvp  = 0x350;

					gSystemInfo.kernelStruct.proc_ro.syscall_filter_mask = 0x28;
					gSystemInfo.kernelStruct.proc_ro.mach_trap_filter_mask = 0x68;
					gSystemInfo.kernelStruct.proc_ro.mach_kobj_filter_mask = 0x70;

					// task
#ifdef __arm64e__
					gSystemInfo.kernelStruct.task.task_can_transfer_memory_ownership = 0x548 + taskJitboxAdjust;
#else
					gSystemInfo.kernelStruct.task.task_can_transfer_memory_ownership = 0x528;
#endif
					// vm_map
					gSystemInfo.kernelStruct.vm_map.flags = 0xB4;

					// trustcache
					gSystemInfo.kernelStruct.trustcache.nextptr = 0x0;
					gSystemInfo.kernelStruct.trustcache.prevptr = 0x8;
					gSystemInfo.kernelStruct.trustcache.size = 0x18;
					gSystemInfo.kernelStruct.trustcache.fileptr = 0x20;
					gSystemInfo.kernelStruct.trustcache.struct_size = 0x28;

					// pmap
#ifdef __arm64e__
					gSystemInfo.kernelStruct.pmap.sw_asid    = 0xB6 + pmapEl2Adjust;
					gSystemInfo.kernelStruct.pmap.wx_allowed = 0xBA + pmapEl2Adjust;
					gSystemInfo.kernelStruct.pmap.type       = 0xC0 + pmapEl2Adjust;
#else
					gSystemInfo.kernelStruct.pmap.sw_asid    = 0x8e;
					gSystemInfo.kernelStruct.pmap.wx_allowed = 0;
					gSystemInfo.kernelStruct.pmap.type       = 0x94 + pmapA11Adjust;
#endif

#ifdef __arm64e__
					// pmap_cs_code_directory
					gSystemInfo.kernelStruct.pmap_cs_code_directory.main_binary = 0x190;
					gSystemInfo.kernelStruct.pmap_cs_code_directory.trust       = 0x1DC;
#endif

					if (strcmp(xnuVersion, "22.1.0") >= 0) { // iOS 16.1+
						gSystemInfo.kernelStruct.ipc_space.table_uses_smr = true;
						if (strcmp(xnuVersion, "22.3.0") >= 0) { // iOS 16.3+
							gSystemInfo.kernelConstant.smrBase = 2;
							if (strcmp(xnuVersion, "22.4.0") >= 0) { // iOS 16.4+
                                // namecache
                                gSystemInfo.kernelStruct.namecache.smr = true;
                                gSystemInfo.kernelStruct.namecache.dvp = 0x48;
                                gSystemInfo.kernelStruct.namecache.vp = 0x50;
                                gSystemInfo.kernelStruct.namecache.hashval = 0x58;
                                gSystemInfo.kernelStruct.namecache.name = 0x60;

								// proc
								gSystemInfo.kernelStruct.proc.flag   = 0x454;
								gSystemInfo.kernelStruct.proc.textvp = 0x548;

#ifdef __arm64e__
								// pmap_cs_code_directory
								gSystemInfo.kernelStruct.pmap_cs_code_directory.trust = 0x1EC;
#endif

								if (strcmp(xnuVersion, "22.4.0") == 0) { // iOS 16.4 ONLY 
									// iOS 16.4 beta 1-3 use the old proc struct, 16.4b4+ use new
									if (gSystemInfo.kernelStruct.proc.struct_size != 0x730) {
										gSystemInfo.kernelStruct.proc.flag    = 0x25C;
										gSystemInfo.kernelStruct.proc.textvp  = 0x350;
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

void jbinfo_initialize_boot_constants(void)
{
	gSystemInfo.kernelConstant.base     = kconstant(staticBase) + gSystemInfo.kernelConstant.slide;
	gSystemInfo.kernelConstant.virtBase = kread64(ksymbol(gVirtBase));
	//gSystemInfo.kernelConstant.virtSize = ...;
	gSystemInfo.kernelConstant.physBase = kread64(ksymbol(gPhysBase));
	gSystemInfo.kernelConstant.physSize = kread64(ksymbol(gPhysSize));
	gSystemInfo.kernelConstant.cpuTTEP  = kread64(ksymbol(cpu_ttep));
}

xpc_object_t jbinfo_get_serialized(void)
{
	xpc_object_t systemInfo = xpc_dictionary_create_empty();
	SYSTEM_INFO_SERIALIZE(systemInfo);
	return systemInfo;
}

uint64_t get_vm_real_kernel_page_size(void)
{
	static uint64_t real_kernel_page_size = 0;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		real_kernel_page_size = vm_kernel_page_size;

		// vm_kernel_page_size is WRONG on A8X
		// Screw Apple
		cpu_subtype_t cpuFamily = 0;
		size_t cpuFamilySize = sizeof(cpuFamily);
		sysctlbyname("hw.cpufamily", &cpuFamily, &cpuFamilySize, NULL, 0);
		if (cpuFamily == CPUFAMILY_ARM_TYPHOON) {
			real_kernel_page_size = 0x1000;
		}
	});
	return real_kernel_page_size;
}


uint64_t get_vm_real_kernel_page_shift(void)
{
	static uint64_t real_kernel_page_shift = 0;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		real_kernel_page_shift = vm_kernel_page_shift;

		// vm_kernel_page_shift is WRONG on A8X
		// Screw Apple
		cpu_subtype_t cpuFamily = 0;
		size_t cpuFamilySize = sizeof(cpuFamily);
		sysctlbyname("hw.cpufamily", &cpuFamily, &cpuFamilySize, NULL, 0);
		if (cpuFamily == CPUFAMILY_ARM_TYPHOON) {
			real_kernel_page_shift = 14;
		}
	});
	return real_kernel_page_shift;
}

uint64_t get_l1_block_size(void)
{
	switch (vm_real_kernel_page_size) {
		case 0x4000:
		return 0x1000000000;
		case 0x1000:
		return 0x40000000;
		default:
		return 0;
	}
}

uint64_t get_l1_block_mask(void)
{
	return get_l1_block_size() - 1;
}

uint64_t get_l1_block_count(void)
{
	switch (vm_real_kernel_page_size) {
		case 0x4000:
		return 8;
		case 0x1000:
		return 256;
		default:
		return 0;
	}
}

uint64_t get_l2_block_size(void)
{
	switch (vm_real_kernel_page_size) {
		case 0x4000:
		return 0x2000000;
		case 0x1000:
		return 0x200000;
		default:
		return 0;
	}
}

uint64_t get_l2_block_mask(void)
{
	return get_l2_block_size() - 1;
}

uint64_t get_l2_block_count(void)
{
	switch (vm_real_kernel_page_size) {
		case 0x4000:
		return 2048;
		case 0x1000:
		return 512;
		default:
		return 0;
	}
}
