#ifndef _VX_DLIMIT_H
#define _VX_DLIMIT_H

#include "virtual.h"
// #include "switch.h"

/*  inode vserver commands */

#define VCMD_add_dlimit		VC_CMD(DLIMIT, 1, 0)
#define VCMD_rem_dlimit		VC_CMD(DLIMIT, 2, 0)

#define VCMD_set_dlimit		VC_CMD(DLIMIT, 5, 0)
#define VCMD_get_dlimit		VC_CMD(DLIMIT, 6, 0)


struct  vcmd_ctx_dlimit_base_v0 {
	char *name;
	uint32_t flags;
};

struct  vcmd_ctx_dlimit_v0 {
	char *name;
	uint32_t space_used;			/* used space in kbytes */
	uint32_t space_total;			/* maximum space in kbytes */
	uint32_t inodes_used;			/* used inodes */
	uint32_t inodes_total;			/* maximum inodes */
	uint32_t reserved;			/* reserved for root in % */
	uint32_t flags;
};

#define CDLIM_UNSET             (0ULL)
#define CDLIM_INFINITY          (~0ULL)
#define CDLIM_KEEP              (~1ULL)


#ifdef	__KERNEL__

struct super_block;

struct dl_info {
	struct hlist_node dl_hlist;		/* linked list of contexts */
	struct rcu_head dl_rcu;			/* the rcu head */
	xid_t dl_xid;				/* context id */
	atomic_t dl_usecnt;			/* usage count */
	atomic_t dl_refcnt;			/* reference count */

	struct super_block *dl_sb;		/* associated superblock */

	struct rw_semaphore dl_sem;		/* protect the values */

	uint64_t dl_space_used;			/* used space in bytes */
	uint64_t dl_space_total;		/* maximum space in bytes */
	uint32_t dl_inodes_used;		/* used inodes */
	uint32_t dl_inodes_total;		/* maximum inodes */

	unsigned int dl_nrlmult;		/* non root limit mult */
};

extern void rcu_free_dl_info(void *);
extern void unhash_dl_info(struct dl_info *);

struct kstatfs;

extern void vx_vsi_statfs(struct super_block *, struct kstatfs *);


extern int vc_add_dlimit(uint32_t, void __user *);
extern int vc_rem_dlimit(uint32_t, void __user *);

extern int vc_set_dlimit(uint32_t, void __user *);
extern int vc_get_dlimit(uint32_t, void __user *);


#endif	/* __KERNEL__ */

#endif	/* _VX_DLIMIT_H */
