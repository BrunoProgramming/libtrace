/* Packet types */
#define TZSP_TYPE_RX 0
#define TZSP_TYPE_TX 1
#define TZSP_TYPE_RESV 2
#define TZSP_TYPE_CONF 3
#define TZSP_TYPE_KEPT_ALIVE 4
#define TZSP_TYPE_PORT 5

/* Encapsulation types */
#define TZSP_ENCAP_ETHERNET 1
#define TZSP_ENCAP_TOKEN_RING 2
#define TZSP_ENCAP_SLIP 3
#define TZSP_ENCAP_PPP 4
#define TZSP_ENCAP_FDDI 5
#define TZSP_ENCAP_RAW 7
#define TZSP_ENCAP_80211 18
#define TZSP_ENCAP_80211_PRISM 119
#define TZSP_ENCAP_80211_AVS 127

/* Tag fields */
#define TZSP_TAG_PADDING 0
#define TZSP_TAG_END 1
#define TZSP_TAG_RAW_RSSI 10
#define TZSP_TAG_SNR 11
#define TZSP_TAG_DATE_RATE 12
#define TZSP_TAG_TIMESTAMP 13
#define TZSP_TAG_CONTENTION_FREE 15
#define TZSP_TAG_DECRYPTED 16
#define TZSP_TAG_FCS_ERROR 17
#define TZSP_TAG_RX_CHANNEL 18
#define TZSP_TAG_PACKET_COUNT 40
#define TZSP_TAG_RX_FRAME_LENGTH 41
#define TZSP_TAG_WLAN_RADIO_HDR_SERIAL 60
#define TZSP_LIBTRACE_CUSTOM_TAG_TIMEVAL 233
