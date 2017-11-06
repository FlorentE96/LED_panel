#include <windows.h>
#include <stdio.h>
#include <CommCtrl.h>
#include <string.h>
#include <iostream>
#include "resource.h"

const char g_szClassName[] = "myWindowClass";
HANDLE hComm;
HINSTANCE hInst;
TCHAR com_port[] = "COMX";
DCB dcbSerialParams = { 0 }; // Initializing DCB (serial) structure

BOOL CALLBACK DlgSerialConf(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void loadFileDlg();
int loadProjectFile(char * filename);
// Step 4: the Window Procedure
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
        case WM_COMMAND:{
            if (HIWORD(wParam) == 1 || HIWORD(wParam) == 0){
                switch(LOWORD(wParam)){
                    case MENU_OPEN_BN:
                        loadFileDlg();
                        break;
                    case MENU_CLOSE_BN:
                        DestroyWindow(hwnd);
                        break;
                    case MENU_CONN_BN:
                        DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_SERIAL_CONF), NULL, (DLGPROC)DlgSerialConf);

                        break;
                    case MENU_SEND_BN:

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
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 500, /*the size of the window*/
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


int loadProjectFile(char * filename) {
    HANDLE hFile = CreateFile((LPCTSTR) filename,
                GENERIC_READ,
                0   ,
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

void loadFileDlg(){
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
      ofn.lpstrTitle   = "Select a File, yo!";
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


}
