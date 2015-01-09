
#include "stun.h"

static unsigned char packet_data[1024];
/* act like sequence N.O. */
static unsigned char transaction_id[16] = {
    0x12, 0x34, 0x56, 0x78,  0x90, 0x00, 0x00, 0x00,
    0x23, 0x34, 0x45, 0x56,  0x00, 0x00, 0x00, 0x00
};

/* Step mask, each */
unsigned int stun_steps_mask = 0;
unsigned int finished_steps  = 0;

int send_stun_bindrequest()
{
    return 0;
}

int need_resend_udp()
{
    int type;

    if(finished_steps < stun_steps_mask)
    {
    }

    return type;
}
/**
 * simply increment the last byte of transaction ID
 *
 */
int gen_transaction_id(char *id)
{
    transaction_id[15] ++;
    memcpy(id, transaction_id, sizeof(transaction_id));
    return 0;
}

/**
 * the Test_I in RFC3489 Page 22
 *
 * @stun_addr : the target stun server address
 *
 */
int test_I(int udp_s, struct sockaddr *stun_addr)
{
    size_t size = 0;
    struct stun_header *hdr;
    struct stun_attr *attr;
    int    packet_len = 0;

    memset(packet_data, 0x00, sizeof(packet_data));
    hdr = (struct stun_header *)packet_data;
    hdr->msgtype = htons(STUN_BINDREQ);  // be note it's in network endian!
    hdr->msglen = htons(8);
    gen_transaction_id(hdr->id);

    assert(sizeof(struct stun_header) == 20);
    attr = (struct stun_attr *) (packet_data + sizeof(struct stun_header));
    attr->attr = ntohs(STUN_CHANGE_REQUEST);
    attr->len = ntohs(4);

    packet_len = sizeof(struct stun_header) + sizeof(struct stun_attr) + 4;

    /* first packet won't contain the attribute! */
    size = sendto(udp_s, packet_data, packet_len, 0 /*flag*/,
            stun_addr, sizeof(struct sockaddr));
    printf("Client -> Server with (%d) bytes data(expected %d byte)\n",
            size, packet_len);
    printf("Transaction ID : STUB"); // 16-byte , too large
    printf("\n\n");

    stun_steps_mask = STUN_TEST_I;

    return 0;
}

int process_udp_backdata(int udp_s, struct sockaddr *src_addr)
{
    size_t size;
    socklen_t len;
    struct stun_header *hdr;

    size = recvfrom(udp_s, packet_data, sizeof(packet_data), 0,
            src_addr, &len);

    printf("Client<---Server with (%d) bytes (expected %d bytes)\n",
            size, sizeof(packet_data));

    if(size <= 0)
    {
        fprintf(stderr, "seems meet an error case for recv UDP\n");
    }
    else
    {
        hdr = (struct stun_header *) packet_data;
        if(!memcmp(hdr->id, transaction_id, sizeof(transaction_id)))
        {
            printf("Got the correect response from Server\n");
        }
        else
        {
            fprintf(stderr, "un-match transaction ID packet from Server, ignore it\n");
        }
    }

    return 0;
}
/**
 * Entry point
 *
 * @stun_server : the string of stun server(such as 'stun.hello.com')
 *
 */
int get_nat_type(const char *stun_server)
{
    int ret = 0;
    int udp_sock;
    int rc;
    unsigned int number_ip = 0;
    struct hostent *ht;
    fd_set read_fds;
    struct sockaddr_in stunserver_addr;
    struct timeval timeout;

    udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(udp_sock == -1)
    {
        fprintf(stderr, "**failed create UDP socket:%d\n", errno);
        return -1;
    }

    printf("=== UDP Socket Created (%d)\n", udp_sock);
	ht = gethostbyname(stun_server);
    if(!ht)
    {
        printf("failed do DNS for %s(%d)\n", stun_server, errno);
        goto failed;
    }

    number_ip = *(int *)ht->h_addr_list[0];
    printf("%s ==> %d.%d.%d.%d\n",
            stun_server, number_ip & 0xff,
            (number_ip& 0xff00) >> 8,
            (number_ip& 0xff0000) >> 16,
            (number_ip& 0xff000000) >> 24);

    printf("\n\n\nBegin Test I ...\n");
    stunserver_addr.sin_family = AF_INET;
    /* FIXME how to handle the IPV6 ? */
    memcpy(&stunserver_addr.sin_addr, ht->h_addr_list[0], 4);
    stunserver_addr.sin_port = htons(STUNPORT);

    test_I(udp_sock, &stunserver_addr);

    while(1) {
        FD_ZERO(&read_fds);
        FD_SET(udp_sock, &read_fds);

        /* FIXME - each time of select() return,
         * timeout will be reset ?
         */
        timeout.tv_sec = 3;
#if 1 //debug
        timeout.tv_usec = 3000;
#else
        timeout.tv_usec = 300; // 300 ms
#endif

        rc = select(udp_sock + 1, &read_fds, NULL, NULL, &timeout);

        if(rc < 0)
        {
            if(errno == EAGAIN || errno == EINTR)
            {
                continue;
            }
            fprintf(stderr, "**failed call select() :%d\n", errno);
            break;
        }
        else if(!rc)
        {
            // time out case
            printf("==time out..\n");
            
        }
        else
        {
            if(FD_ISSET(udp_sock, &read_fds))
            {
                process_udp_backdata(udp_sock, &stunserver_addr);
            }
        }
    }

failed:
    close(udp_sock);

    return ret;
}
