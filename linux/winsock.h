#include "ideastypes.h"
#include <unistd.h>
#include <netdb.h>
#include <resolv.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>
#include "syncobject.h"

#ifndef __WINSOCKH__
#define __WINSOCKH__

typedef HANDLE WSAEVENT;
typedef int SOCKET;

#define NO_ERROR 							0L                                                 // dderror
#define ERROR_INVALID_DATA               	13L
#define ERROR_OUTOFMEMORY                	14L
#define ERROR_INVALID_PARAMETER          	87L    // dderror
#define ERROR_BUFFER_OVERFLOW            	111L
#define ERROR_INSUFFICIENT_BUFFER        	122L    // dderror
#define ERROR_NO_DATA                    	232L
#define ERROR_NO_SYSTEM_RESOURCES        	1450L

#define MAX_INTERFACE_PHYSADDR    			8
#define MAX_INTERFACE_DESCRIPTION 			256
#define MAX_ADAPTER_DESCRIPTION_LENGTH  128 // arb.
#define MAX_ADAPTER_NAME_LENGTH         256 // arb.
#define MAX_ADAPTER_ADDRESS_LENGTH      8   // arb.
#define DEFAULT_MINIMUM_ENTITIES        32  // arb.
#define MAX_HOSTNAME_LEN                128 // arb.
#define MAX_DOMAIN_NAME_LEN             128 // arb.
#define MAX_SCOPE_ID_LEN                256 // arb.

#define MIB_IF_TYPE_OTHER               1
#define MIB_IF_TYPE_ETHERNET            6
#define MIB_IF_TYPE_TOKENRING           9
#define MIB_IF_TYPE_FDDI                15
#define MIB_IF_TYPE_PPP                 23
#define MIB_IF_TYPE_LOOPBACK            24
#define MIB_IF_TYPE_SLIP                28

#define IF_TYPE_OTHER                   1   // None of the below
#define IF_TYPE_REGULAR_1822            2
#define IF_TYPE_HDH_1822                3
#define IF_TYPE_DDN_X25                 4
#define IF_TYPE_RFC877_X25              5
#define IF_TYPE_ETHERNET_CSMACD         6
#define IF_TYPE_IS088023_CSMACD         7
#define IF_TYPE_ISO88024_TOKENBUS       8
#define IF_TYPE_ISO88025_TOKENRING      9
#define IF_TYPE_ISO88026_MAN            10
#define IF_TYPE_STARLAN                 11
#define IF_TYPE_PROTEON_10MBIT          12
#define IF_TYPE_PROTEON_80MBIT          13
#define IF_TYPE_HYPERCHANNEL            14
#define IF_TYPE_FDDI                    15
#define IF_TYPE_LAP_B                   16
#define IF_TYPE_SDLC                    17
#define IF_TYPE_DS1                     18  // DS1-MIB
#define IF_TYPE_E1                      19  // Obsolete; see DS1-MIB
#define IF_TYPE_BASIC_ISDN              20
#define IF_TYPE_PRIMARY_ISDN            21
#define IF_TYPE_PROP_POINT2POINT_SERIAL 22  // proprietary serial
#define IF_TYPE_PPP                     23
#define IF_TYPE_SOFTWARE_LOOPBACK       24
#define IF_TYPE_EON                     25  // CLNP over IP
#define IF_TYPE_ETHERNET_3MBIT          26
#define IF_TYPE_NSIP                    27  // XNS over IP
#define IF_TYPE_SLIP                    28  // Generic Slip
#define IF_TYPE_ULTRA                   29  // ULTRA Technologies
#define IF_TYPE_DS3                     30  // DS3-MIB
#define IF_TYPE_SIP                     31  // SMDS, coffee
#define IF_TYPE_FRAMERELAY              32  // DTE only
#define IF_TYPE_RS232                   33
#define IF_TYPE_PARA                    34  // Parallel port
#define IF_TYPE_ARCNET                  35
#define IF_TYPE_ARCNET_PLUS             36
#define IF_TYPE_ATM                     37  // ATM cells
#define IF_TYPE_MIO_X25                 38
#define IF_TYPE_SONET                   39  // SONET or SDH
#define IF_TYPE_X25_PLE                 40
#define IF_TYPE_ISO88022_LLC            41
#define IF_TYPE_LOCALTALK               42
#define IF_TYPE_SMDS_DXI                43
#define IF_TYPE_FRAMERELAY_SERVICE      44  // FRNETSERV-MIB
#define IF_TYPE_V35                     45
#define IF_TYPE_HSSI                    46
#define IF_TYPE_HIPPI                   47
#define IF_TYPE_MODEM                   48  // Generic Modem
#define IF_TYPE_AAL5                    49  // AAL5 over ATM
#define IF_TYPE_SONET_PATH              50
#define IF_TYPE_SONET_VT                51
#define IF_TYPE_SMDS_ICIP               52  // SMDS InterCarrier Interface
#define IF_TYPE_PROP_VIRTUAL            53  // Proprietary virtual/internal
#define IF_TYPE_PROP_MULTIPLEXOR        54  // Proprietary multiplexing
#define IF_TYPE_IEEE80212               55  // 100BaseVG
#define IF_TYPE_FIBRECHANNEL            56
#define IF_TYPE_HIPPIINTERFACE          57
#define IF_TYPE_FRAMERELAY_INTERCONNECT 58  // Obsolete, use 32 or 44
#define IF_TYPE_AFLANE_8023             59  // ATM Emulated LAN for 802.3
#define IF_TYPE_AFLANE_8025             60  // ATM Emulated LAN for 802.5
#define IF_TYPE_CCTEMUL                 61  // ATM Emulated circuit
#define IF_TYPE_FASTETHER               62  // Fast Ethernet (100BaseT)
#define IF_TYPE_ISDN                    63  // ISDN and X.25
#define IF_TYPE_V11                     64  // CCITT V.11/X.21
#define IF_TYPE_V36                     65  // CCITT V.36
#define IF_TYPE_G703_64K                66  // CCITT G703 at 64Kbps
#define IF_TYPE_G703_2MB                67  // Obsolete; see DS1-MIB
#define IF_TYPE_QLLC                    68  // SNA QLLC
#define IF_TYPE_FASTETHER_FX            69  // Fast Ethernet (100BaseFX)
#define IF_TYPE_CHANNEL                 70
#define IF_TYPE_IEEE80211               71  // Radio spread spectrum
#define IF_TYPE_IBM370PARCHAN           72  // IBM System 360/370 OEMI Channel
#define IF_TYPE_ESCON                   73  // IBM Enterprise Systems Connection
#define IF_TYPE_DLSW                    74  // Data Link Switching
#define IF_TYPE_ISDN_S                  75  // ISDN S/T interface
#define IF_TYPE_ISDN_U                  76  // ISDN U interface
#define IF_TYPE_LAP_D                   77  // Link Access Protocol D
#define IF_TYPE_IPSWITCH                78  // IP Switching Objects
#define IF_TYPE_RSRB                    79  // Remote Source Route Bridging
#define IF_TYPE_ATM_LOGICAL             80  // ATM Logical Port
#define IF_TYPE_DS0                     81  // Digital Signal Level 0
#define IF_TYPE_DS0_BUNDLE              82  // Group of ds0s on the same ds1
#define IF_TYPE_BSC                     83  // Bisynchronous Protocol
#define IF_TYPE_ASYNC                   84  // Asynchronous Protocol
#define IF_TYPE_CNR                     85  // Combat Net Radio
#define IF_TYPE_ISO88025R_DTR           86  // ISO 802.5r DTR
#define IF_TYPE_EPLRS                   87  // Ext Pos Loc Report Sys
#define IF_TYPE_ARAP                    88  // Appletalk Remote Access Protocol
#define IF_TYPE_PROP_CNLS               89  // Proprietary Connectionless Proto
#define IF_TYPE_HOSTPAD                 90  // CCITT-ITU X.29 PAD Protocol
#define IF_TYPE_TERMPAD                 91  // CCITT-ITU X.3 PAD Facility
#define IF_TYPE_FRAMERELAY_MPI          92  // Multiproto Interconnect over FR
#define IF_TYPE_X213                    93  // CCITT-ITU X213
#define IF_TYPE_ADSL                    94  // Asymmetric Digital Subscrbr Loop
#define IF_TYPE_RADSL                   95  // Rate-Adapt Digital Subscrbr Loop
#define IF_TYPE_SDSL                    96  // Symmetric Digital Subscriber Loop
#define IF_TYPE_VDSL                    97  // Very H-Speed Digital Subscrb Loop
#define IF_TYPE_ISO88025_CRFPRINT       98  // ISO 802.5 CRFP
#define IF_TYPE_MYRINET                 99  // Myricom Myrinet
#define IF_TYPE_VOICE_EM                100 // Voice recEive and transMit
#define IF_TYPE_VOICE_FXO               101 // Voice Foreign Exchange Office
#define IF_TYPE_VOICE_FXS               102 // Voice Foreign Exchange Station
#define IF_TYPE_VOICE_ENCAP             103 // Voice encapsulation
#define IF_TYPE_VOICE_OVERIP            104 // Voice over IP encapsulation
#define IF_TYPE_ATM_DXI                 105 // ATM DXI
#define IF_TYPE_ATM_FUNI                106 // ATM FUNI
#define IF_TYPE_ATM_IMA                 107 // ATM IMA
#define IF_TYPE_PPPMULTILINKBUNDLE      108 // PPP Multilink Bundle
#define IF_TYPE_IPOVER_CDLC             109 // IBM ipOverCdlc
#define IF_TYPE_IPOVER_CLAW             110 // IBM Common Link Access to Workstn
#define IF_TYPE_STACKTOSTACK            111 // IBM stackToStack
#define IF_TYPE_VIRTUALIPADDRESS        112 // IBM VIPA
#define IF_TYPE_MPC                     113 // IBM multi-proto channel support
#define IF_TYPE_IPOVER_ATM              114 // IBM ipOverAtm
#define IF_TYPE_ISO88025_FIBER          115 // ISO 802.5j Fiber Token Ring
#define IF_TYPE_TDLC                    116 // IBM twinaxial data link control
#define IF_TYPE_GIGABITETHERNET         117
#define IF_TYPE_HDLC                    118
#define IF_TYPE_LAP_F                   119
#define IF_TYPE_V37                     120
#define IF_TYPE_X25_MLP                 121 // Multi-Link Protocol
#define IF_TYPE_X25_HUNTGROUP           122 // X.25 Hunt Group
#define IF_TYPE_TRANSPHDLC              123
#define IF_TYPE_INTERLEAVE              124 // Interleave channel
#define IF_TYPE_FAST                    125 // Fast channel
#define IF_TYPE_IP                      126 // IP (for APPN HPR in IP networks)
#define IF_TYPE_DOCSCABLE_MACLAYER      127 // CATV Mac Layer
#define IF_TYPE_DOCSCABLE_DOWNSTREAM    128 // CATV Downstream interface
#define IF_TYPE_DOCSCABLE_UPSTREAM      129 // CATV Upstream interface
#define IF_TYPE_A12MPPSWITCH            130 // Avalon Parallel Processor
#define IF_TYPE_TUNNEL                  131 // Encapsulation interface
#define IF_TYPE_COFFEE                  132 // Coffee pot
#define IF_TYPE_CES                     133 // Circuit Emulation Service
#define IF_TYPE_ATM_SUBINTERFACE        134 // ATM Sub Interface
#define IF_TYPE_L2_VLAN                 135 // Layer 2 Virtual LAN using 802.1Q
#define IF_TYPE_L3_IPVLAN               136 // Layer 3 Virtual LAN using IP
#define IF_TYPE_L3_IPXVLAN              137 // Layer 3 Virtual LAN using IPX
#define IF_TYPE_DIGITALPOWERLINE        138 // IP over Power Lines
#define IF_TYPE_MEDIAMAILOVERIP         139 // Multimedia Mail over IP
#define IF_TYPE_DTM                     140 // Dynamic syncronous Transfer Mode
#define IF_TYPE_DCN                     141 // Data Communications Network
#define IF_TYPE_IPFORWARD               142 // IP Forwarding Interface
#define IF_TYPE_MSDSL                   143 // Multi-rate Symmetric DSL
#define IF_TYPE_IEEE1394                144 // IEEE1394 High Perf Serial Bus

#define MAX_IF_TYPE                     144


#define WSADESCRIPTION_LEN      		256
#define WSASYS_STATUS_LEN       		128

#define WSA_INVALID_EVENT       		((WSAEVENT)NULL)
#define WSA_WAIT_FAILED         		(WAIT_FAILED)
#define WSA_WAIT_EVENT_0        		(WAIT_OBJECT_0)
#define WSA_WAIT_IO_COMPLETION  		(WAIT_IO_COMPLETION)
#define WSA_WAIT_TIMEOUT        		(WAIT_TIMEOUT)
#define WSA_INFINITE            		(INFINITE)

#define INVALID_SOCKET          		(SOCKET)(-1)
#define SOCKET_ERROR            		(-1)

#define SD_RECEIVE      				0x00
#define SD_SEND         				0x01
#define SD_BOTH         				0x02

#define FD_READ         0x01
#define FD_WRITE        0x02
#define FD_OOB          0x04
#define FD_ACCEPT       0x08
#define FD_CONNECT      0x10
#define FD_CLOSE        0x20

#define IOCPARM_MASK    0x7f            /* parameters must be < 128 bytes */
#define IOC_VOID        0x20000000      /* no parameters */

typedef struct _InterfaceIndexTable {
  DWORD numIndexes;
  DWORD indexes[1];
} InterfaceIndexTable;

typedef struct WSAData {
	WORD           wVersion;
    WORD           wHighVersion;
    char           szDescription[WSADESCRIPTION_LEN+1];
    char           szSystemStatus[WSASYS_STATUS_LEN+1];
    unsigned short iMaxSockets;
    unsigned short iMaxUdpDg;
    char FAR *     lpVendorInfo;
} WSADATA;

typedef WSADATA FAR *LPWSADATA;

//
// IP_ADDRESS_STRING - store an IP address as a dotted decimal string
//

typedef struct {
    char String[4 * 4];
} IP_ADDRESS_STRING, *PIP_ADDRESS_STRING, IP_MASK_STRING, *PIP_MASK_STRING;

//
// IP_ADDR_STRING - store an IP address with its corresponding subnet mask,
// both as dotted decimal strings
//

typedef struct _IP_ADDR_STRING {
    struct _IP_ADDR_STRING* Next;
    IP_ADDRESS_STRING IpAddress;
    IP_MASK_STRING IpMask;
    DWORD Context;
} IP_ADDR_STRING, *PIP_ADDR_STRING;

//
// ADAPTER_INFO - per-adapter information. All IP addresses are stored as
// strings
//

typedef struct _IP_ADAPTER_INFO {
    struct _IP_ADAPTER_INFO* Next;
    DWORD ComboIndex;
    char AdapterName[MAX_ADAPTER_NAME_LENGTH + 4];
    char Description[MAX_ADAPTER_DESCRIPTION_LENGTH + 4];
    UINT AddressLength;
    BYTE Address[MAX_ADAPTER_ADDRESS_LENGTH];
    DWORD Index;
    UINT Type;
    UINT DhcpEnabled;
    PIP_ADDR_STRING CurrentIpAddress;
    IP_ADDR_STRING IpAddressList;
    IP_ADDR_STRING GatewayList;
    IP_ADDR_STRING DhcpServer;
    BOOL HaveWins;
    IP_ADDR_STRING PrimaryWinsServer;
    IP_ADDR_STRING SecondaryWinsServer;
    time_t LeaseObtained;
    time_t LeaseExpires;
} IP_ADAPTER_INFO, *PIP_ADAPTER_INFO;

//
// IP_PER_ADAPTER_INFO - per-adapter IP information such as DNS server list.
//

typedef struct _IP_PER_ADAPTER_INFO {
    UINT AutoconfigEnabled;
    UINT AutoconfigActive;
    PIP_ADDR_STRING CurrentDnsServer;
    IP_ADDR_STRING DnsServerList;
} IP_PER_ADAPTER_INFO, *PIP_PER_ADAPTER_INFO;

//
// FIXED_INFO - the set of IP-related information which does not depend on DHCP
//

typedef struct {
    char HostName[MAX_HOSTNAME_LEN + 4] ;
    char DomainName[MAX_DOMAIN_NAME_LEN + 4];
    PIP_ADDR_STRING CurrentDnsServer;
    IP_ADDR_STRING DnsServerList;
    UINT NodeType;
    char ScopeId[MAX_SCOPE_ID_LEN + 4];
    UINT EnableRouting;
    UINT EnableProxy;
    UINT EnableDns;
} FIXED_INFO, *PFIXED_INFO;

#define ANY_SIZE 1

typedef struct _MIB_IPADDRROW
{
    DWORD		   dwAddr;
    DWORD		   dwIndex;
    DWORD		   dwMask;
    DWORD		   dwBCastAddr;
    DWORD		   dwReasmSize;
    unsigned short unused1;
    unsigned short unused2;
} MIB_IPADDRROW, *PMIB_IPADDRROW;

typedef struct _MIB_IPADDRTABLE
{
    DWORD         dwNumEntries;
    MIB_IPADDRROW table[ANY_SIZE];
} MIB_IPADDRTABLE, *PMIB_IPADDRTABLE;

class WSAEvent : public LEvent
{
public:
	WSAEvent();
	virtual ~WSAEvent();
	void OnLoop();
	void Select(SOCKET s,long lNetworkEvents);
protected:
	static void *event_loop(void *arg);
	void Destroy();
	long mode;
	SOCKET sock;
	pthread_t event_thread;		
    BOOL bQuit;
};

#define closesocket(a) close(a)
#define ioctlsocket(a,b,c) ioctl(a,b,c)

int WSAStartup(WORD wVersionRequired,LPWSADATA lpWSAData);
int WSACleanup(void);
int WSAGetLastError(void);

WSAEVENT WSACreateEvent(void);
BOOL WSAResetEvent(WSAEVENT hEvent);
BOOL WSASetEvent(WSAEVENT hEvent);
DWORD WSAWaitForMultipleEvents(DWORD cEvents,WSAEVENT *lphEvents,BOOL fWaitAll,DWORD dwTimeout,BOOL fAlertable);
int WSAEventSelect(SOCKET s,WSAEVENT hEventObject,long lNetworkEvents);
BOOL WSACloseEvent(WSAEVENT hEvent);

DWORD GetNetworkParams(PFIXED_INFO pFixedInfo, ULONG *pOutBufLen);
DWORD GetAdaptersInfo(PIP_ADAPTER_INFO pAdapterInfo, ULONG *pOutBufLen);

unsigned long inet_addr(const char* cp);
char *inet_ntoa (struct in_addr in);

#endif


