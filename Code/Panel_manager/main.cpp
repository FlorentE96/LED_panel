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
#define PANEL_HEIGHT      (98)
#define MAX_PANEL_LENGTH  (10)
#define MIN_PANEL_LENGTH  (5)
#define MAX_SCROLL_SPEED  (1500)
#define MIN_SCROLL_SPEED  (50)

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

    char panelData[MAX_PATH+10]; // TODO : define max value
    UINT bytesWritten;

    sprintf(panelData, "%d;%s\n\r", panelLength, characterSetFile);
    printf("%s", panelData);
    WriteFile(hFile, panelData, strlen(panelData)+1, (LPDWORD)&bytesWritten, NULL);

    for(int i=0; i<4; i++)
    {
        sprintf(panelData, "#%d;%d;%s;%s;%s;%s\n\r", i, panelLength,
                                                     color2String(panels[0].fg_color).c_str(), \
                                                     color2String(panels[0].bg_color).c_str(), \
                                                     effect2String(panels[0].effect).c_str(), \
                                                     panels[0].panelText);

        printf("%s", panelData);
        WriteFile(hFile, panelData, strlen(panelData)+1, (LPDWORD)&bytesWritten, NULL);
    }
//             \
//                  #1;%d;%s;%s;%s;%s\n\r \
//                  #2;%d;%s;%s;%s;%s\n\r \
//                  #3;%d;%s;%s;%s;%s\n\r \
//                  #4;%d;%s;%s;%s;%s", \
//            panelLength, characterSetFile, \
//            color2String(panels[0].fg_color),color2String(panels[0].bg_color),effect2String(panels[0].effect),panels[0].panelText, \
//            color2String(panels[1].fg_color),color2String(panels[1].bg_color),effect2String(panels[1].effect),panels[1].panelText, \
//            color2String(panels[2].fg_color),color2String(panels[2].bg_color),effect2String(panels[2].effect),panels[2].panelText, \
//            color2String(panels[3].fg_color),color2String(panels[3].bg_color),effect2String(panels[3].effect),panels[3].panelText);
////


    CloseHandle(hFile);
    return 1;
}

int loadProjectFile(char * filename)
{
    HANDLE hFile = CreateFile((LPCTSTR) filename,
                GENERIC_READ,
                0,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL
    );
    if (hFile == INVALID_HANDLE_VALUE)
        return 0;
    // TODO : parse file
    return 1;
}

int readCharacterBitmap(HANDLE hFile, char charID, BOOL charBMP[7][5])
{
    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
    DWORD nBytesRead=0;
    BOOL bResult = FALSE;
    char charRead = 0;
    do{
        bResult = ReadFile(hFile, &charRead, 1, &nBytesRead, NULL);
    } while(charRead != charID && !(bResult = TRUE && nBytesRead == 0)); // NOTE : if the character doesn't exist, nothing will be displayed
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

void printCharacterOnPanel(HDC hDC, unsigned int panelIndex, int charOffsetX, int ledOffsetY, char characterID)
{
    int panelX = PANEL_OFFSET_LEFT + charOffsetX*70 + 2;
    int panelY = PANEL_OFFSET_TOP + 110*panelIndex + ledOffsetY + 2;

    RECT        LEDRect; //  = { 20, 20, panelLength*74+20, 102+20 }
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

    for(int ledY = 0; ledY < 7; ledY++) {
        for(int ledX = 0; ledX < 5; ledX++) {
                LEDRect = { panelX, panelY, panelX+10, panelY+10 };
                if(myBMP[ledY][ledX])
                    FillRect(hDC,&LEDRect,fgBrush);
                else
                    FillRect(hDC,&LEDRect,bgBrush);
                panelX += 14;
        }
        panelY += 14;
        panelX = PANEL_OFFSET_LEFT + charOffsetX*70 + 2;
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

                    InvalidateRect(hwnd, NULL, TRUE);
                    UpdateWindow(hwnd);
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

                InvalidateRect(hwnd, NULL, TRUE);
                UpdateWindow(hwnd);
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
                    printCharacterOnPanel(hDC, iPanel,iChar,0, myChar);
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
                        hComm = CreateFile((LPCSTR)com_port,          // for COM1—COM9 only
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
                        break;
                    }
                    case MENU_CONF_PANEL:
                        DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_PANEL_CONF), NULL, (DLGPROC)DlgPanelConf);
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
                          ofn.Flags        = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

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
                          ofn.Flags        = OFN_DONTADDTORECENT | OFN_PATHMUSTEXIST;

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
                          ofn.Flags        = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

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
            { /* HIWORD contains BN_CLICKED, etc */
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

    // Step 3: The Message Loop
    while(GetMessage(&Msg, NULL, 0, 0) > 0)
    {   /*this part of the code is like a while loop that runs everytime*/
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    return Msg.wParam;
}
