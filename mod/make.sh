rmmod appflid
make
make modules_install
depmod
#modprobe appflid userif=eth3 resrcif=eth2 confiledir=/home/philar/appflid/conf/
modprobe appflid
echo 1 >/proc/sys/net/netfilter/nf_conntrack_acct
cat /proc/sys/net/netfilter/nf_conntrack_acct
