#include "ideastypes.h"
#include "lstring.h"


#ifdef _LICENSE
//---------------------------------------------------------------------------
#ifndef __licenseH__
#define __licenseH__

#ifdef __GNUC__
typedef unsigned char boolean;
#endif
//---------------------------------------------------------------------------
interface IDrive : IDispatch {
   virtual HRESULT _stdcall Path(BSTR* pbstrPath) PURE;
   virtual HRESULT _stdcall DriveLetter(BSTR* pbstrLetter) PURE;
   virtual HRESULT _stdcall ShareName(BSTR* pbstrShareName) PURE;
   virtual HRESULT _stdcall DriveType(int* pdt) PURE;
   virtual HRESULT _stdcall RootFolder(long** ppfolder) PURE;
   virtual HRESULT _stdcall AvailableSpace(VARIANT* pvarAvail) PURE;
   virtual HRESULT _stdcall FreeSpace(VARIANT* pvarFree) PURE;
   virtual HRESULT _stdcall TotalSize(VARIANT* pvarTotal) PURE;
   virtual HRESULT _stdcall VolumeName(BSTR* pbstrName) PURE;
   virtual HRESULT _stdcall VolumeName(BSTR pbstrName) PURE;
   virtual HRESULT _stdcall FileSystem(BSTR* pbstrFileSystem) PURE;
   virtual HRESULT _stdcall SerialNumber(long* pulSerialNumber) PURE;
   virtual HRESULT _stdcall IsReady(boolean* pfReady) PURE;
};
//---------------------------------------------------------------------------
interface IFileSystem : IDispatch {
   virtual HRESULT _stdcall Drives(long** ppdrives) PURE;
   virtual HRESULT _stdcall BuildPath(BSTR Path,BSTR Name,BSTR* pbstrResult) PURE;
   virtual HRESULT _stdcall GetDriveName(BSTR Path,BSTR* pbstrResult) PURE;
   virtual HRESULT _stdcall GetParentFolderName(BSTR Path,BSTR* pbstrResult) PURE;
   virtual HRESULT _stdcall GetFileName(BSTR Path,BSTR* pbstrResult) PURE;
   virtual HRESULT _stdcall GetBaseName(BSTR Path,BSTR* pbstrResult) PURE;
   virtual HRESULT _stdcall GetExtensionName(BSTR Path,BSTR* pbstrResult) PURE;
   virtual HRESULT _stdcall GetAbsolutePathName(BSTR Path,BSTR* pbstrResult) PURE;
   virtual HRESULT _stdcall GetTempName(BSTR* pbstrResult) PURE;
   virtual HRESULT _stdcall DriveExists(BSTR DriveSpec,boolean* pfExists) PURE;
   virtual HRESULT _stdcall FileExists(BSTR FileSpec,boolean* pfExists) PURE;
   virtual HRESULT _stdcall FolderExists(BSTR FolderSpec,boolean* pfExists) PURE;
   virtual HRESULT _stdcall GetDrive(BSTR DriveSpec,IDrive** ppdrive) PURE;
   virtual HRESULT _stdcall GetFile(BSTR FilePath, void** ppfile) PURE;
   virtual HRESULT _stdcall GetFolder(BSTR FolderPath,void** ppfolder) PURE;
   virtual HRESULT _stdcall GetSpecialFolder(int SpecialFolder,void** ppfolder) PURE;
   virtual HRESULT _stdcall DeleteFile(BSTR FileSpec,boolean Force) PURE;
   virtual HRESULT _stdcall DeleteFolder(BSTR FolderSpec,boolean Force) PURE;
   virtual HRESULT _stdcall MoveFile(BSTR Source,BSTR Destination) PURE;
   virtual HRESULT _stdcall MoveFolder(BSTR Source,BSTR Destination) PURE;
   virtual HRESULT _stdcall CopyFile(BSTR Source,BSTR Destination,boolean OverWriteFiles) PURE;
   virtual HRESULT _stdcall CopyFolder(BSTR Source,BSTR Destination,boolean OverWriteFiles) PURE;
   virtual HRESULT _stdcall CreateFolder(BSTR Path,void** ppfolder) PURE;
   virtual HRESULT _stdcall CreateTextFile(BSTR FileName,boolean Overwrite,boolean Unicode,void** ppts) PURE;
   virtual HRESULT _stdcall OpenTextFile(BSTR FileName,int IOMode,boolean Create,int Format,void** ppts) PURE;
};
//---------------------------------------------------------------------------
BOOL VerifyLicense(LPGUID p,LPGUID p1);
void GetLicenseControlCode(LPGUID p);
LString LicenseToString(LPGUID p);
BOOL CheckLicenseKey(LPGUID p,BOOL isProductKey);
BOOL GetSerialNumber(LPDWORD p);
BOOL CreateLicenseKey();
BOOL InsertLicense();
BOOL get_ProductGuid(LPGUID guid);
BOOL get_LicenseGuid(LPGUID guid);

#endif

#endif // _LICENSE


