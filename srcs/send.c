#include "ft_traceroute.h"

void send_packet(t_traceroute *p)
{
	t_packet	pkt;
	size_t		curr_sequence = p->sequence++;
	
	memset(&pkt, 0, sizeof(t_packet));
	
	pkt.hdr.type = ICMP_ECHO; // 8 = echo request - 0 = echo reply
	pkt.hdr.code = 0;
	pkt.hdr.un.echo.id = htons(getpid());
	pkt.hdr.un.echo.sequence = htons(curr_sequence);

	gettimeofday(&pkt.payload.timestamp, NULL);
	memset(pkt.payload.data, 'A', sizeof(pkt.payload.data));

	pkt.hdr.checksum = 0;
	pkt.hdr.checksum = htons(checksum((void *)&pkt, sizeof(pkt)));

	struct sockaddr_in dest_addr = {
		.sin_family		 	= AF_INET,
		.sin_addr.s_addr	= inet_addr(p->ip_addr),
	};

	if (p->custom_ttl != -1)
	{
		if (setsockopt(p->server_sock, IPPROTO_IP, IP_TTL, &p->custom_ttl, sizeof(p->custom_ttl)) < 0)
		{
			perror("setsockopt TTL failed");
			close(p->server_sock);
			free(p->ip_addr);
			exit(1);
		}
	}

	int ret_send = sendto(
		p->server_sock, &pkt, sizeof(pkt), 0,
		(struct sockaddr *)&dest_addr, sizeof(dest_addr)
	);

	if (ret_send < 0)
	{
		perror("sendto");
		close(p->server_sock);
		free(p->ip_addr);
		exit(1);
	}
	if (p->flood_mode == true && p->quiet_mode == false)
		write(1, ".", 1);
	p->send_count++;
}
