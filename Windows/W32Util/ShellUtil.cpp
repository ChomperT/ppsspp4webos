// NOTE: Apologies for the quality of this code, this is really from pre-opensource Dolphin - that is, 2003.

#include "stdafx.h"
#include "shlobj.h"

#include "ShellUtil.h"

#include <shlobj.h>
#include <commdlg.h>

namespace W32Util
{
	std::string BrowseForFolder(HWND parent, char *title)
	{
		BROWSEINFO info;
		memset(&info,0,sizeof(info));
		info.hwndOwner = parent;
		info.lpszTitle = title;
		info.ulFlags = BIF_EDITBOX | BIF_RETURNONLYFSDIRS | BIF_USENEWUI;

		//info.pszDisplayName
		LPCITEMIDLIST idList = SHBrowseForFolder(&info);

		char temp[MAX_PATH];
		SHGetPathFromIDList(idList, temp);
		if (strlen(temp))
		{
			return temp;
		}
		else
			return "";
	}

	//---------------------------------------------------------------------------------------------------
	// function WinBrowseForFileName
	//---------------------------------------------------------------------------------------------------
	bool BrowseForFileName (bool _bLoad, HWND _hParent, const char *_pTitle,
		const char *_pInitialFolder,const char *_pFilter,const char *_pExtension, 
		std::string& _strFileName)
	{
		char szFile [MAX_PATH+1];
		char szFileTitle [MAX_PATH+1];

		strcpy (szFile,"");
		strcpy (szFileTitle,"");

		OPENFILENAME ofn;

		ZeroMemory (&ofn,sizeof (ofn));

		ofn.lStructSize		= sizeof (OPENFILENAME);
		ofn.lpstrInitialDir = _pInitialFolder;
		ofn.lpstrFilter		= _pFilter;
		ofn.nMaxFile		= sizeof (szFile);
		ofn.lpstrFile		= szFile;
		ofn.lpstrFileTitle	= szFileTitle;
		ofn.nMaxFileTitle	= sizeof (szFileTitle);
		ofn.lpstrDefExt     = _pExtension;
		ofn.hwndOwner		= _hParent;
		ofn.Flags			= OFN_NOCHANGEDIR | OFN_EXPLORER | OFN_HIDEREADONLY;

		if (_strFileName.size () != 0)
			ofn.lpstrFile = (char *)_strFileName.c_str();

		if (((_bLoad)?GetOpenFileName (&ofn):GetSaveFileName (&ofn)))
		{
			_strFileName = ofn.lpstrFile;
			return true;
		}
		else
			return false;
	}
	
	std::vector<std::string> BrowseForFileNameMultiSelect(bool _bLoad, HWND _hParent, const char *_pTitle,
		const char *_pInitialFolder,const char *_pFilter,const char *_pExtension)
	{
		char szFile [MAX_PATH+1+2048*2];
		char szFileTitle [MAX_PATH+1];

		strcpy (szFile,"");
		strcpy (szFileTitle,"");

		OPENFILENAME ofn;

		ZeroMemory (&ofn,sizeof (ofn));

		ofn.lStructSize		= sizeof (OPENFILENAME);
		ofn.lpstrInitialDir = _pInitialFolder;
		ofn.lpstrFilter		= _pFilter;
		ofn.nMaxFile		= sizeof (szFile);
		ofn.lpstrFile		= szFile;
		ofn.lpstrFileTitle	= szFileTitle;
		ofn.nMaxFileTitle	= sizeof (szFileTitle);
		ofn.lpstrDefExt     = _pExtension;
		ofn.hwndOwner		= _hParent;
		ofn.Flags			= OFN_NOCHANGEDIR | OFN_EXPLORER | OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT ;

		std::vector<std::string> files;

		if (((_bLoad) ? GetOpenFileName (&ofn):GetSaveFileName (&ofn)))
		{
			std::string directory = ofn.lpstrFile;
			char *temp = ofn.lpstrFile;
			char *oldtemp = temp;
			temp+=strlen(temp)+1;
			if (*temp==0)
			{
				//we only got one file
				files.push_back(std::string(oldtemp));
			}
			else
			{
				while (*temp)
				{
					files.push_back(directory+"\\"+std::string(temp));
					temp+=strlen(temp)+1;
				}
			}
			return files;
		}
		else
			return std::vector<std::string>(); // empty vector;

	}
	



}