// lsW.cpp : Defines the entry point for the console application.
//How to run:
//           1. lsw.exe -l            list the contents of lsw current directory
//           2. lsw.exe -R -l         list the contents of lsw current directory
//           3. lsw.exe -l c:\*  d:\* list the contents of C:\ drive  and D:\ drive
//           4. lsw.exe -l c:\test*    list the contents of C:\ drive that start with test
//           5. lsw.exe -l c:\*test*    list the contents of C:\ drive that include test
/*           6. D:\systemsprogramming\WSP_4th_edition>.\chapter03\debug\lsw.exe -l ..\..\
				this path is relative to path return by GetCurrentDirectory(). Here this ..\..\  resolves to D:\*

				c:\* is equivalent to c:\ coz of this program. Similarly, ..\..\  is equivalent to ..\..\*
							*/

							//In search pattern * means 0 or more characters; ? means 0 or 1 character
						  /* Chapter 3. Windows lsW file list command. */
						  /* lsW[options][files]
						  List the attributes of one or more files.
						  Options:
						  -R	recursive
						  -l	longList listing(size and time of modification)
						  Depending on the ProcessItem function, this will
						  also list the owner and permissions(See Chapter 5 - security). */
						  /* This program illustrates:
						1.	Search handles and directory traversal
						2.	File attributes, including time
						3.	Using generic string functions to output file information */
						/* THIS PROGRAM IS NOT A FAITHFUL IMPLEMENATION OF THE POSIX ls COMMAND - IT IS INTENDED TO ILLUSRATE THE WINDOWS API */
						/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
						/* BEGIN BOILERPLATE CODE */

#include "stdafx.h"



BOOL TraverseDirectory(LPTSTR, LPTSTR, DWORD, LPBOOL);
BOOL SHOW_DIRECTORY(LPTSTR, LPTSTR, DWORD, LPBOOL);
DWORD FileType(LPWIN32_FIND_DATA);
BOOL ProcessItem(LPWIN32_FIND_DATA, DWORD, LPBOOL);

int _tmain(int argc, LPCTSTR argv[])
{
	BOOL flags[MAX_OPTIONS], ok = TRUE;
	TCHAR searchPattern[MAX_PATH + 1], currPath[MAX_PATH_LONG + 1], parentPath[MAX_PATH_LONG + 1];
	LPTSTR pSlash, pSearchPattern;
	int i, fileIndex;
	DWORD pathLength;

	fileIndex = Options(argc, argv, _T("Rl"), &flags[0], &flags[1], NULL); //flag[0] is for R; flag[0] = TRUE if R option is present on a commandline

	/* "Parse" the search pattern into two parts: the "parent"
		and the file name or wild card expression. The file name
		is the longest suffix not containing a slash.
		The parent is the remaining prefix with the slash.
		This is performed for all command line search patterns.
		If no file is specified, use * as the search pattern. */

	pathLength = GetCurrentDirectory(MAX_PATH_LONG, currPath); //Saving  the current directory
	//failure
	if (pathLength == 0 || pathLength >= MAX_PATH_LONG) { /* pathLength >= MAX_PATH_LONG (32767) should be impossible */
		ReportError(_T("GetCurrentDirectory failed"), 1, TRUE);
	}
	//argc == fileIndex
	LPTSTR str = LPTSTR("*");
	if (argc < fileIndex + 1) //if search pattern not specified; just this command: lsw -R -l or lsw -l
		ok = TraverseDirectory(currPath, str, MAX_OPTIONS, flags);
	else for (i = fileIndex; i < argc; i++) { //for each search pattern
		if (_tcslen(argv[i]) >= MAX_PATH) {
			ReportError(_T("The command line argument is longer than the maximum this program supports"), 2, FALSE);
		}
		_tcscpy(searchPattern, argv[i]);
		_tcscpy(parentPath, argv[i]);

		/* Find the rightmost backslash, if any.
			Set the path and use the rest as the search pattern. */

		pSlash = _tstrrchr(parentPath, _T('\\')); //return pointer to right most backslash
		//extract parent path and search pattern name. parent path becomes current directory
		if (pSlash != NULL) {
			*pSlash = _T('\0');
			_tcscat(parentPath, _T("\\"));    //required by SetCurrentDirectory     
			SetCurrentDirectory(parentPath);
			pSlash = _tstrrchr(searchPattern, _T('\\'));
			pSearchPattern = pSlash + 1;
		}
		else { //no right most slash in the path; 
			_tcscpy(parentPath, _T(".\\"));//current directory
			pSearchPattern = searchPattern;
		}
		ok = TraverseDirectory(parentPath, pSearchPattern, MAX_OPTIONS, flags) && ok;
		SetCurrentDirectory(currPath);	 /* Restore working directory. */
	}//for loop

	return ok ? 0 : 1;
}

static BOOL TraverseDirectory(LPTSTR parentPath, LPTSTR searchPattern, DWORD numFlags, LPBOOL flags)

/* Traverse a directory, carrying out an implementation specific "action" for every
	name encountered. The action in this version is "list, with optional attributes". */
	/* searchPattern: Relative or absolute searchPattern to traverse in the parentPath.  */
	/* On entry, the current direcotry is parentPath, which ends in a \ */
{
	HANDLE searchHandle;
	WIN32_FIND_DATA findData;
	BOOL recursive = flags[0];
	DWORD fType, iPass, lenParentPath;
	TCHAR subdirectoryPath[MAX_PATH_LONG + 1];

	/* Open up the directory search handle and get the
		first file name to satisfy the path name.
		Make two passes. The first processes the files
		and the second processes the directories. */


	if (_tcslen(searchPattern) == 0) {//no search pattern
		_tcscat(searchPattern, _T("*"));
	}
	/* Add a backslash, if needed, at the end of the parent path */
	//this is true if parentPath is the path returned by GetCurrentDirectory()
	if (parentPath[_tcslen(parentPath) - 1] != _T('\\')) { /* Adds \ to the end of the parent path, unless there already is one */
		_tcscat(parentPath, _T("\\"));
	}


	/* Open up the directory search handle and get the
		first file name to satisfy the path name. Make two passes.
		The first pass lists the files and subdirectories and the second pass list the sub directories. */

	for (iPass = 1; iPass <= 2; iPass++) {
		searchHandle = FindFirstFile(searchPattern, &findData);//search is w.r.t. Process Current directory
		if (searchHandle == INVALID_HANDLE_VALUE) { //fails or fails to locate files
			ReportError(_T("Error opening Search Handle.\n"), 0, TRUE);
			return FALSE;
		}

		/* Scan the directory and its subdirectories for files satisfying the pattern. */
		do {

			/* For each file/directory located, get the type. List everything on pass 1.
				On pass 2, display the directory name and recursively process
				the subdirectory contents, if the recursive option is set. */
			fType = FileType(&findData);
			if (iPass == 1) /* ProcessItem is "print attributes". */
				ProcessItem(&findData, MAX_OPTIONS, flags);

			lenParentPath = (DWORD)_tcslen(parentPath);
			/* Traverse the subdirectory on the second pass. */
			if (fType == TYPE_DIR && iPass == 2 && recursive) {
				_tprintf(_T("\n%s%s:"), parentPath, findData.cFileName);//print sub directory
				SetCurrentDirectory(findData.cFileName);//now subdir is current working dir
				if (_tcslen(parentPath) + _tcslen(findData.cFileName) >= MAX_PATH_LONG - 1) { //not counting null character
					ReportError(_T("Path Name is too long"), 10, FALSE);
				}
				_tcscpy(subdirectoryPath, parentPath);
				_tcscat(subdirectoryPath, findData.cFileName); /* The parent path terminates with \ before the _tcscat call */
				SHOW_DIRECTORY(subdirectoryPath, LPTSTR("*"), numFlags, flags);//include all files/directories in a sub-dir
				SetCurrentDirectory(_T("..")); /* Restore the current directory */
			}//directory

			/* Get the next file or directory name. */

		} while (FindNextFile(searchHandle, &findData));

		FindClose(searchHandle);
	}
	return TRUE;
}

static BOOL SHOW_DIRECTORY(LPTSTR parentPath, LPTSTR searchPattern, DWORD numFlags, LPBOOL flags)
{
	HANDLE searchHandle;
	WIN32_FIND_DATA findData;
	BOOL recursive = flags[0];
	DWORD fType, iPass, lenParentPath;
	TCHAR subdirectoryPath[MAX_PATH_LONG + 1];

	/* Open up the directory search handle and get the
		first file name to satisfy the path name.
		Make two passes. The first processes the files
		and the second processes the directories. */


	if (_tcslen(searchPattern) == 0) {//no search pattern
		_tcscat(searchPattern, _T("*"));
	}
	/* Add a backslash, if needed, at the end of the parent path */
	//this is true if parentPath is the path returned by GetCurrentDirectory()
	if (parentPath[_tcslen(parentPath) - 1] != _T('\\')) { /* Adds \ to the end of the parent path, unless there already is one */
		_tcscat(parentPath, _T("\\"));
	}


	/* Open up the directory search handle and get the
		first file name to satisfy the path name. Make two passes.
		The first pass lists the files and subdirectories and the second pass list the sub directories. */

	for (iPass = 1; iPass <= 2; iPass++) {
		searchHandle = FindFirstFile(searchPattern, &findData);//search is w.r.t. Process Current directory
		if (searchHandle == INVALID_HANDLE_VALUE) { //fails or fails to locate files
			ReportError(_T("Error opening Search Handle.\n"), 0, TRUE);
			return FALSE;
		}


		/* Scan the directory and its subdirectories for files satisfying the pattern. */
		do {
			fType = FileType(&findData);
			if (iPass == 1) /* ProcessItem is "print attributes". */
				ProcessItem(&findData, MAX_OPTIONS, flags);
			lenParentPath = (DWORD)_tcslen(parentPath);
		} while (FindNextFile(searchHandle, &findData));
	}
	FindClose(searchHandle);
}


static DWORD FileType(LPWIN32_FIND_DATA pFileData)

/* Return file type from the find data structure.
	Types supported:
		TYPE_FILE:	If this is a file
		TYPE_DIR:	If this is a directory other than . or ..
		TYPE_DOT:	If this is . or .. directory */
{
	BOOL isDir;
	DWORD fType;
	fType = TYPE_FILE;
	isDir = (pFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
	if (isDir)
		if (lstrcmp(pFileData->cFileName, _T(".")) == 0
			|| lstrcmp(pFileData->cFileName, _T("..")) == 0)
			fType = TYPE_DOT;
		else fType = TYPE_DIR;
	return fType;
}
/*  END OF BOILERPLATE CODE */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static BOOL ProcessItem(LPWIN32_FIND_DATA pFileData, DWORD numFlags, LPBOOL flags)

/* Function to process(list attributes, in this case)
	the file or directory. This implementation only shows
	the low order part of the file size. */
{
	const TCHAR fileTypeChar[] = { _T(' '), _T('d') };
	DWORD fType = FileType(pFileData);
	BOOL longList = flags[1];
	SYSTEMTIME lastWrite;

	if (fType != TYPE_FILE && fType != TYPE_DIR) return FALSE;

	_tprintf(_T("\n"));
	if (longList) {//option -l found 
		_tprintf(_T("%c"), fileTypeChar[fType - 1]);
		_tprintf(_T("%20llu"), ((LONGLONG)pFileData->nFileSizeHigh * (MAXDWORD + 1)) + pFileData->nFileSizeLow);
		//Use these two functions to convert FileTime to local time
		// FileTimeToLocalFileTime();
		//FileTimeToSystemTime()
		FileTimeToSystemTime(&(pFileData->ftLastWriteTime), &lastWrite);//SystemTime struct for printing
		_tprintf(_T("	%02d/%02d/%04d %02d:%02d:%02d"),
			lastWrite.wDay, lastWrite.wMonth,
			lastWrite.wYear, lastWrite.wHour,
			lastWrite.wMinute, lastWrite.wSecond);
	}
	_tprintf(_T(" %s"), pFileData->cFileName);
	return TRUE;
}
