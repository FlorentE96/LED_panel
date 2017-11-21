#include <windows.h>
#include <stdio.h>
#include <CommCtrl.h>
#include <string.h>
#include <iostream>
#include "resource.h"
#include "colors.h"

#define TIMER1 1001

//vars
const char g_szClassName[] = "myWindowClass";
HANDLE hComm;
HINSTANCE hInst;
TCHAR com_port[] = "COMX";
DCB dcbSerialParams = { 0 }; // Initializing DCB (serial) structure
HWND hwnd;
int panelLength = 8;
DWORD dwEventMask;
char SerialBuffer[255];//Buffer for storing Rxed Data
COMMTIMEOUTS timeouts;
char REDbank[2][70];
char GREENbank[2][70];
bool characterRED[10][35];
bool characterGREEN[10][35];
int i = 0;
HWND global_hwnd;
//functions
BOOL CALLBACK DlgPanelConf(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgSerialConf(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void printCharacterOnPanel(HDC hDC, unsigned int panelIndex, int charOffsetX, int ledOffsetY);
int openSerial(void);
void storeRXmsg(int bank, bool LEDcolor, char msgSize);
void byteToBool(int bank);
void serialTreat(void);

inline char calculateChecksum(char* inString, int length){
    int sum=0;
    for(int u=0; u<length; u++)
    {
        sum += inString[u];
    }
    return sum&0xFF;
}

void CALLBACK func1(){
DWORD NBytesRead;
char tempChar;
COMSTAT commstat;
DWORD dwErrors = 0;
if(hComm == INVALID_HANDLE_VALUE){
    printf("serial receive error");
    return;
}
memset(&commstat, 0, sizeof(commstat));
ClearCommError(hComm, &dwErrors,  &commstat);
    if (commstat.cbInQue) {
        ReadFile( hComm, &tempChar, 1, &NBytesRead,NULL);
        SerialBuffer[i] = tempChar;
        printf("%s",&SerialBuffer[i]);
        i++;
        if (SerialBuffer[i-1] == 13){
            //if (calculateChecksum(SerialBuffer, i-3) == SerialBuffer[i-2]){
                //now it should serial TX that the checksum worked
                char lpBuffer = 0xaa;
                DWORD dNoOfBytesWritten = 0;    // No of bytes written to the port

                WriteFile(hComm,                // Handle to the Serial port
                          (LPCVOID)lpBuffer,             // Data to be written to the port
                          sizeof(lpBuffer),     // No of bytes to write
                          &dNoOfBytesWritten,   // Bytes written
                          NULL);

                serialTreat();
//            } else {
//                //here it should serial TX that the checksum was wrong
//                char lpBuffer = 0xff;
//                DWORD dNoOfBytesWritten = 0;    // No of bytes written to the port
//
//                WriteFile(hComm,                // Handle to the Serial port
//                          (LPCVOID)lpBuffer,    // Data to be written to the port
//                          sizeof(lpBuffer),     //No of bytes to write
//                          &dNoOfBytesWritten,   //Bytes written
//                          NULL);
//            }
        }
    }
}

void dispUpd(void);

void serialTreat(void){
    printf("\nmensagem totalmente recebida");
    i = 0;
    if (SerialBuffer[0]== 'M'){        /*check if is a message*/
        switch (SerialBuffer[1]){      /*check in which bank it should be stored*/
            case 'r': /*bank 0, red*/
                printf("\nmsg stored at R0");
                storeRXmsg(0, 1, SerialBuffer[2]);
                break;
            case 'R': /*bank 1, red*/
                storeRXmsg(1, 1, SerialBuffer[2]);
                printf("\nmsg stored at R1");
                break;
            case 'g': /*bank 0, green*/
                storeRXmsg(0, 0, SerialBuffer[2]);
                printf("\nmsg stored at G0");
                break;
            case 'G': /*bank 1, green*/
                storeRXmsg(1, 0, SerialBuffer[2]);
                printf("\nmsg stored at G1");
                break;
        }
    }
    if (SerialBuffer[0]=='B'){        /*check if is a bank change*/
        if(SerialBuffer[1]=='0'){
            printf("\ncalled b0");
            byteToBool(0);
            //convert current bank message to bool array and store it at characterRED and characterGREEN
        }
        else{
            printf("\ncalled b1");
            byteToBool(1);
        }

    }
memset(SerialBuffer, 0, 255);
dispUpd();
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{   /*this part of the code process the "events" since it's a event oriented language*/
    global_hwnd = hwnd;
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
            printf("\nWM PAINT");
            HDC         hDC;
            PAINTSTRUCT ps;
            RECT        panel1 = { 10, 20+30, panelLength*70+10, 98+20+30 };

            HBRUSH myBrush = CreateSolidBrush(clrBlack);

            hDC = BeginPaint(hwnd, &ps);

            FillRect(hDC,&panel1,myBrush);
            char g = 0;
            while (g < panelLength){
            printCharacterOnPanel(hDC, 0, g, 0);
            g++;
            }
            EndPaint(hwnd, &ps);
        break;
        }
        case WM_COMMAND:{
            printf("\nWM command");
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
                        break;
                    case MENU_CONF_PANEL:
                        DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_PANEL_CONF), NULL, (DLGPROC)DlgPanelConf);
                        break;
                }
            } else{
                switch(HIWORD(wParam)){

                }
            }
        return 1;
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
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 240, /*the size of the window*/
        NULL, NULL, hInstance, NULL);

    if(hwnd == NULL)
    {
        MessageBox(NULL, "Window Creation Failed!", "Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    ShowWindow(hwnd, nCmdShow);  /*needs to be done to create a windows*/
    UpdateWindow(hwnd);          /*should be done everytime a "widget" is added*/

    while(GetMessage(&Msg, NULL, 0, 0) > 0){
    /*this part of the code is like a while loop that runs everytime*/
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
        SendMessage(GetDlgItem(hwndDlg, COM_COMBO), CB_SETCURSEL, (WPARAM)1, (LPARAM)0);

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
        SendMessage(GetDlgItem(hwndDlg, PAR_COMBO), CB_SETCURSEL, (WPARAM)0, (LPARAM)0);

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
    if (LEDcolor == 1){
        for (int t=0;t<msgSize;t++){
            REDbank[bank][t] = SerialBuffer[t+3];
        }
        printf("\n");
        printf(REDbank[0]);
    }
    else{
        for (int t=0;t<msgSize;t++){
            GREENbank[bank][t] = SerialBuffer[t+3];
        }
    }
 }

int openSerial(void) {
    if(hComm != NULL)
        CloseHandle(hComm);
    hComm = CreateFile(com_port,         // for COM1-COM9 only
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
    else {

        MessageBox(             NULL,
                                (LPCSTR)"COM Port successfully opened",
                                (LPCSTR)"Success",
                                MB_ICONINFORMATION | MB_OK | MB_DEFBUTTON1
        );

    }
    timeouts.ReadIntervalTimeout = 3;
    timeouts.ReadTotalTimeoutMultiplier = 3;
    timeouts.ReadTotalTimeoutConstant = 2;
    timeouts.WriteTotalTimeoutMultiplier = 3;
    timeouts.WriteTotalTimeoutConstant = 2;
    SetTimer(global_hwnd, TIMER1, 5, (TIMERPROC)func1);
    SetCommState(hComm, &dcbSerialParams);
    if (!SetCommTimeouts(hComm, &timeouts)){
        printf("set timeout failed");
    }

    printf("\nconnected");
    return 1;
}

void printCharacterOnPanel(HDC hDC, unsigned int panelIndex, int charOffsetX, int ledOffsetY) {
    int panelX = 10 + charOffsetX*70 + 2;
    int panelY = 50 + 110*panelIndex + ledOffsetY + 2;

    RECT        LEDRect;
    HBRUSH myBrush = CreateSolidBrush(clrRed);
    char pos = 0;
    for(int ledY = 0; ledY < 7; ledY++) {
        for(int ledX = 0; ledX < 5; ledX++) {
            LEDRect = { panelX, panelY, panelX+10, panelY+10 };
            if (characterRED[charOffsetX][pos] & characterGREEN[charOffsetX][pos]){
                HBRUSH myBrush = CreateSolidBrush(clrYellow);
                FillRect(hDC,&LEDRect,myBrush);
                }
            else if (characterRED[charOffsetX][pos] & !characterGREEN[charOffsetX][pos]){
                HBRUSH myBrush = CreateSolidBrush(clrRed);
                FillRect(hDC,&LEDRect,myBrush);
            }
            else if (!characterRED[charOffsetX][pos] & characterGREEN[charOffsetX][pos]){
                HBRUSH myBrush = CreateSolidBrush(clrGreen);
                FillRect(hDC,&LEDRect,myBrush);
            }
            pos++;
            panelX += 14;
        }
        panelY += 14;
        panelX = 10 + charOffsetX*70 + 2;
    }
printf("\nprinted on disp");
}

void byteToBool(int bank){
int pos = 0;
    for(int character = 0; character < panelLength; character ++){
        for (int l = 0; l < 7; l++){
            for (int j = 0; j < 5; j++){
                characterRED[character][pos + j] = (REDbank[bank][l + character * 7 ] & (1 << j));
                printf("%d",characterRED[character][pos + j]);
                characterGREEN[character][pos + j] = (GREENbank[bank][l + character * 7 ] & (1 << j));
            }
        pos = pos + 5;
        }
    pos = 0;
    }
}

void dispUpd(void){
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);
}
