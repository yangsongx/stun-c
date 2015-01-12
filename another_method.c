
#include "stun.h"

#define DEFAULT_TIMEOUT  300
#define GOTO_STRING(x)  case x : return #x

static unsigned char packet_data[1024];
/* act like sequence N.O. */
static unsigned char transaction_id[16] = {
    0x12, 0x34, 0x56, 0x78,  0x90, 0x00, 0x00, 0x00,
    0x23, 0x34, 0x45, 0x56,  0x00, 0x00, 0x00, 0x00
};

/* Step mask, each */
unsigned int stun_steps_mask = 0;
//unsigned int finished_steps  = 0;

static int    ms = (DEFAULT_TIMEOUT*1000); /* time wait(millisecond unit)  during UDP sending */

char *output_stun_hdrtype(struct stun_header *hdr)
{
    switch(hdr->msgtype)
    {
        GOTO_STRING(STUN_BINDRESP);
        GOTO_STRING(STUN_BINDERR);
        GOTO_STRING(STUN_SECRESP);
        GOTO_STRING(STUN_SECERR);

        default:
            return "Unknow";
    }
}

char *output_stun_attrtype(struct stun_attr *attr)
{
    switch(ntohs(attr->attr))
    {
        GOTO_STRING(STUN_MAPPED_ADDRESS);
        GOTO_STRING(STUN_RESPONSE_ADDRESS);
        GOTO_STRING(STUN_CHANGE_REQUEST);
        GOTO_STRING(STUN_SOURCE_ADDRESS);
        GOTO_STRING(STUN_CHANGED_ADDRESS);

        default:
            return "Unknowx)";
    }
}
int send_stun_bindrequest()
{
    return 0;
}

/**
 * Parse payload following stun message header.
 *
 */
int parse_attribute(struct stun_attr *attr, int attr_len)
{
    int index = 0;
    int type;
    int len;
    int i;

    printf("=== Attribute INFO dump ===\n");
    do {
        index += 4;
        type = ntohs(attr->attr);
        len = ntohs(attr->len);
        printf("  type 0x%04x(%s) | len 0x%04x(%d)\n",
                type, output_stun_attrtype(attr), len, len);
        printf("  ------------------\n  ");
        for(i = 0; i < len; i++)
        {
            printf("0x%02x ", attr->value[i]);
        }

        if(type == STUN_MAPPED_ADDRESS || type == STUN_SOURCE_ADDRESS || type == STUN_CHANGED_ADDRESS)
        {
            unsigned short port = *(attr->value + 2);
            printf("  %d.%d.%d.%d:%d\n",
                    attr->value[4], attr->value[5], attr->value[6], attr->value[7],
                    ntohs(port));
        }

        printf("\n  ------------------\n");

        index += len;

        attr = (struct stun_attr *)((char *)attr + (len + 4));

    } while(index < attr_len);

    printf("=== End of Attribute dump ===\n");

    return 0;
}
/**
 *
 * return a type which user should re-send
 */
int need_resend_udp()
{
    return (stun_steps_mask + 1);
}
/**
 * simply increment the last byte of transaction ID
 *
 * @id : [output] The 16-byte transaction ID store an ID value.
 */
int gen_transaction_id(unsigned char *id)
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
    gen_transaction_id((unsigned char *)hdr->id);

    assert(sizeof(struct stun_header) == 20);
    attr = (struct stun_attr *) (packet_data + sizeof(struct stun_header));
    attr->attr = ntohs(STUN_CHANGE_REQUEST);
    attr->len = ntohs(4);

    packet_len = sizeof(struct stun_header) + sizeof(struct stun_attr) + 4;

    /* first packet won't contain the attribute! */
    size = sendto(udp_s, packet_data, packet_len, 0 /*flag*/,
            stun_addr, sizeof(struct sockaddr));
    printf("Client -> Server with (%ld) bytes data(expected %d byte)\n",
            size, packet_len);
    printf("Transaction ID : STUB"); // 16-byte , too large
    printf("\n\n");

    //stun_steps_mask = STUN_TEST_I;

    return 0;
}

/**
 * Send a BINDING REQUEST with change flag set
 */
int test_II(int udp_s, struct sockaddr *stun_addr)
{
    struct stun_attr *attr;
    memset(packet_data, 0x00, sizeof(packet_data));
    hdr = (struct stun_header *)packet_data;
    hdr->msgtype = htons(STUN_BINDREQ);  // be note it's in network endian!
    hdr->msglen = htons(8);
    gen_transaction_id((unsigned char *)hdr->id);

    assert(sizeof(struct stun_header) == 20);
    /* set CHANGE IP and CHANGE Port flags */
    attr = (struct stun_attr *) (packet_data + sizeof(struct stun_header));
    attr->attr = ntohs(STUN_CHANGE_REQUEST);
    attr->len = ntohs(4);

    return 0;
}

int process_udp_backdata(int udp_s, struct sockaddr *src_addr)
{
    size_t size;
    socklen_t len;
    struct stun_header *hdr;

    size = recvfrom(udp_s, packet_data, sizeof(packet_data), 0,
            src_addr, &len);

    printf("Client<---Server with (%ld) bytes (expected %ld bytes)\n",
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
            printf("Got the correect response from Server, type:%s\n",
                    output_stun_hdrtype(hdr));
            stun_steps_mask++;
            //reset the timeout wait time
            ms = DEFAULT_TIMEOUT;
            switch(hdr->msgtype)
            {
                case STUN_BINDRESP:
                    printf("payload %d byte\n", ntohs(hdr->msglen));
                    break;

            }

            parse_attribute((struct stun_attr *)(packet_data + sizeof(struct stun_header)),
                    ntohs(hdr->msglen));
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
    int    type;

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

    test_I(udp_sock, (struct sockaddr *)&stunserver_addr);

    while(1) {
        FD_ZERO(&read_fds);
        FD_SET(udp_sock, &read_fds);

        /* FIXME - each time of select() return,
         * timeout will be reset ?
         */
        timeout.tv_sec = 0;
        timeout.tv_usec = ms;

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
            if(ms > 5000*1000) {
                printf("exceed limit 5 seconds, don't try anymore\n");
                break;
            }
            type = need_resend_udp();
            switch(type)
            {
                case STUN_TEST_I:
                    printf("re-send the TEST_I again(ms = %d)...\n", ms);
                    test_I(udp_sock, (struct sockaddr *) &stunserver_addr);
                    ms += 400 * 1000;
                    break;

                case STUN_TEST_II:
                    break;
            }
#if 0
            printf("==time out..\n");
            if(ms > 5000) {
                printf("exceed limit, don't try anymore\n");
                break;
            }

            type = need_resend_udp();
            switch(type)
            {
                case STUN_TEST_I:
                    ms += 400;
                    break;

                default:
                    printf("Unknow type\n");
                    break;
            }
#endif        
        }
        else
        {
            if(FD_ISSET(udp_sock, &read_fds))
            {
                process_udp_backdata(udp_sock, (struct sockaddr *)&stunserver_addr);
            }
        }
    }

failed:
    close(udp_sock);

    return ret;
}
