#include <linux/string.h>
#include<linux/inet.h>

#include "toa.h"

static char *src_addr_scopes_str = NULL;
module_param(src_addr_scopes_str, charp, 0644);

int gs_nscopes;
static struct src_addr_scope_entry scopes[MAX_ADDR_SCOPE];

static int is_valid_net_mask(const char *num)
{
    int n = 0;

    if (!num || strlen(num) > 3)
        return 0;

    while (*num) {
        if (*num < '0' || *num > '9') 
            return 0;
        n = 10 * n + (*num - '0');
        num++;
    }

    if (n > 32 || n <= 0)
        return 0;

    return n;
}

static int is_valid_ip(const char *ip) 
{ 
    int section = 0;   //每一节的十进制值 
    int dot = 0;       //几个点分隔符 

    if (!ip)
        return 0;

    while (*ip) { 
        if(*ip == '.'){ 
            dot++; 
            if (dot > 3){ 
                return 0; 
            } 

            if (section >= 0 && section <=255){ 
                section = 0; 
            } else { 
                return 0; 
            } 

            if (*(ip+1) == '0') {
                return 0;
            }

        } else if (*ip >= '0' && *ip <= '9'){ 
            section = section * 10 + *ip - '0'; 
        }else{ 
            return 0; 
        } 

        ip++;        
    }

    if(section < 0 || section > 255){ 
        section = 0; 
        return 0;
    } 

    if (dot != 3)
        return 0;

    return 1; 
}


/* TODO use bin search */
static int is_saddr_in_scope(unsigned int saddr)
{
    int i;

    /* not set saddr scope */
    if (!gs_nscopes)
        return 1;

    for (i = 0; i < gs_nscopes; ++i) {
        if (saddr >= scopes[i].begin && saddr <= scopes[i].end) {
            return 1;
        }
    }

    return 0;
}

static int toa_parse_args(char *argv)
{
    int nscope = 0, naddr = 0, i = 0, mask_bit = 0;
    char* const delim_scope = ";";  
    char* const delim_addr = "/";  
    char *token = NULL, *cur = NULL ;  

    if (!argv)
        return 0;

    cur = argv;
    while ((token = strsep(&cur, delim_scope))) {  
        scopes[nscope++].ip_addr = token;

        if (nscope >= MAX_ADDR_SCOPE)
            return 0;
    }  

    for (i = 0; i < nscope; ++i) {
        char *addr_mask[2] = {0};
        unsigned int mask = ~0;
        cur = scopes[i].ip_addr;

        naddr = 0;
        while ((token = strsep(&cur, delim_addr))) {  
            if (naddr >= 2) {
                TOA_INFO("invlid param num:%d\n", naddr);

                return 0;
            }
            addr_mask[naddr++] = token;
        }

        if (!is_valid_ip(addr_mask[0])) {
            TOA_INFO("invlid vip:%s\n", addr_mask[0]);
            return 0;
        }

        if (!(mask_bit = is_valid_net_mask(addr_mask[1]))) {
            TOA_INFO("invlid mask:%s\n", addr_mask[1]);
            return 0;
        }

        mask <<= (32 - mask_bit);
        scopes[i].begin = ntohl(in_aton(addr_mask[0])) & mask;
        scopes[i].end = ntohl(in_aton(addr_mask[0])) | (~mask);

        TOA_INFO("ip [%s/%d:%x] [%u->%u]\n", addr_mask[0], 
                mask_bit, mask, scopes[i].begin, scopes[i].end);
    }

    return nscope;
}

unsigned int is_ro_addr(unsigned long addr)
{
	unsigned int level;
	unsigned int ro_enable = 0;
	pte_t *pte = lookup_address(addr, &level);
	if (pte->pte &~ _PAGE_RW)
	{
		ro_enable = 1;
	}
	
	return ro_enable;
}

void set_addr_rw(unsigned long addr)
{
    unsigned int level;
    pte_t *pte = lookup_address(addr, &level);

    if (pte->pte &~ _PAGE_RW)
    {
	    pte->pte |= _PAGE_RW;
    }
}

void set_addr_ro(unsigned long addr)
{
    unsigned int level;
    pte_t *pte = lookup_address(addr, &level);

    pte->pte = pte->pte &~_PAGE_RW;
}

/*
 * TOA	a new Tcp Option as Address,
 *	here address including IP and Port.
 *	the real {IP,Port} can be added into option field of TCP header,
 *	with LVS FULLNAT model, the realservice are still able to receive real {IP,Port} info.
 *	So far, this module only supports IPv4 and IPv6 mapped IPv4.
 *
 * Authors: 
 * 	Wen Li	<steel.mental@gmail.com>
 *	Yan Tian   <tianyan.7c00@gmail.com>
 *	Jiaming Wu <pukong.wjm@taobao.com>
 *	Jiajun Chen  <mofan.cjj@taobao.com>
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 * 	2 of the License, or (at your option) any later version.
 *
 */

unsigned long sk_data_ready_addr = 0;

/*
 * Statistics of toa in proc /proc/net/toa_stats 
 */

struct toa_stats_entry toa_stats[] = {
	TOA_STAT_ITEM("syn_recv_sock_toa", SYN_RECV_SOCK_TOA_CNT),
	TOA_STAT_ITEM("syn_recv_sock_no_toa", SYN_RECV_SOCK_NO_TOA_CNT),
	TOA_STAT_ITEM("syn_recv_not_in_scope", SYN_RECV_SADDR_NOT_IN_SCOPE),
	TOA_STAT_ITEM("syn_recv_in_scope", SYN_RECV_SADDR_IN_SCOPE),
	TOA_STAT_ITEM("getname_toa_ok", GETNAME_TOA_OK_CNT),
	TOA_STAT_ITEM("getname_toa_mismatch", GETNAME_TOA_MISMATCH_CNT),
	TOA_STAT_ITEM("getname_toa_bypass", GETNAME_TOA_BYPASS_CNT),
	TOA_STAT_ITEM("getname_toa_empty", GETNAME_TOA_EMPTY_CNT),
	TOA_STAT_END
};

DEFINE_TOA_STAT(struct toa_stat_mib, ext_stats);

/*
 * Funcs for toa hooks 
 */

/* Parse TCP options in skb, try to get client ip, port
 * @param skb [in] received skb, it should be a ack/get-ack packet.
 * @return NULL if we don't get client ip/port;
 *         value of toa_data in ret_ptr if we get client ip/port.
 */
static void * get_toa_data(struct sk_buff *skb)
{
	struct tcphdr *th;
	int length;
	unsigned char *ptr;

	struct toa_data tdata;

	void *ret_ptr = NULL;

	//TOA_DBG("get_toa_data called\n");

	if (NULL != skb) {
		th = tcp_hdr(skb);
		length = (th->doff * 4) - sizeof (struct tcphdr);
		ptr = (unsigned char *) (th + 1);

		while (length > 0) {
			int opcode = *ptr++;
			int opsize;
			switch (opcode) {
			case TCPOPT_EOL:
				return NULL;
			case TCPOPT_NOP:	/* Ref: RFC 793 section 3.1 */
				length--;
				continue;
			default:
				opsize = *ptr++;
				if (opsize < 2)	/* "silly options" */
					return NULL;
				if (opsize > length)
					return NULL;	/* don't parse partial options */
				if (TCPOPT_TOA == opcode && TCPOLEN_TOA == opsize) {
					memcpy(&tdata, ptr - 2, sizeof (tdata));
					//TOA_DBG("find toa data: ip = %u.%u.%u.%u, port = %u\n", NIPQUAD(tdata.ip),
						//ntohs(tdata.port));
					memcpy(&ret_ptr, &tdata, sizeof (ret_ptr));
					//TOA_DBG("coded toa data: %p\n", ret_ptr);
					return ret_ptr;
				}
				ptr += opsize - 2;
				length -= opsize;
			}
		}
	}
	return NULL;
}

/* get client ip from socket 
 * @param sock [in] the socket to getpeername() or getsockname()
 * @param uaddr [out] the place to put client ip, port
 * @param uaddr_len [out] lenth of @uaddr
 * @peer [in] if(peer), try to get remote address; if(!peer), try to get local address
 * @return return what the original inet_getname() returns.
 */
static int
inet_getname_toa(struct socket *sock, struct sockaddr *uaddr, int *uaddr_len, int peer)
{
	int retval = 0;
	struct sock *sk = sock->sk;
	struct sockaddr_in *sin = (struct sockaddr_in *) uaddr;
	struct toa_data tdata;

	//TOA_DBG("inet_getname_toa called, sk->sk_user_data is %p\n", sk->sk_user_data);

	/* call orginal one */
	retval = inet_getname(sock, uaddr, uaddr_len, peer);

	/* set our value if need */
	if (retval == 0 && NULL != sk->sk_user_data && peer) {
		if (sk_data_ready_addr == (unsigned long) sk->sk_data_ready) {
			memcpy(&tdata, &sk->sk_user_data, sizeof (tdata));
			if (TCPOPT_TOA == tdata.opcode && TCPOLEN_TOA == tdata.opsize) {
				TOA_INC_STATS(ext_stats, GETNAME_TOA_OK_CNT);
				//TOA_DBG("inet_getname_toa: set new sockaddr, ip %u.%u.%u.%u -> %u.%u.%u.%u, port %u -> %u\n",
				//		NIPQUAD(sin->sin_addr.s_addr), NIPQUAD(tdata.ip), ntohs(sin->sin_port),
				//		ntohs(tdata.port));
				sin->sin_port = tdata.port;
				sin->sin_addr.s_addr = tdata.ip;
			} else { /* sk_user_data doesn't belong to us */
				TOA_INC_STATS(ext_stats, GETNAME_TOA_MISMATCH_CNT);
				//TOA_DBG("inet_getname_toa: invalid toa data, ip %u.%u.%u.%u port %u opcode %u opsize %u\n",
				//		NIPQUAD(tdata.ip), ntohs(tdata.port), tdata.opcode, tdata.opsize);
			}
		} else {
			TOA_INC_STATS(ext_stats, GETNAME_TOA_BYPASS_CNT);
		}
	} else { /* no need to get client ip */
		TOA_INC_STATS(ext_stats, GETNAME_TOA_EMPTY_CNT);
	} 

	return retval;
}

/* The three way handshake has completed - we got a valid synack -
 * now create the new socket.
 * We need to save toa data into the new socket.
 * @param sk [out]  the socket
 * @param skb [in] the ack/ack-get packet
 * @param req [in] the open request for this connection
 * @param dst [out] route cache entry
 * @return NULL if fail new socket if succeed.
 */
static struct sock *
tcp_v4_syn_recv_sock_toa(struct sock *sk, struct sk_buff *skb, struct request_sock *req, struct dst_entry *dst)
{
	struct sock *newsock = NULL;
    int is_in_scope = 0;
    struct iphdr *iph;

	//TOA_DBG("tcp_v4_syn_recv_sock_toa called\n");

	/* call orginal one */
	newsock = tcp_v4_syn_recv_sock(sk, skb, req, dst);
    skb_transport_header(skb);
    iph = ip_hdr(skb);

    is_in_scope = is_saddr_in_scope(ntohl(iph->saddr));
    if (is_in_scope) {
		TOA_INC_STATS(ext_stats, SYN_RECV_SADDR_IN_SCOPE);
    } else {
		TOA_INC_STATS(ext_stats, SYN_RECV_SADDR_NOT_IN_SCOPE);
    }

	/* set our value if need */
	if (NULL != newsock && NULL == newsock->sk_user_data && is_in_scope) {
		newsock->sk_user_data = get_toa_data(skb);
		if(NULL != newsock->sk_user_data){
			TOA_INC_STATS(ext_stats, SYN_RECV_SOCK_TOA_CNT);
		} else {
			TOA_INC_STATS(ext_stats, SYN_RECV_SOCK_NO_TOA_CNT);
		}
		//TOA_DBG("tcp_v4_syn_recv_sock_toa: set sk->sk_user_data to %p\n", newsock->sk_user_data);
	}
	return newsock;
}

/*
 * HOOK FUNCS 
 */

/* replace the functions with our functions */
static inline int
hook_toa_functions(void)
{
	struct proto_ops *inet_stream_ops_p;
	struct inet_connection_sock_af_ops *ipv4_specific_p;
	int rw_enable = 0;

	/* hook inet_getname for ipv4 */
	inet_stream_ops_p = (struct proto_ops *)&inet_stream_ops;
	if(is_ro_addr((unsigned long)(&inet_stream_ops.getname)))
	{
		set_addr_rw((unsigned long)(&inet_stream_ops.getname));
		rw_enable = 1;
	}
	inet_stream_ops_p->getname = inet_getname_toa;
	if(rw_enable == 1)
	{
		set_addr_ro((unsigned long)(&inet_stream_ops.getname));
		rw_enable = 0;
	}
	TOA_INFO("CPU [%u] hooked inet_getname <%p> --> <%p>\n", smp_processor_id(), inet_getname,
		 inet_stream_ops_p->getname);

	/* hook tcp_v4_syn_recv_sock for ipv4 */
	ipv4_specific_p = (struct inet_connection_sock_af_ops *)&ipv4_specific;

	if(is_ro_addr((unsigned long)(&ipv4_specific.syn_recv_sock)))
	{
		set_addr_rw((unsigned long)(&ipv4_specific.syn_recv_sock));
		rw_enable = 1;
	}
	ipv4_specific_p->syn_recv_sock = tcp_v4_syn_recv_sock_toa;
	if(rw_enable == 1)
	{
		set_addr_ro((unsigned long)(&ipv4_specific.syn_recv_sock));
		rw_enable = 0;
	}

	TOA_INFO("CPU [%u] hooked tcp_v4_syn_recv_sock <%p> --> <%p>\n", smp_processor_id(), tcp_v4_syn_recv_sock,
		 ipv4_specific_p->syn_recv_sock);

	return 0;
}

/* replace the functions to original ones */
static int
unhook_toa_functions(void)
{
        struct proto_ops *inet_stream_ops_p;
        struct inet_connection_sock_af_ops *ipv4_specific_p;
	int rw_enable = 0;
	
	/* unhook inet_getname for ipv4 */
	inet_stream_ops_p = (struct proto_ops *)&inet_stream_ops;
	if(is_ro_addr((unsigned long)(&inet_stream_ops.getname)))
	{
		set_addr_rw((unsigned long)(&inet_stream_ops.getname));
		rw_enable = 1;
	}
	inet_stream_ops_p->getname = inet_getname;
	if(rw_enable == 1)
	{
		set_addr_ro((unsigned long)(&inet_stream_ops.getname));
		rw_enable = 0;
	}
	TOA_INFO("CPU [%u] unhooked inet_getname\n", smp_processor_id());

	/* unhook tcp_v4_syn_recv_sock for ipv4 */
	ipv4_specific_p = (struct inet_connection_sock_af_ops *)&ipv4_specific;
	if(is_ro_addr((unsigned long)(&ipv4_specific.syn_recv_sock)))
	{
		set_addr_rw((unsigned long)(&ipv4_specific.syn_recv_sock));
		rw_enable = 1;
	}
	ipv4_specific_p->syn_recv_sock = tcp_v4_syn_recv_sock;
	if(rw_enable == 1)
	{
		set_addr_ro((unsigned long)(&ipv4_specific.syn_recv_sock));
		rw_enable = 0;
	}

	TOA_INFO("CPU [%u] unhooked tcp_v4_syn_recv_sock\n", smp_processor_id());

	return 0;
}

/*
 * Statistics of toa in proc /proc/net/toa_stats 
 */
static int toa_stats_show(struct seq_file *seq, void *v){
	int i, j;

	/* print CPU first */
	seq_printf(seq, "                                  ");
	for (i = 0; i < NR_CPUS; i++)
		if (cpu_online(i))
			seq_printf(seq, "CPU%d       ", i);
	seq_putc(seq, '\n');

	i = 0;
	while (NULL != toa_stats[i].name) {
		seq_printf(seq, "%-25s:", toa_stats[i].name);
		for (j = 0; j < NR_CPUS; j++) {
			if (cpu_online(j)) {
				seq_printf(seq, "%10lu ",
					   *(((unsigned long *) per_cpu_ptr(ext_stats, j)) + toa_stats[i].entry));
			}
		}
		seq_putc(seq, '\n');
		i++;
	}
	return 0;
}

static int toa_stats_seq_open(struct inode *inode, struct file *file)
{
	return single_open(file, toa_stats_show, NULL);
}

static const struct file_operations toa_stats_fops = {
	.owner = THIS_MODULE,
	.open = toa_stats_seq_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

/*
 * TOA module init and destory 
 */

static struct proc_dir_entry *proc_file = NULL;
/* module init */
static int __init
toa_init(void)
{
	TOA_INFO("TOA " TOA_VERSION " by pukong.wjm\n");

	/* alloc statistics array for toa */
	if (NULL == (ext_stats = alloc_percpu(struct toa_stat_mib)))
		return 1;

	proc_file = proc_create("toa_stats", S_IRUSR, init_net.proc_net, &toa_stats_fops);
        if (!proc_file)
                goto err;
	
	/* get the address of function sock_def_readable
	 * so later we can know whether the sock is for rpc, tux or others 
	 */
	sk_data_ready_addr = kallsyms_lookup_name("sock_def_readable");
	TOA_INFO("CPU [%u] sk_data_ready_addr = kallsyms_lookup_name(sock_def_readable) = %lu\n", 
		 smp_processor_id(), sk_data_ready_addr);
	if(0 == sk_data_ready_addr) {
		TOA_INFO("cannot find sock_def_readable.\n");
		goto err_file;
	}

	/* hook funcs for parse and get toa */
	hook_toa_functions();

    gs_nscopes = toa_parse_args(src_addr_scopes_str);
	TOA_INFO("nscopes:%d  toa loaded\n", gs_nscopes);

	return 0;

err_file:
	remove_proc_entry("toa_stats", init_net.proc_net);
err:
        if (NULL != ext_stats) {
                free_percpu(ext_stats);
                ext_stats = NULL;
        }

	return 1;
}

/* module cleanup*/
static void __exit
toa_exit(void)
{
	unhook_toa_functions();
	synchronize_net();
        if (proc_file)
	        remove_proc_entry("toa_stats", init_net.proc_net);
	if (NULL != ext_stats) {
		free_percpu(ext_stats);
		ext_stats = NULL;
	}
	TOA_INFO("toa unloaded\n");
}

module_init(toa_init);
module_exit(toa_exit);
MODULE_LICENSE("GPL");
