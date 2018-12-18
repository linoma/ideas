#include <string.h>
#include "winsock.h"

static int resInit = 0;
static char winsock_string[257];
#define ifreq_len(ifr) sizeof(struct ifreq)

#define INITIAL_INTERFACES_ASSUMED 4
#define ETH_ALEN    6

#define min(a,b) a < b ? a : b
//---------------------------------------------------------------------------
char *toIPAddressString(unsigned int addr, char string[16])
{
  	struct in_addr iAddr;
  	
  	if (string) {    	
    	iAddr.s_addr = addr;
    	strncpy(string,inet_ntoa(iAddr),16);
  	}
  	return string;
}
//---------------------------------------------------------------------------
static int isLoopbackInterface(int fd, const char *name)
{
	struct ifreq ifr;
  	int ret = 0;
  	
  	if (name) {    	
    	strncpy(ifr.ifr_name, name, IFNAMSIZ);
    	if (ioctl(fd, SIOCGIFFLAGS, &ifr) == 0)
      		ret = ifr.ifr_flags & IFF_LOOPBACK;
  	}
  	return ret;
}
//---------------------------------------------------------------------------
static DWORD getNumNonLoopbackInterfaces(void)
{
  	DWORD numInterfaces;
  	int fd;
  	struct if_nameindex *indexes,*p;
  	
  	fd = socket(PF_INET, SOCK_DGRAM, 0);
  	if (fd != -1) {
    	indexes = if_nameindex();
    	if (indexes) { 		
      		for (p = indexes, numInterfaces = 0; p && p->if_name; p++)
        		if (!isLoopbackInterface(fd, p->if_name))
          			numInterfaces++;
      		if_freenameindex(indexes);
    	}
    	else
      		numInterfaces = 0;
    	close(fd);
  	}
  	else
    	numInterfaces = 0;
  	return numInterfaces;
}
//---------------------------------------------------------------------------
static DWORD enumIPAddresses(LPDWORD pcAddresses, struct ifconf *ifc)
{
  	DWORD ret;
  	int fd,ioctlRet;
    DWORD guessedNumAddresses, numAddresses;
    caddr_t ifPtr;
    int lastlen;

	ioctlRet = 0;
	guessedNumAddresses = numAddresses = 0;
  	fd = socket(PF_INET, SOCK_DGRAM, 0);
  	if (fd != -1) {
    	ret = NO_ERROR;
    	ifc->ifc_len = 0;
    	ifc->ifc_buf = NULL;
    	do {
      		lastlen = ifc->ifc_len;
      		if(ifc->ifc_buf != NULL)
      			free(ifc->ifc_buf);      		
      		if (guessedNumAddresses == 0)
        		guessedNumAddresses = INITIAL_INTERFACES_ASSUMED;
      		else
        		guessedNumAddresses *= 2;
      		ifc->ifc_len = sizeof(struct ifreq) * guessedNumAddresses;
      		ifc->ifc_buf = (char *)calloc(ifc->ifc_len,1);
      		ioctlRet = ioctl(fd, SIOCGIFCONF, ifc);
   		} while ((ioctlRet == 0) && (ifc->ifc_len != lastlen));
    	if (ioctlRet == 0) {
      		ifPtr = ifc->ifc_buf;
      		while (ifPtr && ifPtr < ifc->ifc_buf + ifc->ifc_len) {
        		struct ifreq *ifr = (struct ifreq *)ifPtr;
        		if (ifr->ifr_addr.sa_family == AF_INET)
          			numAddresses++;
        		ifPtr += ifreq_len((struct ifreq *)ifPtr);
      		}
    	}
    	else
      		ret = ERROR_INVALID_PARAMETER; /* FIXME: map from errno to Win32 */
    	if (!ret)
      		*pcAddresses = numAddresses;
    	else
    	{
      		if(ifc->ifc_buf != NULL){      		
      			free(ifc->ifc_buf);
      			ifc->ifc_buf = NULL;
      		}
    	}
    	close(fd);
  	}
  	else
    	ret = ERROR_NO_SYSTEM_RESOURCES;
  	return ret;
}
//---------------------------------------------------------------------------
static DWORD getNumIPAddresses(void)
{
  	DWORD numAddresses = 0;
  	struct ifconf ifc;

  	if (!enumIPAddresses(&numAddresses, &ifc)){
  		if(ifc.ifc_buf != NULL)
  			free(ifc.ifc_buf);
	}
  	return numAddresses;
}
//---------------------------------------------------------------------------
static char *getInterfaceNameByIndex(DWORD index, char *name)
{
	return if_indextoname(index, name);
}
//---------------------------------------------------------------------------
static DWORD getInterfaceIndexByName(const char *name, LPDWORD index)
{
  	DWORD ret;
  	unsigned int idx;

  	if (!name)
    	return ERROR_INVALID_PARAMETER;
  	if (!index)
    	return ERROR_INVALID_PARAMETER;
  	idx = if_nametoindex(name);
  	if (idx) {
    	*index = idx;
    	ret = NO_ERROR;
  	}
  	else
    	ret = ERROR_INVALID_DATA;
  	return ret;
}
//---------------------------------------------------------------------------
InterfaceIndexTable *getNonLoopbackInterfaceIndexTable(void)
{
  	DWORD numInterfaces;
  	InterfaceIndexTable *ret;
  	struct if_nameindex *indexes,*p;
  	int fd;
  	
  	fd = socket(PF_INET, SOCK_DGRAM, 0);
  	if (fd != -1) {
    	indexes = if_nameindex();
    	if (indexes) {
      		for (p = indexes, numInterfaces = 0; p && p->if_name; p++)
        		if (!isLoopbackInterface(fd, p->if_name))
          			numInterfaces++;
      		ret = (InterfaceIndexTable *)calloc(sizeof(InterfaceIndexTable) + (numInterfaces - 1) * sizeof(DWORD),1);
      		if (ret) {
        		for (p = indexes; p && p->if_name; p++)
          			if (!isLoopbackInterface(fd, p->if_name))
            			ret->indexes[ret->numIndexes++] = p->if_index;
      		}
      		if_freenameindex(indexes);
    	}
    	else
      		ret = NULL;
    	close(fd);
  	}
  	else
    	ret = NULL;
  	return ret;
}
//---------------------------------------------------------------------------
static DWORD getInterfaceMaskByName(const char *name)
{
  	DWORD ret = INADDR_NONE;
	int fd;
	struct ifreq ifr;
	
  	if (name) {
    	fd = socket(PF_INET, SOCK_DGRAM, 0);
    	if (fd != -1) {      	
      		strncpy(ifr.ifr_name, name, IFNAMSIZ);
      		if (ioctl(fd, SIOCGIFNETMASK, &ifr) == 0)
        		memcpy(&ret, ifr.ifr_addr.sa_data + 2, sizeof(DWORD));
      		close(fd);
    	}
  	}
  	return ret;
}
//---------------------------------------------------------------------------
static DWORD getInterfaceBCastAddrByName(const char *name)
{
  	struct ifreq ifr;
  	DWORD ret = INADDR_ANY;
	int fd;
	
  	if (name) {
    	fd = socket(PF_INET, SOCK_DGRAM, 0);
    	if (fd != -1) {	      		
      		strncpy(ifr.ifr_name, name, IFNAMSIZ);
      		if (ioctl(fd, SIOCGIFBRDADDR, &ifr) == 0)
        		memcpy(&ret, ifr.ifr_addr.sa_data + 2, sizeof(DWORD));
      		close(fd);
    	}
  	}
  	return ret;
}
//---------------------------------------------------------------------------
static DWORD getIPAddrTable(PMIB_IPADDRTABLE *ppIpAddrTable, DWORD flags)
{
  	DWORD ret;
    DWORD numAddresses = 0;
    struct ifconf ifc;
	DWORD i = 0, bcast;
	caddr_t ifPtr;
	struct ifreq *ifr;
	
  	if (!ppIpAddrTable)
    	ret = ERROR_INVALID_PARAMETER;
  	else
  	{
    	ret = enumIPAddresses(&numAddresses, &ifc);
    	if (!ret)
    	{
      		*ppIpAddrTable = (PMIB_IPADDRTABLE)calloc(sizeof(MIB_IPADDRTABLE) + (numAddresses - 1) * sizeof(MIB_IPADDRROW),1);
      		if (*ppIpAddrTable) {
        		ret = NO_ERROR;
        		(*ppIpAddrTable)->dwNumEntries = numAddresses;
        		ifPtr = ifc.ifc_buf;
        		while (!ret && ifPtr && ifPtr < ifc.ifc_buf + ifc.ifc_len) {
          			ifr = (struct ifreq *)ifPtr;
          			ifPtr += ifreq_len(ifr);
          			if (ifr->ifr_addr.sa_family != AF_INET)
             			continue;
          			ret = getInterfaceIndexByName(ifr->ifr_name,&(*ppIpAddrTable)->table[i].dwIndex);
          			memcpy(&(*ppIpAddrTable)->table[i].dwAddr, ifr->ifr_addr.sa_data + 2,sizeof(DWORD));
          			(*ppIpAddrTable)->table[i].dwMask = getInterfaceMaskByName(ifr->ifr_name);
          			bcast = getInterfaceBCastAddrByName(ifr->ifr_name);
          			(*ppIpAddrTable)->table[i].dwBCastAddr = (bcast & (*ppIpAddrTable)->table[i].dwMask) ? 1 : 0;
          			(*ppIpAddrTable)->table[i].dwReasmSize = 65535;
          			(*ppIpAddrTable)->table[i].unused1 = 0;
          			//(*ppIpAddrTable)->table[i].wType = 0;
          			i++;
        		}
      		}
      		else
        		ret = ERROR_OUTOFMEMORY;
      		free(ifc.ifc_buf);
    	}
  	}
  	return ret;
}
//---------------------------------------------------------------------------
DWORD getInterfacePhysicalByName(const char *name, LPDWORD len, LPBYTE addr,LPDWORD type)
{
	DWORD ret;
  	int fd;
	struct ifreq ifr;
	unsigned int addrLen;
	
  	if (!name || !len || !addr || !type)
    	return ERROR_INVALID_PARAMETER;
  	fd = socket(PF_INET, SOCK_DGRAM, 0);
  	if (fd != -1) {
    	memset(&ifr, 0, sizeof(struct ifreq));
    	strncpy(ifr.ifr_name, name, IFNAMSIZ);
    	if ((ioctl(fd, SIOCGIFHWADDR, &ifr)))
      		ret = ERROR_INVALID_DATA;
    	else {      
      		switch (ifr.ifr_hwaddr.sa_family)
      		{
      		  	case ARPHRD_LOOPBACK:
      		  	  	addrLen = 0;
      		  	  	*type = MIB_IF_TYPE_LOOPBACK;
      		  	break;
      		  	case ARPHRD_ETHER:
      		  		addrLen = ETH_ALEN;
      		  		*type = MIB_IF_TYPE_ETHERNET;
      		  	break;
      		  	case ARPHRD_FDDI:
      		  	  	addrLen = ETH_ALEN;
      		  	  	*type = MIB_IF_TYPE_FDDI;
      		  	break;
      		  	case ARPHRD_IEEE802: /* 802.2 Ethernet && Token Ring, guess TR? */
      		  	  	addrLen = ETH_ALEN;
      		  	  	*type = MIB_IF_TYPE_TOKENRING;
      		  	break;
      		  	case ARPHRD_IEEE802_TR: /* also Token Ring? */
      		  	  	addrLen = ETH_ALEN;
      		  	  	*type = MIB_IF_TYPE_TOKENRING;
      		  	break;
      		  	case ARPHRD_SLIP:
      		  	  	addrLen = 0;
      		  	  	*type = MIB_IF_TYPE_SLIP;
      		  	break;
      		  	case ARPHRD_PPP:
      		  	  	addrLen = 0;
      		  	  	*type = MIB_IF_TYPE_PPP;
      		  	break;
      		  	default:
      		  	  	addrLen = min(MAX_INTERFACE_PHYSADDR, sizeof(ifr.ifr_hwaddr.sa_data));
      		  	  	*type = MIB_IF_TYPE_OTHER;
      		  	break;
      		}
      		if (addrLen > *len) {
      		  	ret = ERROR_INSUFFICIENT_BUFFER;
      		  	*len = addrLen;
      		}
      		else {
      		  	if (addrLen > 0)
      		    	memcpy(addr, ifr.ifr_hwaddr.sa_data, addrLen);
      		  	memset(addr + addrLen, 0, *len - addrLen);
      		  	*len = addrLen;
      		  	ret = NO_ERROR;
      		}
    	}
    	close(fd);
  	}
  	else
    	ret = ERROR_NO_MORE_FILES;
  	return ret;
}
//---------------------------------------------------------------------------
DWORD getInterfacePhysicalByIndex(DWORD index, LPDWORD len, LPBYTE addr,LPDWORD type)
{
	char *name,nameBuf[IF_NAMESIZE];
	
	name = getInterfaceNameByIndex(index, nameBuf);
	if (name)
	  	return getInterfacePhysicalByName(name, len, addr, type);
 	return ERROR_INVALID_DATA;
}
//---------------------------------------------------------------------------
DWORD GetNetworkParams(PFIXED_INFO pFixedInfo, ULONG *pOutBufLen)
{
    PIP_ADDR_STRING ptr;
	DWORD size;
	
  	if (!pOutBufLen)
    	return ERROR_INVALID_PARAMETER;
    if(resInit == 0){
	    res_init();
        resInit = 1;
    }
  	size = sizeof(FIXED_INFO) + (_res.nscount > 0 ? (_res.nscount  - 1) * sizeof(IP_ADDR_STRING) : 0);
  	if (!pFixedInfo || *pOutBufLen < size) {
    	*pOutBufLen = size;
    	return ERROR_BUFFER_OVERFLOW;
  	}
  	memset(pFixedInfo, 0, size); 	
    ptr = (PIP_ADDR_STRING)&pFixedInfo->DnsServerList;
	for (int i = 0 ; i < _res.nscount && ptr != NULL; i++, ptr = ptr->Next) {
    	toIPAddressString(_res.nsaddr_list[i].sin_addr.s_addr,ptr->IpAddress.String);
      	if (i == _res.nscount - 1)
        	ptr->Next = NULL;
      	else if (i == 0)
        	ptr->Next = (PIP_ADDR_STRING)((LPBYTE)pFixedInfo + sizeof(FIXED_INFO));
      	else
        	ptr->Next = (PIP_ADDR_STRING)((BYTE *)ptr + sizeof(IP_ADDR_STRING));
    }
//	gethostname(lino,500);
//    res = gethostbyname_r (lino, &hostentry, lino1, ebufsize, &host, &locerr );
	return NO_ERROR;
}
//---------------------------------------------------------------------------
DWORD GetAdaptersInfo(PIP_ADAPTER_INFO pAdapterInfo, ULONG *pOutBufLen)
{
	DWORD numNonLoopbackInterfaces,numIPAddresses,ret,addrLen,type,ndx;
    ULONG size;
   	InterfaceIndexTable *table;
   	PMIB_IPADDRTABLE ipAddrTable;
    HKEY hKey;
    BOOL winsEnabled;
    IP_ADDRESS_STRING primaryWINS, secondaryWINS;
	PIP_ADAPTER_INFO ptr;
    PIP_ADDR_STRING currentIPAddr;
    bool firstIPAddr;
    int i;
    
   	table = NULL;
   	ipAddrTable = NULL;
	winsEnabled = FALSE;
  	if (!pOutBufLen)
    	ret = ERROR_INVALID_PARAMETER;
  	else {
    	numNonLoopbackInterfaces = getNumNonLoopbackInterfaces();
    	if (numNonLoopbackInterfaces > 0) {
      		numIPAddresses = getNumIPAddresses();
      	    size = sizeof(IP_ADAPTER_INFO) * numNonLoopbackInterfaces;
      	    size += numIPAddresses  * sizeof(IP_ADDR_STRING); 
      	    if (!pAdapterInfo || *pOutBufLen < size) {
        	    *pOutBufLen = size;
        	    ret = ERROR_BUFFER_OVERFLOW;
      	    }
      	    else {
        	    ret = getIPAddrTable(&ipAddrTable, 0);
        	    if (!ret)
        		    table = getNonLoopbackInterfaceIndexTable();
        	    if (table) {
        	        size = sizeof(IP_ADAPTER_INFO) * table->numIndexes;
        	        size += ipAddrTable->dwNumEntries * sizeof(IP_ADDR_STRING); 
        	        if (*pOutBufLen < size) {
        	            *pOutBufLen = size;
        	            ret = ERROR_INSUFFICIENT_BUFFER;
        			}
        			else {
        	        	PIP_ADDR_STRING nextIPAddr = (PIP_ADDR_STRING)((LPBYTE)pAdapterInfo	+ numNonLoopbackInterfaces * sizeof(IP_ADAPTER_INFO));
        	    		memset(pAdapterInfo, 0, size);
/*        	    		if (RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Network", &hKey) == ERROR_SUCCESS) {
        	      			DWORD size = sizeof(primaryWINS.String);
        	      			unsigned long addr;
        	        	
        	      			RegQueryValueExA(hKey, "WinsServer", NULL, NULL,(LPBYTE)primaryWINS.String, &size);
        	      			addr = inet_addr(primaryWINS.String);
        	      			if (addr != INADDR_NONE && addr != INADDR_ANY)
        	        			winsEnabled = TRUE;
        	      			size = sizeof(secondaryWINS.String);
        	      			RegQueryValueExA(hKey, "BackupWinsServer", NULL, NULL,(LPBYTE)secondaryWINS.String, &size);
        	      			addr = inet_addr(secondaryWINS.String);
        	      			if (addr != INADDR_NONE && addr != INADDR_ANY)
        	        			winsEnabled = TRUE;
        	      			RegCloseKey(hKey);
        	    		}*/
        	    		for (ndx = 0; ndx < table->numIndexes; ndx++) {
        	      			ptr = &pAdapterInfo[ndx];
        	      			addrLen = sizeof(ptr->Address), type, i;
        	      			currentIPAddr = &ptr->IpAddressList;
        	      			firstIPAddr = TRUE;       	        	
        	      			getInterfaceNameByIndex(table->indexes[ndx], ptr->AdapterName);
        	      			getInterfacePhysicalByIndex(table->indexes[ndx], &addrLen,ptr->Address, &type);
        	      			ptr->AddressLength = addrLen;
        	      			ptr->Type = type;
        	      			ptr->Index = table->indexes[ndx];
        	      			for (int i = 0; i < ipAddrTable->dwNumEntries; i++) {
        	        			if (ipAddrTable->table[i].dwIndex == ptr->Index) {
        	        	  			if (firstIPAddr) {
        	        	    			toIPAddressString(ipAddrTable->table[i].dwAddr,
        	        	     			ptr->IpAddressList.IpAddress.String);
        	        	    			toIPAddressString(ipAddrTable->table[i].dwMask,
        	        	     			ptr->IpAddressList.IpMask.String);
        	        	    			firstIPAddr = FALSE;
        	        	  			}
        	        	  			else {
        	        	    			currentIPAddr->Next = nextIPAddr;
        	        	    			currentIPAddr = nextIPAddr;
        	        	    			toIPAddressString(ipAddrTable->table[i].dwAddr,
        	        	     			currentIPAddr->IpAddress.String);
        	        	    			toIPAddressString(ipAddrTable->table[i].dwMask,
        	        	     			currentIPAddr->IpMask.String);
        	        	    			nextIPAddr++;
        	        	  			}
        	        			}
							}
        	      			if (winsEnabled) {
        	        			ptr->HaveWins = TRUE;
        	        			memcpy(ptr->PrimaryWinsServer.IpAddress.String,
        	        	 		primaryWINS.String, sizeof(primaryWINS.String));
        	        			memcpy(ptr->SecondaryWinsServer.IpAddress.String,
        	        	 		secondaryWINS.String, sizeof(secondaryWINS.String));
        	      			}
        	      			if (ndx < table->numIndexes - 1)
        	        			ptr->Next = &pAdapterInfo[ndx + 1];
        	      			else
        	        			ptr->Next = NULL;
						}
        	    		ret = NO_ERROR;
					}
        			free(table);
				}
    			else	
        			ret = ERROR_OUTOFMEMORY;
        		free(ipAddrTable);
			}
		}
    	else
      		ret = ERROR_NO_DATA;
    }
	return ret;
}
//---------------------------------------------------------------------------
unsigned long inet_addr(const char* cp)
{
    char *p,*p1;
    unsigned long ret;
	int shift;
    
    p = (char *)cp;    
    p1 = NULL;
    ret = 0;
	shift = 0;
    while(*p != 0 && shift < 32){
		if(p1 == NULL)
			p1 = p;
		if(*p == '.'){
			ret |= atoi(p1) << shift;
			shift += 8;
			p1 = NULL;
		}
		p++;
	}
	if(p1 != NULL && *p1 != 0)
		ret |= atoi(p1) << shift;
    return ret;
}
//---------------------------------------------------------------------------
char *inet_ntoa (struct in_addr in)
{
	int i,value;
	char s[10];
	
	*((int *)winsock_string) = 0;
	for(i=0;i<4;i++){
		value = in.s_addr & 0xFF;
		sprintf(s,"%3d",value);
		strcat(winsock_string,s);
		if(i < 3)
			strcat(winsock_string,".");
		in.s_addr >>= 8;
	}
	return winsock_string;
}
//---------------------------------------------------------------------------
int WSAStartup(WORD wVersionRequired,LPWSADATA lpWSAData)
{
	if(lpWSAData == NULL)
		return 1;
	memset(lpWSAData,0,sizeof(WSADATA));
	lpWSAData->wVersion = wVersionRequired; 
    if(resInit == 0){
	    res_init();
        resInit = 1;
    }
	return 0;
}
//---------------------------------------------------------------------------
int WSACleanup(void)
{
    return 0;
}
//---------------------------------------------------------------------------
int WSAGetLastError(void)
{   
    return 0;
}
//---------------------------------------------------------------------------
WSAEVENT WSACreateEvent(void)
{
	WSAEvent *event;

	event = new WSAEvent();
	if(event != NULL)
		event->Create(TRUE,FALSE,NULL);
	return event;
}
//---------------------------------------------------------------------------
BOOL WSAResetEvent(WSAEVENT hEvent)
{
    if(hEvent == NULL)
        return FALSE;
    ((WSAEvent *)hEvent)->Reset();
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL WSASetEvent(WSAEVENT hEvent)
{
    if(hEvent == NULL)
        return FALSE;
    ((WSAEvent *)hEvent)->Set();
	return TRUE;
}
//---------------------------------------------------------------------------
DWORD WSAWaitForMultipleEvents(DWORD cEvents,WSAEVENT *lphEvents,BOOL fWaitAll,DWORD dwTimeout,BOOL fAlertable)
{
	return WaitForMultipleObjects(cEvents,lphEvents,fWaitAll,dwTimeout);
}
//---------------------------------------------------------------------------
int WSAEventSelect(SOCKET s,WSAEVENT hEventObject,long lNetworkEvents)
{
    if(hEventObject == NULL)
        return 0;
    ((WSAEvent *)hEventObject)->Select(s,lNetworkEvents);
	return 1;
}
//---------------------------------------------------------------------------
BOOL WSACloseEvent(WSAEVENT hEvent)
{
    if(hEvent == NULL)
        return FALSE;
    delete (WSAEvent *)hEvent;
	return TRUE;
}
//---------------------------------------------------------------------------
WSAEvent::WSAEvent() : LEvent()
{
	event_thread = 0;
	sock = INVALID_SOCKET;
    bQuit = FALSE;
}
//---------------------------------------------------------------------------
WSAEvent::~WSAEvent()
{
	Destroy();
}
//---------------------------------------------------------------------------
void WSAEvent::Destroy()
{
    bQuit = TRUE; 
	if(event_thread != 0)
		pthread_join((pthread_t)event_thread,0);   
    LEvent::Destroy();
}
//---------------------------------------------------------------------------
void WSAEvent::Select(SOCKET s,long lNetworkEvents)
{
    if(bQuit || s == INVALID_SOCKET)
        return;
    sock = s;
    mode = lNetworkEvents;
    if(event_thread != 0)
        return;
    pthread_create(&event_thread,NULL,event_loop,(LPVOID)this);
}
//---------------------------------------------------------------------------
void WSAEvent::OnLoop()
{
	fd_set rfds;
    struct timeval tv;
    int retval;

	/*
	int s1, s2;
	int rv;	
	struct pollfd ufds[2];
	
	ufds[0].fd = s1;
	ufds[0].events = POLLIN | POLLPRI; // check for normal or out-of-band
	rv = poll(ufds, 2, 3500);

	if (rv == -1) {
    	perror("poll"); // error occurred in poll()
	} else if (rv == 0) {
    	printf("Timeout occurred!  No data after 3.5 seconds.\n");
	} else {
    	// check for events on s1:
    	if (ufds[0].revents & POLLIN) {
        	recv(s1, buf1, sizeof buf1, 0); // receive normal data
    	}
    	if (ufds[0].revents & POLLPRI) {
        	recv(s1, buf1, sizeof buf1, MSG_OOB); // out-of-band data
    	}
	}
	*/
    while(!bQuit){
    	FD_ZERO(&rfds);
    	FD_SET(0,&rfds);
		tv.tv_sec = 0;
    	tv.tv_usec = 10000;
   		retval = select(sock+1, &rfds, NULL, NULL, &tv);
   		if (retval == -1)
        	continue;
    	else if (retval)
    		Set();
    }
}
//---------------------------------------------------------------------------
void *WSAEvent::event_loop(void *arg)
{
	if(arg != NULL)
		((WSAEvent *)arg)->OnLoop();
	pthread_exit(NULL);		
}

