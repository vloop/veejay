/* vjnet - low level network I/O for VeeJay
 *
 *           (C) 2005 Niels Elburg <nwelburg@gmail.com> 
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "cmd.h"
#include <libvjmsg/vj-msg.h>
#include <sys/types.h>
#include <sys/utsname.h>
#ifdef STRICT_CHECKING
#include <assert.h>
#endif
vj_sock_t	*alloc_sock_t(void)
{
	vj_sock_t *s = (vj_sock_t*) malloc(sizeof(vj_sock_t));
	if(!s) return NULL;
	return s;
}

void		sock_t_free(vj_sock_t *s )
{
	if(s) free(s);
}

#define TIMEOUT 3

int			sock_t_connect_and_send_http( vj_sock_t *s, char *host, int port, char *buf, int buf_len )
{
	s->he = gethostbyname( host );
	if(s->he==NULL)
		return 0;
	s->sock_fd = socket( AF_INET, SOCK_STREAM , 0);
	if(s->sock_fd < 0)
	{
		return 0;
	}
	s->port_num = port;
	s->addr.sin_family = AF_INET;
	s->addr.sin_port   = htons( port );
	s->addr.sin_addr   = *( (struct in_addr*) s->he->h_addr );	
	if( connect( s->sock_fd, (struct sockaddr*) &s->addr,
			sizeof( struct sockaddr )) == -1 )
	{
		return 0;
	}

	struct sockaddr_in sinfo;
	socklen_t sinfolen=0;
	char server_name[1024];
	if( getsockname(s->sock_fd,(struct sockaddr*) &sinfo,&sinfolen)==0) {
		char *tmp = inet_ntoa( sinfo.sin_addr );
		strncpy( server_name, tmp, 1024);
	} else {
		return 0;
	}

	int len = strlen(server_name) + 128 + buf_len;
	char *msg = (char*) malloc(sizeof(char) * len );
	struct utsname name;
	if( uname(&name) == -1 ) {
		snprintf(msg,len,"%s%s/veejay-%s\n\n",buf,server_name,PACKAGE_VERSION);
	} else {
		snprintf(msg,len,"%s%s/veejay-%s/%s-%s\n\n",buf,server_name,PACKAGE_VERSION,name.sysname,name.release );
	}
	int msg_len = strlen(msg);
	int n = send(s->sock_fd,msg,msg_len, 0 );
	free(msg);
	
	if( n == -1 )
	{
		return 0;
	}

	return n;
}

int			sock_t_connect( vj_sock_t *s, char *host, int port )
{
	s->he = gethostbyname( host );
	if(s->he==NULL)
		return 0;
	s->sock_fd = socket( AF_INET, SOCK_STREAM , 0);
	if(s->sock_fd < 0)
	{
		veejay_msg(VEEJAY_MSG_ERROR, "Socket error with Veejay host %s:%d %s ", host,port,strerror(errno));
		return 0;
	}
	s->port_num = port;
	s->addr.sin_family = AF_INET;
	s->addr.sin_port   = htons( port );
	s->addr.sin_addr   = *( (struct in_addr*) s->he->h_addr );	

	if( connect( s->sock_fd, (struct sockaddr*) &s->addr,
			sizeof( struct sockaddr )) == -1 )
	{
		veejay_msg(VEEJAY_MSG_ERROR, "Connection error with Veejay host %s:%d %s",
				host, port, strerror(errno));
		return 0;
	}
	unsigned int tmp = sizeof(int);
	if( getsockopt( s->sock_fd , SOL_SOCKET, SO_SNDBUF, (unsigned char*) &(s->send_size), &tmp) < 0 )
	{
		veejay_msg(VEEJAY_MSG_ERROR, "Unable to get buffer size for output: %s", strerror(errno));
		return 0;
	}
	if( getsockopt( s->sock_fd, SOL_SOCKET, SO_RCVBUF, (unsigned char*) &(s->recv_size), &tmp) < 0 )
	{
		veejay_msg(VEEJAY_MSG_ERROR, "Unable to get buffer size for input: %s", strerror(errno));
		return 0;
	}

	return 1;
}

int			sock_t_poll_w(vj_sock_t *s )
{
	int	status;
	fd_set	fds;
	struct timeval no_wait;
	no_wait.tv_sec = TIMEOUT;
	no_wait.tv_usec = 0;
	memset( &no_wait, 0, sizeof(no_wait) );

	FD_ZERO( &fds );
	FD_SET( s->sock_fd, &fds );

	status = select( s->sock_fd + 1,NULL, &fds, NULL, &no_wait );
	if( status < 0 )
	{
		veejay_msg(VEEJAY_MSG_ERROR, "Unable to poll socket for immediate write: %s", strerror(errno));
		return 0;
	}
	if( status == 0 )
	{
		veejay_msg(VEEJAY_MSG_ERROR, "Timeout occured");
		return 0;
	}

	if( FD_ISSET( s->sock_fd, &fds ))
	{
		return 1;
	}

	return 0;
}


int			sock_t_poll( vj_sock_t *s )
{
	int	status;
	fd_set	fds;
	struct timeval no_wait;
	memset( &no_wait, 0, sizeof(no_wait) );

	FD_ZERO( &fds );
	FD_SET( s->sock_fd, &fds );

	status = select( s->sock_fd + 1, &fds, 0, 0, &no_wait );
	if( status < 0 )
	{
		veejay_msg(VEEJAY_MSG_ERROR, "Unable to poll socket for immediate read: %s", strerror(errno));
		return 0;
	}
	if( FD_ISSET( s->sock_fd, &fds ) )
	{
		return 1;
	}
	return 0;
}


static		int	timed_recv( int fd, void *buf, const int len, int timeout )
{
	fd_set fds;
	int	n;

	struct timeval tv;

	FD_ZERO(&fds);
	FD_SET( fd,&fds );

	tv.tv_sec  = TIMEOUT;
	tv.tv_usec = 0;

	n	  = select( fd + 1, &fds, NULL, NULL, &tv );
	if( n == 0 ) {
		veejay_msg(VEEJAY_MSG_DEBUG, "\tsocket %x :: requested %d bytes", fd, len );
	}

	if( n == -1 )
		return -1;

	if( n == 0 )
		return -5;

	return recv( fd, buf, len, 0 );
}

int			sock_t_recv_w( vj_sock_t *s, void *dst, int len )
{
	int n = 0;
	if( len < s->recv_size )
	{
		n = recv( s->sock_fd, dst, len, MSG_WAITALL );
		if(n==-1)
		{
			veejay_msg(VEEJAY_MSG_ERROR, "%s", strerror(errno));
			return -1;
		}
		return n;
	}
	else
	{
		int done = 0;
		int bytes_left = s->recv_size;

		while( done < len )
		{
			n = recv( s->sock_fd, dst + done,bytes_left,MSG_WAITALL );
			if( n == -1)
			{
				veejay_msg(VEEJAY_MSG_ERROR, "%s",strerror(errno));
				return -1;
			}
			if( n == 0 ) 
				break;

			done += n;
			
			if( (len-done) < s->recv_size)
				bytes_left = len - done;
		}
		return done;
	}
	return 0;
}

int			sock_t_recv( vj_sock_t *s, void *dst, int len )
{
	int done = 0;
	int bytes_left = s->recv_size;
	int n;
	if( len < bytes_left )
		bytes_left = len;

	while( done < len )
	{	
		n = timed_recv( s->sock_fd, dst+done,bytes_left, TIMEOUT );
		if( n == -5 ) {
			veejay_msg(VEEJAY_MSG_DEBUG, "Timeout while receiving data");	
			return -1;
		} else if ( n == -1 ) {
			veejay_msg(VEEJAY_MSG_ERROR, "%s", strerror(errno));
			return -1;
		}

		if( n == 0 )
			break;
		done += n;

		if( (len-done) < s->recv_size )
			bytes_left = len - done;
	}
	return done;
}

int			sock_t_send( vj_sock_t *s, unsigned char *buf, int len )
{
	int n; 
#ifdef STRICT_CHECKING
	assert( buf != NULL );
#endif
	int bs   = s->send_size;
	if( len < bs )
		bs = len;
	int done  = 0;
	while( done < len )
	{
		n = send( s->sock_fd, buf + done, bs , MSG_DONTWAIT );
		if(n == -1)
		{
			veejay_msg(VEEJAY_MSG_ERROR, "TCP send error: %s", strerror(errno));
			return 0;
		}
		if( n == 0 )
			break;
		
		done += n;
		if( (len-done) < s->send_size)
			bs = len - done;
	}
	return done;
}

int			sock_t_send_fd( int fd, int send_size, unsigned char *buf, int len )
{
	int n; 
#ifdef STRICT_CHECKING
	assert( buf != NULL );
#endif
	int done = 0;
	int bs   = send_size;
	if( len < bs )
		bs = len;

	while( done < len )
	{
		n = send( fd, buf + done, bs , MSG_DONTWAIT  );
		if(n == -1)
		{
			veejay_msg(VEEJAY_MSG_ERROR, "TCP send error: %s", strerror(errno));
			return 0;
		}
		if( n == 0 )
			break;
		
		done += n;
		if( (len-done) < send_size)
			bs = len - done;
	}
	return done;
}

void			sock_t_close( vj_sock_t *s )
{
	if(s)
	{
		close(s->sock_fd);
		s->sock_fd = 0;
	}
}
