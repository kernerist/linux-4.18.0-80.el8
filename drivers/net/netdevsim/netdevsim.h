/*
 * Copyright (C) 2017 Netronome Systems, Inc.
 *
 * This software is licensed under the GNU General License Version 2,
 * June 1991 as shown in the file COPYING in the top-level directory of this
 * source tree.
 *
 * THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE PROGRAM "AS IS"
 * WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE. THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE
 * OF THE PROGRAM IS WITH YOU. SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME
 * THE COST OF ALL NECESSARY SERVICING, REPAIR OR CORRECTION.
 */

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/netdevice.h>
#include <linux/u64_stats_sync.h>
#include <net/xdp.h>

#define DRV_NAME	"netdevsim"

#define NSIM_XDP_MAX_MTU	4000

#define NSIM_EA(extack, msg)	NL_SET_ERR_MSG_MOD((extack), msg)

struct bpf_prog;
struct dentry;
struct nsim_vf_config;

struct netdevsim {
	struct net_device *netdev;

	u64 tx_packets;
	u64 tx_bytes;
	struct u64_stats_sync syncp;

	struct device dev;

	struct dentry *ddir;

	unsigned int num_vfs;
	struct nsim_vf_config *vfconfigs;

	struct bpf_prog	*bpf_offloaded;
	u32 bpf_offloaded_id;

	struct xdp_attachment_info xdp;
	struct xdp_attachment_info xdp_hw;

	u32 prog_id_gen;

	bool bpf_bind_accept;
	u32 bpf_bind_verifier_delay;
	struct dentry *ddir_bpf_bound_progs;
	struct list_head bpf_bound_progs;

	bool bpf_tc_accept;
	bool bpf_tc_non_bound_accept;
	bool bpf_xdpdrv_accept;
	bool bpf_xdpoffload_accept;

	bool bpf_map_accept;
	struct list_head bpf_bound_maps;
#if IS_ENABLED(CONFIG_NET_DEVLINK)
	struct devlink *devlink;
#endif
};

extern struct dentry *nsim_ddir;

#ifdef CONFIG_BPF_SYSCALL
int nsim_bpf_init(struct netdevsim *ns);
void nsim_bpf_uninit(struct netdevsim *ns);
int nsim_bpf(struct net_device *dev, struct netdev_bpf *bpf);
int nsim_bpf_disable_tc(struct netdevsim *ns);
int nsim_bpf_setup_tc_block_cb(enum tc_setup_type type,
			       void *type_data, void *cb_priv);
#else
static inline int nsim_bpf_init(struct netdevsim *ns)
{
	return 0;
}

static inline void nsim_bpf_uninit(struct netdevsim *ns)
{
}

static inline int nsim_bpf(struct net_device *dev, struct netdev_bpf *bpf)
{
	return bpf->command == XDP_QUERY_PROG ? 0 : -EOPNOTSUPP;
}

static inline int nsim_bpf_disable_tc(struct netdevsim *ns)
{
	return 0;
}

static inline int
nsim_bpf_setup_tc_block_cb(enum tc_setup_type type, void *type_data,
			   void *cb_priv)
{
	return -EOPNOTSUPP;
}
#endif

#if IS_ENABLED(CONFIG_NET_DEVLINK)
enum nsim_resource_id {
	NSIM_RESOURCE_NONE,   /* DEVLINK_RESOURCE_ID_PARENT_TOP */
	NSIM_RESOURCE_IPV4,
	NSIM_RESOURCE_IPV4_FIB,
	NSIM_RESOURCE_IPV4_FIB_RULES,
	NSIM_RESOURCE_IPV6,
	NSIM_RESOURCE_IPV6_FIB,
	NSIM_RESOURCE_IPV6_FIB_RULES,
};

int nsim_devlink_setup(struct netdevsim *ns);
void nsim_devlink_teardown(struct netdevsim *ns);

int nsim_devlink_init(void);
void nsim_devlink_exit(void);

int nsim_fib_init(void);
void nsim_fib_exit(void);
u64 nsim_fib_get_val(struct net *net, enum nsim_resource_id res_id, bool max);
int nsim_fib_set_max(struct net *net, enum nsim_resource_id res_id, u64 val,
		     struct netlink_ext_ack *extack);
#else
static inline int nsim_devlink_setup(struct netdevsim *ns)
{
	return 0;
}

static inline void nsim_devlink_teardown(struct netdevsim *ns)
{
}

static inline int nsim_devlink_init(void)
{
	return 0;
}

static inline void nsim_devlink_exit(void)
{
}
#endif

static inline struct netdevsim *to_nsim(struct device *ptr)
{
	return container_of(ptr, struct netdevsim, dev);
}
