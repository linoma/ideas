#include "dldiplugin.h"
#include "resource.h"
#include "lds.h"

//static DLDIPARAM dldi_data;
/*static struct {
	int index;
	int bit;
	u8 data[64];
} dr;
static int nStartup;*/
//---------------------------------------------------------------------------
DLDIPlugIn::DLDIPlugIn() : PlugIn()
{
	dwType = PIT_DLDI;
}
//---------------------------------------------------------------------------
BOOL DLDIPlugIn::InitSetPlugInInfo(LPSETPLUGININFO p,DWORD dwState,DWORD dwStateMask)
{
	if(!PlugIn::InitSetPlugInInfo(p,dwState,dwStateMask))
		return FALSE;
	sp.io_mem = io_mem;
   sp.lpFat = ds.get_FatInterface();
	p->lParam = (LPARAM)&sp;
	return TRUE;
}
//---------------------------------------------------------------------------
DLDIPlugList::DLDIPlugList() : PlugInList ("DLDI")
{
}
//---------------------------------------------------------------------------
DLDIPlugIn *DLDIPlugList::BuildPlugIn(char *path)
{
	DLDIPlugIn *p;

	if(path == NULL || (p = new DLDIPlugIn()) == NULL)
		return NULL;
	p->SetLibraryPath(path);
	return p;
}
//---------------------------------------------------------------------------
BOOL DLDIPlugList::PreLoad(WORD *wID)
{
	DLDIPlugIn *pPlug;

	*wID = ID_PLUGIN_DLDI_START;
	pPlug = new R4PlugIn();
	if(pPlug != NULL){
		pPlug->SetMenuID(*wID);
   	(*wID)++;
		if(!Add(pPlug)){
       	(*wID)--;
	        delete pPlug;
       }
   }
	pPlug = new NJPlugIn();
	if(pPlug != NULL){
		pPlug->SetMenuID(*wID);
   	(*wID)++;
		if(!Add(pPlug)){
       	(*wID)--;
	        delete pPlug;
       }
   }
/*	pPlug = new AK2PlugIn();
	if(pPlug != NULL){
		pPlug->SetMenuID(*wID);
   	(*wID)++;
		if(!Add(pPlug)){
       	(*wID)--;
	        delete pPlug;
       }
   }*/

	return TRUE;
}
//---------------------------------------------------------------------------
BOOL DLDIPlugList::OnEnablePlug(WORD wID)
{
   PlugIn *p;
   elem_list *tmp;
	GUID guid;

   tmp = First;
   while(tmp){
   	p = (PlugIn *)(tmp->Ele);
   	if(p->GetMenuID() == wID)
			break;
       p = NULL;
		tmp = tmp->Next;
   }
   if(p != NULL){
       p->GetGuid(&guid);
       Enable(&guid,TRUE);
   }
   else if((p = pActivePlugIn) != NULL){
		p->GetGuid(&guid);
       Enable(&guid,FALSE);
   }
	return TRUE;
}
//---------------------------------------------------------------------------
R4PlugIn::R4PlugIn() : DLDIPlugIn()
{
	GUID g = {0xFD701757,0x76E3,0x4416,{0x8A,0xF9,0xB0,0x41,0x20,0x44,0xDC,0x97}};
   
	dwType = PIT_DLDI;
	dwFlags = PIT_DYNAMIC;
	memcpy(&guid,&g,sizeof(GUID));
   name = "R4 - Revolution for DS";
//   dldi_data.io_mem = sp.io_mem = io_mem;
//   dldi_data.lpFat = sp.lpFat = ds.get_FatInterface();
}
//---------------------------------------------------------------------------
DWORD R4PlugIn::Run(DWORD adr,LPDWORD data,BYTE accessMode)
{
	switch(accessMode & 0xF0){
		case AMM_CNT:
			switch((u8)adr){
       		case 0xBB:
               case 0xB9:
//                   *data = ((adr & 0xFF000000) >> 24) | ((adr & 0xFF00) << 8) | ((adr & 0xFF0000) >> 8);
                   *data = ((adr & 0xFF00) << 16) | (adr & 0xFF0000)|((adr & 0xFF000000) >> 16) | (u8)*data;
               	if(sp.lpFat->Seek(*data,0))
                   	*data = 0x80 << 4;
                   else
                   	*data = 0;
                   if((u8)adr == 0xBB)
                       adr = PIR_DLDI_ADJUSTIOCONTROL_CHECK;
                   else
                       adr = 0;
                   adr |= PIR_DLDI_DATAISVALID|PIR_DLDI_SEEK|PIR_DLDI_ADJUSTIOCONTROL|PIR_DLDI_SETLENGTH|PIR_DLDI_CONTROL|PIR_DLDI_FAT;
                   return adr;
       		case 0xD0:
   			case 0xD1:
               case 0xD2:
       			return PIR_DLDI_NULL|PIR_DLDI_ADJUSTIOCONTROL;
               default:
              		return PIR_DLDI_UNSUPPORTED;
   		}
       break;
       case AMM_READ:
			switch((u8)adr){
           	case 0xB0:
                   *data = 4;
                   return PIR_DLDI_CONTROL|PIR_DLDI_ADJUSTIOCONTROL|PIR_DLDI_TRIGGERIRQ|PIR_DLDI_DATAISVALID;
               break;
               case 0xB8:
           	case 0xBC:
               	return PIR_DLDI_NULL|PIR_DLDI_ADJUSTIOCONTROL;
				break;
           	case 0xB9:
               	*data = 0;
                   return PIR_DLDI_NULL|PIR_DLDI_TRIGGERIRQ|PIR_DLDI_DATAISVALID;
               break;
           	case 0xBA:
               	*data = sp.lpFat->Read(0,0);
					return PIR_DLDI_READ|PIR_DLDI_ADJUSTIOCONTROL|PIR_DLDI_TRIGGERIRQ|PIR_DLDI_DATAISVALID|PIR_DLDI_FAT;
				break;
               default:
               	return PIR_DLDI_UNSUPPORTED;
           }
       break;
       case AMM_WRITE:
       	return PIR_DLDI_UNSUPPORTED;
       break;
   }
}
//---------------------------------------------------------------------------
NJPlugIn::NJPlugIn()
{
	GUID g = {0xD5EBA976,0x0FCE,0x435A,{0xB0,0x88,0xC0,0x9D,0x89,0x50,0x6E,0x6}};

	dwType = PIT_DLDI;
	dwFlags = PIT_DYNAMIC;
	memcpy(&guid,&g,sizeof(GUID));
   name = "Default SD adapter";
}
//---------------------------------------------------------------------------
DWORD NJPlugIn::Run(DWORD adr,LPDWORD data,BYTE accessMode)
{
   DWORD res;

	switch(accessMode & 0xF0){
		case AMM_CNT:
			switch((u8)adr){
               case 0x20:
               	return PIR_DLDI_NULL|PIR_DLDI_ADJUSTIOCONTROL;
               default:
               	switch((u8)adr >> 4){
                  		case 0xE://CLK
                       	switch((adr >> 16) & 0xBF){
                           	case 0x12:
                               	command = 0x12;
                                   *data = (*data << 8) | (adr >> 24);
									*data = ((*data & 0xFF) << 24) | ((*data & 0xFF00) << 8) | ((*data & 0xFF0000) >> 8) | (*data >> 24);
               					if(sp.lpFat->Seek(*data,0))
                                       *data = 0x80 << 2;
                   				else
                   					*data = 0;
									res = PIR_DLDI_DATAISVALID|PIR_DLDI_SEEK|PIR_DLDI_ADJUSTIOCONTROL|PIR_DLDI_SETLENGTH|PIR_DLDI_CONTROL|PIR_DLDI_FAT;
                               break;
                           	default:
                       			*data = 0;
                       			res =  PIR_DLDI_TRIGGERIRQ_MC|PIR_DLDI_CONTROL|PIR_DLDI_ADJUSTIOCONTROL|PIR_DLDI_ADJUSTIOCONTROL_CHECK;
                               break;
                           }
                           return res;
                  		case 0xF:
               			res = PIR_DLDI_TRIGGERIRQ_MC|PIR_DLDI_CONTROL;
                       	switch((adr >> 16) & 0xBF){
               				case 0x2://ALL_SEND_CID
                               	*data = 17 << 2;
                                   res |= PIR_DLDI_SETLENGTH|PIR_DLDI_DATAISVALID;
                           	break;
                           	case 0x3://SEND_RELATIVE_ADDR
                           	case 0x7://SELECT_CARD
                           	case 0xD://SEND_STATUS
                           	case 0x10://SET_BLOCKLEN
                           	case 0x29://SD_APP_OP_COND
                           	case 0x37://APP_CMD
                               	*data = 6 << 2;
                                   res |= PIR_DLDI_SETLENGTH|PIR_DLDI_DATAISVALID;
                               break;
                           	case 0x9://SEND_CSD
                               	*data = 17 << 2;
                                   res |= PIR_DLDI_SETLENGTH|PIR_DLDI_DATAISVALID;
                           	break;
                               case 0x11://Seek
                                   command = 0x11;
                                   *data = (*data << 8) | (adr >> 24);
									*data = ((*data & 0xFF) << 24) | ((*data & 0xFF00) << 8) | ((*data & 0xFF0000) >> 8) | (*data >> 24);
               					if(sp.lpFat->Seek(*data,0))
                                       *data = 0x80 << 2;
                   				else
                   					*data = 0;
                   				res = PIR_DLDI_DATAISVALID|PIR_DLDI_SEEK|PIR_DLDI_ADJUSTIOCONTROL|PIR_DLDI_SETLENGTH|PIR_DLDI_CONTROL|PIR_DLDI_FAT;
                               break;
                           	default:
                                    *data = 0;
                                    res |= PIR_DLDI_ADJUSTIOCONTROL|PIR_DLDI_ADJUSTIOCONTROL_CHECK;
                           	break;
               			}
                           return res;
                  		default:
                   		return PIR_DLDI_UNSUPPORTED;
                	}
            	break;
         	}
       break;
       case AMM_READ:
			switch((u8)adr){
           	case 0x41:
					return PIR_DLDI_TRIGGERIRQ_MC|PIR_DLDI_CONTROL|PIR_DLDI_ADJUSTIOCONTROL|PIR_DLDI_ADJUSTIOCONTROL_CHECK;
               case 0x40:
               	*data = sp.lpFat->Read(0,0);
					res = PIR_DLDI_READ|PIR_DLDI_ADJUSTIOCONTROL|PIR_DLDI_TRIGGERIRQ|PIR_DLDI_DATAISVALID|PIR_DLDI_FAT;
                   if(command == 18)
                   	res |= PIR_DLDI_TRIGGERIRQ_MC;
					return res;
               default:
                   switch((u8)adr >> 4){
                       case 0xE://CLK
           				return PIR_DLDI_TRIGGERIRQ_MC|PIR_DLDI_NULL|PIR_DLDI_ADJUSTIOCONTROL;
               		case 0xF:
               			res = PIR_DLDI_UNSUPPORTED;
                       	switch((adr >> 16) & 0xBF){
                   			case 0x2://ALL_SEND_CID
                   			case 0x3://SEND_RELATIVE_ADDR                        case 0x7://SELECT_CARD            case 0x9://SEND_CSD
           					case 0xD://SEND_STATUS
           					case 0x10://SET_BLOCKLEN
               					*data = 0;
               					res = PIR_DLDI_READ|PIR_DLDI_ADJUSTIOCONTROL|PIR_DLDI_TRIGGERIRQ;
           					break;
           					case 0x29:
               					if(*data == 2)
           							*data = 0x8000;
                           		else
                       				*data = 0;
                       			res = PIR_DLDI_READ|PIR_DLDI_ADJUSTIOCONTROL|PIR_DLDI_TRIGGERIRQ;
                       		break;
                   			case 0x37:
                        			if(*data == 2)
                          				*data = 0x37;
                          			else
                       				*data = 0;
                       			res = PIR_DLDI_READ|PIR_DLDI_ADJUSTIOCONTROL|PIR_DLDI_TRIGGERIRQ;
                       		break;
                   		}
                   		return res;
               		default:
                       	res = 0;
               			return PIR_DLDI_UNSUPPORTED;
                   }
   			break;
           }
       break;
       case AMM_WRITE:
       	return PIR_DLDI_UNSUPPORTED;
       break;
   }
}
//---------------------------------------------------------------------------
/*AK2PlugIn::AK2PlugIn()
{
	GUID g = {0xB258D351,0xEB88,0x40CD,{0x9C,0xD5,0xD9,0x14,0x8E,0x67,0x43,0x02}};

	dwType = PIT_DLDI;
	dwFlags = PIT_DYNAMIC;
	memcpy(&guid,&g,sizeof(GUID));
   name = "AceKard2 PlugIn";
   dldi_data.io_mem = sp.io_mem = io_mem;
   dldi_data.lpFat = sp.lpFat = ds.get_FatInterface();
}
//---------------------------------------------------------------------------
DWORD AK2PlugIn::Run(DWORD adr,LPDWORD data,BYTE accessMode)
{
   DWORD res;
   u32 value;
   int i;
   char s[50];

   switch(accessMode & 0xF0){
        case AMM_CNT:
            switch((u8)adr){
                case 0xD0:
                    nStartup |= 1;
                break;
                case 0xD1:
                    nStartup |= 2;
                break;
                case 0xB8://ak2
                	if(ds.get_IsHomebrew(&i) == S_OK && i)
                   	return PIR_DLDI_CONTROL|PIR_DLDI_SETLENGTH;
                	return PIR_DLDI_UNSUPPORTED;
                case 0xB7:
                    switch((*((u32 *)&dldi_data.io_mem[0x1AC]) >> 8) & 0xFF){
                        case 0x13:
                            return PIR_DLDI_CONTROL|PIR_DLDI_SETLENGTH;
                        case 0x11:
                        case 0x12://ak2
                            value = ((adr & 0xFF00) << 16) | (adr & 0xFF0000)|((adr & 0xFF000000) >> 16) | (u8)*data;
                            dldi_data.lpFat->Seek(value,0);
                            return PIR_DLDI_FAT|PIR_DLDI_SETLENGTH|PIR_DLDI_SEEK|PIR_DLDI_CONTROL;
                        default:
                            if((nStartup & 3) != 3)
                                return PIR_DLDI_UNSUPPORTED;
                            *data = ((adr & 0xFF00) << 16) | (adr & 0xFF0000)|((adr & 0xFF000000) >> 16) | (u8)*data;
                            return PIR_DLDI_FAT|PIR_DLDI_SETLENGTH|PIR_DLDI_SEEK|PIR_DLDI_ONLYDECODE|PIR_DLDI_CONTROL;
                    }
                case 0xC0:// ak2
                break;
                case 0xD5:
                    switch(adr >> 24){
                        case 0x3:
                            memset(&dr,0,sizeof(dr));
                            return PIR_DLDI_SETLENGTH|PIR_DLDI_CONTROL;
                        case 0x6:
                            memset(&dr,0,sizeof(dr));
                            return PIR_DLDI_SETLENGTH|PIR_DLDI_CONTROL;
                        case 0x7:
                            memset(&dr,0,sizeof(dr));
                            return PIR_DLDI_SETLENGTH|PIR_DLDI_CONTROL;
                        case 0x8:
                            memset(&dr,0,sizeof(dr));
                            dr.data[4] = 0x6;dr.data[5]=0xA8;
                            return PIR_DLDI_SETLENGTH|PIR_DLDI_CONTROL;
                        case 0xC://ak2
                        	*data = 0;
							return PIR_DLDI_CONTROL|PIR_DLDI_DATAISVALID|PIR_DLDI_FAT|PIR_DLDI_SETLENGTH|PIR_DLDI_SEEK|PIR_DLDI_ADJUSTIOCONTROL;
                        case 0xD:
                            memset(&dr,0,sizeof(dr));
                            dr.data[4] = 0x20;
                            return PIR_DLDI_SETLENGTH|PIR_DLDI_CONTROL;
                        case 0x12: // ak2
                            value = MAKELONG(MAKEWORD(dldi_data.io_mem[0x1AF],dldi_data.io_mem[0x1AE]),
                                MAKEWORD(dldi_data.io_mem[0x1AD],dldi_data.io_mem[0x1AC]));
                            dldi_data.lpFat->Seek(value<<9,0); //Ak2
                            *data = 0xFFFFFFFF;
                            return PIR_DLDI_CONTROL|PIR_DLDI_DATAISVALID|PIR_DLDI_FAT|PIR_DLDI_SETLENGTH|PIR_DLDI_SEEK|PIR_DLDI_ADJUSTIOCONTROL;
                        break;
                        case 0x11:
                            //value = *((u32 *)&dldi_data.io_mem[0x1AC]); //RPG
                            value = MAKELONG(MAKEWORD(dldi_data.io_mem[0x1AF],dldi_data.io_mem[0x1AE]),
                                MAKEWORD(dldi_data.io_mem[0x1AD],dldi_data.io_mem[0x1AC]));
                            //dldi_data.lpFat->Seek(value,0); RPG
                            dldi_data.lpFat->Seek(value<<9,0); //Ak2
                            *data = 512;
                            return PIR_DLDI_CONTROL|PIR_DLDI_DATAISVALID|PIR_DLDI_FAT|PIR_DLDI_SETLENGTH|PIR_DLDI_SEEK|PIR_DLDI_ADJUSTIOCONTROL;
                        case 0x37:
                            memset(&dr,0,sizeof(dr));
                            return PIR_DLDI_SETLENGTH|PIR_DLDI_CONTROL;
                        case 0x29:
                            memset(&dr,0,sizeof(dr));
                            dr.data[1] = 0x3;
                            return PIR_DLDI_SETLENGTH|PIR_DLDI_CONTROL;
                    }
                break;
            }
        break;
        case AMM_READ:
            switch((u8)adr){
                case 0xD5:
                    switch(adr >> 24){
                        case 0x3:
                        case 0x6:
                        case 0x7:
                        case 0x8:
                        case 0xD:
                        case 0x29:
                        case 0x37:
                            *data = 0;
                            for(i=0;i<4;i++){
                                value = dr.data[dr.index] & (1 << dr.bit--);
                                *data |= (value != 0 ? 0x80 : 0) << (8*i);
                                if(dr.bit < 0){
                                    dr.bit = 7;
                                    dr.index++;
                                }
                            }
                            return PIR_DLDI_READ|PIR_DLDI_DATAISVALID|PIR_DLDI_ADJUSTIOCONTROL|PIR_DLDI_TRIGGERIRQ;
                    }
                break;
                case 0xB8:
                	if(ds.get_IsHomebrew(&i) == S_OK && i){
                        // RPG
                        // if((nStartup & 3) != 3)
                        // 	return PIR_DLDI_UNSUPPORTED;
                        *data = 0xFC2;
                        return PIR_DLDI_CONTROL|PIR_DLDI_ADJUSTIOCONTROL|PIR_DLDI_TRIGGERIRQ|PIR_DLDI_DATAISVALID;
                    }
                    return PIR_DLDI_UNSUPPORTED;
                case 0xB7:
                	if(ds.get_IsHomebrew(&i) == S_OK && i){
                   	// if((nStartup & 3) != 3)
                       // 	return PIR_DLDI_UNSUPPORTED;
                       return PIR_DLDI_ONLYDECODE|PIR_DLDI_READ|PIR_DLDI_ADJUSTIOCONTROL|PIR_DLDI_TRIGGERIRQ|PIR_DLDI_FAT;
                   }
					return PIR_DLDI_UNSUPPORTED;
                case 0xC0://ak2
                    *data = 0x70;
                    return PIR_DLDI_CONTROL|PIR_DLDI_ADJUSTIOCONTROL|PIR_DLDI_TRIGGERIRQ|PIR_DLDI_DATAISVALID;
                default:
                    return 0;
            }
        break;
        case AMM_WRITE:
            return PIR_DLDI_UNSUPPORTED;
	}
}*/


