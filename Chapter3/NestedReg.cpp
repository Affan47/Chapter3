// lsReg.cpp : Defines the entry point for the console application.
//



/* Chapter 3. lsREG - Registry list command. */
/* lsREG [options] SubKey    //Subkey starts with predefined key name.
	List the key-value pairs.
	Options:
		-R	recursive
		-l  List extended information; namely, the last write time
			and the value type
lsReg.exe -l HKEY_CURRENT_USER\dcis\systemprogramming\abc

			*/


			/* This program illustrates:
					1.	Registry handles and traversal
					2.	Registry values
					3.	The similarity and differences between directory
						and registry traversal

				Note that there are no wild card file names and you specify the
				subkey, with all key-value pairs being listed. This is similar to
				ls with "SubKey\*" as the file specifier             */

				/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "stdafx.h"


	BOOL TraverseRegistry(HKEY, LPTSTR, LPTSTR, LPBOOL);
BOOL DisplayPair(LPTSTR, DWORD, LPBYTE, DWORD, LPBOOL);
BOOL DisplaySubKey(LPTSTR, LPTSTR, PFILETIME, LPBOOL);
BOOL SHOW_KEY(HKEY, LPTSTR, LPTSTR, LPBOOL);

int _tmain(int argc, LPTSTR argv[])
{
	BOOL flags[2], ok = TRUE;
	TCHAR keyName[MAX_PATH + 1];
	LPTSTR pScan;
	DWORD i, keyIndex;
	HKEY hKey, hNextKey;



	/* Tables of predefined key names and keys */
	//standard parent keys
	LPCTSTR PreDefKeyNames[] = {
		_T("HKEY_LOCAL_MACHINE"),
		_T("HKEY_CLASSES_ROOT"),
		_T("HKEY_CURRENT_USER"),
		_T("HKEY_CURRENT_CONFIG"),
		NULL };
	HKEY PreDefKeys[] = {
		HKEY_LOCAL_MACHINE,
		HKEY_CLASSES_ROOT,
		HKEY_CURRENT_USER,
		HKEY_CURRENT_CONFIG };

	if (argc < 2) {
		_tprintf(_T("Usage: lsREG[options] SubKey\n"));
		return 1;
	}

	keyIndex = Options(argc, (LPCTSTR*)argv, _T("Rl"), &flags[0], &flags[1], NULL);

	if (keyIndex == argc) { // make sure key specified as arg
		_tprintf(_T("Usage: lsREG[options] SubKey\n"));
		return 1;
	}


	/* "Parse" the search pattern into two parts: the "key"
		and the "subkey". The key is the first back-slash terminated
		string and must be one of HKEY_LOCAL_MACHINE, HKEY_CLASSES_ROOT,HKEY_CURRENT_CONFIG
		or HKEY_CURRENT_USER. The subkey is everything else.
		The key and subkey names will be copied from argv[keyIndex]. */

		/*  Build the Key */
	pScan = argv[keyIndex];   // i < MAX_PATH verify??? 
	for (i = 0; *pScan != _T('\\') && *pScan != _T('\0') && i < MAX_PATH; pScan++, i++)
		keyName[i] = *pScan; //copy string

	keyName[i] = _T('\0'); //terminate string
	if (*pScan == _T('\\')) pScan++;

	//make predefined keyname upper case
	for (int c = 0; c < _tcslen(keyName); c++) {
		keyName[c] = _totupper(keyName[c]);

	}
	/* Translate predefined key name to an HKEY */
	for (i = 0;
		PreDefKeyNames[i] != NULL && _tcscmp(PreDefKeyNames[i], keyName) != 0; //0 means equal
		i++);
	if (PreDefKeyNames[i] == NULL) ReportError(_T("Use a Predefined Key"), 1, FALSE);
	hKey = PreDefKeys[i]; //Predefine key handle.

	/*  pScan points to the start of the subkey string. It is not directly
		documented that \ is the separator, but it works fine */
		//hKey is the parent key handle
		//open the specified pScan key; pScan is not case-sensitive
		//KEY_READ is the desired access rights to the key to be opened.
	if (RegOpenKeyEx(hKey, pScan, 0, KEY_READ, &hNextKey) != ERROR_SUCCESS)
		ReportError(_T("Cannot open subkey properly"), 2, TRUE);

	hKey = hNextKey;//pointer to predefined (i.e.hKey) or subkey (specified by pScan) depending on whether pScan is NULL or empty string

	ok = TraverseRegistry(hKey, argv[keyIndex], NULL, flags);
	RegCloseKey(hKey);

	return ok ? 0 : 1;
}

BOOL TraverseRegistry(HKEY hKey, LPTSTR fullKeyName, LPTSTR subKey, LPBOOL flags)

/*	Traverse a registry key, listing the named-value pairs for each key and
	traversing subkeys if the -R option is set.
	fullKeyName is a "full key name" starting with one of the open key
	names, such as "HKEY_LOCAL_MACHINE".
	SubKey, which can be null, is the rest of the path to be traversed. */

{
	HKEY hSubKey;
	BOOL recursive = flags[0];
	LONG result;
	DWORD valueType, index;
	DWORD numSubKeys, maxSubKeyLen, numValues, maxValueNameLen, maxValueLen;
	DWORD subKeyNameLen, valueNameLen, valueLen;
	FILETIME lastWriteTime;
	LPTSTR subKeyName, valueName;
	LPBYTE value;
	/****  Having a large array such as fullSubKeyName on the stack is a bad idea
	 *  or, if you prefer, an extemely poor programming practice, even a crime (I plead no contest) ****/
	 /* 1) It can consume lots of space as you traverse the directory
	  * 2) You risk a stack overflow, which is a security risk
	  * 3) You cannot deal with long paths (> MAX_PATH), using the \\?\ prefix
	  *    SUGGESTION: See lsW (Chapter 3) for a better implementation and fix this program accordingly.
	  */

	TCHAR fullSubKeyName[1024] = _T("");;

	/* Open up the key handle. */
	//when subkey is NULL or empty string then hSubKey = hkey
	/*KEY_READ is the access right. The function fails if the security descriptor of the
	key does not permit the requested access for the calling process. */
	/*KEY_READ Combines the STANDARD_RIGHTS_READ, KEY_QUERY_VALUE,
	KEY_ENUMERATE_SUB_KEYS, and KEY_NOTIFY values.*/
	if (RegOpenKeyEx(hKey, subKey, 0, KEY_READ, &hSubKey) != ERROR_SUCCESS) //ERROR_SUCCESS = 0
		ReportError(_T("\nCannot open subkey"), 2, TRUE);

	/*  Retrieves information about the specified registry key */
	//
	_tprintf(_T("\n"));
	_tprintf(_T("==================================================================================="));
	_tprintf(_T("\n"));
	if (RegQueryInfoKey(hSubKey, NULL, NULL, NULL,   // infor abt each key
		&numSubKeys, &maxSubKeyLen, NULL, //maxSubKeyLen is char count not including \0
		&numValues, &maxValueNameLen, &maxValueLen, //maxValueLen in bytes
		NULL, &lastWriteTime) != ERROR_SUCCESS)
		ReportError(_T("Cannot query subkey information"), 3, TRUE);
	subKeyName = (LPTSTR)malloc(TSIZE * (maxSubKeyLen + 1));   /* +1 for null character */
	valueName = (LPTSTR)malloc(TSIZE * (maxValueNameLen + 1));
	value = (LPBYTE)malloc(maxValueLen);      /* size in bytes */

	_tprintf(_T("\nSubKey Count: %d, Values Count: %d\n"), numSubKeys, numValues); //by me for info


	/*  First pass for named-value pairs. */
	/*  Important assumption: No one edits the registry under this subkey */
	/*  during this loop. Doing so could change add new values */

	for (index = 0; index < numValues; index++) {
		valueNameLen = maxValueNameLen + 1; /* A very common bug is to forget to set */
		valueLen = maxValueLen + 1;     /* these values; both are in/out params  */

		//enumerates values for the specified key
		result = RegEnumValue(hSubKey, index,
			valueName, &valueNameLen, //value name buffer and its length in characters
			NULL, &valueType,
			value, &valueLen);//value buffer and its length in bytes

		if (result == ERROR_SUCCESS) //&& GetLastError() == 0
			DisplayPair(valueName, valueType, value, valueLen, flags);
		/*  If you wanted to change a value, this would be the place to do it.
			RegSetValueEx(hSubKey, valueName, 0, valueType, pNewValue, NewValueSize); */
	}

	_tprintf(_T("\n"));
	/*  Second pass for subkeys Just Printing */

	for (index = 0; index < numSubKeys; index++) {
		subKeyNameLen = maxSubKeyLen + 1;
		result = RegEnumKeyEx(hSubKey, index, subKeyName, &subKeyNameLen, NULL, NULL, NULL, &lastWriteTime);//fill buffer with subkey name 
		if (result == ERROR_SUCCESS) {
			DisplaySubKey(fullKeyName, subKeyName, &lastWriteTime, flags);
			/*  Display subkey components if -R is specified */
			if (recursive) {
				_stprintf(fullSubKeyName, _T("%s\\%s"), fullKeyName, subKeyName);//full sub key name
			}
		}
	}

	/*  Second pass for subkeys */
	for (index = 0; index < numSubKeys; index++) {
		subKeyNameLen = maxSubKeyLen + 1;
		result = RegEnumKeyEx(hSubKey, index, subKeyName, &subKeyNameLen, NULL, NULL, NULL, &lastWriteTime);//fill buffer with subkey name 
		if (result == ERROR_SUCCESS) {
			_tprintf(_T("\n"));
			_tprintf(_T("=================================================================================================="));
			_tprintf(_T("\n"));
			DisplaySubKey(fullKeyName, subKeyName, &lastWriteTime, flags);
			/*  Display subkey components if -R is specified */
			if (recursive) {
				_stprintf(fullSubKeyName, _T("%s\\%s"), fullKeyName, subKeyName);//full sub key name
				SHOW_KEY(hSubKey, fullSubKeyName, subKeyName, flags);
			}
		}
	}
	_tprintf(_T("\n"));
	free(subKeyName);
	free(valueName);
	free(value);
	RegCloseKey(hSubKey);
	return TRUE;
}



BOOL SHOW_KEY(HKEY hKey, LPTSTR fullKeyName, LPTSTR subKey, LPBOOL flags) {
	HKEY hSubKey;
	BOOL recursive = flags[0];
	LONG result;
	DWORD valueType, index;
	DWORD numSubKeys, maxSubKeyLen, numValues, maxValueNameLen, maxValueLen;
	DWORD subKeyNameLen, valueNameLen, valueLen;
	FILETIME lastWriteTime;
	LPTSTR subKeyName, valueName;
	LPBYTE value;
	/****  Having a large array such as fullSubKeyName on the stack is a bad idea
	 *  or, if you prefer, an extemely poor programming practice, even a crime (I plead no contest) ****/
	 /* 1) It can consume lots of space as you traverse the directory
	  * 2) You risk a stack overflow, which is a security risk
	  * 3) You cannot deal with long paths (> MAX_PATH), using the \\?\ prefix
	  *    SUGGESTION: See lsW (Chapter 3) for a better implementation and fix this program accordingly.
	  */



	TCHAR fullSubKeyName[1024] = _T("");;

	/* Open up the key handle. */
	//when subkey is NULL or empty string then hSubKey = hkey
	/*KEY_READ is the access right. The function fails if the security descriptor of the
	key does not permit the requested access for the calling process. */
	/*KEY_READ Combines the STANDARD_RIGHTS_READ, KEY_QUERY_VALUE,
	KEY_ENUMERATE_SUB_KEYS, and KEY_NOTIFY values.*/
	if (RegOpenKeyEx(hKey, subKey, 0, KEY_READ, &hSubKey) != ERROR_SUCCESS) //ERROR_SUCCESS = 0
		ReportError(_T("\nCannot open subkey"), 2, TRUE);

	/*  Retrieves information about the specified registry key */
	//
	if (RegQueryInfoKey(hSubKey, NULL, NULL, NULL,   // infor abt each key
		&numSubKeys, &maxSubKeyLen, NULL, //maxSubKeyLen is char count not including \0
		&numValues, &maxValueNameLen, &maxValueLen, //maxValueLen in bytes
		NULL, &lastWriteTime) != ERROR_SUCCESS)
		ReportError(_T("Cannot query subkey information"), 3, TRUE);
	subKeyName = (LPTSTR)malloc(TSIZE * (maxSubKeyLen + 1));   /* +1 for null character */
	valueName = (LPTSTR)malloc(TSIZE * (maxValueNameLen + 1));
	value = (LPBYTE)malloc(maxValueLen);      /* size in bytes */

	_tprintf(_T("\nSubKey Count: %d, Values Count: %d\n"), numSubKeys, numValues); //by me for info


	/*  First pass for named-value pairs. */
	/*  Important assumption: No one edits the registry under this subkey */
	/*  during this loop. Doing so could change add new values */
	for (index = 0; index < numValues; index++) {
		valueNameLen = maxValueNameLen + 1; /* A very common bug is to forget to set */
		valueLen = maxValueLen + 1;     /* these values; both are in/out params  */

		//enumerates values for the specified key
		result = RegEnumValue(hSubKey, index,
			valueName, &valueNameLen, //value name buffer and its length in characters
			NULL, &valueType,
			value, &valueLen);//value buffer and its length in bytes
		if (result == ERROR_SUCCESS) //&& GetLastError() == 0
			DisplayPair(valueName, valueType, value, valueLen, flags);
		/*  If you wanted to change a value, this would be the place to do it.
			RegSetValueEx(hSubKey, valueName, 0, valueType, pNewValue, NewValueSize); */
	}
	_tprintf(_T("\n"));
	/*  Second pass for subkeys */
	for (index = 0; index < numSubKeys; index++) {
		subKeyNameLen = maxSubKeyLen + 1;
		result = RegEnumKeyEx(hSubKey, index, subKeyName, &subKeyNameLen, NULL, NULL, NULL, &lastWriteTime);//fill buffer with subkey name 
		if (result == ERROR_SUCCESS) {
			DisplaySubKey(fullKeyName, subKeyName, &lastWriteTime, flags);
			/*  Display subkey components if -R is specified */
			if (recursive) {
				_stprintf(fullSubKeyName, _T("%s\\%s"), fullKeyName, subKeyName);//full sub key name
			}
		}
	}

	_tprintf(_T("\n"));
	_tprintf(_T("_______________________________________________________________________________________________"));
	_tprintf(_T("\n"));
	free(subKeyName);
	free(valueName);
	free(value);
	RegCloseKey(hSubKey);
	return TRUE;
}


BOOL DisplayPair(LPTSTR valueName, DWORD valueType,
	LPBYTE value, DWORD valueLen,
	LPBOOL flags)

	/* Function to display key-value pairs. */

{

	LPBYTE pV = value;
	DWORD i;

	_tprintf(_T("\n%s = "), valueName);
	switch (valueType) {
	case REG_FULL_RESOURCE_DESCRIPTOR: /* 9: Resource list in the hardware description */
	case REG_BINARY: /*  3: Binary data in any form. */
		for (i = 0; i < valueLen; i++, pV++)
			_tprintf(_T(" %x"), *pV);
		break;

	case REG_DWORD: /* 4: A 32-bit number. */
		_tprintf(_T("%x"), (DWORD)*value);
		break;

	case REG_EXPAND_SZ: /* 2: null-terminated string with unexpanded references to environment variables (for example, �%PATH%�). */
	case REG_SZ: /* 1: A null-terminated string. */
		_tprintf(_T("%s"), (LPTSTR)value);
		break;
	case REG_MULTI_SZ: /* 7: An array of null-terminated strings, terminated by two null characters. E.g. string01\0string02\0..\0\0 */
		_tprintf(_T("Begin Multiple String:\n"));
		while (*pV) {
			_tprintf(_T("\t\t%s"), (LPTSTR)pV);
			pV = pV + _tcslen((LPTSTR)pV) + 1;//+1 for null char

		}
		_tprintf(_T("End Multiple String:\n"));
		break;
	case REG_DWORD_BIG_ENDIAN: /* 5:  A 32-bit number in big-endian format. */
		for (i = valueLen - 1; i >= 0; i--) //MSB ...LSB
			_tprintf(_T(" %x"), pV[i]);//*(pV+i)
		break;
	case REG_LINK: /* 6: A Unicode symbolic link. */
		_tprintf(_T("%s"), (LPTSTR)value);
		break;
	case REG_NONE: /* 0: No defined value type. */
		_tprintf(_T("No Defined Type"));
		break;
	case REG_RESOURCE_LIST: /* 8: A device-driver resource list. */
	default: _tprintf(_T(" ** Cannot display value of type: %d. Exercise for reader\n"), valueType);
		break;
	}

	return TRUE;
}

BOOL DisplaySubKey(LPTSTR keyName, LPTSTR subKeyName, PFILETIME pLastWrite, LPBOOL flags)
{
	BOOL longList = flags[1];
	SYSTEMTIME sysLastWrite;

	_tprintf(_T("\n%s"), keyName);
	if (_tcslen(subKeyName) > 0) _tprintf(_T("\\%s "), subKeyName);
	if (longList) {
		FileTimeToSystemTime(pLastWrite, &sysLastWrite);
		_tprintf(_T("	%02d/%02d/%04d %02d:%02d:%02d"),
			sysLastWrite.wMonth, sysLastWrite.wDay,
			sysLastWrite.wYear, sysLastWrite.wHour,
			sysLastWrite.wMinute, sysLastWrite.wSecond);
	}
	return TRUE;
}
