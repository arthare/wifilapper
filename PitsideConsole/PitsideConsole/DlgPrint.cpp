#pragma once
#include "Stdafx.h"
#include "resource.h"
#include "DlgPrint.h"
#include "PitsideConsole.h"
#include <Winspool.h>
//#include <CommDlg.h>
#include <fstream>


//   variables  
TCHAR szFileName[500]= L"\0";
OPENFILENAME ofn;
RECT rect;
HBITMAP hBitmap;
BITMAP bitmap;
int bxWidth, bxHeight, flag=0;
HDC hdc,hdcMem;
HMENU menu;
HPALETTE hpal;
int cxsize = 0, cxpage = 0;
int cysize = 0, cypage = 0;

//  Declare  procedures  
//LRESULT CALLBACK WindowProcedure (HWND, UINT, WPARAM, LPARAM);
HDC GetPrinterDC (HWND Hwnd);
BOOL OpenFileDialog(HWND hwnd, LPTSTR pFileName ,LPTSTR pTitleName);
BOOL SaveFileDialog(HWND hwnd, LPTSTR pFileName ,LPTSTR pTitleName);
void InitializeDialog(HWND hwnd);
PBITMAPINFO CreateBitmapInfoStruct(HWND hwnd, HBITMAP hBmp);
void SaveBMPFile(HWND hwnd, LPTSTR pszFile, PBITMAPINFO pbi, HBITMAP hBMP, HDC hDC);
BITMAP ConvertToGrayscale(BITMAP source, HDC hDC);




/*Start of Program Entry point*/
/*
int WINAPI WinMain (HINSTANCE hThisInstance,
                    HINSTANCE hPrevInstance,
                    LPSTR lpszArgument,
                    int nCmdShow)
{
    HWND hMenu;
	HWND hwnd;               // This is the handle for our window 
    MSG messages;            // Here messages to the application are saved 
    WNDCLASSEX wincl;        // Data structure for the windowclass 

    // The Window structure 
    wincl.hInstance = hThisInstance;
    wincl.lpszClassName = L"szClassName";
    wincl.lpfnWndProc = WindowProcedure;      // This function is called by windows 
    wincl.style = CS_DBLCLKS;                 // Catch double-clicks 
    wincl.cbSize = sizeof (WNDCLASSEX);

    // Use default icon and mouse-pointer 
    wincl.hIcon = LoadIcon (GetModuleHandle(NULL), MAKEINTRESOURCE(ICO1));
    wincl.hIconSm = LoadIcon (GetModuleHandle(NULL), MAKEINTRESOURCE(ICO1));
    wincl.hCursor = LoadCursor (NULL, IDC_ARROW);
    wincl.lpszMenuName = MAKEINTRESOURCE(hMenu);                 // No menu 
    wincl.cbClsExtra = 0;                      // No extra bytes after the window class 
    wincl.cbWndExtra = 0;                      // structure or the window instance 
    // Use Windows's default colour as the background of the window 
    wincl.hbrBackground = (HBRUSH) COLOR_BACKGROUND;

    // Register the window class, and if it fails quit the program 
    if (!RegisterClassEx (&wincl))
        return 0;

    // The class is registered, let's create the program
    hwnd = CreateWindowEx (
               0,                   // Extended possibilites for variation 
               L"szClassName",         // Classname 
               L"szClassName",       // Title Text 
               WS_OVERLAPPEDWINDOW, // default window 
               CW_USEDEFAULT,       // Windows decides the position 
               CW_USEDEFAULT,       // where the window ends up on the screen 
               544,                 // The programs width 
               375,                 // and height in pixels 
               HWND_DESKTOP,        // The window is a child-window to desktop 
               NULL,                // menu 
               hThisInstance,       // Program Instance handler 
               NULL                 // No Window Creation data 
           );

    // Make the window visible on the screen 
    ShowWindow (hwnd, SW_MAXIMIZE);

    // Run the message loop. It will run until GetMessage() returns 0 
    while (GetMessage (&messages, NULL, 0, 0))
    {
        // Translate virtual-key messages into character messages 
        TranslateMessage(&messages);
        // Send message to WindowProcedure 
        DispatchMessage(&messages);
    }

    // The program return-value is 0 - The value that PostQuitMessage() gave 
    return messages.wParam;
}
*/

/*  This function is called by the Windows function DispatchMessage()  */
/*
LRESULT CALLBACK WindowProcedure (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{

    PAINTSTRUCT ps;
	

    switch (message)                  // handle the messages 
    {
    case WM_CREATE:
        InitializeDialog(hwnd);
        menu = GetMenu(hwnd);
        return 0;

    case WM_QUERYNEWPALETTE:
        if (!hpal)
            return FALSE;
        hdc = GetDC(hwnd);
        SelectPalette (hdc, hpal, FALSE);
        RealizePalette (hdc);
        InvalidateRect(hwnd,NULL,TRUE);
        ReleaseDC(hwnd,hdc);
        return TRUE;

    case WM_PALETTECHANGED:
        if (!hpal || (HWND)wParam == hwnd)
            break;
        hdc = GetDC(hwnd);
        SelectPalette (hdc, hpal, FALSE);
        RealizePalette (hdc);
        UpdateColors(hdc);
        ReleaseDC(hwnd,hdc);
        break;




    case WM_COMMAND:
        switch LOWORD(wParam)
        {

        case IDM_OPEN_BM:
        {
            
            OpenFileDialog(hwnd, szFileName, L"Open a Bitmap File.");

            if(szFileName)
            {
                ZeroMemory(&hBitmap, sizeof(HBITMAP));
                hBitmap = (HBITMAP)LoadImage(NULL,szFileName,IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION|LR_DEFAULTSIZE|LR_LOADFROMFILE|LR_VGACOLOR);
                if(hBitmap)
                {
                    EnableMenuItem(menu, IDM_SAVE_BM, MF_ENABLED);
                    EnableMenuItem(menu, IDM_PRINT_BM, MF_ENABLED);

                    cxpage = GetDeviceCaps (hdc, HORZRES);
                    cypage = GetDeviceCaps (hdc, VERTRES);
                    GetObject(hBitmap,sizeof(BITMAP),&bitmap);
                    bxWidth = bitmap.bmWidth;
                    bxHeight = bitmap.bmHeight;

                    rect.left = 0;
                    rect.top =0;
                    rect.right = (long)&cxpage;
                    rect.bottom = (long)&cypage;


                    InvalidateRect(hwnd,&rect,true);
                }
            }
            return 0;
		}

            case IDM_PRINT_BM:
            {
                DOCINFO di= { sizeof (DOCINFO), TEXT ("Printing Picture...")};

                HDC prn;

                

                prn = GetPrinterDC(hwnd);
                cxpage = GetDeviceCaps (prn, HORZRES);
                cypage = GetDeviceCaps (prn, VERTRES);
                hdcMem = CreateCompatibleDC(prn);
                HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hBitmap);

                StartDoc (prn, &di);
                StartPage (prn) ;
                SetMapMode (prn, MM_ISOTROPIC);
                SetWindowExtEx(prn, cxpage,cypage, NULL);
                SetViewportExtEx(prn, cxpage, cypage,NULL);

                SetViewportOrgEx(prn, 0, 0, NULL);
                StretchBlt(prn, 0, 0, cxpage, cypage, hdcMem, 0, 0,bxWidth,bxHeight, SRCCOPY);
                EndPage (prn);
                EndDoc(prn);
                DeleteDC(prn);
                SelectObject(hdcMem, hbmOld);
                DeleteDC(hdcMem);

                return 0;
            }

            case IDM_SAVE_BM:
            {
                BOOL result = SaveFileDialog(hwnd,szFileName,L"Save a Bitmap.");
                if(result != false)
                {
                    PBITMAPINFO pbi = CreateBitmapInfoStruct(hwnd, hBitmap);
                    hdc= GetDC(hwnd);
                    SaveBMPFile(hwnd, szFileName, pbi, hBitmap, hdc);
                }
                return 0;
            }

//            case IDM_EXIT:
//            {
//                PostQuitMessage(0);
//                return 0;
//            }
       



        break;
        }
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);

        hdcMem = CreateCompatibleDC(hdc);

        SelectObject(hdcMem, hBitmap);


        cxpage = GetDeviceCaps (hdc, HORZRES);
        cypage = GetDeviceCaps (hdc, VERTRES);

        SetMapMode (hdc, MM_ISOTROPIC);
        SetWindowExtEx(hdc, cxpage,cypage, NULL);
        SetViewportExtEx(hdc, cxsize, cysize,NULL);

        SetViewportOrgEx(hdc, 0, 0, NULL);

        SetStretchBltMode(hdc,COLORONCOLOR);

        StretchBlt(hdc, 0, 0, bxWidth, bxHeight, hdcMem, 0, 0,bxWidth,bxHeight, SRCCOPY);



        DeleteDC(hdcMem);





        EndPaint(hwnd, &ps);
        DeleteDC(hdcMem);
        return 0;

    case  WM_SIZE:

        cxsize = LOWORD(lParam);
        cysize = HIWORD(lParam);
        rect.right = cxsize;
        rect.bottom = cysize;

        InvalidateRect(hwnd, &rect, true);
        return 0;




    case WM_DESTROY:
        PostQuitMessage (0);       // send a WM_QUIT to the message queue 
        break;
    default:                      // for messages that we don't deal with 
        return DefWindowProc (hwnd, message, wParam, lParam);

    }
    return 0;

}
*/


HDC GetPrinterDC (HWND Hwnd)
{

// Initialize a PRINTDLG structure's size and set the PD_RETURNDC flag set the Owner flag to hwnd.
// The PD_RETURNDC flag tells the dialog to return a printer device context.
    HDC hdc;
    PRINTDLG pd = {0};
    pd.lStructSize = sizeof( pd );
    pd.hwndOwner = Hwnd;
    pd.Flags = PD_RETURNDC;

// Retrieves the printer DC
    PrintDlg(&pd);
    hdc =pd.hDC;
    return hdc ;


}

BOOL OpenFileDialog(HWND hwnd, LPTSTR pFileName ,LPTSTR pTitleName)

{
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hInstance = GetModuleHandle(NULL);
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 0;

    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = pFileName;
    ofn.lpstrFileTitle = NULL;
    ofn.lpstrTitle = pTitleName;
    ofn.Flags = OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST;
    ofn.lpstrFilter = TEXT("Bitmap Files (*.bmp)\0*.bmp\0\0");



    return GetOpenFileName(&ofn);
}

BOOL SaveFileDialog(HWND hwnd, LPTSTR pFileName ,LPTSTR pTitleName)

{
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hInstance = GetModuleHandle(NULL);
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 0;

    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = pFileName;
    ofn.lpstrFileTitle = NULL;
    ofn.lpstrTitle = pTitleName;
    ofn.Flags = OFN_EXPLORER|OFN_OVERWRITEPROMPT;
    ofn.lpstrFilter = TEXT("Bitmap Files (*.bmp)\0*.bmp\0\0");



    return GetSaveFileName(&ofn);
}

void InitializeDialog(HWND hwnd)
{
    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hwnd;
    ofn.hInstance = NULL;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 0;
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = 500;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = MAX_PATH;
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrTitle = NULL;
    ofn.Flags = 0;
    ofn.nFileOffset = 0;
    ofn.nFileExtension = 0;
    ofn.lpstrDefExt = NULL;
    ofn.lCustData = 0L;
    ofn.lpfnHook = NULL;
    ofn.lpTemplateName = NULL;
}

/*
void SaveBMPFile(HWND hwnd, LPTSTR pszFile, PBITMAPINFO pbi, HBITMAP hBMP, HDC hDC)
{
    std::wofstream hf;                  // file handle
    BITMAPFILEHEADER hdr;       // bitmap file-header
    PBITMAPINFOHEADER pbih;     // bitmap info-header
    LPBYTE lpBits;              // memory pointer
    DWORD dwTotal;              // total count of bytes
    DWORD cb;                   // incremental count of bytes
    BYTE *hp;                   // byte pointer
    //DWORD dwTmp;

    pbih = (PBITMAPINFOHEADER) pbi;
    lpBits = (LPBYTE) GlobalAlloc(GMEM_FIXED, pbih->biSizeImage);

    if (!lpBits)
    {
        MessageBox(hwnd,L"GlobalAlloc",L"Error", MB_OK );
    }
// Retrieve the color table (RGBQUAD array) and the bits
// (array of palette indices) from the DIB.
    if (!GetDIBits(hDC, hBMP, 0, (WORD) pbih->biHeight, lpBits, pbi,DIB_RGB_COLORS))
    {
        MessageBox(hwnd,L"GetDIBits",L"Error",MB_OK );
    }
// Create the .BMP file.
    hf.open(pszFile,std::ios::binary);
    if (hf.fail() == true)
    {
        MessageBox( hwnd,L"CreateFile",L"Error", MB_OK);
    }

    hdr.bfType = 0x4d42;  // File type designator "BM" 0x42 = "B" 0x4d = "M"
// Compute the size of the entire file.
    hdr.bfSize = (DWORD) (sizeof(BITMAPFILEHEADER) + pbih->biSize + pbih->biClrUsed * sizeof(RGBQUAD) + pbih->biSizeImage);
    hdr.bfReserved1 = 0;
    hdr.bfReserved2 = 0;
// Compute the offset to the array of color indices.
    hdr.bfOffBits = (DWORD) sizeof(BITMAPFILEHEADER) + pbih->biSize + pbih->biClrUsed * sizeof (RGBQUAD);
// Copy the BITMAPFILEHEADER into the .BMP file.
    hf.write((wchar_t*) &hdr, sizeof(BITMAPFILEHEADER));
    if (hf.fail() == true )
    {
        MessageBox(hwnd,L"WriteFileHeader",L"Error",MB_OK );
    }
// Copy the BITMAPINFOHEADER and RGBQUAD array into the file.
     hf.write((wchar_t*) pbih, sizeof(BITMAPINFOHEADER) + pbih->biClrUsed * sizeof (RGBQUAD));
    if (hf.fail() == true )
    {
        MessageBox(hwnd,L"WriteInfoHeader",L"Error",MB_OK );
    }
// Copy the array of color indices into the .BMP file.
    dwTotal = cb = pbih->biSizeImage;
    hp = lpBits;
    hf.write((wchar_t*) hp, (int) cb);
    if (hf.fail() == true )
    {
        MessageBox(hwnd,L"WriteData",L"Error",MB_OK );
    }
// Close the .BMP file.
    hf.close();
    if (hf.fail() == true)
    {
        MessageBox(hwnd,L"CloseHandle",L"Error",MB_OK );
    }

// Free memory.
    GlobalFree((HGLOBAL)lpBits);
}
//End of BMP Save


PBITMAPINFO CreateBitmapInfoStruct(HWND hwnd, HBITMAP hBmp)
{
    BITMAP bmp;
    PBITMAPINFO pbmi;
    WORD cClrBits;
// Retrieve the bitmap color format, width, and height.
    if (!GetObject(hBmp, sizeof(BITMAP), (LPSTR)&bmp))
    {
        MessageBox(hwnd,L"GetObject",L"Error",MB_OK );
    }
// Convert the color format to a count of bits.
    cClrBits = (WORD)(bmp.bmPlanes * bmp.bmBitsPixel);
    if (cClrBits == 1)
        cClrBits = 1;
    else if (cClrBits <= 4)
        cClrBits = 4;
    else if (cClrBits <= 8)
        cClrBits = 8;
    else if (cClrBits <= 16)
        cClrBits = 16;
    else if (cClrBits <= 24)
        cClrBits = 24;
    else cClrBits = 32;

// Allocate memory for the BITMAPINFO structure. (This structure
// contains a BITMAPINFOHEADER structure and an array of RGBQUAD
// data structures.)

    if (cClrBits != 24)
    {
        pbmi = (PBITMAPINFO) LocalAlloc(LPTR,sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * (1<< cClrBits));
    }
// There is no RGBQUAD array for the 24-bit-per-pixel format.
    else
        pbmi = (PBITMAPINFO) LocalAlloc(LPTR, sizeof(BITMAPINFOHEADER));

// Initialize the fields in the BITMAPINFO structure.
    pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biWidth = bmp.bmWidth;
    pbmi->bmiHeader.biHeight = bmp.bmHeight;
    pbmi->bmiHeader.biPlanes = bmp.bmPlanes;
    pbmi->bmiHeader.biBitCount = bmp.bmBitsPixel;
    if (cClrBits < 24)
    {
        pbmi->bmiHeader.biClrUsed = (1<<cClrBits);
    }
// If the bitmap is not compressed, set the BI_RGB flag.
    pbmi->bmiHeader.biCompression = BI_RGB;

// Compute the number of bytes in the array of color
// indices and store the result in biSizeImage.
// For Windows NT, the width must be DWORD aligned unless
// the bitmap is RLE compressed. This example shows this.
// For Windows 95/98/Me, the width must be WORD aligned unless the
// bitmap is RLE compressed.
    pbmi->bmiHeader.biSizeImage = ((pbmi->bmiHeader.biWidth * cClrBits +31) & ~31) /8 * pbmi->bmiHeader.biHeight;
// Set biClrImportant to 0, indicating that all of the
// device colors are important.
    pbmi->bmiHeader.biClrImportant = 0;

    return pbmi; //return BITMAPINFO
}


*/