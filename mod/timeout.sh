for tcpfile in $(ls /proc/sys/net/netfilter/nf_conntrack_tcp_timeout_*)
do
	echo 1800 > $tcpfile
done
