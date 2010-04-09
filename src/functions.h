/*
 * File:   libmicrocai-functions.h
 * Author: cai
 *
 * Created on 2009年4月15日, 上午10:24
 */

#ifndef _LIBMICROCAI_FUNCTIONS_H
#define	_LIBMICROCAI_FUNCTIONS_H

_EXTERN_ void
	formatMAC(const u_char * MAC_ADDR,char * strmac);
/*_EXTERN_ void
	convertMAC(char mac[6],const char * strmac);*/
_EXTERN_ void
	convertMAC(char mac[6],const char * strmac);

_EXTERN_ u_int16_t
    checksum(u_int16_t *buffer, int size);
#ifdef ENABLE_HOTEL

_EXTERN_ gboolean
	GetMac(const char *ip, char MAC_ADDR[],u_char mac_addr[]);

_EXTERN_ void
	run_cmd(const gchar *  strcmd );

_EXTERN_ gboolean
	mac_is_alowed(u_char mac[6]);
_EXTERN_ gboolean
	mac_is_alowed_with_ip(u_char mac[6],const u_int32_t ip);
_EXTERN_ gboolean
	ip_is_allowed(void*,in_addr_t)__attribute_noinline__;
_EXTERN_ void
	mac_set_allowed(u_char mac[6],gboolean allow,in_addr_t ip);
_EXTERN_ gboolean
	set_client_data( u_char mac[6],  Clients_DATA * pcd );
_EXTERN_ gboolean
	get_client_data(u_char mac[6],Clients_DATA * pcd );
_EXTERN_ void
	redirect_to_local_http( u_int32_t ,const u_char *,struct iphdr* );
_EXTERN_ void
	init_http_redirector(const gchar* dest);
#endif

_EXTERN_ struct tm *
	GetCurrentTime();
_EXTERN_ int
	utf8_gbk(char *outbuf, size_t outlen, const char *inbuf, size_t inlen);
_EXTERN_ int
	gbk_utf8(char *outbuf, size_t outlen, const char *inbuf, size_t inlen);

#endif	/* _LIBMICROCAI_FUNCTIONS_H */

