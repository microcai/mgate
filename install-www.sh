#! /bin/sh


DIST_WWW="/var/www"
HTTP_CONF="/etc/httpd/conf/httpd.conf"
HTTP_CONF_PATCH="httpd.conf.patch"
httpd="/etc/init.d/httpd"
mysqld="/etc/init.d/mysqld"

if [ ! -e ${httpd} ] ; then
	echo "没有安装 apache,请安装!"
	exit 1
fi

if [ ! -e ${mysqld} ] ; then
	echo "没有安装 mysql ,请安装!"
	exit 1
fi

if [ "${UID}" != "0" ] ; then
	echo "need to be root !"
	exit 1
	else
	echo "check root.. yes"
fi

if [ -f ./www.tar.bz2 ] ; then
	echo "安装 网页到 /var/www "
	tar -xf www.tar.bz2 -C ${DIST_WWW}
fi

set -x
exit 0

if [ -f ${HTTP_CONF} ] ;  then
	echo "修改 http 配置文件"
	patch ${HTTP_CONF}  ${HTTP_CONF_PATCH}
fi

echo "重启 httpd,mysqld 服务"
${httpd} restart
${mysqld} restart

@echo
@echo

echo "安装monitor系统准备工作结束"









