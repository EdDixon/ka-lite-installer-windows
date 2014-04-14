#include <Windows.h>
#include <stdio.h>
#include "resource.h"
#include "config.h"
#include "version.h"
#include "traynotification.h"
#include "Shlwapi.h"

#include "wininet.h"

#include <tchar.h>
#include <strsafe.h>

#pragma comment(lib, "Wininet")

using namespace std;

// Window Menu ID's
#define ID_FILE_EXIT 9001
#define ID_STUFF_GO 9002
#define ID_HELP_ABOUT 9003
#define ID_OPTIONS_RUNSTARTUP 9004
#define ID_OPTIONS_AUTOMINIMIZE 9005
#define ID_OPTIONS_AUTOSTART 9006
#define ID_OPTIONS_RUNUSERLOGIN 9007

// Main Buttons ID's.
#define ID_MINIMIZE_BUTTON 1003
#define ID_OPEN_IN_BROWSER 1004
#define ID_START_STOP_BUTTON 1005

// Tray Menu options ID.
#define ID_TRAY_EXIT_CONTEXT_MENU_ITEM  3000
#define ID_TRAY_BROWSER_CONTEXT_MENU_ITEM 3003
#define ID_TRAY_RESTORE_CONTEXT_MENU_ITEM 3004
#define ID_TRAY_START_STOP_CONTEXT_MENU_ITEM 3005

// setStartupShortcut options
#define CONFIG_YES               '0'
#define CONFIG_NO                '1'

// UINT that defines enabled/disabled state tray buttons
UINT TRAY_ENABLED    = MF_ENABLED;
UINT TRAY_DISABLED   = MF_DISABLED | MF_GRAYED;

// UINT that will hold a new defined message.
UINT WM_TASKBARCREATED = 0;

// Handle to main window.
HWND hwnd;

// Handle to window Menu.
HMENU hMenu;
HMENU hMainMenu;
HMENU hFileSubMenu;
HMENU hOptionsSubMenu;
HMENU hAboutSubMenu;

// Handle to main buttons.
HWND hstart;
HWND hstop;

// Device context ( used to paint ).
HDC hDC;

// Main module handle.
HINSTANCE hINSTANCE;

// Tray icon.
HBITMAP bitmap;

// Structures to specify how will both start and stop script run.
SHELLEXECUTEINFO startServerShellExecuteInfo;
SHELLEXECUTEINFO stopServerShellExecuteInfo;

// Structure for user login autostart
SHELLEXECUTEINFO winshortcutInfo;

// Structure for startup task
SHELLEXECUTEINFO startuptaskInfo;

// Structure for server state check
SHELLEXECUTEINFO server_state_info;

// Process ID.
DWORD winshortcutID;

// Process handle.
HANDLE winshortcutHandle;

// Component messages.
UINT RUN_AT_STARTUP_CHECK_STATE;
UINT AUTO_MINIMIZE_CHECK_STATE;
UINT AUTO_START_SERVER_CHECK_STATE;
UINT RUN_AT_USER_LOGIN_CHECK_STATE;

// Control flags.
bool SERVERISRUNNING;
bool CHANGEDSTATE;
bool SERVERISONLINE;
bool NEEDTOCHECK;

// Mutex handle;
HANDLE ghMutex;

// winshortcut script return value.
DWORD winshortcutReturn;

// server state check return value.
DWORD server_state_return;

// Program class name.
TCHAR className[] = L"KA Lite Class";

// Configuration options section.
char runAtStartupOption    [10];
char autoMinimizeOption    [10];
char autoStartOption       [10];
char runAtUserLoginOption  [10];



/*
*	Functions declaration.
*/
void loadConfigurations();
void setStartupShortcut(char CONFIG_OPTION);
void prepareStartServerStructure();
void prepareStopServerStructure();
void startServerCommand();
void stopServerCommand(int op);
bool serverIsRunning();
void refreshServerStateTrayMenuText(LPCWSTR serverAction, LPCWSTR openBrowser, LPCWSTR restoreWindow, LPCWSTR exit);
void enabledTrayMenuButtons(UINT serverAction, UINT openBrowser, UINT restoreWindow, UINT exit);
void enabledMainWindowButtons(bool serverAction, bool openBrowser);
void CheckMenus();
LRESULT CALLBACK AboutDialogProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
HWND GetRunningWindow();
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
DWORD WINAPI isServerOnline(LPVOID lpParam);
void isServerOnlineThread();



/*
*	This section is to load configurations.
*/
void loadConfigurations()
{
	if(getConfigurationValue("RUN_AT_USER_LOGIN", runAtUserLoginOption, 10) == 0 )
	{
		if(compareKeys(runAtUserLoginOption, "TRUE"))
		{
			setStartupShortcut(CONFIG_YES);
			RUN_AT_USER_LOGIN_CHECK_STATE = SW_SHOWNA;
		}
	}

	if(getConfigurationValue("RUN_AT_STARTUP", runAtStartupOption, 10) == 0 )
	{
		if(compareKeys(runAtStartupOption, "TRUE"))
		{
			//setStartupShortcut(CONFIG_YES);
			RUN_AT_STARTUP_CHECK_STATE = SW_SHOWNA;
		}
	}

	if(getConfigurationValue("AUTO_MINIMIZE", autoMinimizeOption, 10) == 0)
	{
		if(compareKeys(autoMinimizeOption, "TRUE"))
		{
			AUTO_MINIMIZE_CHECK_STATE = SW_SHOWNA;
		}
	}

	if(getConfigurationValue("AUTO_START_SERVER", autoStartOption, 10) == 0)
	{
		if(compareKeys(autoStartOption, "TRUE"))
		{
			AUTO_START_SERVER_CHECK_STATE = SW_SHOWNA;
			startServerCommand();
		}
	}
}



/*
*	This function is used to call winshortcut script.
*/
void setStartupShortcut(char CONFIG_OPTION)
{
	winshortcutInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	winshortcutInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	winshortcutInfo.hwnd = NULL;
	winshortcutInfo.lpVerb = L"open";
	winshortcutInfo.lpFile = L"guitools.vbs" ;
	if(CONFIG_OPTION == CONFIG_YES)
	{
		winshortcutInfo.lpParameters = L"0";
	}
	else if(CONFIG_OPTION == CONFIG_NO)
	{
		winshortcutInfo.lpParameters = L"1";
	}
	winshortcutInfo.lpDirectory = NULL;
	winshortcutInfo.nShow = SW_HIDE;
	winshortcutInfo.hInstApp = NULL;

	if(!ShellExecuteEx(&winshortcutInfo))
	{
		MessageBox(NULL, L"Failed to create startup shortcut!", L"Error", MB_OK | MB_ICONERROR);
	}
}



/*
*	This function is used to call the script that adds a task to run KA Lite at windows startup
*/
void setStartupTask(char CONFIG_OPTION)
{
	startuptaskInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	startuptaskInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	startuptaskInfo.hwnd = NULL;
	startuptaskInfo.lpVerb = L"open";
	startuptaskInfo.lpFile = L"guitools.vbs";

	if(CONFIG_OPTION == CONFIG_YES)
	{
		startuptaskInfo.lpParameters = L"4";
	}
	else if(CONFIG_OPTION == CONFIG_NO)
	{
		startuptaskInfo.lpParameters = L"5";
	}

	startuptaskInfo.lpDirectory = NULL;
	startuptaskInfo.nShow = SW_SHOW;
	startuptaskInfo.hInstApp = NULL;

	if(!ShellExecuteEx(&startuptaskInfo))
	{
		if(CONFIG_OPTION == CONFIG_YES)
		{
			MessageBox(NULL, L"Failed to add startup task!", L"Error", MB_OK | MB_ICONERROR);
		}
		else
		{
			MessageBox(NULL, L"Failed to remove startup task!", L"Error", MB_OK | MB_ICONERROR);
		}
	}
}



/*
*	This prepares the structures needed to start and stop the server.
*/
void prepareStartServerStructure()
{
	startServerShellExecuteInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	startServerShellExecuteInfo.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;
	startServerShellExecuteInfo.hwnd = NULL;
	startServerShellExecuteInfo.lpVerb = L"open";
	startServerShellExecuteInfo.lpFile = L"cmd.exe" ;
	startServerShellExecuteInfo.lpParameters = L"/C start.bat";
	startServerShellExecuteInfo.lpDirectory = L"ka-lite\\";
	startServerShellExecuteInfo.nShow = SW_HIDE;
	startServerShellExecuteInfo.hInstApp = NULL;
}

void prepareStopServerStructure()
{
	stopServerShellExecuteInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	stopServerShellExecuteInfo.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;
	stopServerShellExecuteInfo.hwnd = NULL;
	stopServerShellExecuteInfo.lpVerb = L"open";
	stopServerShellExecuteInfo.lpFile = L"cmd.exe";
	stopServerShellExecuteInfo.lpParameters = L"/C stop.bat";
	stopServerShellExecuteInfo.lpDirectory = L"ka-lite\\";
	stopServerShellExecuteInfo.nShow = SW_HIDE;
	stopServerShellExecuteInfo.hInstApp = NULL;
}



/*
*	Check if a file with the pid exists to determine if the server was running before.
*/
bool serverIsRunning()
{
	server_state_info.cbSize = sizeof(SHELLEXECUTEINFO);
	server_state_info.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;
	server_state_info.hwnd = NULL;
	server_state_info.lpVerb = L"open";
	server_state_info.lpFile = L"guitools.vbs";
	server_state_info.lpParameters = L"3";
	server_state_info.lpDirectory = NULL;
	server_state_info.nShow = SW_HIDE;
	server_state_info.hInstApp = NULL;

	DWORD exit_code;

	if(ShellExecuteEx(&server_state_info))
	{
		WaitForSingleObject( server_state_info.hProcess, INFINITE );

		GetExitCodeProcess(server_state_info.hProcess, &exit_code);
		if(exit_code==0)
		{
			return true;
		}
		return false;
	}
	return false;
}



/*
*	This function starts the server.
*/
void startServerCommand()
{
	prepareStartServerStructure();
	if(ShellExecuteEx(&startServerShellExecuteInfo))
	{
		SERVERISRUNNING = TRUE;
		sendTrayMessage(hwnd, "KA Lite is running", "The server will be accessible locally at: http://127.0.0.1:8008/ or you can press \"Open KA Lite button\"");
	}
	else
	{
		MessageBox(hwnd,L"Could not start the server!", L"Starting error", MB_OK | MB_ICONERROR);
	}					
}



/*
*	This function stops the server and kill any python process started by KA Lite.
*/
void stopServerCommand(int op)
{	
	prepareStopServerStructure();
	if(ShellExecuteEx(&stopServerShellExecuteInfo))
	{
		SERVERISRUNNING = FALSE;
		if(op==0)
		{
			sendTrayMessage(hwnd, "KA Lite has successfully stopped", "You may now close the aplication or restart the server.");
		}
	}
	else
	{
		MessageBox(hwnd,L"Could not stop the server!", L"Stopping error", MB_OK | MB_ICONERROR);
	}
}



/*
*	This function sets the tray menu options text.
*/
void refreshServerStateTrayMenuText(LPCWSTR serverAction, LPCWSTR openBrowser, LPCWSTR restoreWindow, LPCWSTR exit)
{
	AppendMenu(hMenu, MF_STRING, ID_TRAY_START_STOP_CONTEXT_MENU_ITEM, serverAction);											
	AppendMenu(hMenu, MF_STRING, ID_TRAY_BROWSER_CONTEXT_MENU_ITEM, openBrowser);
	AppendMenu(hMenu, MF_STRING, ID_TRAY_RESTORE_CONTEXT_MENU_ITEM, restoreWindow);
	AppendMenu(hMenu, MF_STRING, ID_TRAY_EXIT_CONTEXT_MENU_ITEM, exit);
}



/*
*	This function refresh the text over start/stop button.
*/
void refreshMainWindowStartStopButtonText(LPCWSTR serverAction)
{
	if(CHANGEDSTATE && IsWindowVisible(hwnd))
	{
		SetDlgItemTextW(hwnd, ID_START_STOP_BUTTON, serverAction);
		CHANGEDSTATE = false;
	}
}



/*
*	This function enables or disables the tray buttons menu.
*   Options: TRAY_ENABLED/TRAY_DISABLED
*/
void enabledTrayMenuButtons(UINT serverAction, UINT openBrowser, UINT restoreWindow, UINT exit)
{
	EnableMenuItem(hMenu, ID_TRAY_START_STOP_CONTEXT_MENU_ITEM, serverAction);
	EnableMenuItem(hMenu, ID_TRAY_BROWSER_CONTEXT_MENU_ITEM, openBrowser);
	EnableMenuItem(hMenu, ID_TRAY_RESTORE_CONTEXT_MENU_ITEM, restoreWindow);
	EnableMenuItem(hMenu, ID_TRAY_EXIT_CONTEXT_MENU_ITEM, exit);
}



/*
*	This function enables or disables the main window buttons.
*/
void enabledMainWindowButtons(bool serverAction, bool openBrowser)
{
	EnableWindow(GetDlgItem(hwnd,ID_START_STOP_BUTTON), serverAction);
	EnableWindow(GetDlgItem(hwnd,ID_OPEN_IN_BROWSER), openBrowser);
}



/*
*	This function is used to check and control the text and availability of all the
*   options in each mode (server running / server stoped).
*/
void CheckMenus()
{
	if(SERVERISRUNNING)
	{
		if(SERVERISONLINE)
		{
			enabledTrayMenuButtons(TRAY_ENABLED, TRAY_ENABLED, TRAY_ENABLED, TRAY_ENABLED);
			enabledMainWindowButtons(TRUE, TRUE);
		}
		else
		{
			enabledTrayMenuButtons(TRAY_DISABLED, TRAY_DISABLED, TRAY_ENABLED, TRAY_ENABLED);
			enabledMainWindowButtons(FALSE, FALSE);
		}
		
		hMenu = CreatePopupMenu();
		refreshServerStateTrayMenuText(L"Stop server", L"Open KA Lite", L"Restore window", L"Exit KA Lite");
		refreshMainWindowStartStopButtonText(L"Stop server");	
	}
	else
	{
		enabledTrayMenuButtons(TRAY_ENABLED, TRAY_DISABLED, TRAY_ENABLED, TRAY_ENABLED);
		enabledMainWindowButtons(TRUE, FALSE);	
		hMenu = CreatePopupMenu();
		refreshServerStateTrayMenuText(L"Start server", L"Open KA Lite", L"Restore window", L"Exit KA Lite");
		refreshMainWindowStartStopButtonText(L"Start server");	
	}
}



/*
*	This function is used to handle the messages that come to "about" dialog.
*/
LRESULT CALLBACK AboutDialogProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch(Message)
	{
	case WM_INITDIALOG:
		return TRUE;
	case WM_COMMAND:
		switch(wParam)
		{
		case IDOK:
			EndDialog(hwnd, IDOK);
			break;
		case IDCANCEL:
			EndDialog(hwnd, IDCANCEL);
			break;
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}



/*
*	Main window function that processes the messages that come to the main window.
*/
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

	CheckMenus();

	if (SERVERISRUNNING)
	{
		if (NEEDTOCHECK)
		{
			isServerOnlineThread();

			DWORD dwWaitResult = WaitForSingleObject(ghMutex, INFINITE);

			switch (dwWaitResult) 
			{
				case WAIT_OBJECT_0: 
					__try 
					{ 
						if (SERVERISONLINE)
						{
							NEEDTOCHECK = FALSE;
						}
					} 
			
					__finally
					{ 
						if (!ReleaseMutex(ghMutex)) 
						{ 
							MessageBox(NULL, L"Failed to release the Mutex", L"Error",MB_ICONEXCLAMATION | MB_OK);
						} 
					} 
					break; 

				case WAIT_ABANDONED:
					break; 
			}
		}
	}

	if( msg==WM_TASKBARCREATED && !IsWindowVisible(hwnd))
	{
		Minimize(hwnd, "KA Lite", "Right click over the tray icon for more options.");
		return 0;
	}

	switch(msg)
	{

	case WM_PAINT:
		{
			HDC hDC, MemDCLogo;
			PAINTSTRUCT Ps;
			HBITMAP bmp1;

			hDC = BeginPaint(hwnd, &Ps);

			bmp1 = LoadBitmap(hINSTANCE, MAKEINTRESOURCE(IDB_BITMAP2));

			if(bmp1 == NULL)
				MessageBox(hwnd, L"Image not loaded", L"Image Error", MB_OK | MB_ICONINFORMATION);

			MemDCLogo = CreateCompatibleDC(hDC);
			if(MemDCLogo == NULL)
				MessageBox(hwnd, L"HDC error", L"HDC Error", MB_OK | MB_ICONINFORMATION);


			SelectObject(MemDCLogo, bmp1);
			BitBlt(hDC, 3, 3, 261, 66, MemDCLogo, 0, 0, SRCCOPY); 
			DeleteDC(MemDCLogo);
			DeleteObject(bmp1);			
			EndPaint(hwnd, &Ps);
		}
		break;

	case WM_CREATE:
		{
			HICON hIcon, hIconSm;
			hMenu = CreatePopupMenu();

			if(SERVERISRUNNING)
			{
				AppendMenu(hMenu, MF_STRING, ID_TRAY_START_STOP_CONTEXT_MENU_ITEM, L"Stop server");
			} 
			else 
			{
				AppendMenu(hMenu, MF_STRING, ID_TRAY_START_STOP_CONTEXT_MENU_ITEM, L"Start server");
			}

			AppendMenu(hMenu, MF_STRING, ID_TRAY_BROWSER_CONTEXT_MENU_ITEM, L"Open KA Lite");
			AppendMenu(hMenu, MF_STRING, ID_TRAY_RESTORE_CONTEXT_MENU_ITEM, L"Restore window");
			AppendMenu(hMenu, MF_STRING, ID_TRAY_EXIT_CONTEXT_MENU_ITEM, L"Exit KA Lite");				

			hMainMenu = CreateMenu();
			hFileSubMenu = CreatePopupMenu();
			AppendMenu(hFileSubMenu, MF_STRING, ID_FILE_EXIT, L"E&xit");
			AppendMenu(hMainMenu, MF_STRING | MF_POPUP, (UINT)hFileSubMenu, L"&File");

			hOptionsSubMenu = CreatePopupMenu();
			AppendMenu(hOptionsSubMenu, MF_STRING, ID_OPTIONS_RUNUSERLOGIN, L"Run KA Lite when the user log");
			AppendMenu(hOptionsSubMenu, MF_STRING, ID_OPTIONS_RUNSTARTUP, L"Run KA Lite at system startup");
			AppendMenu(hOptionsSubMenu, MF_STRING, ID_OPTIONS_AUTOMINIMIZE, L"Minimize to tray when KA Lite is run");
			AppendMenu(hOptionsSubMenu, MF_STRING, ID_OPTIONS_AUTOSTART, L"Auto-start server when KA Lite is run");
			AppendMenu(hMainMenu, MF_STRING | MF_POPUP, (UINT)hOptionsSubMenu, L"Options");

			hAboutSubMenu = CreatePopupMenu();
			AppendMenu(hAboutSubMenu, MF_STRING, ID_HELP_ABOUT, L"&About");
			AppendMenu(hMainMenu, MF_STRING | MF_POPUP, (UINT)hAboutSubMenu, L"&Help");

			SetMenu(hwnd, hMainMenu);

			hIcon = LoadIcon(hINSTANCE, MAKEINTRESOURCE(IDI_ICON1));
			if(hIcon)
			{
				SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
			}
			else
			{
				MessageBox(hwnd, L"Failed to load the image.", L"Error", MB_OK | MB_ICONERROR);
			}

			hIconSm = LoadIcon(hINSTANCE,MAKEINTRESOURCE(IDI_ICON1));
			if(hIcon)
			{
				SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
			}
			else
			{
				MessageBox(hwnd, L"Failed to load the image.", L"Error", MB_OK | MB_ICONERROR);
			}

			CreateWindowEx(WS_EX_CLIENTEDGE, L"BUTTON", L"Start server", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 10, 83, 120, 30, hwnd, (HMENU) ID_START_STOP_BUTTON,(HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE), NULL);
			CreateWindowEx(WS_EX_CLIENTEDGE, L"BUTTON", L"Open KA lite",  WS_TABSTOP |WS_VISIBLE|WS_CHILD | BS_DEFPUSHBUTTON, 140, 83, 120,30, hwnd, (HMENU) ID_OPEN_IN_BROWSER, (HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE), NULL);

			if (RUN_AT_USER_LOGIN_CHECK_STATE == SW_SHOWNA)
			{
				CheckMenuItem(hOptionsSubMenu, ID_OPTIONS_RUNUSERLOGIN, MF_CHECKED);
			}
			else
			{
				CheckMenuItem(hOptionsSubMenu, ID_OPTIONS_RUNUSERLOGIN, MF_UNCHECKED);
			}

			if (RUN_AT_STARTUP_CHECK_STATE == SW_SHOWNA) 
			{
				CheckMenuItem(hOptionsSubMenu, ID_OPTIONS_RUNSTARTUP, MF_CHECKED);
			}
			else 
			{
				CheckMenuItem(hOptionsSubMenu, ID_OPTIONS_RUNSTARTUP, MF_UNCHECKED);
			}

			if (AUTO_MINIMIZE_CHECK_STATE == SW_SHOWNA)
			{
				CheckMenuItem(hOptionsSubMenu, ID_OPTIONS_AUTOMINIMIZE, MF_CHECKED);
			} 
			else
			{
				CheckMenuItem(hOptionsSubMenu, ID_OPTIONS_AUTOMINIMIZE, MF_UNCHECKED);
			}

			if (AUTO_START_SERVER_CHECK_STATE == SW_SHOWNA)
			{
				CheckMenuItem(hOptionsSubMenu, ID_OPTIONS_AUTOSTART, MF_CHECKED);
			} 
			else
			{
				CheckMenuItem(hOptionsSubMenu, ID_OPTIONS_AUTOSTART, MF_UNCHECKED);
			}

			CHANGEDSTATE = true;
		}
		break;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{

		case ID_START_STOP_BUTTON:
			if(SERVERISRUNNING)
			{
				stopServerCommand(0);
				CHANGEDSTATE = true;
				NEEDTOCHECK = FALSE;
				SERVERISONLINE = FALSE;	
			} 
			else 
			{
				startServerCommand();
				CHANGEDSTATE = true;
				NEEDTOCHECK = TRUE;	
			}				
			break;

		case ID_MINIMIZE_BUTTON:
			{
				Minimize(hwnd, "KA Lite", "Right click over the tray icon for more options.");
			}
			break;

		case ID_OPEN_IN_BROWSER:
			if((int)ShellExecute(hwnd,L"open",L"http://127.0.0.1:8008/",NULL,NULL,SW_MAXIMIZE) <= 36)
			{
				MessageBox(hwnd,L"Cannot open KA Lite in browser!", L"Opening KA Lite error", MB_OK | MB_ICONINFORMATION);
			}
			break;
		
		case ID_OPTIONS_RUNUSERLOGIN:
			{
				RUN_AT_USER_LOGIN_CHECK_STATE = GetMenuState(hOptionsSubMenu, ID_OPTIONS_RUNUSERLOGIN, MF_BYCOMMAND);

				if (RUN_AT_USER_LOGIN_CHECK_STATE == SW_SHOWNA)
				{
					CheckMenuItem(hOptionsSubMenu, ID_OPTIONS_RUNUSERLOGIN, MF_UNCHECKED);
					setConfigurationValue("RUN_AT_USER_LOGIN", "FALSE");
					setStartupShortcut(CONFIG_NO);
				} 
				else 
				{
					CheckMenuItem(hOptionsSubMenu, ID_OPTIONS_RUNUSERLOGIN, MF_CHECKED);
					setConfigurationValue("RUN_AT_USER_LOGIN", "TRUE");
					setStartupShortcut(CONFIG_YES);

					CheckMenuItem(hOptionsSubMenu, ID_OPTIONS_RUNSTARTUP, MF_UNCHECKED);
					setConfigurationValue("RUN_AT_STARTUP", "FALSE");
					setStartupTask(CONFIG_NO);
				}
			}
			break;

		case ID_OPTIONS_RUNSTARTUP:
			{
				RUN_AT_STARTUP_CHECK_STATE = GetMenuState(hOptionsSubMenu, ID_OPTIONS_RUNSTARTUP, MF_BYCOMMAND);

				if (RUN_AT_STARTUP_CHECK_STATE == SW_SHOWNA)
				{
					CheckMenuItem(hOptionsSubMenu, ID_OPTIONS_RUNSTARTUP, MF_UNCHECKED);
					setConfigurationValue("RUN_AT_STARTUP", "FALSE");
					setStartupTask(CONFIG_NO);
				} 
				else 
				{
					CheckMenuItem(hOptionsSubMenu, ID_OPTIONS_RUNSTARTUP, MF_CHECKED);
					setConfigurationValue("RUN_AT_STARTUP", "TRUE");
					setStartupTask(CONFIG_YES);

					CheckMenuItem(hOptionsSubMenu, ID_OPTIONS_RUNUSERLOGIN, MF_UNCHECKED);
					setConfigurationValue("RUN_AT_USER_LOGIN", "FALSE");
					setStartupShortcut(CONFIG_NO);
				}
			}
			break;

		case ID_OPTIONS_AUTOMINIMIZE:
			{
				AUTO_MINIMIZE_CHECK_STATE = GetMenuState(hOptionsSubMenu, ID_OPTIONS_AUTOMINIMIZE, MF_BYCOMMAND);

				if (AUTO_MINIMIZE_CHECK_STATE == SW_SHOWNA) 
				{
					CheckMenuItem(hOptionsSubMenu, ID_OPTIONS_AUTOMINIMIZE, MF_UNCHECKED);
					setConfigurationValue("AUTO_MINIMIZE", "FALSE");
				}
				else
				{
					CheckMenuItem(hOptionsSubMenu, ID_OPTIONS_AUTOMINIMIZE, MF_CHECKED);
					setConfigurationValue("AUTO_MINIMIZE", "TRUE");
				}
			}
			break;

		case ID_OPTIONS_AUTOSTART:
			{
				AUTO_START_SERVER_CHECK_STATE = GetMenuState(hOptionsSubMenu, ID_OPTIONS_AUTOSTART, MF_BYCOMMAND);

				if (AUTO_START_SERVER_CHECK_STATE == SW_SHOWNA)
				{
					CheckMenuItem(hOptionsSubMenu, ID_OPTIONS_AUTOSTART, MF_UNCHECKED);
					setConfigurationValue("AUTO_START_SERVER", "FALSE");
				} 
				else
				{
					CheckMenuItem(hOptionsSubMenu, ID_OPTIONS_AUTOSTART, MF_CHECKED);
					setConfigurationValue("AUTO_START_SERVER", "TRUE");
				}
			}
			break;

		case ID_HELP_ABOUT:
			{
				int ret = DialogBox(hINSTANCE, MAKEINTRESOURCE(IDD_DIALOG1), hwnd, reinterpret_cast<DLGPROC>(AboutDialogProc));
			}
			break;

		case ID_FILE_EXIT:
			{
				if(MessageBox(hwnd, L"Are you sure you want to exit from KA Lite?", L"KA Lite Exit", MB_YESNO | MB_ICONQUESTION) == IDYES)
				{
					stopServerCommand(1);
					PostQuitMessage(0);
				}
			}
			break;

		case ID_STUFF_GO:
			break;
		}
		break;

	case WM_TRAYICON:
		{
			switch(wParam)
			{
			case ID_TRAY_APP_ICON:
				// Its the ID_TRAY_APP_ICON. One app can have several tray icons
				break;
			}

			// React when the mouse button is released.
			if (lParam == WM_LBUTTONUP)
			{
				// The window is restored.
				Restore(hwnd);
			}
			else if (lParam == WM_RBUTTONDOWN) 
			{
				// Show the context menu.
				// Get current mouse position.
				POINT curPoint ;
				GetCursorPos(&curPoint);

				// Sets the main window in foreground.
				SetForegroundWindow(hwnd);        

				// TrackPopupMenu blocks the application until TrackPopupMenu returns.
				UINT clicked = TrackPopupMenu(
					hMenu,
					TPM_RETURNCMD | TPM_NONOTIFY, // Don't send WM_COMMAND messages about this window, instead return the identifier of the clicked menu item.
					curPoint.x,
					curPoint.y,
					0,
					hwnd,
					NULL
					);

				// Exit from KA Lite.
				if (clicked == ID_TRAY_EXIT_CONTEXT_MENU_ITEM)
				{				
					if(MessageBox(hwnd, L"Are you sure you want to exit from KA Lite?", L"KA Lite Exit", MB_YESNO | MB_ICONQUESTION) == IDYES)
					{
						stopServerCommand(1);
						PostQuitMessage(0);
					}
				} 

				// Starts and stops the server.
				if(clicked == ID_TRAY_START_STOP_CONTEXT_MENU_ITEM)
				{
					if(SERVERISRUNNING)
					{
						stopServerCommand(0);
						CHANGEDSTATE = true;
						NEEDTOCHECK = FALSE;
						SERVERISONLINE = FALSE;
					}
					else 
					{
						startServerCommand();
						CHANGEDSTATE = true;
						NEEDTOCHECK = TRUE;	
					}
				}

				// Open in browser.
				if(clicked == ID_TRAY_BROWSER_CONTEXT_MENU_ITEM)
				{
					if((int)ShellExecute(hwnd,L"open",L"http://127.0.0.1:8008/",NULL,NULL,SW_MAXIMIZE) <= 36)
					{
						MessageBox(hwnd,L"Cannot open KA Lite in browser!", L"Opening KA Lite error", MB_OK | MB_ICONINFORMATION);
					}
				}

				// Restore window.
				if(clicked == ID_TRAY_RESTORE_CONTEXT_MENU_ITEM)
				{
					Restore(hwnd);
				}
			}		  
		}
		break;

	case WM_NCHITTEST:
		{
			// This tests if you are not in the client area. (Out of the window).
			UINT uHitTest = DefWindowProc(hwnd, WM_NCHITTEST, wParam, lParam);
			if(uHitTest == HTCLIENT)
			{
				return HTCAPTION;
			}
			else
			{
				return uHitTest;
			}
		}		
		break;

	case WM_CLOSE:
		{
			Minimize(hwnd, "KA Lite", "Right click over the tray icon for more options.") ;
			return 0;
		}
		break;

	case WM_DESTROY:
		{
			if(SERVERISRUNNING)
			{
				stopServerCommand(1);
			}		
			PostQuitMessage(0);
		}
		break;

	default:
		{
			return DefWindowProc(hwnd, msg, wParam, lParam);
		}
	}
	return 0;
}



/*
*	This function checks if there is already an instance of KA Lite running.
*/
HWND GetRunningWindow()
{
	// Check if exists an application with the same class name as this application
	HWND hWnd = FindWindow(className, NULL);
	if (IsWindow(hWnd))
	{
		HWND hWndPopup = GetLastActivePopup(hWnd);
		if (IsWindow(hWndPopup))
		{
			hWnd = hWndPopup; // Previous instance exists
		}
	}
	else
	{
		hWnd = NULL; // Previous instance doesnt exist
		return hWnd;
	}
	return hWnd;
}



/*
*	This function checks if the server is online.
*/
DWORD WINAPI isServerOnline(LPVOID lpParam)
{
	UNREFERENCED_PARAMETER(lpParam);

	HINTERNET hSession = InternetOpen(L"Check KA Lite Server", 0,NULL, NULL, 0);
	HINTERNET hOpenUrl = InternetOpenUrl(hSession,L"http://127.0.0.1:8008/", NULL,0, INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_PRAGMA_NOCACHE, 1);

	if( hOpenUrl == NULL){

		InternetCloseHandle(hOpenUrl);
		InternetCloseHandle(hSession);

		return 1;
	}

	InternetCloseHandle(hOpenUrl);
	InternetCloseHandle(hSession);

	DWORD dwWaitResult = WaitForSingleObject(ghMutex, INFINITE); 
 
    switch (dwWaitResult) 
    {
        case WAIT_OBJECT_0: 
            __try 
			{ 
                SERVERISONLINE = TRUE;
            } 
			
			__finally
			{ 
				if (!ReleaseMutex(ghMutex)) 
                { 
                    MessageBox(NULL, L"Failed to release the Mutex", L"Error",MB_ICONEXCLAMATION | MB_OK);
                } 
            } 
            break; 

        case WAIT_ABANDONED:
			return 1; 
    }

	return 0;
}



/*
*	This function calls isServerOnline() in a thread.
*/
void isServerOnlineThread()
{
	DWORD dwThreadId;
	HANDLE hThreadArray[1];
	hThreadArray[0] = CreateThread( 
            NULL,                 
            0,                      
            isServerOnline,      
            NULL,         
            0,             
            &dwThreadId);
	CloseHandle(hThreadArray[0]);
}



/*
*	This is the main application window.
*/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow){

	// Checks for previous instance of our program
	HWND hOtherWnd = GetRunningWindow();

	// Allow only one instance of the program
	if (hOtherWnd) // hOtherWnd != NULL -> Previous instance exists
	{
		SetForegroundWindow(hOtherWnd); // Activate it &
		if (IsIconic(hOtherWnd))
		{
			ShowWindow(hOtherWnd, SW_RESTORE); // restore it
		}
		return FALSE; // Exit program
	}

	//Deprecated.
	//TCHAR windowTitle[30] = {};
	//setKALiteVersion(windowTitle,30);

	CHANGEDSTATE = FALSE;
	SERVERISRUNNING = FALSE;
	SERVERISONLINE = FALSE;
	NEEDTOCHECK = FALSE;

	ghMutex = CreateMutex(NULL, FALSE, NULL);

	if(ghMutex == NULL)
	{
		MessageBox(NULL, L"Failed to create MUTEX.", L"Error", MB_ICONEXCLAMATION | MB_OK);
		return 1;
	}

	WM_TASKBARCREATED = RegisterWindowMessageA("TaskbarCreated");

	hINSTANCE = hInstance;
	WNDCLASSEX wc = {0};
	MSG Msg;

	// Registering the window class.
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hINSTANCE,MAKEINTRESOURCE(IDI_ICON1));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)COLOR_APPWORKSPACE;
	wc.lpszClassName = className;
	wc.hIconSm = LoadIcon(hINSTANCE,MAKEINTRESOURCE(IDI_ICON1));
	wc.style=CS_HREDRAW|CS_VREDRAW;

	if(!RegisterClassEx(&wc)){
		MessageBox(NULL, L"Failed to register the window.", L"Error", MB_ICONEXCLAMATION | MB_OK);
		return 1;
	}

	loadConfigurations();
	if(serverIsRunning())
	{
		SERVERISRUNNING = TRUE;
	}

	// Creating the window.
	DWORD windowStyle = WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU| CS_NOCLOSE;
	hwnd = CreateWindowEx(NULL, className, L"KA Lite - 0.11.1", windowStyle , CW_USEDEFAULT, CW_USEDEFAULT, 275, 170, NULL,  NULL, hInstance, NULL);	

	if(hwnd == NULL){
		MessageBox(NULL, L"Failed to create the window.", L"Error",MB_ICONEXCLAMATION | MB_OK);
		return 1;
	}

	// Disables MINIMIZE and MAXIMIZE Buttons
	SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) & ~WS_MINIMIZEBOX);
	SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) & ~WS_MAXIMIZEBOX);

	// Initialize the NOTIFYICONDATA structure.
	InitNotifyIconData(hwnd, hINSTANCE);	

	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);	

	if(AUTO_MINIMIZE_CHECK_STATE == SW_SHOWNA)
	{
		if(SERVERISRUNNING)
		{
			Minimize(hwnd, "KA Lite is running", "The server should now be accessible locally at: http://127.0.0.1:8008/ or you can press \"Open KA Lite button\"");
		}
		else 
		{
			Minimize(hwnd, "KA Lite", "Right click over the tray icon for more options.");
		}
	}

	// The Message Loop.
	while(GetMessage(&Msg, NULL, 0 , 0) > 0){
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}

	// After exiting KA Lite, the icon is removed from system tray.
	if(!IsWindowVisible(hwnd))
	{
		destroyTrayIcon();
	}

	return Msg.wParam;
}

