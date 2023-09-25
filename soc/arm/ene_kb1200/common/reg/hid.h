/**************************************************************************//**
 * @file     hid.h
 * @brief    Define HID's function
 *           
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/

#ifndef HID_H
#define HID_H

//***************************************************************
// User Define                                                
//***************************************************************
#define HID_V_OFFSET                USR_HID_V_OFFSET

//***************************************************************
//  Kernel Define                                             
//***************************************************************
//-- Constant Define -------------------------------------------
#define HID_USAGE_PAGE_0x01         DISABLE
#define HID_USAGE_PAGE_0x0C         DISABLE

#define Windows_OS                  0
#define Android_OS                  1

//-- Macro Define -----------------------------------------------

//-- Struction Define --------------------------------------------
typedef struct _HID_kbd_in_rep_buf              // Size = 14
{
    unsigned char LEN_L;
    unsigned char LEN_H;
    unsigned char CNT;
    unsigned char MOD;                          // Modifier keys
    unsigned char RSV;                          // Default 0
    unsigned char N_KEY[6];
    unsigned char S_KEY;
    unsigned char C_KEY[2];
}_hid_kbd_in_rep_buf;

typedef struct _HID_aux_in_rep_buf              // Size = 11
{
    unsigned char LEN_L;
    unsigned char LEN_H;
    unsigned char CNT;
    unsigned char DAT[8];
}_hid_aux_in_rep_buf;


//***************************************************************
//  Extern Varible Declare                                          
//***************************************************************
#if VIRTUAL_TP
extern unsigned char Real_TP_On_Off;    // 0 = Off; 1 = On
#endif  // VIRTUAL_TP
#if VIRTUAL_KB
extern unsigned char Real_KB_On_Off;    // 0 = Off; 1 = On
#endif  // VIRTUAL_KB
extern unsigned char DAT_Package;
extern unsigned char OS_Select;         // 0: HIF, 1: HIDI2C_Bus
extern unsigned char VD_KBD_HID_DAT_IND;
extern _hid_kbd_in_rep_buf HID_KBD_IN_REP_BUF;
extern _hid_aux_in_rep_buf HID_AUX_IN_REP_BUF;


//***************************************************************
//  Extern Function Declare                                          
//***************************************************************
//-- Kernel Use ------------------------------------------------
unsigned char TP_Data_To_HID_Host_ISR(unsigned char Data);

//-- For OEM Use -----------------------------------------------
unsigned char Transfer_MV_HID(unsigned char matrix_value, unsigned char MB_flag);
void HID_System_Control_Key(unsigned char matrix_value, unsigned char MB_flag);
void HID_Normal_Key(unsigned char hid_key, unsigned char MB_flag);
void HID_Consumer_Keys(unsigned char matrix_value, unsigned char MB_flag);
void Set_Modifier_Key(unsigned char Modifier_Key_Flag, unsigned char MBFlag);
void Enter_Windows_OS(void);
void Exit_Windows_OS(void);
void Enter_Android_OS(void);
void Exit_Android_OS(void);
void Switch_OS(unsigned char OS_SEL);

#define HID_KEY_NONE                    0x00 // No key pressed
#define HID_KEY_ERR_OVF                 0x01 // Keyboard Error Roll Over - used for all slots if too many keys are pressed ("Phantom key")
#define HID_KEY_FAIL                    0x02 // Keyboard POST Fail
#define HID_KEY_ERR_UNDEF               0x03 // Keyboard Error Undefined
#define HID_KEY_A                       0x04 // Keyboard a and A
#define HID_KEY_B                       0x05 // Keyboard b and B
#define HID_KEY_C                       0x06 // Keyboard c and C
#define HID_KEY_D                       0x07 // Keyboard d and D
#define HID_KEY_E                       0x08 // Keyboard e and E
#define HID_KEY_F                       0x09 // Keyboard f and F
#define HID_KEY_G                       0x0a // Keyboard g and G
#define HID_KEY_H                       0x0b // Keyboard h and H
#define HID_KEY_I                       0x0c // Keyboard i and I
#define HID_KEY_J                       0x0d // Keyboard j and J
#define HID_KEY_K                       0x0e // Keyboard k and K
#define HID_KEY_L                       0x0f // Keyboard l and L
#define HID_KEY_M                       0x10 // Keyboard m and M
#define HID_KEY_N                       0x11 // Keyboard n and N
#define HID_KEY_O                       0x12 // Keyboard o and O
#define HID_KEY_P                       0x13 // Keyboard p and P
#define HID_KEY_Q                       0x14 // Keyboard q and Q
#define HID_KEY_R                       0x15 // Keyboard r and R
#define HID_KEY_S                       0x16 // Keyboard s and S
#define HID_KEY_T                       0x17 // Keyboard t and T
#define HID_KEY_U                       0x18 // Keyboard u and U
#define HID_KEY_V                       0x19 // Keyboard v and V
#define HID_KEY_W                       0x1a // Keyboard w and W
#define HID_KEY_X                       0x1b // Keyboard x and X
#define HID_KEY_Y                       0x1c // Keyboard y and Y
#define HID_KEY_Z                       0x1d // Keyboard z and Z

#define HID_KEY_1                       0x1e // Keyboard 1 and !
#define HID_KEY_2                       0x1f // Keyboard 2 and @
#define HID_KEY_3                       0x20 // Keyboard 3 and #
#define HID_KEY_4                       0x21 // Keyboard 4 and $
#define HID_KEY_5                       0x22 // Keyboard 5 and %
#define HID_KEY_6                       0x23 // Keyboard 6 and ^
#define HID_KEY_7                       0x24 // Keyboard 7 and &
#define HID_KEY_8                       0x25 // Keyboard 8 and *
#define HID_KEY_9                       0x26 // Keyboard 9 and (
#define HID_KEY_0                       0x27 // Keyboard 0 and )

#define HID_KEY_ENTER                   0x28 // Keyboard Return (ENTER)
#define HID_KEY_ESC                     0x29 // Keyboard ESCAPE
#define HID_KEY_BACKSPACE               0x2a // Keyboard DELETE (Backspace)
#define HID_KEY_TAB                     0x2b // Keyboard Tab
#define HID_KEY_SPACE                   0x2c // Keyboard Spacebar
#define HID_KEY_MINUS                   0x2d // Keyboard - and _
#define HID_KEY_EQUAL                   0x2e // Keyboard = and +
#define HID_KEY_LEFTBRACE               0x2f // Keyboard [ and {
#define HID_KEY_RIGHTBRACE              0x30 // Keyboard ] and }
#define HID_KEY_BACKSLASH               0x31 // Keyboard \ and |
#define HID_KEY_HASHTILDE               0x32 // Keyboard Non-US # and ~
#define HID_KEY_SEMICOLON               0x33 // Keyboard ; and :
#define HID_KEY_APOSTROPHE              0x34 // Keyboard ' and "
#define HID_KEY_GRAVE                   0x35 // Keyboard ` and ~
#define HID_KEY_COMMA                   0x36 // Keyboard , and <
#define HID_KEY_DOT                     0x37 // Keyboard . and >
#define HID_KEY_SLASH                   0x38 // Keyboard / and ?
#define HID_KEY_CAPSLOCK                0x39 // Keyboard Caps Lock

#define HID_KEY_F1                      0x3a // Keyboard F1
#define HID_KEY_F2                      0x3b // Keyboard F2
#define HID_KEY_F3                      0x3c // Keyboard F3
#define HID_KEY_F4                      0x3d // Keyboard F4
#define HID_KEY_F5                      0x3e // Keyboard F5
#define HID_KEY_F6                      0x3f // Keyboard F6
#define HID_KEY_F7                      0x40 // Keyboard F7
#define HID_KEY_F8                      0x41 // Keyboard F8
#define HID_KEY_F9                      0x42 // Keyboard F9
#define HID_KEY_F10                     0x43 // Keyboard F10
#define HID_KEY_F11                     0x44 // Keyboard F11
#define HID_KEY_F12                     0x45 // Keyboard F12

#define HID_KEY_PRINTSCREEN             0x46 // Keyboard Print Screen
#define HID_KEY_SCROLLLOCK              0x47 // Keyboard Scroll Lock
#define HID_KEY_PAUSE                   0x48 // Keyboard Pause
#define HID_KEY_INSERT                  0x49 // Keyboard Insert
#define HID_KEY_HOME                    0x4a // Keyboard Home
#define HID_KEY_PAGEUP                  0x4b // Keyboard Page Up
#define HID_KEY_DELETE                  0x4c // Keyboard Delete Forward
#define HID_KEY_END                     0x4d // Keyboard End
#define HID_KEY_PAGEDOWN                0x4e // Keyboard Page Down
#define HID_KEY_RIGHT                   0x4f // Keyboard Right Arrow
#define HID_KEY_LEFT                    0x50 // Keyboard Left Arrow
#define HID_KEY_DOWN                    0x51 // Keyboard Down Arrow
#define HID_KEY_UP                      0x52 // Keyboard Up Arrow

#define HID_KEY_NUMLOCK                 0x53 // Keyboard Num Lock and Clear
#define HID_KEY_KPSLASH                 0x54 // Keypad /
#define HID_KEY_KPASTERISK              0x55 // Keypad *
#define HID_KEY_KPMINUS                 0x56 // Keypad -
#define HID_KEY_KPPLUS                  0x57 // Keypad +
#define HID_KEY_KPENTER                 0x58 // Keypad ENTER
#define HID_KEY_KP1                     0x59 // Keypad 1 and End
#define HID_KEY_KP2                     0x5a // Keypad 2 and Down Arrow
#define HID_KEY_KP3                     0x5b // Keypad 3 and PageDn
#define HID_KEY_KP4                     0x5c // Keypad 4 and Left Arrow
#define HID_KEY_KP5                     0x5d // Keypad 5
#define HID_KEY_KP6                     0x5e // Keypad 6 and Right Arrow
#define HID_KEY_KP7                     0x5f // Keypad 7 and Home
#define HID_KEY_KP8                     0x60 // Keypad 8 and Up Arrow
#define HID_KEY_KP9                     0x61 // Keypad 9 and Page Up
#define HID_KEY_KP0                     0x62 // Keypad 0 and Insert
#define HID_KEY_KPDOT                   0x63 // Keypad . and Delete

#define HID_KEY_102ND                   0x64 // Keyboard Non-US \ and |
#define HID_KEY_APP                     0x65 // Keyboard Application
#define HID_KEY_POWER                   0x66 // Keyboard Power
#define HID_KEY_KPEQUAL                 0x67 // Keypad =

#define HID_KEY_F13                     0x68 // Keyboard F13
#define HID_KEY_F14                     0x69 // Keyboard F14
#define HID_KEY_F15                     0x6a // Keyboard F15
#define HID_KEY_F16                     0x6b // Keyboard F16
#define HID_KEY_F17                     0x6c // Keyboard F17
#define HID_KEY_F18                     0x6d // Keyboard F18
#define HID_KEY_F19                     0x6e // Keyboard F19
#define HID_KEY_F20                     0x6f // Keyboard F20
#define HID_KEY_F21                     0x70 // Keyboard F21
#define HID_KEY_F22                     0x71 // Keyboard F22
#define HID_KEY_F23                     0x72 // Keyboard F23
#define HID_KEY_F24                     0x73 // Keyboard F24

#define HID_KEY_EXECUTE                 0x74 // Keyboard Execute
#define HID_KEY_HELP                    0x75 // Keyboard Help
#define HID_KEY_MENU                    0x76 // Keyboard Menu
#define HID_KEY_SELECT                  0x77 // Keyboard Select
#define HID_KEY_STOP                    0x78 // Keyboard Stop
#define HID_KEY_AGAIN                   0x79 // Keyboard Again
#define HID_KEY_UNDO                    0x7a // Keyboard Undo
#define HID_KEY_CUT                     0x7b // Keyboard Cut
#define HID_KEY_COPY                    0x7c // Keyboard Copy
#define HID_KEY_PASTE                   0x7d // Keyboard Paste
#define HID_KEY_FIND                    0x7e // Keyboard Find
#define HID_KEY_MUTE                    0x7f // Keyboard Mute
#define HID_KEY_VOLUMEUP                0x80 // Keyboard Volume Up
#define HID_KEY_VOLUMEDOWN              0x81 // Keyboard Volume Down
#define HID_KEY_KBLOCK_CAPSLOCK         0x82 // Keyboard Locking Caps Lock
#define HID_KEY_KBLOCK_NUMLOCK          0x83 // Keyboard Locking Num Lock
#define HID_KEY_KBLOCK_SCROLLLOCK       0x84 // Keyboard Locking Scroll Lock
#define HID_KEY_KPCOMMA                 0x85 // Keypad Comma
#define HID_KEY_KPEQUALSIGN             0x86 // Keypad Equal Sign

#define HID_KEY_RO                      0x87 // Keyboard International1
#define HID_KEY_KATAKANAHIRAGANA        0x88 // Keyboard International2
#define HID_KEY_YEN                     0x89 // Keyboard International3
#define HID_KEY_HENKAN                  0x8a // Keyboard International4
#define HID_KEY_MUHENKAN                0x8b // Keyboard International5
#define HID_KEY_KPJPCOMMA               0x8c // Keyboard International6
// 0x8d  Keyboard International7
// 0x8e  Keyboard International8
// 0x8f  Keyboard International9
#define HID_KEY_HANGEUL                 0x90 // Keyboard LANG1
#define HID_KEY_HANJA                   0x91 // Keyboard LANG2
#define HID_KEY_KATAKANA                0x92 // Keyboard LANG3
#define HID_KEY_HIRAGANA                0x93 // Keyboard LANG4
#define HID_KEY_ZENKAKUHANKAKU          0x94 // Keyboard LANG5
// 0x95 Keyboard LANG6
// 0x96 Keyboard LANG7
// 0x97 Keyboard LANG8
// 0x98 Keyboard LANG9
// 0x99 Keyboard Alternate Erase
// 0x9a Keyboard SysReq/Attention
// 0x9b Keyboard Cancel
// 0x9c Keyboard Clear
// 0x9d Keyboard Prior
// 0x9e Keyboard Return
// 0x9f Keyboard Separator
// 0xa0 Keyboard Out
// 0xa1 Keyboard Oper
// 0xa2 Keyboard Clear/Again
// 0xa3 Keyboard CrSel/Props
// 0xa4 Keyboard ExSel

//Modifier Key
#define HID_KEY_MOD_LEFTCTRL            0xe0 // Keyboard Left Control
#define HID_KEY_MOD_LEFTSHIFT           0xe1 // Keyboard Left Shift
#define HID_KEY_MOD_LEFTALT             0xe2 // Keyboard Left Alt
#define HID_KEY_MOD_LEFTGUI             0xe3 // Keyboard Left GUI
#define HID_KEY_MOD_RIGHTCTRL           0xe4 // Keyboard Right Control
#define HID_KEY_MOD_RIGHTSHIFT          0xe5 // Keyboard Right Shift
#define HID_KEY_MOD_RIGHTALT            0xe6 // Keyboard Right Alt
#define HID_KEY_MOD_RIGHTGUI            0xe7 // Keyboard Right GUI

#define HID_MOD_LCTRL                   0x01
#define HID_MOD_LSHIFT                  0x02
#define HID_MOD_LALT                    0x04
#define HID_MOD_LGUI                    0x08
#define HID_MOD_RCTRL                   0x10
#define HID_MOD_RSHIFT                  0x20
#define HID_MOD_RALT                    0x40
#define HID_MOD_RGUI                    0x80

//Media Key
#define HID_ID_MEDIA_NEXTTRACK          0x00b5
#define HID_ID_MEDIA_PREVIOUSTRACK      0x00b6
#define HID_ID_MEDIA_STOP               0x00b7
#define HID_ID_MEDIA_PLAYPAUSE          0x00cd
#define HID_ID_MEDIA_MUTE               0x00e2
#define HID_ID_MEDIA_BASSBOOST          0x00e5
#define HID_ID_MEDIA_LOUDNESS           0x00e7
#define HID_ID_MEDIA_VOLUMEUP           0x00e9
#define HID_ID_MEDIA_VOLUMEDOWN         0x00ea
#define HID_ID_MEDIA_BASSUP             0x0152
#define HID_ID_MEDIA_BASSDOWN           0x0153
#define HID_ID_MEDIA_TREBLEUP           0x0154
#define HID_ID_MEDIA_TREBLEDOWN         0x0155
#define HID_ID_MEDIA_MEDIASELECT        0x0183
#define HID_ID_MEDIA_MAIL               0x018a
#define HID_ID_MEDIA_CALCULATOR         0x0192
#define HID_ID_MEDIA_MYCOMPUTER         0x0194
#define HID_ID_MEDIA_WWW_SEARCH         0x0221
#define HID_ID_MEDIA_WWW_HOME           0x0223
#define HID_ID_MEDIA_WWW_BACK           0x0224
#define HID_ID_MEDIA_WWW_FORWARD        0x0225
#define HID_ID_MEDIA_WWW_STOP           0x0226
#define HID_ID_MEDIA_WWW_REFRESH        0x0227
#define HID_ID_MEDIA_WWW_FAVORITES      0x022a
#endif // HID_H
