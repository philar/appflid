rmmod appflid
make
make modules_install
depmod
modprobe appflid userif=eth3 resrcif=eth2 confiledir=/home/philar/application_flow/appflid/conf/
