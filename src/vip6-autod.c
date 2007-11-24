/*
 * $Id$
 * Copyright (c) 2007 The Trustees of Princeton University
 * Author: Daniel Hokka Zakrisson <daniel@hozac.com>
 *
 * Licensed under the terms of the GNU General Public License
 * version 2 or later.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>

#include <asm/types.h>
#include <netlink/netlink.h>
#include <netlink/route/addr.h>

#include <vserver.h>
#include "pathconfig.h"

#define HAS_ADDRESS	0x01
#define HAS_PREFIX	0x02

struct nid_list {
	nid_t nid;
	struct nid_list *next;
};
struct prefix {
	uint32_t mask;
	int ifindex;
	struct {
		struct in6_addr addr;
		int prefix_len;
		time_t valid_until;
	} prefix;
	struct {
		struct in6_addr addr;
		int prefix_len;
		time_t valid_until;
	} address;
};
struct nid_prefix_map {
	struct {
		struct nid_prefix_map *prev;
		struct nid_prefix_map *next;
	} n;
	struct {
		struct nid_prefix_map *prev;
		struct nid_prefix_map *next;
	} p;
	struct prefix *prefix;
	nid_t nid;
};

struct nl_handle *handle;

/* from linux/include/net/ipv6.h */
static inline int ipv6_prefix_equal(struct in6_addr *prefix,
				    struct in6_addr *addr, int prefixlen)
{
	uint32_t *a1 = prefix->s6_addr32, *a2 = addr->s6_addr32;
	unsigned pdw, pbi;

	/* check complete u32 in prefix */
	pdw = prefixlen >> 5;
	if (pdw && memcmp(a1, a2, pdw << 2))
		return 0;

	/* check incomplete u32 in prefix */
	pbi = prefixlen & 0x1f;
	if (pbi && ((a1[pdw] ^ a2[pdw]) & htonl((0xffffffff) << (32 - pbi))))
		return 0;

	return 1;
}

static int add_address_to_interface(int ifindex, struct in6_addr *address,
				    int prefix)
{
	int err = -1;
	struct rtnl_addr *rta;
	struct nl_addr *nl;

	nl = nl_addr_build(AF_INET6, address, sizeof(struct in6_addr));
	rta = rtnl_addr_alloc();

	rtnl_addr_set_family(rta, AF_INET6);
	rtnl_addr_set_ifindex(rta, ifindex);
	rtnl_addr_set_local(rta, nl);
	rtnl_addr_set_prefixlen(rta, prefix);

	if (rtnl_addr_add(handle, rta, NLM_F_REPLACE) != -1 || errno == EEXIST)
		err = 0;

	rtnl_addr_free(rta);
	nl_addr_destroy(nl);
	return err;
}

static inline int remove_address_from_interface(struct nid_prefix_map *entry)
{
	struct rtnl_addr *rta;
	struct nl_addr *nl;
	struct in6_addr a;
	int ret;

	memcpy(&a, &entry->prefix->address.addr, sizeof(a));
	if (entry->nid != 0) {
		a.s6_addr[11] = (entry->nid & 0x7f80) >> 7;
		a.s6_addr[12] = (entry->nid & 0x7f) << 1;
	}

	nl = nl_addr_build(AF_INET6, &a, sizeof(a));
	if (!nl)
		return -1;
	rta = rtnl_addr_alloc();
	if (!rta)
		return -1;

	rtnl_addr_set_family(rta, AF_INET6);
	rtnl_addr_set_ifindex(rta, entry->prefix->ifindex);
	rtnl_addr_set_local(rta, nl);
	rtnl_addr_set_prefixlen(rta, entry->prefix->address.prefix_len);

	ret = rtnl_addr_delete(handle, rta, 0);

	rtnl_addr_free(rta);
	nl_addr_destroy(nl);

	return ret;
}

static int add_to_map(struct nid_prefix_map *map, struct nid_prefix_map *new)
{
	struct nid_prefix_map *i;
#define PUT_IT_IN_PLACE(node, member, om)				\
	/* find the correct location in the list */			\
	for (i = map->node.next; i->node.next && i->member <		\
	     new->member; i = i->node.next)				\
		;							\
	if (i && i->member == new->member && i->om == new->om)		\
		return 0;						\
	/* first in the list */						\
	if (!i || !i->node.prev) {					\
		new->node.prev = NULL;					\
		new->node.next = i;					\
		map->node.next = new;					\
		if (i)							\
			i->node.prev = new;				\
	}								\
	/* last in the list */						\
	else if (i->node.next == NULL) {				\
		new->node.prev = i;					\
		new->node.next = NULL;					\
		i->node.next = new;					\
	}								\
	/* somewhere in the middle */					\
	else {								\
		new->node.prev = i->node.prev;				\
		new->node.next = i;					\
		i->node.prev->node.next = new;				\
		i->node.prev = new;					\
	}
	PUT_IT_IN_PLACE(p, prefix, nid)
	PUT_IT_IN_PLACE(n, nid, prefix)
	return 1;
}

static inline void remove_from_map(struct nid_prefix_map *map,
				   struct nid_prefix_map *entry)
{
	if (map->n.next == entry)
		map->n.next = entry->n.next;
	if (map->n.prev == entry)
		map->n.prev = entry->n.prev;
	if (map->p.next == entry)
		map->p.next = entry->p.next;
	if (map->p.prev == entry)
		map->p.prev = entry->p.prev;
}

static inline void remove_from_map_and_free(struct nid_prefix_map *map,
					    struct nid_prefix_map *entry)
{
	remove_from_map(map, entry);
	free(entry);
}

static int add_nid_to_map(struct nid_prefix_map *map, struct prefix *prefix,
			  nid_t nid)
{
	struct nid_prefix_map *new = calloc(1, sizeof(struct nid_prefix_map));
	int ret;

	if (!new)
		return -1;

	new->prefix = prefix;
	new->nid = nid;
	ret = add_to_map(map, new);

	if (ret == 0)
		free(new);

	return ret;
}

static int add_prefix_to_map(struct nid_prefix_map *map, struct prefix *prefix)
{
	return add_nid_to_map(map, prefix, 0);
}

static void cleanup_prefix(struct nid_prefix_map *map,
			   struct nid_prefix_map *first)
{
	struct nid_prefix_map *i, *p = NULL;

	for (i = first; i && first->prefix == i->prefix; i = i->p.next) {
		if (p)
			remove_from_map_and_free(map, p);

		/* ignore errors */
		remove_address_from_interface(i);

		p = i;
	}
	if (p)
		remove_from_map_and_free(map, p);
}

static inline int add_nid_to_list(struct nid_list **head, nid_t nid)
{
	struct nid_list *i, *new;

	for (i = *head; i && i->next && i->next->nid < nid; i = i->next)
		;
	/* check if this nid is first in the list */
	if (i && i->nid == nid)
		return 0;
	/* check if it's already in the list */
	if (i && i->next && i->next->nid == nid)
		return 0;

	/* add it */
	new = calloc(1, sizeof(struct nid_list));
	if (!new)
		return -1;
	new->nid = nid;

	/* this is the lowest nid in the list */
	if (i == *head) {
		*head = new;
		new->next = i;
	}
	/* in the middle/at the end */
	else if (i) {
		new->next = i->next;
		i->next = new;
	}
	/* there was no list */
	else
		*head = new;

	return 1;
}

static inline void free_nid_list(struct nid_list *head)
{
	struct nid_list *p;
	for (p = NULL; head; head = head->next) {
		if (p)
			free(p);
		p = head;
	}
	if (p)
		free(p);
}

static inline void cleanup_nid(struct nid_prefix_map *map,
			       nid_t nid)
{
	struct nid_prefix_map *i, *p = NULL;
	for (i = map->n.next; i->nid < nid; i = i->n.next)
		;
	/* this nid doesn't have any entries in the map */
	if (i->nid != nid)
		return;
	for (; i->nid == nid; i = i->n.next) {
		if (p)
			remove_from_map_and_free(map, p);
		remove_address_from_interface(i);
		p = i;
	}
	if (p)
		remove_from_map_and_free(map, p);
}

static inline void cleanup_nids(struct nid_prefix_map *map,
				struct nid_list *previous,
				struct nid_list *current)
{
	struct nid_list *p, *pprev = NULL, *c;
	for (p = previous, c = current; p; pprev = p, p = p->next) {
		if (pprev)
			free(pprev);
		while (c->nid < p->nid)
			c = c->next;
		if (c->nid == p->nid)
			continue;
		/* this context has disappeared */
		cleanup_nid(map, p->nid);
	}
	if (pprev)
		free(pprev);
}

static void do_slices_autoconf(struct nid_prefix_map *map)
{
	DIR *dp;
	struct dirent *de;
	struct vc_net_addr addr;
	struct nid_prefix_map *i;
	struct nid_list *current = NULL, *n;
	static struct nid_list *previous = NULL;

	if ((dp = opendir("/proc/virtnet")) == NULL)
		return;
	while ((de = readdir(dp)) != NULL) {
		nid_t nid;

		if (!isdigit(de->d_name[0]))
			continue;

		nid = strtoul(de->d_name, NULL, 10);
		addr.vna_type = VC_NXA_TYPE_IPV6 | VC_NXA_TYPE_ANY;
		if (vc_net_remove(nid, &addr) == -1) {
			syslog(LOG_ERR, "vc_net_remove(%u): %s", nid, strerror(errno));
			continue;
		}

		add_nid_to_list(&current, nid);
	}
	closedir(dp);

	for (n = current; n; n = n->next) {
		for (i = map->p.next; i && i->nid == 0;) {
			/* expired */
			if (i->prefix->mask & HAS_PREFIX && i->prefix->prefix.valid_until < time(NULL)) {
				struct nid_prefix_map *tmp;
				char buf[64];

				inet_ntop(AF_INET6, &i->prefix->address.addr, buf, sizeof(buf));
				syslog(LOG_NOTICE, "Address %s timed out", buf);

				tmp = i->p.next;

				cleanup_prefix(map, i);

				i = tmp;
				continue;
			}
			if (i->prefix->mask != (HAS_ADDRESS|HAS_PREFIX))
				goto next;

			addr.vna_type = VC_NXA_TYPE_IPV6 | VC_NXA_TYPE_ADDR;
			memcpy(&addr.vna_v6_ip, &i->prefix->address.addr, sizeof(struct in6_addr));
			addr.vna_prefix = i->prefix->prefix.prefix_len;
			if (addr.vna_prefix == 64) {
				addr.vna_v6_mask.s6_addr32[0] = addr.vna_v6_mask.s6_addr32[1] = 0xffffffff;
				addr.vna_v6_mask.s6_addr32[2] = addr.vna_v6_mask.s6_addr32[3] = 0;
			}
			addr.vna_v6_ip.s6_addr[11] = (n->nid & 0x7f80) >> 7;
			addr.vna_v6_ip.s6_addr[12] = (n->nid & 0x007f) << 1;
			if (vc_net_add(n->nid, &addr) == -1) {
				syslog(LOG_ERR, "vc_net_add(%u): %s", n->nid, strerror(errno));
				goto next;
			}
			if (add_address_to_interface(i->prefix->ifindex, &addr.vna_v6_ip, addr.vna_prefix) == -1) {
				syslog(LOG_ERR, "add_address_to_interface: %s", strerror(errno));
				goto next;
			}
			if (add_nid_to_map(map, i->prefix, n->nid) == -1) {
				syslog(LOG_ERR, "add_nid_to_map: %s", strerror(errno));
				goto next;
			}
next:
			i = i->p.next;
		}
	}

	cleanup_nids(map, previous, current);
	previous = current;
}

/* XXX These two functions are very similar */
static int add_prefix(struct nid_prefix_map *map, struct prefixmsg *msg,
		      struct in6_addr *prefix, struct prefix_cacheinfo *cache)
{
	struct nid_prefix_map *i = map;
	struct prefix *new;

	if (!msg || !prefix || !cache)
		return -1;
	/* XXX IF_PREFIX_AUTOCONF == 0x02 */
	if (!(msg->prefix_flags & 0x02))
		return -1;

	do {
		if (i->p.next != NULL)
			i = i->p.next;
		if (ipv6_prefix_equal(prefix, &i->prefix->prefix.addr, msg->prefix_len) ||
		    ipv6_prefix_equal(prefix, &i->prefix->address.addr, msg->prefix_len)) {
			i->prefix->mask |= HAS_PREFIX;
			i->prefix->ifindex = msg->prefix_ifindex;
			memcpy(&i->prefix->prefix.addr, prefix, sizeof(*prefix));
			i->prefix->prefix.prefix_len = msg->prefix_len;
			i->prefix->prefix.valid_until = time(NULL) + cache->preferred_time;
			return 0;
		}
	} while (i->p.next && i->nid == 0);

	/* not yet in the map */
	new = calloc(1, sizeof(*new));
	if (!new)
		return -1;
	new->mask = HAS_PREFIX;
	memcpy(&new->prefix.addr, prefix, sizeof(*prefix));
	new->prefix.prefix_len = msg->prefix_len;
	new->prefix.valid_until = time(NULL) + cache->preferred_time;
	if (add_prefix_to_map(map, new) == -1)
		return -1;

	return 1;
}

static inline int add_address(struct nid_prefix_map *map, struct ifaddrmsg *msg,
			      struct in6_addr *address, struct ifa_cacheinfo *cache)
{
	struct nid_prefix_map *i = map;
	struct prefix *new;

	if (!msg || !address || !cache)
		return -1;

	if (address->s6_addr[11] != 0xFF || address->s6_addr[12] != 0xFE)
		return -1;

	do {
		if (i->p.next != NULL)
			i = i->p.next;
		if (ipv6_prefix_equal(address, &i->prefix->prefix.addr, msg->ifa_prefixlen) ||
		    ipv6_prefix_equal(address, &i->prefix->address.addr, 128)) {
			i->prefix->mask |= HAS_ADDRESS;
			memcpy(&i->prefix->address.addr, address, sizeof(*address));
			i->prefix->address.prefix_len = msg->ifa_prefixlen;
			i->prefix->address.valid_until = time(NULL) + cache->ifa_prefered;
			return 0;
		}
	} while (i->p.next && i->nid == 0);

	new = calloc(1, sizeof(*new));
	if (!new)
		return -1;
	new->mask = HAS_ADDRESS;
	memcpy(&new->address.addr, address, sizeof(*address));
	new->address.prefix_len = msg->ifa_prefixlen;
	new->address.valid_until = time(NULL) + cache->ifa_prefered;
	if (add_prefix_to_map(map, new) == -1)
		return -1;

	return 1;
}

static struct nla_policy addr_policy[IFA_MAX+1] = {
	[IFA_ADDRESS]	= { .minlen = sizeof(struct in6_addr) },
	[IFA_LABEL]	= { .type = NLA_STRING,
			    .maxlen = IFNAMSIZ },
	[IFA_CACHEINFO]	= { .minlen = sizeof(struct ifa_cacheinfo) },
};
static struct nla_policy prefix_policy[PREFIX_MAX+1] = {
	[PREFIX_ADDRESS]   = { .minlen = sizeof(struct in6_addr) },
	[PREFIX_CACHEINFO] = { .minlen = sizeof(struct prefix_cacheinfo) },
};
int handle_valid_msg(struct nl_msg *msg, void *arg)
{
	struct nlmsghdr *nlh = nlmsg_hdr(msg);
	int ret = -1;
	char *payload;
	struct sockaddr_nl *source = nlmsg_get_src(msg);

	payload = nlmsg_data(nlh);
	if (source->nl_groups == RTMGRP_IPV6_PREFIX) {
		struct prefixmsg *prefixmsg;
		struct in6_addr *prefix = NULL;
		struct prefix_cacheinfo *cacheinfo = NULL;
		struct nlattr *tb[PREFIX_MAX+1];

		if (nlmsg_parse(nlh, sizeof(struct prefixmsg), tb, PREFIX_MAX, prefix_policy) < 0) {
			syslog(LOG_ERR, "Failed to parse prefixmsg");
			return -1;
		}

		prefixmsg = (struct prefixmsg *) payload;
		if (tb[PREFIX_ADDRESS])
			prefix = nl_data_get(nla_get_data(tb[PREFIX_ADDRESS]));
		if (tb[PREFIX_CACHEINFO])
			cacheinfo = nl_data_get(nla_get_data(tb[PREFIX_CACHEINFO]));
		ret = add_prefix(arg, prefixmsg, prefix, cacheinfo);
	}	
	else if (source->nl_groups == RTMGRP_IPV6_IFADDR) {
		struct ifaddrmsg *ifaddrmsg;
		struct in6_addr *address = NULL;
		struct ifa_cacheinfo *cacheinfo = NULL;
		struct nlattr *tb[IFA_MAX+1];

		if (nlmsg_parse(nlh, sizeof(struct ifaddrmsg), tb, IFA_MAX, addr_policy) < 0) {
			syslog(LOG_ERR, "Failed to parse ifaddrmsg");
			return -1;
		}

		ifaddrmsg = (struct ifaddrmsg *) payload;
		if (tb[IFA_ADDRESS])
			address = nl_data_get(nla_get_data(tb[IFA_ADDRESS]));
		if (tb[IFA_CACHEINFO])
			cacheinfo = nl_data_get(nla_get_data(tb[IFA_CACHEINFO]));
		ret = add_address(arg, ifaddrmsg, address, cacheinfo);
	}
	if (ret >= 0)
		do_slices_autoconf(arg);

	return 0;
}

int handle_error_msg(struct sockaddr_nl *source, struct nlmsgerr *err,
		     void *arg)
{
	syslog(LOG_ERR, "%s", strerror(err->error));
	return 0;
}

int handle_no_op(struct nl_msg *msg, void *arg)
{
	return 0;
}

/* only for access in the signal handler */
struct nid_prefix_map map = {
	.n = {
		.next = NULL,
		.prev = NULL,
	},
	.p = {
		.next = NULL,
		.prev = NULL,
	},
};
void signal_handler(int signal)
{
	switch (signal) {
	case SIGUSR1:
		do_slices_autoconf(&map);
		break;
	}
}

static int write_pidfile(const char *filename)
{
	FILE *fp;
	fp = fopen(filename, "w");
	if (!fp)
		return -1;
	fprintf(fp, "%d\n", getpid());
	fclose(fp);
	return 0;
}

int main(int argc, char *argv[])
{
	struct nl_cb *cbs;

	openlog("vip6-autod", LOG_PERROR, LOG_DAEMON);

	handle = nl_handle_alloc_nondefault(NL_CB_VERBOSE);
	cbs = nl_handle_get_cb(handle);
	nl_cb_set(cbs, NL_CB_VALID, NL_CB_CUSTOM, handle_valid_msg, &map);
	nl_cb_set(cbs, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, handle_no_op, NULL);
	nl_cb_err(cbs, NL_CB_CUSTOM, handle_error_msg, &map);
	nl_disable_sequence_check(handle);

	nl_join_groups(handle, RTMGRP_IPV6_PREFIX|RTMGRP_IPV6_IFADDR);
	if (nl_connect(handle, NETLINK_ROUTE) == -1) {
		syslog(LOG_CRIT, "nl_connect: %s", strerror(errno));
		exit(1);
	}

	if (daemon(0, 0) == -1)
		return -1;

	/* XXX .. here is a hack */
	write_pidfile(DEFAULT_PKGSTATEDIR "/../vip6-autod.pid");

	signal(SIGUSR1, signal_handler);

	while (nl_recvmsgs(handle, cbs) > 0);

	nl_close(handle);
	closelog();
	return 0;
}
