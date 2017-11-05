#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <CommCtrl.h>
#include "resource.h"
const char g_szClassName[] = "myWindowClass";
HINSTANCE hInst;
HANDLE hComm;
TCHAR com_port[] = "COMX";

DCB dcbSerialParams = { 0 }; // Initializing DCB structure

BOOL CALLBACK DlgMain(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
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

        SendMessage(GetDlgItem(hwndDlg, BAUD_COMBO),(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) "4800");
        SendMessage(GetDlgItem(hwndDlg, BAUD_COMBO),(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) "9600");
        SendMessage(GetDlgItem(hwndDlg, BAUD_COMBO),(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) "19200");
        SendMessage(GetDlgItem(hwndDlg, BAUD_COMBO),(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) "38400");
        SendMessage(GetDlgItem(hwndDlg, BAUD_COMBO),(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) "57600");
        SendMessage(GetDlgItem(hwndDlg, BAUD_COMBO),(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) "115200");

        SendMessage(GetDlgItem(hwndDlg, PAR_COMBO),(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) "No parity");
        SendMessage(GetDlgItem(hwndDlg, PAR_COMBO),(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) "Odd");
        SendMessage(GetDlgItem(hwndDlg, PAR_COMBO),(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) "Even");

        SendMessage(GetDlgItem(hwndDlg, STOPBITS_COMBO),(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) "1");
        SendMessage(GetDlgItem(hwndDlg, STOPBITS_COMBO),(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) "2");

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
        case WM_COMMAND:
        {
            if(HIWORD(wParam) == 0 || HIWORD(wParam) == 1) /* Menu command */
            {
                switch(LOWORD(wParam))
                {
                    case MENU_CONF_SERIAL:
                        DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG1), NULL, (DLGPROC)DlgMain);
                        break;
                    case MENU_SERIAL_CONNECT:
                    {
                        hComm = CreateFile((LPCSTR)com_port,          // for COM1—COM9 only
                           GENERIC_READ | GENERIC_WRITE, //Read/Write
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
                                DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG1), NULL, (DLGPROC)DlgMain);
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
                }
            }
            else /* control commands */
            { /* HIWORD contains BN_CLIKCED, etc */
                switch(HIWORD(wParam))
                {
                    case BN_CLICKED:
                        switch(LOWORD(wParam))
                        {
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
    HWND hwnd;
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
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 500, /*the size of the window*/
        NULL, NULL, hInstance, NULL);

    if(hwnd == NULL)
    {
        MessageBox(NULL, "Window Creation Failed!", "Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    //====== buttons and shit
//    HWND hwndButton = CreateWindow("BUTTON",   /*a button and every other widget is called "window" but have different parameters*/
//                                   "Add",
//                                   WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
//                                   10, /*x*/
//                                   10, /*y*/
//                                   50, /*size x*/
//                                   30, /*size y*/
//                                   hwnd,
//                                   (HMENU)TEST_BUTTON,
//                                   (HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE),
//                                   NULL);

    ShowWindow(hwnd, nCmdShow);  /*needs to be done to create a windows*/
    UpdateWindow(hwnd);          /*should be done everytime a "widget" is added*/

    // Step 3: The Message Loop
    while(GetMessage(&Msg, NULL, 0, 0) > 0)
    {   /*this part of the code is like a while loop that runs everytime*/
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    return Msg.wParam;
}
