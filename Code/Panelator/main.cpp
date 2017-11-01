#include <windows.h>
#include <stdio.h>
#include <CommCtrl.h>
#include "resource.h"
const char g_szClassName[] = "myWindowClass";

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
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
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
                                   (HMENU)TEST_BUTTON,
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
                                   (HMENU)TEST_BUTTON,
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
                                   (HMENU)TEST_BUTTON,
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
                                   (HMENU)TEST_BUTTON,
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
