//---------------------------------------------------------------------------
#ifndef decryptH
#define decryptH
//---------------------------------------------------------------------------
BOOL decryptFirmware();
void decrypt_64bit_up(u32 *ptr);
void crypt_64bit_up(u32 *ptr);
void crypt_64bit_down(u32 *ptr);
void apply_keycode(u32 modulo);
void init_keycode(u32 idcode,u32 level,u32 modulo);

#endif
