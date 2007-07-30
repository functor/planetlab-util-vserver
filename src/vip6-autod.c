/*
 * $Id: vip6-autod.c,v 1.3 2007/07/26 23:08:00 dhozac Exp $
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
/* not defined for gcc -ansi */
typedef uint64_t __u64;
typedef int64_t __s64;
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
struct prefix_list {
	struct prefix_list *prev;
	struct prefix_list *next;
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
	struct nid_list *nids;
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

static int add_address_to_interface(int ifindex, struct in6_addr *address, int prefix)
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

static int add_nid_to_list(struct prefix_list *i, nid_t nid)
{
	struct nid_list *n;
	n = calloc(1, sizeof(struct nid_list));
	if (!n)
		return -1;
	n->nid = nid;
	n->next = i->nids;
	i->nids = n;
	return 0;
}

static void cleanup_prefix(struct prefix_list *i)
{
	struct nid_list *n;

	for (n = i->nids; n; n = n->next) {
		struct rtnl_addr *rta;
		struct nl_addr *nl;
		struct in6_addr a;

		memcpy(&a, &i->address.addr, sizeof(a));
		rta = rtnl_addr_alloc();
		nl = nl_addr_build(AF_INET6, &a, sizeof(a));

		rtnl_addr_set_family(rta, AF_INET6);
		rtnl_addr_set_ifindex(rta, i->ifindex);
		rtnl_addr_set_local(rta, nl);
		rtnl_addr_set_prefixlen(rta, i->address.prefix_len);

		/* ignore errors */
		rtnl_addr_delete(handle, rta, 0);

		nl_addr_destroy(nl);
		rtnl_addr_free(rta);
	}
}

static void do_slices_autoconf(struct prefix_list *head)
{
	DIR *dp;
	struct dirent *de;
	nid_t nid;
	struct vc_net_nx addr;
	struct prefix_list *i;

	if ((dp = opendir("/proc/virtnet")) == NULL)
		return;
	while ((de = readdir(dp)) != NULL) {
		if (!isdigit(de->d_name[0]))
			continue;

		nid = strtoul(de->d_name, NULL, 10);
		addr.type = vcNET_IPV6A;
		addr.count = 0;
		if (vc_net_remove(nid, &addr) == -1) {
			syslog(LOG_ERR, "vc_net_remove: %s", strerror(errno));
			continue;
		}

		for (i = head->next; i;) {
			/* expired */
			if (i->mask & HAS_PREFIX && i->prefix.valid_until < time(NULL)) {
				struct prefix_list *tmp;
				char buf[64];

				inet_ntop(AF_INET6, &i->address.addr, buf, sizeof(buf));
				syslog(LOG_NOTICE, "Address %s timed out", buf);

				if (i->next)
					i->next->prev = i->prev;
				if (i->prev)
					i->prev->next = i->next;
				tmp = i->next;

				cleanup_prefix(i);

				free(i);
				i = tmp;
				continue;
			}
			if (i->mask != (HAS_ADDRESS|HAS_PREFIX))
				goto next;

			addr.count = 1;
			addr.mask[0] = i->prefix.prefix_len;
			memcpy(addr.ip, &i->address.addr, sizeof(struct in6_addr));
			addr.ip[2] = htonl((ntohl(addr.ip[2]) & 0xffffff00) | ((nid & 0x7f00) >> 7));
			addr.ip[3] = htonl((ntohl(addr.ip[3]) & 0x00ffffff) | ((nid & 0xff) << 25));
			if (vc_net_add(nid, &addr) == -1) {
				syslog(LOG_ERR, "vc_net_add: %s", strerror(errno));
				exit(1);
			}
			if (add_address_to_interface(i->ifindex, (struct in6_addr *) addr.ip, i->prefix.prefix_len) == -1) {
				syslog(LOG_ERR, "add_address_to_interface: %s", strerror(errno));
				exit(1);
			}
			if (add_nid_to_list(i, nid) == -1) {
				syslog(LOG_ERR, "add_nid_to_list: %s", strerror(errno));
				exit(1);
			}
next:
			i = i->next;
		}
	}
	closedir(dp);
}

static int add_prefix(struct prefix_list *head, struct prefixmsg *msg,
		      struct in6_addr *prefix, struct prefix_cacheinfo *cache)
{
	struct prefix_list *i = head;
	if (!msg || !prefix || !cache)
		return -1;
	/* XXX IF_PREFIX_AUTOCONF == 0x02 */
	if (!(msg->prefix_flags & 0x02))
		return -1;

	do {
		if (i->next != NULL)
			i = i->next;
		if (ipv6_prefix_equal(prefix, &i->prefix.addr, msg->prefix_len) ||
		    ipv6_prefix_equal(prefix, &i->address.addr, msg->prefix_len)) {
			i->mask |= HAS_PREFIX;
			i->ifindex = msg->prefix_ifindex;
			memcpy(&i->prefix.addr, prefix, sizeof(*prefix));
			i->prefix.prefix_len = msg->prefix_len;
			i->prefix.valid_until = time(NULL) + cache->preferred_time;
			return 0;
		}
	} while (i->next);

	i->next = calloc(1, sizeof(*i));
	if (!i->next)
		return -1;
	i->next->prev = i;
	i = i->next;
	i->mask = HAS_PREFIX;
	memcpy(&i->prefix.addr, prefix, sizeof(*prefix));
	i->prefix.prefix_len = msg->prefix_len;
	i->prefix.valid_until = time(NULL) + cache->preferred_time;

	return 1;
}

static inline int add_address(struct prefix_list *head, struct ifaddrmsg *msg,
			      struct in6_addr *address, struct ifa_cacheinfo *cache)
{
	struct prefix_list *i = head;
	if (!msg || !address || !cache)
		return -1;

	if (address->s6_addr[11] != 0xFF || address->s6_addr[12] != 0xFE)
		return -1;

	do {
		if (i->next != NULL)
			i = i->next;
		if (ipv6_prefix_equal(address, &i->prefix.addr, msg->ifa_prefixlen) ||
		    ipv6_prefix_equal(address, &i->address.addr, msg->ifa_prefixlen)) {
			i->mask |= HAS_ADDRESS;
			memcpy(&i->address.addr, address, sizeof(*address));
			i->address.prefix_len = msg->ifa_prefixlen;
			i->address.valid_until = time(NULL) + cache->ifa_prefered;
			return 0;
		}
	} while (i->next);

	i->next = calloc(1, sizeof(*i));
	if (!i->next)
		return -1;
	i->next->prev = i;
	i = i->next;
	i->mask = HAS_ADDRESS;
	memcpy(&i->address.addr, address, sizeof(*address));
	i->address.prefix_len = msg->ifa_prefixlen;
	i->address.valid_until = time(NULL) + cache->ifa_prefered;

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
struct prefix_list head;
void signal_handler(int signal)
{
	switch (signal) {
	case SIGUSR1:
		do_slices_autoconf(&head);
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
	head.prev = head.next = NULL;

	openlog("vip6-autod", LOG_PERROR, LOG_DAEMON);

	handle = nl_handle_alloc_nondefault(NL_CB_VERBOSE);
	cbs = nl_handle_get_cb(handle);
	nl_cb_set(cbs, NL_CB_VALID, NL_CB_CUSTOM, handle_valid_msg, &head);
	nl_cb_set(cbs, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, handle_no_op, NULL);
	nl_cb_err(cbs, NL_CB_CUSTOM, handle_error_msg, &head);
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
