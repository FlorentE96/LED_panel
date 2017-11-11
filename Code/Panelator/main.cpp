#include <windows.h>
#include <stdio.h>
#include <CommCtrl.h>
#include <string.h>
#include <iostream>
#include "resource.h"
#include "colors.h"
//vars
const char g_szClassName[] = "myWindowClass";
HANDLE hComm;
HINSTANCE hInst;
TCHAR com_port[] = "COMX";
DCB dcbSerialParams = { 0 }; // Initializing DCB (serial) structure
HWND hwnd;
int panelLength = 8;
DWORD dwEventMask;
bool Status;
char SerialBuffer[256];//Buffer for storing Rxed Data
char REDbank[2][56];
char GREENbank[2][56];
bool character[35] = {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,1,1,1,1};

//functions
BOOL CALLBACK DlgPanelConf(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgSerialConf(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void printCharacterOnPanel(HDC hDC, unsigned int panelIndex, int charOffsetX, int ledOffsetY, bool character[35]);
int openSerial(void);
void storeRXmsg(int bank, bool LEDcolor, char msgSize);

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{   /*this part of the code process the "events" since it's a event oriented language*/
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
            RECT        panel1 = { 10, 20+30, panelLength*70+10, 98+20+30 };

            HBRUSH myBrush = CreateSolidBrush(clrBlack);

            hDC = BeginPaint(hwnd, &ps);

            FillRect(hDC,&panel1,myBrush);

            printCharacterOnPanel(hDC, 0, 0, 0, character);

            EndPaint(hwnd, &ps);
        }
        case WM_COMMAND:{
            if (HIWORD(wParam) == 1 || HIWORD(wParam) == 0){
                switch(LOWORD(wParam)){
                    case MENU_CLOSE_BN:
                        DestroyWindow(hwnd);
                        break;
                    case MENU_CONN_BN:
                        DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_SERIAL_CONF), NULL, (DLGPROC)DlgSerialConf);
                        break;
                    case MENU_START_BN:
                        openSerial();
                        SetCommMask(hComm, EV_RXCHAR);
                        break;
                    case MENU_CONF_PANEL:
                        DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_PANEL_CONF), NULL, (DLGPROC)DlgPanelConf);
                        break;
                }
            } else{
                switch(HIWORD(wParam)){
                    case BN_CLICKED:{
                        switch(LOWORD(wParam)){
                            case ADD_BN:
                                printf("add");
                                break;
                            case DELETE_BN:
                                printf("add");
                                break;
                            case INSERT_BN:
                                printf("add");
                                break;
                            case PLAY_BN:
                                printf("add");
                                break;
                        }
                        break;
                    }
                }
            }
        break;
        }
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
            break;
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow) /*these parameters are given by the OS to the application so it can run on top of it*/
{
    WNDCLASSEX wc;
    HWND hwnd;      /*window handler*/
    MSG Msg;

    //Step 1: Registering the Window Class
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

    // Step 2: Creating the Window
    hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        g_szClassName,
        "Panel Animator",  /*window name*/
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 220, /*the size of the window*/
        NULL, NULL, hInstance, NULL);

    if(hwnd == NULL)
    {
        MessageBox(NULL, "Window Creation Failed!", "Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    //====== buttons and shit
    HWND hwndButton = CreateWindow("BUTTON",   /*a button and every other widget is called "window" but have different parameters*/
                                   "Add",
                                   WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                                   10, /*x*/
                                   10, /*y*/
                                   50, /*size x*/
                                   30, /*size y*/
                                   hwnd,
                                   (HMENU)ADD_BN,
                                   (HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE),
                                   NULL);

    HWND hwndButton2 = CreateWindow("BUTTON",
                                   "Insert",
                                   WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                                   70,
                                   10,
                                   50,
                                   30,
                                   hwnd,
                                   (HMENU)INSERT_BN,
                                   (HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE),
                                   NULL);

    HWND hwndButton3 = CreateWindow("BUTTON",
                                   "Delete",
                                   WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                                   130,
                                   10,
                                   50,
                                   30,
                                   hwnd,
                                   (HMENU)DELETE_BN,
                                   (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
                                   NULL);

    HWND hwndButton4 = CreateWindow("BUTTON",
                                   "Play",
                                   WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                                   190,
                                   10,
                                   50,
                                   30,
                                   hwnd,
                                   (HMENU)PLAY_BN,
                                   (HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE),
                                   NULL);



    HWND hWndComboBox = CreateWindow( WC_COMBOBOX,
                                      TEXT("yay"),
                                      CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE,
                                      320,
                                      10,
                                      70,
                                      30,
                                      hwnd,
                                      NULL,
                                      (HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE),
                                      NULL);

    HWND hWndComboBox2 = CreateWindow( WC_COMBOBOX,
                                      TEXT("yay"),
                                      CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE,
                                      400,
                                      10,
                                      70,
                                      30,
                                      hwnd,
                                      NULL,
                                      (HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE),
                                      NULL);

    ShowWindow(hwnd, nCmdShow);  /*needs to be done to create a windows*/
    UpdateWindow(hwnd);          /*should be done everytime a "widget" is added*/

    TextOut(GetDC(hwnd), 250, 15, "frame 1/1", 9);  /*IDK why, but the text only worked here */

    // Step 3: The Message Loop
    while(GetMessage(&Msg, NULL, 0, 0) > 0)
    {   /*this part of the code is like a while loop that runs everytime*/
        if(WaitCommEvent(hComm, &dwEventMask, NULL)){
            printf("receiving data");
            char TempChar; //Temporary character used for reading
            DWORD NoBytesRead;
            int i = 0;
                do{
                   ReadFile( hComm,           //Handle of the Serial port
                             &TempChar,       //Temporary character
                             sizeof(TempChar),//Size of TempChar
                             &NoBytesRead,    //Number of bytes read
                             NULL);

                   SerialBuffer[i] = TempChar;// Store Tempchar into buffer
                   i++;
                }while (NoBytesRead > 0);

            if (SerialBuffer[0]== 'M'){        /*check if is a message*/
                switch (SerialBuffer[1]){      /*check from which bank it should be*/
                    case 'r': /*bank 0, red*/
                        storeRXmsg(0, 1, SerialBuffer[2]);
                        break;
                    case 'R': /*bank 1, red*/
                        storeRXmsg(1, 1, SerialBuffer[2]);
                        break;
                    case 'g': /*bank 0, green*/
                        storeRXmsg(0, 0, SerialBuffer[2]);
                        break;
                    case 'G': /*bank 1, green*/
                        storeRXmsg(1, 0, SerialBuffer[2]);
                        break;

                }
            }
            if (SerialBuffer[0]=='B'){        /*check if is a bank change*/


            }
        }

        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    return Msg.wParam;
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
                (TCHAR) SendMessage(GetDlgItem(hwndDlg, COM_COMBO), (UINT) CB_GETLBTEXT,
                    (WPARAM) ItemIndex, (LPARAM) ListItem);
                strcpy(com_port, ListItem);

                ItemIndex = SendMessage(GetDlgItem(hwndDlg, BAUD_COMBO), (UINT) CB_GETCURSEL,
                        (WPARAM) 0, (LPARAM) 0);
                (TCHAR) SendMessage(GetDlgItem(hwndDlg, BAUD_COMBO), (UINT) CB_GETLBTEXT,
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

BOOL CALLBACK DlgPanelConf(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
    case WM_INITDIALOG:
    {
        SendMessage(GetDlgItem(hwndDlg, PANEL_LENGTH_SLIDER), TBM_SETRANGE,FALSE, MAKELONG(5, 10));
        SendMessage(GetDlgItem(hwndDlg, PANEL_LENGTH_SLIDER), TBM_SETPOS,TRUE, panelLength);
        SetDlgItemInt(hwndDlg, PANEL_LENGTH_TEXT, panelLength, FALSE);
        return TRUE;
    }
    case WM_CLOSE:
        EndDialog(hwndDlg, 0);
        return TRUE;
    case WM_HSCROLL:
        SetDlgItemInt(hwndDlg, PANEL_LENGTH_TEXT, SendMessage(GetDlgItem(hwndDlg, PANEL_LENGTH_SLIDER), TBM_GETPOS, 0, 0), FALSE);
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

void storeRXmsg(int bank, bool LEDcolor, char msgSize){
/*
    bank = 0 -> bank 0
    bank = 1 -> bank 1
    LEDcolor = 1 -> red
    LEDcolor = 0 -> green
    msgSize is a char containing the number of bytes to be read
*/
    if (LEDcolor==1){
        for (int i=0;i=msgSize;i++){
            REDbank[bank][i] = SerialBuffer[i+2];
        }
    }
    else{
        for (int i=0;i=msgSize;i++){
            GREENbank[bank][i] = SerialBuffer[i+2];
        }
    }


}

int openSerial(void) {
     hComm = CreateFile(com_port,          // for COM1—COM9 only
                        GENERIC_READ,    // Read only
                        0,               // No Sharing
                        NULL,            // No Security
                        OPEN_EXISTING,   // Open existing port only
                        0,               // Non Overlapped I/O
                        NULL);

    if (hComm == INVALID_HANDLE_VALUE){
        MessageBox(hwnd,
                        "Serial port could not be started!\nCheck your connection settings and try again.",
                        "Serial fail",
                        MB_OK);
        return 0;
    }
    printf("connected");
    return 1;
}

void printCharacterOnPanel(HDC hDC, unsigned int panelIndex, int charOffsetX, int ledOffsetY, bool character[35]) {
    int panelX = 10 + charOffsetX*70 + 2;
    int panelY = 50 + 110*panelIndex + ledOffsetY + 2;

    RECT        LEDRect;
    HBRUSH myBrush = CreateSolidBrush(clrRed);
    char pos = 0;
    for(int ledY = 0; ledY < 7; ledY++) {
        for(int ledX = 0; ledX < 5; ledX++) {
            LEDRect = { panelX, panelY, panelX+10, panelY+10 };
            if (character[pos] == 1){
            FillRect(hDC,&LEDRect,myBrush);
            }
            pos++;
            panelX += 14;
        }
        panelY += 14;
        panelX = 10 + charOffsetX*70 + 2;
    }
}
