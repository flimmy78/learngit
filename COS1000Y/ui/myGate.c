/*************************************************************************
 *	
 *
**************************************************************************/
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include "utility.h"
//#include "et850_data.h"
//#include "device.h"
#include "fotawin.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
static struct in_addr stInAddr;

#define SAVE_TIMER_ID				601
static int save_timer_id = 0;
static char message[32];

static HWND		hMainWnd = NULL;

static HDC		hBgMemDC = NULL;
static HDC		hfocusMemDC = NULL;
static HBITMAP	hBgBitmap, hOldBgBitmap;
static HBITMAP	hfocusBitmap, hOldfocusBitmap;
static char	keynum[15];
static char x = 0;
static char y = 0;
static s32_t myIP,myMask,myGate,myType;
static u8_t point;			// '.'数量
static u8_t segment[4];		// 段长，上限3字符


static char		character[] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K',
														  'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
														  'W', 'X', 'Y' ,'Z', '.', '*', '_', '\\','/', ' ', ' ',
														'0', '1', '2','3', '4', '5', '6', '7', '8', '9', ' ' };
 

static LRESULT CALLBACK MainProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
static int OnCreate(HWND hWnd, WPARAM wParam, LPARAM lParam);
static int OnDestroy(HWND hWnd, WPARAM wParam, LPARAM lParam);
static int OnPaint(HWND hWnd, WPARAM wParam, LPARAM lParam);
static int OnKeyDown(HWND hWnd, WPARAM wParam, LPARAM lParam);
static int OnTimer(HWND hWnd, WPARAM wParam, LPARAM lParam);

static void refresh_keytable(HWND hWnd, char old_x, char old_y, char pos_x, char pos_y);

int myGatewin_create(HWND hWnd)
{
	WNDCLASS	wndclass;

	wndclass.style          = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc    = (WNDPROC)MainProc;
	wndclass.cbClsExtra     = 0;
	wndclass.cbWndExtra     = 0;
	wndclass.hInstance      = 0;
	wndclass.hIcon          = 0;
	wndclass.hCursor        = 0;
	wndclass.hbrBackground  = NULL;
	wndclass.lpszMenuName   = NULL;
	wndclass.lpszClassName  = (LPCSTR)"MYGATE";
	RegisterClass(&wndclass);
	hMainWnd = CreateWindowEx(0L,
							  (LPCSTR)"MYGATE",
							  (LPCSTR)"",
							  WS_CHILD | WS_VISIBLE,
							  0,
							  0,
							  WINDOW_WIDTH,
							  WINDOW_HEIGHT,
							  hWnd,
							  NULL,
							  NULL,
							  NULL);

 SetFocus(hMainWnd);
	return 0;
}

static LRESULT CALLBACK MainProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch (iMsg) {
		case WM_CREATE:
			OnCreate(hWnd, wParam, lParam);
			break;
		case WM_PAINT:
			OnPaint(hWnd, wParam, lParam);
			break;
		case WM_KEYDOWN:
			OnKeyDown(hWnd, wParam, lParam);
			break;
		case WM_TIMER:
			OnTimer(hWnd, wParam, lParam);
			break;
		case WM_DESTROY:
			OnDestroy(hWnd, wParam, lParam);
			break;
		default:
			return DefWindowProc(hWnd, iMsg, wParam, lParam);
	}
	return (0);
}

static int OnCreate(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	HDC			hDC;
	RECT		rect;
	HBRUSH	hBrush;
	
	char *ipaddr = NULL;
	
	hDC = GetDC(hWnd);
	hBrush = CreateSolidBrush(RGB(255, 255, 255));
	hBgMemDC = CreateCompatibleDC(hDC);													//创建兼容HDC
	hBgBitmap = CreateCompatibleBitmap(hBgMemDC, WINDOW_WIDTH, WINDOW_HEIGHT);						    //创建兼容位图
	hOldBgBitmap = SelectObject(hBgMemDC, hBgBitmap);
	
	rect.left = 0;rect.right = WINDOW_WIDTH;rect.top = 0;rect.bottom = WINDOW_HEIGHT;
	FillRect(hBgMemDC, &rect, hBrush);                                           //绘图之前进行位图清除
	GdDrawImageFromFile(hBgMemDC->psd, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, "/bmp/fota/keyboardwin.bmp", 0);     //显示	网点号白色字样图

	hfocusMemDC = CreateCompatibleDC(hDC);
	hfocusBitmap = CreateCompatibleBitmap(hfocusMemDC, WINDOW_WIDTH, WINDOW_HEIGHT);
	hOldfocusBitmap = SelectObject(hfocusMemDC, hfocusBitmap);	
	FillRect(hfocusMemDC, &rect, hBrush);
	GdDrawImageFromFile(hfocusMemDC->psd, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, "/bmp/fota/keyboard1win.bmp", 0);     //	网点号黄色字样图

	DeleteObject(hBrush);
	ReleaseDC(hWnd, hDC);
	message[0] = '\0';
	keynum[0] = '\0';
	net1_interface_get(&myType,&myIP,&myMask,&myGate);
	stInAddr.s_addr = myGate;
	ipaddr = inet_ntoa(stInAddr);
	strcpy(keynum,ipaddr);
	printf("%s\n",keynum);

	u8_t i=0;
	point = 0;
	memset(segment,0,sizeof(segment));
	do
	{
		if(keynum[i++] == '.')
		{
			
			if(point == 3)
			{
				printf("point is out of range 3!\n");
				break;
			}
			point++;

		}
		else
		{
			segment[point]++;
		}
	}while(keynum[i]);	
	return 0;
}

static int OnDestroy(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	if (save_timer_id)
			KillTimer(hWnd, SAVE_TIMER_ID);
		save_timer_id = 0;

	SelectObject(hBgMemDC, hOldBgBitmap);
	DeleteObject(hBgBitmap);
	DeleteDC(hBgMemDC);
	SelectObject(hfocusMemDC, hOldfocusBitmap);
	DeleteObject(hfocusBitmap);
	DeleteDC(hfocusMemDC);

	hMainWnd = NULL;

	return 0;
}

static int OnTimer(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	switch(wParam) {
		case SAVE_TIMER_ID:
				if (save_timer_id)
					KillTimer(hWnd, SAVE_TIMER_ID);
				save_timer_id = 0;
				message[0] = '\0';
				InvalidateRect(hWnd, NULL, FALSE);
				break;
		default:
				break;
	}
	return 0;
}

static int OnPaint(HWND hWnd, WPARAM wParam, LPARAM lParam) 
{
	HDC				hDC, hMemDC;
	HBITMAP	hBitmap, hOldBitmap;
	PAINTSTRUCT		ps;
	HFONT			hOldFont;
	s8_t            j=0;
	
	hDC = BeginPaint(hWnd, &ps);
	hMemDC = CreateCompatibleDC(hDC);
	hBitmap = CreateCompatibleBitmap(hMemDC, WINDOW_WIDTH, WINDOW_HEIGHT);
	hOldBitmap = SelectObject(hMemDC, hBitmap);
	BitBlt(hMemDC, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, hBgMemDC, 0, 0, SRCCOPY);
	hOldFont = SelectObject(hMemDC, (HFONT)GetFont32Handle());
	SetBkColor(hMemDC, RGB(0, 255, 0));
	SetBkMode(hMemDC, TRANSPARENT);
	SetTextColor(hMemDC, RGB(255, 0, 0));			
	if(strlen(message)!=0)
		TextOut(hMemDC, 165, 225, message,strlen(message));
	printf("message = %d\n",strlen(message));
	SetTextColor(hMemDC, RGB(255, 255, 255));
	TextOut(hMemDC, 77, 240, "My Gate Addr", -1);
	j = strlen(keynum);
	
	
	TextOut(hMemDC, 77, 283, keynum, j);		
	TextOut(hMemDC, 155, 7, "机器网关输入", -1);

	
	BitBlt(hMemDC, 65+30*x, 47+42*y, 30, 42,hfocusMemDC, 65+30*x, 47+42*y, SRCCOPY);
	BitBlt(hDC, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, hMemDC, 0, 0, SRCCOPY);
	SelectObject(hDC, hOldFont);
	
	SelectObject(hMemDC, hOldBitmap);
	DeleteObject(hBitmap);
	DeleteDC(hMemDC);	
	EndPaint(hWnd, &ps);
	
	return 0;
}

static int OnKeyDown(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	int ret = 0;
	u8_t len = 0; 
	len = strlen(keynum);
	switch(wParam) {
		
		case VK_F1:    //right
		  if(x == 10) {
		  	x = 0;
				refresh_keytable(hWnd, 10, y, x, y);
		  }
		  else {
		  	x++;
				refresh_keytable(hWnd, x-1, y, x, y);
			}
			break;
		case VK_F2:     //left
		if(x == 0) {
			x = 10;
			refresh_keytable(hWnd, 0, y, x, y);
		}
		else {
			x--;
			refresh_keytable(hWnd, x+1, y, x, y);
		}
		break;
		case VK_F3:       //up
	    if(y == 0) {
	   		y = 3;
				refresh_keytable(hWnd, x, 0, x, y);
	   	}
	   	else {
	    	y--;
				refresh_keytable(hWnd, x, y+1, x, y);
			}
			break;
		case VK_F4:     //down
			if(y == 3) {
				y = 0;
				refresh_keytable(hWnd, x, 3, x, y);
			}
			else {
				y++;
				refresh_keytable(hWnd, x, y-1, x, y);
			}
			break;
		case VK_F5:    //clear
			if (!len)
				break;
			if (len) 
			{
				if(keynum[len - 1 ] == '.')
				{
					point--;
				}
				else
				{
					segment[point]--;
				}
				
				keynum[len - 1 ] =	'\0';
				len--;
			}
			InvalidateRect(hWnd, NULL, FALSE);
			break;
		case VK_F6: 	//yes
			// 字符合法性判断
			if(character[y*11+x] == '.')
			{
				// '.'号上限3个
				if(point == 3)
					break;
				
				point++;
			}
			else if(character[y*11+x] >= '0' && character[y*11+x] <= '9')
			{
				// 段长上限3个字符
				if(segment[point] == 3)
					break;

				segment[point]++;
			}
			else
			{
				// 其余字符非法
				break;
			}
						
			keynum[len] = character[y*11+x];
			keynum[len+1] = '\0';
			InvalidateRect(hWnd, NULL, FALSE);
			break;
		case VK_F7: 	  //save
			inet_aton(keynum,&stInAddr);
			myGate = stInAddr.s_addr;
			ret  = net1_interface_save(myType,myIP,myMask,myGate);
			printf("%x\n",myGate);

			
			if(ret==0)
				strcpy(message, "保存成功");
			else 
				strcpy(message, "保存失败");
			 
			printf("message = %s\n", message);   //debug
			if (save_timer_id)
				KillTimer(hWnd, save_timer_id);	
			
			InvalidateRect(hWnd, NULL, FALSE);
			save_timer_id = SetTimer(hWnd, SAVE_TIMER_ID, 1000, NULL);
			break;
		case VK_F8:  	
			DestroyWindow(hWnd);
			myPortwin_create(parasetwin_handle_get());
			break;
		default:	
			spi_keyalert();
			DestroyWindow(hWnd);
			break;
	}
	return 0;
}


static void refresh_keytable(HWND hWnd, char old_x, char old_y, char pos_x, char pos_y)
{
	HDC				hDC;
	if ((pos_x == old_x) && (pos_y == old_y))
		return;
	hDC = GetDC(hWnd);

		BitBlt(hDC, 65+30*pos_x, 47+42*pos_y, 30, 42,hfocusMemDC, 65+30*pos_x, 47+42*pos_y, SRCCOPY);
		BitBlt(hDC, 65+30*old_x, 47+42*old_y, 30, 42,hBgMemDC, 65+30*old_x, 47+42*old_y, SRCCOPY);
	
	ReleaseDC(hWnd, hDC);
}

