/**********************************************************************
 * file:  sr_router.c
 * date:  Mon Feb 18 12:50:42 PST 2002
 * Contact: casado@stanford.edu
 *
 * Description:
 *
 * This file contains all the functions that interact directly
 * with the routing table, as well as the main entry method
 * for routing.
 *
 **********************************************************************/

#include <stdio.h>
#include <assert.h>


#include "sr_if.h"
#include "sr_rt.h"
#include "sr_router.h"
#include "sr_protocol.h"
#include "sr_arpcache.h"
#include "sr_utils.h"

/*---------------------------------------------------------------------
 * Method: sr_init(void)
 * Scope:  Global
 *
 * Initialize the routing subsystem
 *
 *---------------------------------------------------------------------*/

void sr_init(struct sr_instance* sr)
{
    /* REQUIRES */
    assert(sr);

    /* Initialize cache and cache cleanup thread */
    sr_arpcache_init(&(sr->cache));

    pthread_attr_init(&(sr->attr));
    pthread_attr_setdetachstate(&(sr->attr), PTHREAD_CREATE_JOINABLE);
    pthread_attr_setscope(&(sr->attr), PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setscope(&(sr->attr), PTHREAD_SCOPE_SYSTEM);
    pthread_t thread;

    pthread_create(&thread, &(sr->attr), sr_arpcache_timeout, sr);
    
    /* Add initialization code here! */

} /* -- sr_init -- */

/*---------------------------------------------------------------------
 * Method: sr_handlepacket(uint8_t* p,char* interface)
 * Scope:  Global
 *
 * This method is called each time the router receives a packet on the
 * interface.  The packet buffer, the packet length and the receiving
 * interface are passed in as parameters. The packet is complete with
 * ethernet headers.
 *
 * Note: Both the packet buffer and the character's memory are handled
 * by sr_vns_comm.c that means do NOT delete either.  Make a copy of the
 * packet instead if you intend to keep it around beyond the scope of
 * the method call.
 *
 *---------------------------------------------------------------------*/

void sr_handlepacket(struct sr_instance* sr,
        uint8_t * packet/* lent */,
        unsigned int len,
        char* interface/* lent */)
{
  /* REQUIRES */
  assert(sr);
  assert(packet);
  assert(interface);
  printf("*** -> Received packet of length %d \n",len);

  printf("~*~*~*~ Printing Packet Headers. ~*~*~*~\n\n");
  /* printing the ethernet header fields to test */
  print_hdrs(packet, len);
	printf("~*~*~*~ Header finished, Starting printing packet processing. ~*~*~*~\n\n");
	

	if (ethertype(packet) == ethertype_arp) /* If this is an ARP packet */
    printf ("This is an ARP Packet!\n");
    /* If it's a reply to me: Cache it, go through my request queue and send outstanding packets */
      /* fill in code here */

    /* If it's a request to me: Construct an ARP reply and send it back */
      /* fill in code here */

	if (ethertype(packet) == ethertype_ip) { /* If this is an IP packet */
  printf ("This is an IP Packet!\n");
  sr_ip_hdr_t *iphdr = (sr_ip_hdr_t *) (packet + sizeof(sr_ethernet_hdr_t));
		if (sr_if_list_contains_ip(sr, iphdr->ip_dst)) { /* If the packet is for the router */
      printf ("The IP packet is for me!\n");

      /* If it's ICMP echo req, send echo reply. */
      sr_icmp_hdr_t *icmp_hdr = (sr_icmp_hdr_t *)(packet + sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t)); 
      if (icmp_hdr->icmp_type == 8 && icmp_hdr->icmp_code == 0) { /* If this is an echo request */
        /* fill in code here */
        printf ("This is an ICMP Echo request!\n");
      } 

      /* Else if it's TCP/UDP, send ICMP port unreachable */
        /* fill in code here */

		} else { /* If the packet is not for the router */
			printf ("This IP packet is not for me!\n");

      /* Perform checksums */
      /* If the packet is not for the router, check routing table, perform LPM */

		}
	}

  printf("~*~*~*~ Finished printing packet processing. ~*~*~*~\n\n");

}/* end sr_ForwardPacket */
