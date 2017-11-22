#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <string.h>
#include "colors.h"
#include <CommCtrl.h>
#include "resource.h"

#define PANEL_OFFSET_LEFT (120)
#define PANEL_OFFSET_TOP  (20)
#define CHAR_WIDTH        (70)
#define CHAR_WIDTH_LED    (5)
#define CHAR_HEIGHT_LED   (7)
#define PANEL_HEIGHT      (98)
#define MAX_PANEL_LENGTH  (10)
#define MIN_PANEL_LENGTH  (5)
#define MAX_SCROLL_SPEED  (1500)
#define MIN_SCROLL_SPEED  (50)
#define PANEL_NONE        (-1)

typedef enum Efect {NONE, LR, RL, NEG} Effect;
typedef struct Panel
{
    COLORREF bg_color;
    COLORREF fg_color;
    //UINT panelLength;
    Effect effect;
    char panelText[MAX_PANEL_LENGTH+1];

    Panel():bg_color(clrBlack),fg_color(clrRed),effect(NONE)
    {}
} Panel;

const char g_szClassName[] = "myWindowClass";
HINSTANCE hInst;
HWND hwnd;
HANDLE hComm;
TCHAR com_port[] = "COMX";
TCHAR characterSetFile[MAX_PATH] = "void";
Panel panels[4];
COLORREF colorsList[4] = {clrRed, clrBlack, clrYellow, clrGreen};
UINT panelLength = 5;
UINT scrollSpeedMillisec = 500;
DCB dcbSerialParams = { 0 }; // Initializing DCB structure
COMMTIMEOUTS timeouts = { 0 };
int panelTextOffset = 0;
int panelSendingIndex = PANEL_NONE;
int bankFlipFlop = 0;

inline void redrawPanels()
{
    RECT        panel1 = { PANEL_OFFSET_LEFT, PANEL_OFFSET_TOP, MAX_PANEL_LENGTH*CHAR_WIDTH+PANEL_OFFSET_LEFT, PANEL_HEIGHT+PANEL_OFFSET_TOP };
    RECT        panel2 = { PANEL_OFFSET_LEFT, 130, MAX_PANEL_LENGTH*CHAR_WIDTH+PANEL_OFFSET_LEFT, PANEL_HEIGHT+130 };
    RECT        panel3 = { PANEL_OFFSET_LEFT, 240, MAX_PANEL_LENGTH*CHAR_WIDTH+PANEL_OFFSET_LEFT, PANEL_HEIGHT+240 };
    RECT        panel4 = { PANEL_OFFSET_LEFT, 350, MAX_PANEL_LENGTH*CHAR_WIDTH+PANEL_OFFSET_LEFT, PANEL_HEIGHT+350 };
    RedrawWindow(hwnd, &panel1, NULL, RDW_INVALIDATE);
    RedrawWindow(hwnd, &panel2, NULL, RDW_INVALIDATE);
    RedrawWindow(hwnd, &panel3, NULL, RDW_INVALIDATE);
    RedrawWindow(hwnd, &panel4, NULL, RDW_INVALIDATE);
    //UpdateWindow(hwnd);
}

inline char calculateChecksum(char* inString, int length)
{
    int sum=0;
    for(int i=0; i<length; i++)
    {
        sum += inString[i];
    }
    return sum&0xFF;
}

inline bool hasGreen(COLORREF color)
{
    if(color == clrGreen || color == clrYellow)
        return true;
    return false;
}

inline bool hasRed(COLORREF color)
{
    if(color == clrRed || color == clrYellow)
        return true;
    return false;
}

inline std::string color2String(COLORREF color)
{
    if(color == clrRed)
        return "Red";
    if(color == clrGreen)
        return "Green";
    if(color == clrBlack)
        return "Black";
    if(color == clrYellow)
        return "Yellow";
    else
        return "Black";
}

inline COLORREF string2Color(char color[])
{
    if(!strcmp(color, "Red"))
        return clrRed;
    if(!strcmp(color, "Green"))
        return clrGreen;
    if(!strcmp(color, "Black"))
        return clrBlack;
    if(!strcmp(color, "Yellow"))
        return clrYellow;
    else
        return clrBlack;
}

inline std::string effect2String(Effect effect)
{
    if(effect == NONE)
        return "None";
    if(effect == LR)
        return "LeftRight";
    if(effect == RL)
        return "RightLeft";
    if(effect == NEG)
        return "Negative";
    else
        return "None";
}

inline Effect string2Effect(char effect[])
{
    if(!strcmp(effect, "None"))
        return NONE;
    if(!strcmp(effect, "LeftRight"))
        return LR;
    if(!strcmp(effect, "RightLeft"))
        return RL;
    if(!strcmp(effect, "Negative"))
        return NEG;
    else
        return NONE;
}

inline COLORREF negativeMap(COLORREF color)
{
    if(color == clrRed)
        return clrGreen;
    if(color == clrGreen)
        return clrRed;
    if(color == clrBlack)
        return clrYellow;
    if(color == clrYellow)
        return clrBlack;
    else
        return clrBlack;
}

int saveProjectFile(char * filename)
{
    HANDLE hFile = CreateFile((LPCTSTR) filename,
                              GENERIC_WRITE,
                              0,
                              NULL,
                              CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL
                             );
    if (hFile == INVALID_HANDLE_VALUE)
        return 0;
    // [length];[character set path];\n#[num panel];[fg_color];[bg_color];[effect];[text\n


    UINT bytesWritten;
    {
        char panelData[MAX_PATH+10]; // TODO : define max value

        sprintf(panelData, "[global]\n\rlength=%d\n\rspeed=%d\n\rcharacter_file=%s\n\r", panelLength, scrollSpeedMillisec, characterSetFile);
        WriteFile(hFile, panelData, strlen(panelData)+1, (LPDWORD)&bytesWritten, NULL);
    }

    char panelData[300]; // TODO : define max value
    for(int i=0; i<4; i++)
    {
        sprintf(panelData, "[panel%d]\n\rfg_color=%s\n\rbg_color=%s\n\reffect=%s\n\rtext=%s\n\r", i,
                color2String(panels[i].fg_color).c_str(), \
                color2String(panels[i].bg_color).c_str(), \
                effect2String(panels[i].effect).c_str(), \
                panels[i].panelText);
        WriteFile(hFile, panelData, strlen(panelData)+1, (LPDWORD)&bytesWritten, NULL);
    }
    CloseHandle(hFile);
    return 1;
}

int loadProjectFile(char * filename)
{
    char myResult[10];
    GetPrivateProfileString("global","length","5",myResult,sizeof(myResult),filename);
    panelLength = atoi(myResult);

    GetPrivateProfileString("global","speed","500",myResult,sizeof(myResult),filename);
    scrollSpeedMillisec = atoi(myResult);

    GetPrivateProfileString("global","character_file","void",characterSetFile,sizeof(characterSetFile),filename);

    for(int i=0; i<4; i++)
    {
        char panelString[6];
        sprintf(panelString, "panel%d", i);

        char data[15];

        GetPrivateProfileString(panelString,"fg_color","red",data,sizeof(data),filename);
        panels[i].fg_color = string2Color(data);
        GetPrivateProfileString(panelString,"bg_color","black",data,sizeof(data),filename);
        panels[i].bg_color = string2Color(data);
        GetPrivateProfileString(panelString,"effect","none",data,sizeof(data),filename);
        panels[i].effect = string2Effect(data);

        GetPrivateProfileString(panelString,"text","",panels[i].panelText,sizeof(panels[i].panelText),filename);

    }
    SetTimer(hwnd,             // handle to main window
        IDT_TIMER1,            // timer identifier
        scrollSpeedMillisec,                 // 10-second interval
        (TIMERPROC) NULL);     // no timer callback
    redrawPanels();
    return 1;
}

int readCharacterBitmap(HANDLE hFile, char charID, BOOL charBMP[7][5])
{
    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
    DWORD nBytesRead=0;
    BOOL bResult = FALSE;
    char charRead = 0;
    do
    {
        bResult = ReadFile(hFile, &charRead, 1, &nBytesRead, NULL);
    }
    while(charRead != charID && !(bResult = TRUE && nBytesRead == 0));   // NOTE : if the character doesn't exist, nothing will be displayed
    SetFilePointer(hFile, 2, NULL, FILE_CURRENT); // got to next line

    for(int iLine=0; iLine<7; iLine++)
    {
        for(int iCol=0; iCol<5; iCol++)
        {
            ReadFile(hFile, &charRead, 1, &nBytesRead, NULL);
            if(charRead=='0')
                charBMP[iLine][iCol] = FALSE;
            else if (charRead=='1')
                charBMP[iLine][iCol] = TRUE;

        }
        SetFilePointer(hFile, 2, NULL, FILE_CURRENT); // got to next line
    }

    return 1;
}

void character2Serial(char characterID, char outputRed[7], char outputGreen[7], Panel panel)
{
    BOOL myBMP[7][5] = {{0,},};
    HANDLE hFile = CreateFile((LPCTSTR) characterSetFile,
                              GENERIC_READ,
                              0,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL
                             );
    readCharacterBitmap(hFile, characterID, myBMP);
    CloseHandle(hFile);
    for(int y=0; y<7; y++)
    {
        outputGreen[y] = 0;
        outputRed[y] = 0;
        for(int x=0; x<5; x++)
        {
            if(hasGreen(panel.fg_color))
            {
                if(hasGreen(panel.bg_color))
                    outputGreen[y] = 1;
                else
                    outputGreen[y] |= ((char)myBMP[y][4-x]<<x);
            }
            else
            {
                if(hasGreen(panel.bg_color))
                    outputGreen[y] |= (~(char)myBMP[y][4-x]&0x01)<<x;
                else
                    outputGreen[y] = 0;
            }

            if(hasRed(panel.fg_color))
            {
                if(hasRed(panel.bg_color))
                    outputRed[y] = 1;
                else
                    outputRed[y] |= ((char)myBMP[y][4-x]<<x);
            }
            else
            {
                if(hasRed(panel.bg_color))
                    outputRed[y] |= (~(char)myBMP[y][4-x]&0x01)<<x;
                else
                    outputRed[y] = 0;
            }
        }
    }
}

bool inline changeBank(int bank)
{
    char lpBuffer[2] = {'B',bank};

    DWORD dNoOfBytesWritten = 0;     // No of bytes written to the port
    WriteFile(hComm,        // Handle to the Serial port
              lpBuffer,     // Data to be written to the port
              sizeof(lpBuffer),  //No of bytes to write
              &dNoOfBytesWritten, //Bytes written
              NULL);

    char TempChar; //Temporary character used for reading
    DWORD NoBytesRead;
    ReadFile( hComm,           //Handle of the Serial port
              &TempChar,       //Temporary character
              sizeof(TempChar),//Size of TempChar
              &NoBytesRead,    //Number of bytes read
              NULL);

    if(TempChar == 0xAA)
        return true;
    else
        return false;
}

inline bool sendPanelText(unsigned int panelIndex, int ledOffsetX, char text[], int bank)
{
    char red[7] = {0,};
    char green[7] = {0,};

    char *lpBufferRed;
    lpBufferRed = new char[panelLength*7+4]; // 'M', 'r', length, chksm
    lpBufferRed[0] = 'M';
    lpBufferRed[1] = bank?'R':'r';
    lpBufferRed[2] = panelLength*7;

    char *lpBufferGreen;
    lpBufferGreen = new char[panelLength*7+4]; // 'M', 'r', length, chksm
    lpBufferGreen[0] = 'M';
    lpBufferGreen[1] = bank?'G':'g';
    lpBufferGreen[2] = panelLength*7; // we always send a whole panel

    int j=0;
    while(text[j] != '\0') // for each character of the text
    {
        character2Serial(text[j], red, green, panels[panelIndex]); // retrieve the serial data of the character
        for(int i=0; i<7; i++) // for each byte of the character's serial data
        {
            lpBufferRed[7*j+i+3] = red[i];
        }
        for(int i=0; i<7; i++) // for each byte of the character's serial data
        {
            lpBufferGreen[7*j+i+3] = green[i];
        }
        j++;
    }

    for (j=j*7+3; j<panelLength*7+3; j++) // for the rest of the length
    {
        if(panels[panelIndex].bg_color == clrBlack)
        {
            lpBufferRed[j] = 0;
            lpBufferGreen[j] = 0;
        }
        else if(panels[panelIndex].bg_color == clrRed)
        {
            lpBufferRed[j] = 0x1F;
            lpBufferGreen[j] = 0;
        }
        else if(panels[panelIndex].bg_color == clrGreen)
        {
            lpBufferRed[j] = 0;
            lpBufferGreen[j] = 0x1F;
        }
        else if(panels[panelIndex].bg_color == clrYellow)
        {
            lpBufferRed[j] = 0x1F;
            lpBufferGreen[j] = 0x1F;
        }
    }

    lpBufferRed[panelLength*7+3] = calculateChecksum(lpBufferRed, panelLength+3);
    lpBufferGreen[panelLength*7+3] = calculateChecksum(lpBufferGreen, panelLength+3);
    DWORD dNoOfBytesWritten = 0;     // No of bytes written to the port

//  =========== DEBUG ===========
    printf("%c ", lpBufferRed[0]);
    printf("%c ", lpBufferRed[1]);
    printf("%d ", lpBufferRed[2]);
    for(char i=3; i<panelLength*7+4; i++) // NOTE : no sizeof with dynamically allocated arrays
        printf("%02x ", lpBufferRed[i]);
    printf("\n\r");

    printf("%c ", lpBufferGreen[0]);
    printf("%c ", lpBufferGreen[1]);
    printf("%d ", lpBufferGreen[2]);
    for(char i=3; i<panelLength*7+4; i++) // NOTE : no sizeof with dynamically allocated arrays
        printf("%02x ", lpBufferGreen[i]);
    printf("\n\r");

    char TempChar; //Temporary character used for reading
    DWORD NoBytesRead;
/*
    WriteFile(hComm,        // Handle to the Serial port
              lpBufferRed,     // Data to be written to the port
              sizeof(lpBufferRed),  //No of bytes to write
              &dNoOfBytesWritten, //Bytes written
              NULL);

    ReadFile( hComm,           //Handle of the Serial port
              &TempChar,       //Temporary character
              sizeof(TempChar),//Size of TempChar
              &NoBytesRead,    //Number of bytes read
              NULL);

    WriteFile(hComm,        // Handle to the Serial port
              lpBufferGreen,     // Data to be written to the port
              sizeof(lpBufferGreen),  //No of bytes to write
              &dNoOfBytesWritten, //Bytes written
              NULL);

    ReadFile( hComm,           //Handle of the Serial port
              &TempChar,       //Temporary character
              sizeof(TempChar),//Size of TempChar
              &NoBytesRead,    //Number of bytes read
              NULL);
*/
    delete []lpBufferGreen;
    delete []lpBufferRed;

    return true;
}

void printCharacterOnGUIPanel(HDC hDC, unsigned int panelIndex, int charOffsetX, int ledOffsetX, char characterID)
{
    if((1+charOffsetX+(double)ledOffsetX/CHAR_WIDTH_LED)>=panelLength)
        ledOffsetX -= panelLength*CHAR_WIDTH_LED;
    int panelX = PANEL_OFFSET_LEFT + charOffsetX*70  + ledOffsetX*14 + 2;
    int panelY = PANEL_OFFSET_TOP + 110*panelIndex + 2;

    RECT LEDRect;
    HBRUSH bgBrush = CreateSolidBrush(panels[panelIndex].bg_color);
    HBRUSH fgBrush = CreateSolidBrush(panels[panelIndex].fg_color);

    HANDLE hFile = CreateFile((LPCTSTR) characterSetFile,
                              GENERIC_READ,
                              0,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL
                             );

    BOOL myBMP[7][5] = {{0,},};
    readCharacterBitmap(hFile, characterID, myBMP);
    CloseHandle(hFile);

    for(int ledY = 0; ledY < 7; ledY++)
    {
        for(int ledX = 0; ledX < 5; ledX++)
        {
            LEDRect = { panelX, panelY, panelX+10, panelY+10 };
            if(myBMP[ledY][ledX])
                FillRect(hDC,&LEDRect,fgBrush);
            else
                FillRect(hDC,&LEDRect,bgBrush);
            panelX = panelX + 14;
        }
        panelY += 14;
        panelX = PANEL_OFFSET_LEFT + charOffsetX*70 + ledOffsetX*14 + 2;
    }
}

BOOL CALLBACK DlgPanelSettings(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static WORD panel;
    switch(uMsg)
    {
    case WM_INITDIALOG:
    {
        panel = HIWORD(lParam);
        SendMessage(GetDlgItem(hwndDlg, FG_COLOR_COMBO),(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) "Red");
        SendMessage(GetDlgItem(hwndDlg, FG_COLOR_COMBO),(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) "Black");
        SendMessage(GetDlgItem(hwndDlg, FG_COLOR_COMBO),(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) "Yellow");
        SendMessage(GetDlgItem(hwndDlg, FG_COLOR_COMBO),(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) "Green");
        SendMessage(GetDlgItem(hwndDlg, FG_COLOR_COMBO), CB_SETCURSEL, (WPARAM)0, (LPARAM)0);

        SendMessage(GetDlgItem(hwndDlg, BG_COLOR_COMBO),(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) "Red");
        SendMessage(GetDlgItem(hwndDlg, BG_COLOR_COMBO),(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) "Black");
        SendMessage(GetDlgItem(hwndDlg, BG_COLOR_COMBO),(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) "Yellow");
        SendMessage(GetDlgItem(hwndDlg, BG_COLOR_COMBO),(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) "Green");
        SendMessage(GetDlgItem(hwndDlg, BG_COLOR_COMBO), CB_SETCURSEL, (WPARAM)1, (LPARAM)0);

        SendMessage(GetDlgItem(hwndDlg, PANEL_TEXT_EDIT),(UINT) EM_SETLIMITTEXT,(WPARAM) panelLength,(LPARAM) 0);

        SendMessage(GetDlgItem(hwndDlg, PANEL_TEXT_EDIT),(UINT) WM_SETTEXT, 0,(LPARAM) panels[panel].panelText);

        CheckRadioButton(hwndDlg, FX_NONE_RAD, FX_NEG_RAD, FX_NONE_RAD);


        // TODO : try to preselect the previous color

        return TRUE;
    }
    case WM_CLOSE:
        EndDialog(hwndDlg, 0);
        return TRUE;
    case WM_COMMAND:
    {
        switch(HIWORD(wParam))
        {
        case BN_CLICKED:
            switch(LOWORD(wParam))
            {
            case IDOK_PANEL_SET:
            {
                if(!strcmp(characterSetFile, "void"))
                {
                    MessageBox(
                        NULL,
                        (LPCSTR)"Please load a character set first.",
                        (LPCSTR)"Problem",
                        MB_ICONWARNING | MB_OK | MB_DEFBUTTON1
                    );
                    EndDialog(hwndDlg,0);
                }
                int ItemIndex = SendMessage(GetDlgItem(hwndDlg, BG_COLOR_COMBO), (UINT) CB_GETCURSEL,
                                            (WPARAM) 0, (LPARAM) 0);
                panels[panel].bg_color = colorsList[ItemIndex];

                ItemIndex = SendMessage(GetDlgItem(hwndDlg, FG_COLOR_COMBO), (UINT) CB_GETCURSEL,
                                        (WPARAM) 0, (LPARAM) 0);
                panels[panel].fg_color = colorsList[ItemIndex];

                SendMessage(GetDlgItem(hwndDlg, PANEL_TEXT_EDIT), WM_GETTEXT, (WPARAM)MAX_PANEL_LENGTH+1, (LPARAM)panels[panel].panelText);

                if(IsDlgButtonChecked(hwndDlg, FX_NONE_RAD) == BST_UNCHECKED)
                {
                    if(IsDlgButtonChecked(hwndDlg, FX_LR_RAD) == BST_CHECKED)
                    {
                        panels[panel].effect = LR;
                    }
                    else if(IsDlgButtonChecked(hwndDlg, FX_RL_RAD) == BST_CHECKED)
                    {
                        panels[panel].effect = RL;
                    }
                    else if(IsDlgButtonChecked(hwndDlg, FX_NEG_RAD) == BST_CHECKED)
                    {
                        panels[panel].effect = NEG;
                        panels[panel].fg_color = negativeMap(panels[panel].fg_color);
                        panels[panel].bg_color = negativeMap(panels[panel].bg_color);
                    }
                }

                redrawPanels();
                EndDialog(hwndDlg,0);
                break;
            }
            case IDCANCEL_PANEL_SET:
                EndDialog(hwndDlg,0);
                break;
            }
            break;
        }
    }
    return TRUE;
    }
    return FALSE;
}

BOOL CALLBACK DlgPanelConf(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
    case WM_INITDIALOG:
    {
        SendMessage(GetDlgItem(hwndDlg, PANEL_LENGTH_SLIDER), TBM_SETRANGE,FALSE, MAKELONG(MIN_PANEL_LENGTH, MAX_PANEL_LENGTH));
        SendMessage(GetDlgItem(hwndDlg, PANEL_LENGTH_SLIDER), TBM_SETPOS,TRUE, panelLength);
        SetDlgItemInt(hwndDlg, PANEL_LENGTH_TEXT, panelLength, FALSE);

        SendMessage(GetDlgItem(hwndDlg, SCROLL_SPEED_SLIDER), TBM_SETRANGE,FALSE, MAKELONG(MIN_SCROLL_SPEED, MAX_SCROLL_SPEED));
        SendMessage(GetDlgItem(hwndDlg, SCROLL_SPEED_SLIDER), TBM_SETTICFREQ,50, 0);

        SendMessage(GetDlgItem(hwndDlg, SCROLL_SPEED_SLIDER), TBM_SETPOS,TRUE, scrollSpeedMillisec);
        SetDlgItemInt(hwndDlg, SCROLL_SPEED_TEXT, scrollSpeedMillisec, FALSE);
        return TRUE;
    }
    case WM_CLOSE:
        EndDialog(hwndDlg, 0);
        return TRUE;
    case WM_HSCROLL:
        SetDlgItemInt(hwndDlg, PANEL_LENGTH_TEXT, SendMessage(GetDlgItem(hwndDlg, PANEL_LENGTH_SLIDER), TBM_GETPOS, 0, 0), FALSE);
        SetDlgItemInt(hwndDlg, SCROLL_SPEED_TEXT, SendMessage(GetDlgItem(hwndDlg, SCROLL_SPEED_SLIDER), TBM_GETPOS, 0, 0), FALSE);
        return TRUE;
    case WM_COMMAND:
    {
        switch(HIWORD(wParam))
        {
        case BN_CLICKED:
            switch(LOWORD(wParam))
            {
            case IDOK_PORT:
            {
                panelLength = SendMessage(GetDlgItem(hwndDlg, PANEL_LENGTH_SLIDER), TBM_GETPOS, 0, 0);
                scrollSpeedMillisec = SendMessage(GetDlgItem(hwndDlg, SCROLL_SPEED_SLIDER), TBM_GETPOS, 0, 0);
                KillTimer(hwnd, IDT_TIMER1);
                SetTimer(hwnd,             // handle to main window
                    IDT_TIMER1,            // timer identifier
                    scrollSpeedMillisec,                 // 10-second interval
                    (TIMERPROC) NULL);     // no timer callback
                redrawPanels();
                EndDialog(hwndDlg,0);
                break;
            }
            case IDCANCEL_PORT:
                EndDialog(hwndDlg,0);
                break;
            }
            break;
        }
    }
    return TRUE;
    }
    return FALSE;
}

BOOL CALLBACK DlgSerialConf(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
    case WM_INITDIALOG:
    {
        SendMessage(GetDlgItem(hwndDlg, COM_COMBO),(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) "COM1");
        SendMessage(GetDlgItem(hwndDlg, COM_COMBO),(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) "COM2");
        SendMessage(GetDlgItem(hwndDlg, COM_COMBO),(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) "COM3");
        SendMessage(GetDlgItem(hwndDlg, COM_COMBO),(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) "COM4");
        SendMessage(GetDlgItem(hwndDlg, COM_COMBO),(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) "COM5");
        SendMessage(GetDlgItem(hwndDlg, COM_COMBO), CB_SETCURSEL, (WPARAM)2, (LPARAM)0);

        SendMessage(GetDlgItem(hwndDlg, BAUD_COMBO),(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) "4800");
        SendMessage(GetDlgItem(hwndDlg, BAUD_COMBO),(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) "9600");
        SendMessage(GetDlgItem(hwndDlg, BAUD_COMBO),(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) "19200");
        SendMessage(GetDlgItem(hwndDlg, BAUD_COMBO),(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) "38400");
        SendMessage(GetDlgItem(hwndDlg, BAUD_COMBO),(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) "57600");
        SendMessage(GetDlgItem(hwndDlg, BAUD_COMBO),(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) "115200");
        SendMessage(GetDlgItem(hwndDlg, BAUD_COMBO), CB_SETCURSEL, (WPARAM)1, (LPARAM)0);

        SendMessage(GetDlgItem(hwndDlg, PAR_COMBO),(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) "No parity");
        SendMessage(GetDlgItem(hwndDlg, PAR_COMBO),(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) "Odd");
        SendMessage(GetDlgItem(hwndDlg, PAR_COMBO),(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) "Even");
        SendMessage(GetDlgItem(hwndDlg, PAR_COMBO), CB_SETCURSEL, (WPARAM)1, (LPARAM)0);

        SendMessage(GetDlgItem(hwndDlg, STOPBITS_COMBO),(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) "1");
        SendMessage(GetDlgItem(hwndDlg, STOPBITS_COMBO),(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) "2");
        SendMessage(GetDlgItem(hwndDlg, STOPBITS_COMBO), CB_SETCURSEL, (WPARAM)0, (LPARAM)0);

        SendMessage(GetDlgItem(hwndDlg, BYTESIZE_SPIN), (UINT) UDM_SETRANGE,
                    (WPARAM) 0, MAKELPARAM((WORD) 16, (WORD) 4));

        SendMessage(GetDlgItem(hwndDlg, TIMEOUT_SPIN), (UINT) UDM_SETRANGE,
                    (WPARAM) 0, MAKELPARAM((WORD) 50, (WORD) 300));

        SetDlgItemInt(hwndDlg, BYTE_SIZE_ENTRY,8,0);
        SetDlgItemInt(hwndDlg, TIMEOUT_ENTRY,100,0);
        break;
    }
    return TRUE;

    case WM_CLOSE:
    {
        EndDialog(hwndDlg, 0);
    }
    return TRUE;

    case WM_COMMAND:
    {
        switch(HIWORD(wParam))
        {
        case BN_CLICKED:
            switch(LOWORD(wParam))
            {
            case IDOK_PORT:
            {
                timeouts.ReadIntervalTimeout         = 50; // in milliseconds
                timeouts.ReadTotalTimeoutConstant    = 50; // in milliseconds
                timeouts.ReadTotalTimeoutMultiplier  = 10; // in milliseconds
                timeouts.WriteTotalTimeoutConstant   = 50; // in milliseconds
                timeouts.WriteTotalTimeoutMultiplier = 10; // in milliseconds

                dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
                int ItemIndex = SendMessage(GetDlgItem(hwndDlg, COM_COMBO), (UINT) CB_GETCURSEL,
                                            (WPARAM) 0, (LPARAM) 0);
                TCHAR  ListItem[256];
                SendMessage(GetDlgItem(hwndDlg, COM_COMBO), (UINT) CB_GETLBTEXT,
                            (WPARAM) ItemIndex, (LPARAM) ListItem);
                strcpy(com_port, ListItem);

                ItemIndex = SendMessage(GetDlgItem(hwndDlg, BAUD_COMBO), (UINT) CB_GETCURSEL,
                                        (WPARAM) 0, (LPARAM) 0);
                SendMessage(GetDlgItem(hwndDlg, BAUD_COMBO), (UINT) CB_GETLBTEXT,
                            (WPARAM) ItemIndex, (LPARAM) ListItem);
                dcbSerialParams.BaudRate = atoi(ListItem);

                ItemIndex = SendMessage(GetDlgItem(hwndDlg, PAR_COMBO), (UINT) CB_GETCURSEL,
                                        (WPARAM) 0, (LPARAM) 0);
                dcbSerialParams.Parity   = ItemIndex;

                ItemIndex = SendMessage(GetDlgItem(hwndDlg, STOPBITS_COMBO), (UINT) CB_GETCURSEL,
                                        (WPARAM) 0, (LPARAM) 0);
                dcbSerialParams.StopBits = ItemIndex;

                ItemIndex = SendMessage(GetDlgItem(hwndDlg, BYTESIZE_SPIN), (UINT) UDM_GETPOS,
                                        (WPARAM) 0, (LPARAM) 0);
                dcbSerialParams.ByteSize  = ItemIndex;

                ItemIndex = SendMessage(GetDlgItem(hwndDlg, TIMEOUT_SPIN), (UINT) UDM_GETPOS,
                                        (WPARAM) 0, (LPARAM) 0);
                EndDialog(hwndDlg,0);
                break;
            }
            case IDCANCEL_PORT:
                EndDialog(hwndDlg,0);
                break;
            }
            break;
        }
    }
    return TRUE;
    }
    return FALSE;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_TIMER:
    {
        if(panelSendingIndex == PANEL_NONE)
            break;
        if(panels[panelSendingIndex].effect == LR || panels[panelSendingIndex].effect == RL)
        {
            panelTextOffset = (panelTextOffset+1)%(panelLength*CHAR_WIDTH_LED);
        }
        sendPanelText(panelSendingIndex, panelTextOffset, panels[panelSendingIndex].panelText, bankFlipFlop);
        bankFlipFlop = bankFlipFlop?0:1;
        changeBank(bankFlipFlop);
        redrawPanels();
        break;
    }
    case WM_PAINT:
    {
        HDC         hDC;
        PAINTSTRUCT ps;
        RECT        panel1 = { PANEL_OFFSET_LEFT, PANEL_OFFSET_TOP, panelLength*CHAR_WIDTH+PANEL_OFFSET_LEFT, PANEL_HEIGHT+PANEL_OFFSET_TOP };
        RECT        panel2 = { PANEL_OFFSET_LEFT, 130, panelLength*CHAR_WIDTH+PANEL_OFFSET_LEFT, PANEL_HEIGHT+130 };
        RECT        panel3 = { PANEL_OFFSET_LEFT, 240, panelLength*CHAR_WIDTH+PANEL_OFFSET_LEFT, PANEL_HEIGHT+240 };
        RECT        panel4 = { PANEL_OFFSET_LEFT, 350, panelLength*CHAR_WIDTH+PANEL_OFFSET_LEFT, PANEL_HEIGHT+350 };
        HBRUSH myBrush = CreateSolidBrush(clrBlack);

        hDC = BeginPaint(hwnd, &ps);

        FillRect(hDC,&panel1,myBrush);
        FillRect(hDC,&panel2,myBrush);
        FillRect(hDC,&panel3,myBrush);
        FillRect(hDC,&panel4,myBrush);

        for(int iPanel = 0; iPanel < 4; iPanel++)
        {
            int iChar=0;
            char myChar;
            while(panels[iPanel].panelText[iChar] != '\0' && iChar < MAX_PANEL_LENGTH+1)
            {
                myChar = panels[iPanel].panelText[iChar];
                if(panels[iPanel].effect == LR)
                    printCharacterOnGUIPanel(hDC, iPanel,iChar, panelTextOffset, myChar);
                else if(panels[iPanel].effect == RL)
                    printCharacterOnGUIPanel(hDC, iPanel,iChar, panelLength*7 - panelTextOffset, myChar); // TODO : right left
                else
                    printCharacterOnGUIPanel(hDC, iPanel,iChar, 0, myChar);
                iChar++;
            }
        }

        EndPaint(hwnd, &ps);
    }
    return 0;
    case WM_COMMAND:
    {
        if(lParam == 0) /* Menu command */
        {
            switch(LOWORD(wParam))
            {
            case MENU_CONF_SERIAL:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_SERIAL_CONF), NULL, (DLGPROC)DlgSerialConf);
                break;
            case MENU_SERIAL_CONNECT:
            {
                if(hComm != NULL)
                    CloseHandle(hComm);
                hComm = CreateFile((LPCSTR)com_port,       // for COM1—COM9 only
                                   GENERIC_READ | GENERIC_WRITE, // Read/Write
                                   0,               // No Sharing
                                   NULL,            // No Security
                                   OPEN_EXISTING,   // Open existing port only
                                   0,               // Non Overlapped I/O
                                   NULL);
                if (hComm == INVALID_HANDLE_VALUE)
                {
                    int msgboxID = MessageBox(
                                       NULL,
                                       (LPCSTR)"Impossible to open COM port\nDo you want to change the settings?",
                                       (LPCSTR)"Problem",
                                       MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2
                                   );
                    switch (msgboxID)
                    {
                    case IDYES:
                        DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_SERIAL_CONF), NULL, (DLGPROC)DlgSerialConf);
                        break;
                    case IDNO:
                        break;
                    }
                }
                else
                    MessageBox(
                        NULL,
                        (LPCSTR)"COM Port successfully opened",
                        (LPCSTR)"Success",
                        MB_ICONINFORMATION | MB_OK | MB_DEFBUTTON1
                    );

                SetCommState(hComm, &dcbSerialParams);
                SetCommTimeouts(hComm, &timeouts);
                break;
            }
            case MENU_CONF_PANEL:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_PANEL_CONF), NULL, (DLGPROC)DlgPanelConf);
                break;
            case MENU_FILE_QUIT:
                DestroyWindow(hwnd);
                break;
            case MENU_FILE_LOAD_PROJECT:
            {
                char filename[ MAX_PATH ];
                OPENFILENAME ofn;
                ZeroMemory( &filename, sizeof( filename ) );
                ZeroMemory( &ofn,      sizeof( ofn ) );
                ofn.lStructSize  = sizeof( ofn );
                ofn.lpstrDefExt = ".pmp";
                ofn.hwndOwner    = NULL;  // If you have a window to center over, put its HANDLE here
                ofn.lpstrFilter  = "PM Project Files\0*.pmp\0Any File\0*.*\0";
                ofn.lpstrFile    = filename;
                ofn.nMaxFile     = MAX_PATH;
                ofn.lpstrTitle   = "Please select a file...";
                ofn.Flags        = OFN_FILEMUSTEXIST;

                if (GetOpenFileName( &ofn ))
                {
                    if(!loadProjectFile(filename))
                        MessageBox(
                            NULL,
                            (LPCSTR)"Impossible to open the file.\nPlease refer to the log file.",
                            (LPCSTR)"Problem",
                            MB_ICONWARNING | MB_OK | MB_DEFBUTTON1
                        );
                }
                else
                {
                    MessageBox(
                        NULL,
                        (LPCSTR)"Impossible to open the file.\nPlease refer to the log file.",
                        (LPCSTR)"Problem",
                        MB_ICONWARNING | MB_OK | MB_DEFBUTTON1
                    );
                    // TODO : write log
                }
                break;
            }
            case MENU_FILE_SAVE_PROJECT:
            {
                char filename[ MAX_PATH ];
                OPENFILENAME ofn;
                ZeroMemory( &filename, sizeof( filename ) );
                ZeroMemory( &ofn,      sizeof( ofn ) );
                ofn.lStructSize  = sizeof( ofn );
                ofn.hwndOwner    = NULL;  // If you have a window to center over, put its HANDLE here
                ofn.lpstrDefExt = ".pmp";
                ofn.lpstrFilter  = "PM Project Files\0*.pmp\0Any File\0*.*\0";
                ofn.lpstrFile    = filename;
                ofn.nMaxFile     = MAX_PATH;
                ofn.lpstrTitle   = "Please select a file...";
                ofn.Flags        = OFN_PATHMUSTEXIST;

                if (GetSaveFileName( &ofn ))
                {
                    if(!saveProjectFile(filename))
                        MessageBox(
                            NULL,
                            (LPCSTR)"Impossible to save the file.\nPlease refer to the log file.",
                            (LPCSTR)"Problem",
                            MB_ICONWARNING | MB_OK | MB_DEFBUTTON1
                        );
                }
                else
                {
                    MessageBox(
                        NULL,
                        (LPCSTR)"Impossible to save the file.\nPlease refer to the log file.",
                        (LPCSTR)"Problem",
                        MB_ICONWARNING | MB_OK | MB_DEFBUTTON1
                    );
                    // TODO : write log
                }
                break;
            }
            case MENU_FILE_LOAD_CHAR_SET:
            {
                char filename[ MAX_PATH ];
                OPENFILENAME ofn;
                ZeroMemory( &filename, sizeof( filename ) );
                ZeroMemory( &ofn,      sizeof( ofn ) );
                ofn.lStructSize  = sizeof( ofn );
                ofn.lpstrDefExt = ".pcs";
                ofn.hwndOwner    = NULL;  // If you have a window to center over, put its HANDLE here
                ofn.lpstrFilter  = "Panel Character Sets\0*.pcs\0Any File\0*.*\0";
                ofn.lpstrFile    = filename;
                ofn.nMaxFile     = MAX_PATH;
                ofn.lpstrTitle   = "Please select a character set...";
                ofn.Flags        = OFN_FILEMUSTEXIST;

                if (GetOpenFileName( &ofn ))
                {
                    strcpy(characterSetFile, filename);
                }
                else
                {
                    MessageBox(
                        NULL,
                        (LPCSTR)"Impossible to save the file.\nPlease refer to the log file.",
                        (LPCSTR)"Problem",
                        MB_ICONWARNING | MB_OK | MB_DEFBUTTON1
                    );
                    // TODO : write log
                }
                break;
            }
            }
        }
        else /* control commands */
        {
            /* HIWORD contains BN_CLICKED, etc */
            switch(HIWORD(wParam))
            {
            case BN_CLICKED:
                switch(LOWORD(wParam))
                {
                case MAIN_CONF1_BUTTON:
                    DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_PANEL_SETTINGS),NULL,(DLGPROC)DlgPanelSettings,
                                   MAKELPARAM((WORD)0,(WORD)0)
                                  );
                    break;
                case MAIN_CONF2_BUTTON:
                    DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_PANEL_SETTINGS),NULL,(DLGPROC)DlgPanelSettings,
                                   MAKELPARAM((WORD)0,(WORD)1)
                                  );
                    break;
                case MAIN_CONF3_BUTTON:
                    DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_PANEL_SETTINGS),NULL,(DLGPROC)DlgPanelSettings,
                                   MAKELPARAM((WORD)0,(WORD)2)
                                  );
                    break;
                case MAIN_CONF4_BUTTON:
                    DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_PANEL_SETTINGS),NULL,(DLGPROC)DlgPanelSettings,
                                   MAKELPARAM((WORD)0,(WORD)3)
                                  );
                    break;
                case MAIN_SEND1_BUTTON:
                {
                    panelSendingIndex = 0;
                    break;
                }
                case MAIN_SEND2_BUTTON:
                {
                    panelSendingIndex = 1;
                    break;
                }
                case MAIN_SEND3_BUTTON:
                {
                    panelSendingIndex = 2;
                    break;
                }
                case MAIN_SEND4_BUTTON:
                {
                    panelSendingIndex = 3;
                    break;
                }
                }
                break;
            }
        }
        return 0;
    }
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASSEX wc;
    MSG Msg;
    hInst = hInstance;

    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = 0;
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName  = MAKEINTRESOURCE(IDR_MENU1);
    wc.lpszClassName = g_szClassName;
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

    if(!RegisterClassEx(&wc))
    {
        MessageBox(NULL, "Window Registration Failed!", "Error!",
                   MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    hwnd = CreateWindowEx(
               WS_EX_CLIENTEDGE,
               g_szClassName,
               "Panel Manager",  /*window name*/
               WS_OVERLAPPEDWINDOW,
               CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, /*the size of the window*/
               NULL, NULL, hInstance, NULL);

    if(hwnd == NULL)
    {
        MessageBox(NULL, "Window Creation Failed!", "Error!",
                   MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    CreateWindow("BUTTON","Configure",WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,20,20,80,30,
                 hwnd,(HMENU)MAIN_CONF1_BUTTON,(HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE),NULL);
    CreateWindow("BUTTON","Configure",WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,20,130,80,30,
                 hwnd,(HMENU)MAIN_CONF2_BUTTON,(HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE),NULL);
    CreateWindow("BUTTON","Configure",WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,20,240,80,30,
                 hwnd,(HMENU)MAIN_CONF3_BUTTON,(HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE),NULL);
    CreateWindow("BUTTON","Configure",WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,20,350,80,30,
                 hwnd,(HMENU)MAIN_CONF4_BUTTON,(HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE),NULL);
    CreateWindow("BUTTON","Send",WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,20,60,80,30,
                 hwnd,(HMENU)MAIN_SEND1_BUTTON,(HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE),NULL);
    CreateWindow("BUTTON","Send",WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,20,170,80,30,
                 hwnd,(HMENU)MAIN_SEND2_BUTTON,(HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE),NULL);
    CreateWindow("BUTTON","Send",WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,20,280,80,30,
                 hwnd,(HMENU)MAIN_SEND3_BUTTON,(HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE),NULL);
    CreateWindow("BUTTON","Send",WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,20,390,80,30,
                 hwnd,(HMENU)MAIN_SEND4_BUTTON,(HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE),NULL);
    ShowWindow(hwnd, nCmdShow);  /*needs to be done to create a windows*/
    UpdateWindow(hwnd);          /*should be done every time a "widget" is added*/
    SetTimer(hwnd,             // handle to main window
        IDT_TIMER1,            // timer identifier
        scrollSpeedMillisec,                 // 10-second interval
        (TIMERPROC) NULL);     // no timer callback
    // Step 3: The Message Loop
    while(GetMessage(&Msg, NULL, 0, 0) > 0)
    {
        /*this part of the code is like a while loop that runs everytime*/
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    return Msg.wParam;
}
