#ifndef fle_win32_framework
#define fle_win32_framework

#include <Windows.h>
#include <stdio.h>
#include <map>

using namespace std;

#define ID_TRAY_APP_ICON 5000
#define WM_TRAYICON ( WM_USER + 1 )

//GLOBAL and structs

static UINT CURRENT_VALID_ID = WM_TRAYICON + 1;


// GLOBAL functions
UINT getAvailableID();

UINT getAvailableID()
{
	CURRENT_VALID_ID++;
	return CURRENT_VALID_ID;
}


// CLASS
class fle_BaseWindow;
class fle_TrayMenuItem
{
	private:
		HMENU hMenu;
		UINT id;
		TCHAR * title;
		void (*f_action)(void);
		UINT menuType;
	public:
		fle_TrayMenuItem(TCHAR*, void (*action_function)(void));
		//void setActionFunction(void (*action_function)(void));
		void action(void);
		UINT getID(void);
		TCHAR* getTitle(void);
		void addSubMenu(fle_TrayMenuItem*);
		HMENU getMenu(void);
		void setSubMenu(void);
		UINT getMenuType(void);
};

fle_TrayMenuItem::fle_TrayMenuItem(TCHAR * m_title, void (*action_function)(void))
{
	hMenu = CreatePopupMenu();
	id = getAvailableID();
	title = new TCHAR[sizeof(m_title)];
	lstrcpy(title, m_title);
	f_action = action_function;
	menuType = MF_STRING;
}

void fle_TrayMenuItem::action(void)
{
	if(f_action != NULL)
	{
		f_action();
	}
}

UINT fle_TrayMenuItem::getID()
{
	return this->id;
}

TCHAR* fle_TrayMenuItem::getTitle()
{
	return this->title;
}

HMENU fle_TrayMenuItem::getMenu()
{
	return this->hMenu;
}

void fle_TrayMenuItem::setSubMenu()
{
	this->menuType = MF_STRING | MF_POPUP;
}

UINT fle_TrayMenuItem::getMenuType()
{
	return this->menuType;
}







class fle_BaseWindow
{
	private:
		HINSTANCE * p_hInstance;
		WNDCLASSEX * p_wc;
		HWND hwnd;

		static void (*main_loop_function)(void);
		static HMENU hMenu;
		static std::map<UINT, fle_TrayMenuItem*> tray_children_map;
		static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
	public:
		fle_BaseWindow(HINSTANCE*, int, int, TCHAR*, TCHAR*);
		void show(void);
		void test(void);
		HWND& getWindowReference(void);
		HINSTANCE* getInstanceReference(void);
		static void processTrayMenu(WPARAM, LPARAM, HWND*, HMENU*);
		static HMENU& getMainMenu(void);
		static void addTrayMenu(fle_TrayMenuItem*);
		static std::map<UINT, fle_TrayMenuItem*>& getTrayMap(void);
		static void setMainLoopFunction(void (*main_loop_function)(void));

};

std::map<UINT, fle_TrayMenuItem*> &fle_BaseWindow::getTrayMap()
{
	return fle_BaseWindow::tray_children_map;
}

fle_BaseWindow::fle_BaseWindow(HINSTANCE * hInstance, int WIDTH, int HEIGHT, TCHAR * CLASS_NAME, TCHAR * TITLE)
{
	p_hInstance = hInstance;
	WNDCLASSEX wc = { 0 };
	hMenu = CreatePopupMenu();

	// Registering the window class.
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpfnWndProc = &fle_BaseWindow::WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = *p_hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH) COLOR_APPWORKSPACE;
	wc.lpszClassName = CLASS_NAME;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	//wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON));
	//wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wc.hIcon = NULL;
	wc.hIconSm = NULL;

	p_wc = &wc;

	if(!RegisterClassEx(&wc)){
		MessageBox(NULL, L"Failed to register the window.", L"Error", MB_ICONEXCLAMATION | MB_OK);
	}

	// Creating the window.
	DWORD windowStyle = WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU;
	this -> hwnd = CreateWindowEx(NULL, CLASS_NAME, TITLE, windowStyle, CW_USEDEFAULT, CW_USEDEFAULT, WIDTH, HEIGHT, NULL,  NULL, *p_hInstance, NULL);	

	if(hwnd == NULL){
		MessageBox(NULL, L"Failed to create the window.", L"Error", MB_ICONEXCLAMATION | MB_OK);
	}

	tray_children_map = map<UINT, fle_TrayMenuItem*>();
}

HWND& fle_BaseWindow::getWindowReference()
{
	return this->hwnd;
}

HINSTANCE * fle_BaseWindow::getInstanceReference()
{
	return this->p_hInstance;
}

LRESULT CALLBACK fle_BaseWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if(fle_BaseWindow::main_loop_function != NULL)
	{
		fle_BaseWindow::main_loop_function();
	}

	switch(msg)
		{
			case WM_PAINT:
				{
			
				}
				break;

			case WM_CREATE:
				{
			
				}
				break;

			case WM_COMMAND:
				{
					switch(LOWORD(wParam))
						{
							//case ID_MINIMIZE_BUTTON:
							//case ID_OPEN_IN_BROWSER:
							//case ID_OPTIONS_RUNUSERLOGIN:
							//case ID_OPTIONS_RUNSTARTUP:
							//case ID_OPTIONS_AUTOMINIMIZE:
							//case ID_OPTIONS_AUTOSTART:
							//case ID_HELP_ABOUT:
							//case ID_FILE_EXIT:
							//case ID_STUFF_GO:
						}
				}
				break;

			case WM_TRAYICON:
				{
					processTrayMenu(wParam, lParam, &hwnd, &getMainMenu());
				}
				break;

			case WM_NCHITTEST:
				{
				}
				break;

			case WM_CLOSE:
				{
					PostQuitMessage(0);
				}
				break;

			case WM_DESTROY:
				{

				}
				break;

			default:
				{
					return DefWindowProc(hwnd, msg, wParam, lParam);
				}
		}

	return 0;
};

void fle_BaseWindow::processTrayMenu(WPARAM wParam, LPARAM lParam, HWND * hwnd, HMENU * hMenu)
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
		static const TCHAR * str = TEXT("CLICK ESQUERDO\n");
		OutputDebugString(str);		
	}
	else if (lParam == WM_RBUTTONDOWN) 
	{
		// Show the context menu.
		// Get current mouse position.
		POINT curPoint ;
		GetCursorPos(&curPoint);

		// Sets the main window in foreground.
		SetForegroundWindow(*hwnd);        

		// TrackPopupMenu blocks the application until TrackPopupMenu returns.
		UINT clicked = TrackPopupMenu(
			*hMenu,
			TPM_RETURNCMD | TPM_NONOTIFY, // Don't send WM_COMMAND messages about this window, instead return the identifier of the clicked menu item.
			curPoint.x,
			curPoint.y,
			0,
			*hwnd,
			NULL
			);



		for (std::map<UINT,fle_TrayMenuItem*>::iterator it=tray_children_map.begin(); it!=tray_children_map.end(); ++it) 
		{
			if(clicked == it->first)
			{
				(it->second)->action();
			}
		}

	}		  
};

HMENU& fle_BaseWindow::getMainMenu()
{
	return fle_BaseWindow::hMenu;
};

void fle_BaseWindow::addTrayMenu(fle_TrayMenuItem * menu)
{
	if(menu != NULL && menu->getID() != NULL && menu->getTitle() != NULL)
	{
		AppendMenu(getMainMenu(), menu->getMenuType(), (UINT)menu->getMenu(), menu->getTitle());
		tray_children_map.insert(std::pair<UINT, fle_TrayMenuItem*>((UINT)menu->getMenu(), menu));
	}
}

void fle_BaseWindow::test()
{
	static const TCHAR * str = TEXT("WELCOME FLE FRAMEWORK\n");
    OutputDebugString(str);
};

void fle_BaseWindow::setMainLoopFunction(void (*target_function)(void))
{
	fle_BaseWindow::main_loop_function = target_function;
}

HMENU fle_BaseWindow::hMenu;
std::map<UINT, fle_TrayMenuItem*> fle_BaseWindow::tray_children_map;
void (*fle_BaseWindow::main_loop_function)(void);



void fle_TrayMenuItem::addSubMenu(fle_TrayMenuItem * menu)
{
	AppendMenu(hMenu, menu->getMenuType(), (UINT)menu->getMenu(), menu->getTitle());
	fle_BaseWindow::getTrayMap().insert(std::pair<UINT, fle_TrayMenuItem*>((UINT)menu->getMenu(), menu));
}



class fle_TrayWindow : public fle_BaseWindow
{
	private:
		NOTIFYICONDATA *notifyIconData;
		HINSTANCE * p_hInstance;
		HMENU hMenu;
	public:
		fle_TrayWindow(HINSTANCE*);
		NOTIFYICONDATA* getNotifyIconDataStructure(void);
		HINSTANCE* getInstanceReference(void);
		void setTrayIcon(UINT);
		void show(void);
		void addMenu(fle_TrayMenuItem *);
		void setStatusFunction(void (*target_function)(void));
};

fle_TrayWindow::fle_TrayWindow(HINSTANCE * hInstance) : fle_BaseWindow(hInstance, 0, 0, L"DEFAULT", L"DEFAULT")
{
	p_hInstance = hInstance;
	hMenu = CreatePopupMenu();

	// Allocate memory for the structure.
	//memset(notifyIconData, 0, sizeof(NOTIFYICONDATA));
	notifyIconData = (NOTIFYICONDATA*)malloc(sizeof(NOTIFYICONDATA));
	notifyIconData->cbSize = sizeof(NOTIFYICONDATA);

	// Bind the NOTIFYICONDATA structure to our global hwnd ( handle to main window ).
	notifyIconData->hWnd = fle_BaseWindow::getWindowReference();

	// Set the NOTIFYICONDATA ID. HWND and uID form a unique identifier for each item in system tray.
	notifyIconData->uID = ID_TRAY_APP_ICON;

	// Set up flags.
	// 1 - Guarantees that the hIcon member will be a valid icon.
	// 2 - When someone clicks in the system tray icon, we want a WM_ type message to be sent to our WNDPROC.
	// 3 -
	// 4 -
	// 5 - // Show tooltip.
	notifyIconData->uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO | NIF_SHOWTIP;

	// This message must be handled in hwnd's window procedure.
	notifyIconData->uCallbackMessage = WM_TRAYICON;

	// Set the tooltip text.
	lstrcpy(notifyIconData->szTip, L"KA Lite");

	// Time to display the tooltip.
	notifyIconData->uTimeout = 100;

	// Type of tooltip (balloon).
	notifyIconData->dwInfoFlags = NIIF_INFO;

	// Copy text to the structure.
	lstrcpy(notifyIconData->szInfo, L"");
	lstrcpy(notifyIconData->szInfoTitle, L"");
}

NOTIFYICONDATA* fle_TrayWindow::getNotifyIconDataStructure()
{
	return notifyIconData;
}

HINSTANCE* fle_TrayWindow::getInstanceReference()
{
	return p_hInstance;
}

void fle_TrayWindow::setTrayIcon(UINT icon_id)
{
	// Load the icon as a resource.
	fle_TrayWindow::getNotifyIconDataStructure()->hIcon = LoadIcon(*getInstanceReference(), MAKEINTRESOURCE(icon_id));
}

void fle_TrayWindow::addMenu(fle_TrayMenuItem * menu)
{
	fle_BaseWindow::addTrayMenu(menu);
}

void fle_TrayWindow::setStatusFunction(void (*target_function)(void))
{
	fle_BaseWindow::setMainLoopFunction(target_function);
}

void fle_TrayWindow::show()
{
	Shell_NotifyIcon(NIM_ADD, notifyIconData);

	MSG Msg;

	while(GetMessage(&Msg, NULL, 0 , 0) > 0){
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}
	
	fle_BaseWindow::test();
}









class fle_Window : public fle_BaseWindow
{
	private:
		HINSTANCE * p_hInstance;
	public:
		fle_Window(HINSTANCE*, int, int, TCHAR*, TCHAR*);
		void show(void);
};

fle_Window::fle_Window(HINSTANCE * hInstance, int WIDTH, int HEIGHT, TCHAR * CLASS_NAME, TCHAR * TITLE) : fle_BaseWindow(hInstance, WIDTH, HEIGHT, CLASS_NAME, TITLE)
{
	p_hInstance = hInstance;
}

void fle_Window::show()
{
	MSG Msg;

	ShowWindow(fle_BaseWindow::getWindowReference(), SW_SHOW);

	while(GetMessage(&Msg, NULL, 0 , 0) > 0){
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}
	
	fle_BaseWindow::test();
}

#endif