#include "ft_traceroute.h"

void send_packet(t_traceroute *t)
{
	t_icmp_packet	pkt;
	size_t		curr_sequence = t->sequence++;
	
	memset(&pkt, 0, sizeof(t_icmp_packet));
	
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
		.sin_addr.s_addr	= inet_addr(t->ip_addr),
	};

	if (t->first_hop != -1)
	{
		if (setsockopt(t->server_sock, IPPROTO_IP, IP_TTL, &t->first_hop, sizeof(t->first_hop)) < 0)
		{
			perror("setsockopt TTL failed");
			close(t->server_sock);
			free(t->ip_addr);
			exit(1);
		}
	}

	int ret_send = sendto(
		t->server_sock, &pkt, sizeof(pkt), 0,
		(struct sockaddr *)&dest_addr, sizeof(dest_addr)
	);

	if (ret_send < 0)
	{
		perror("sendto");
		close(t->server_sock);
		free(t->ip_addr);
		exit(1);
	}
	gettimeofday(&t->time_send, NULL);
}
