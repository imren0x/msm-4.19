#include <linux/init.h>
#include <linux/module.h>
#include <linux/timer.h>
#include <linux/time.h>
#include <linux/types.h>
#include <net/sock.h>
#include <net/netlink.h>

#define NETLINK_TEST 25
#define MAX_MSGSIZE 32

int stringlength(char *s);
void vince_sendnlmsg(char *message);

int vince_pid;
struct sock *vince_nl_sk = NULL;

struct gf_uk_channel {
    int channel_id;
    int reserved;
    char buf[3*1024];
    int len;
};

void vince_sendnlmsg(char *message)
{
    struct sk_buff *skb_1;
    struct nlmsghdr *nlh;
    int len = NLMSG_SPACE(MAX_MSGSIZE);
    int slen = 0;
    int ret = 0;
    
    if (!message || !vince_nl_sk || !vince_pid) {
        return;
    }
    skb_1 = alloc_skb(len, GFP_KERNEL);
    if (!skb_1) {
		printk(KERN_ERR "my_net_link:alloc_skb_1 error\n");
	}
    slen = strlen(message);
    nlh = nlmsg_put(skb_1, 0, 0, 0, MAX_MSGSIZE, 0);

    NETLINK_CB(skb_1).portid = 0;
    NETLINK_CB(skb_1).dst_group = 0;

    message[slen] = '\0';
    memcpy(NLMSG_DATA(nlh), message, slen+1);

    ret = netlink_unicast(vince_nl_sk, skb_1, vince_pid, MSG_DONTWAIT);
    if (!ret) {
        printk("send msg from kernel to usespace failed ret 0x%x \n", ret);
    }
}

void vince_nl_data_ready(struct sk_buff *__skb)
{
    struct sk_buff *skb;
    struct nlmsghdr *nlh;
    char str[100];
    
    skb = skb_get(__skb);
    if (skb->len >= NLMSG_SPACE(0)) {
        nlh = nlmsg_hdr(skb);

        memcpy(str, NLMSG_DATA(nlh), sizeof(str));
        vince_pid = nlh->nlmsg_pid;

        if (vince_pid)
            printk("Message pid %d received:%s\n", vince_pid, str);
        kfree_skb(skb);
    }
}

int vince_netlink_init(void)
{
    struct netlink_kernel_cfg netlink_cfg;
    memset(&netlink_cfg, 0, sizeof(struct netlink_kernel_cfg));

    netlink_cfg.groups = 0;
    netlink_cfg.flags = 0;
    netlink_cfg.input = vince_nl_data_ready;
    netlink_cfg.cb_mutex = NULL;

    vince_nl_sk = netlink_kernel_create(&init_net, NETLINK_TEST,
            &netlink_cfg);

    if (!vince_nl_sk) {
        printk(KERN_ERR "my_net_link: create netlink socket error.\n");
        return 1;
    }

    return 0;
}

void vince_netlink_exit(void)
{
    if (vince_nl_sk != NULL) {
        netlink_kernel_release(vince_nl_sk);
        vince_nl_sk = NULL;
    }

    printk("my_net_link: self module exited\n");
}