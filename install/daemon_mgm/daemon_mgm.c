/*
 * Copyright (c) 2003 - 2005 NetGroup, Politecnico di Torino (Italy).
 * Copyright (c) 2005 CACE Technologies, Davis (California).
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Politecnico di Torino, CACE Technologies
 * nor the names of its contributors may be used to endorse or promote 
 * products derived from this software without specific prior written 
 * permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <lmerr.h>
#include <aclapi.h>

int delete_service(FILE* log,LPCTSTR ServiceName);
int stop_service(FILE* log,LPCTSTR ServiceName);
int start_service(FILE* log,LPCTSTR ServiceName);
int create_service(FILE* log,LPCTSTR ServiceName,LPCTSTR ServiceDescriptionShort,LPCTSTR ServiceDescriptionLong,LPCTSTR ServicePath);
int change_start_type_service(FILE* log,LPCTSTR ServiceName, DWORD StartType);
void ShowHelp();

void DisplayErrorText(DWORD dwLastError,FILE* output);

#define SERVICE_NAME "rpcapd"
#define SERVICE_DESCRIPTION_SHORT "Remote Packet Capture Protocol v.0 (experimental)"
#define SERVICE_DESCRIPTION_LONG "Allows to capture traffic on this machine from a remote machine."
#define SERVICE_EXECUTABLE "\"%ProgramFiles%\\WinPcap\\rpcapd.exe\" -d -f \"%ProgramFiles%\\WinPcap\\rpcapd.ini\""


typedef WINADVAPI BOOL  (WINAPI *MyChangeServiceConfig2)(
  SC_HANDLE hService,  // handle to service
  DWORD dwInfoLevel,   // information level
  LPVOID lpInfo        // new data
);

void ShowHelp()
{
	printf("******************************************************************************\n");
	printf("%s Management\n\n", SERVICE_DESCRIPTION_SHORT);
	printf("Copyright (C) 2003-2005  NetGroup, Politecnico di Torino\n");
	printf("Copyright (C) 2005  CACE Technologies, Davis, California\n");
	printf("******************************************************************************\n\n");
	printf("syntax: daemon_mgm -s -x -u -i -r -a -d\n\n");
	printf("	-s starts the service\n");
	printf("	-x stops the service\n");
	printf("	-u uninstalls the service\n");
	printf("	-i installs the service\n");
	printf("	-r uninstalls and reinstalls the service\n");
	printf("	-a changes the service start-type to auto-start\n");
	printf("	-d changes the service start-type to demand-start\n");

}

int main(int argc, char **argv)
{
	FILE *log;

	log=stdout;
	if (argc<2)
	{
		ShowHelp();
		return -1;
	}

	switch(argv[1][1])
	{
	case 's':
		return start_service(log,SERVICE_NAME);

	case 'x':
		return stop_service(log,SERVICE_NAME);

	case 'u':
		return delete_service(log,SERVICE_NAME);

	case 'i':
		return create_service(log,SERVICE_NAME,SERVICE_DESCRIPTION_SHORT,SERVICE_DESCRIPTION_LONG,SERVICE_EXECUTABLE);

	case 'r':
		(void)delete_service(log,SERVICE_NAME);
		Sleep(100);
		return create_service(log,SERVICE_NAME,SERVICE_DESCRIPTION_SHORT,SERVICE_DESCRIPTION_LONG,SERVICE_EXECUTABLE);

	case 'a':
		return change_start_type_service(log,SERVICE_NAME, SERVICE_AUTO_START);

	case 'd':
		return change_start_type_service(log,SERVICE_NAME, SERVICE_DEMAND_START);



	default:
		ShowHelp();
		return -1;
	}

	return 0;
}

int delete_service(FILE* log,LPCTSTR ServiceName)
{
	SC_HANDLE SCM_Handle;
	SC_HANDLE ServiceHandle;
	SERVICE_STATUS ServiceStatus;

	DWORD ReturnValue;

	SCM_Handle=OpenSCManager(NULL,  /*local machine  */
		NULL,						/*active database*/
		SC_MANAGER_ALL_ACCESS);

	if (SCM_Handle==NULL)
	{
		if (log != NULL)	
		{
			fprintf(log,"Error opening Service Control Manager: ");
			DisplayErrorText(GetLastError(),log);
		}
		return -1;
	}

	ServiceHandle=OpenService(SCM_Handle,
					ServiceName,
					SERVICE_ALL_ACCESS);


	if (ServiceHandle==NULL)
	{
		if (log != NULL)
		{
			fprintf(log,"Error opening Service Control Manager: ");
			DisplayErrorText(GetLastError(),log);
		}
		
		if (!CloseServiceHandle(SCM_Handle))
			if (log != NULL)
				fprintf(log,"Error closing Service control Manager\n");
					
		return -1;
	}

	ReturnValue=0;

	if (!ControlService(ServiceHandle,SERVICE_CONTROL_STOP,&ServiceStatus))
	{
		DWORD Err=GetLastError();

		if (Err != ERROR_SERVICE_NOT_ACTIVE)
		{
			if (log != NULL)
			{
				fprintf(log,"Error stopping service %s: ",ServiceName);
				DisplayErrorText(Err,log);
			}
			ReturnValue=-1;
		}
	}
	else
		if (log != NULL)
			fprintf(log,"Service %s successfully stopped\n",ServiceName);

	if (!DeleteService(ServiceHandle))
	{
		if (log != NULL)
		{
			fprintf(log,"Error deleting service %s: ",ServiceName);
			DisplayErrorText(GetLastError(),log);
		}
		ReturnValue=-1;
		
	}
	else
		if (log != NULL)
			fprintf(log,"Service %s successfully deleted\n",ServiceName);
	
	if (!CloseServiceHandle(ServiceHandle))
	{
		if (log != NULL)
		{
			fprintf(log,"Error closing service %s: ",ServiceName);
			DisplayErrorText(GetLastError(),log);
		}
		ReturnValue=-1;
	}


	if (!CloseServiceHandle(SCM_Handle))
	{
		if (log != NULL)
			fprintf(log,"Error closing Service control Manager\n");
		ReturnValue=-1;
	}
	
	return ReturnValue;

}

int stop_service(FILE* log,LPCTSTR ServiceName)
{
	SC_HANDLE SCM_Handle;
	SC_HANDLE ServiceHandle;
	SERVICE_STATUS ServiceStatus;
	DWORD ReturnValue;

	SCM_Handle=OpenSCManager(NULL,  /*local machine  */
		NULL,						/*active database*/
		SC_MANAGER_ALL_ACCESS);

	if (SCM_Handle==NULL)
	{
		if (log !=NULL)
		{
			fprintf(log,"Error opening Service Control Manager: ");
			DisplayErrorText(GetLastError(),log);
		}
		
		return -1;
	}

	ServiceHandle=OpenService(SCM_Handle,
					ServiceName,
					SERVICE_ALL_ACCESS);


	if (ServiceHandle==NULL)
	{
		if (log != NULL)
		{
			fprintf(log,"Error opening service %s:",ServiceName);
			DisplayErrorText(GetLastError(),log);
		}

		if (!CloseServiceHandle(SCM_Handle))
			if (log != NULL)		
				fprintf(log,"Error closing Service control Manager\n");
					
		return -1;
	}

	ReturnValue=0;

	if (!ControlService(ServiceHandle,SERVICE_CONTROL_STOP,&ServiceStatus))
	{
		if (log != NULL)
		{
			fprintf(log,"Error stopping service %s: ",ServiceName);
			DisplayErrorText(GetLastError(),log);
		}
		ReturnValue=-1;
		
	}
	else
		if (log != NULL)
			fprintf(log,"Service %s successfully stopped\n",ServiceName);
	
	if (!CloseServiceHandle(ServiceHandle))
	{
		if (log != NULL)
		{
			fprintf(log,"Error closing service %s: ",ServiceName);
			DisplayErrorText(GetLastError(),log);
		}
		
		ReturnValue=-1;
	}

	if (!CloseServiceHandle(SCM_Handle))
	{
		if (log != NULL)
			fprintf(log,"Error closing Service control Manager\n");
		ReturnValue=-1;
	}
	
	return ReturnValue;

}


int start_service(FILE* log,LPCTSTR ServiceName)
{
	SC_HANDLE SCM_Handle;
	SC_HANDLE ServiceHandle;
	DWORD ReturnValue;

	SCM_Handle=OpenSCManager(NULL,  /*local machine  */
		NULL,						/*active database*/
		SC_MANAGER_ALL_ACCESS);

	if (SCM_Handle == NULL)
	{
		if (log != NULL)
		{
			fprintf(log,"Error opening Service Control Manager: ");
			DisplayErrorText(GetLastError(),log);
		}
		return -1;
	}

	ServiceHandle=OpenService(SCM_Handle,
					ServiceName,
					SERVICE_ALL_ACCESS);


	if (ServiceHandle == NULL)
	{
		if (log != NULL)
		{
			fprintf(log,"Error opening service %s: ",ServiceName);
			DisplayErrorText(GetLastError(),log);
		}

		if (!CloseServiceHandle(SCM_Handle))
			if (log != NULL)
				fprintf(log,"Error closing Service control Manager\n");
					
		return -1;
	}

	ReturnValue=0;

	if (!StartService(ServiceHandle,0,NULL))
	{
		if (log != NULL)
		{
			fprintf(log, "Error starting service %s:",ServiceName);
			DisplayErrorText(GetLastError(),log);
		}
		ReturnValue=-1;
		
	}
	else
		if (log != NULL)
			fprintf(log,"Service %s successfully started\n",ServiceName);
	
	if (!CloseServiceHandle(ServiceHandle))
	{
		if (log != NULL)
			fprintf(log,"Error closing service %s\n",ServiceName);
		ReturnValue=-1;
	}


	if (!CloseServiceHandle(SCM_Handle))
	{
		if (log != NULL)
			fprintf(log,"Error closing Service control Manager\n");
		ReturnValue=-1;
	}
	
	return ReturnValue;

}


int create_service(FILE* log,LPCTSTR ServiceName,LPCTSTR ServiceDescriptionShort,LPCTSTR ServiceDescriptionLong,LPCTSTR ServicePath)
{
	SC_HANDLE SCM_Handle;
	SC_HANDLE ServiceHandle;
	DWORD ReturnValue;
	SERVICE_DESCRIPTION ServiceDescription;
	HMODULE hModule;
	MyChangeServiceConfig2 fMyChangeServiceConfig2;


	SCM_Handle=OpenSCManager(NULL,  /*local machine  */
		NULL,						/*active database*/
		SC_MANAGER_ALL_ACCESS);

	if (SCM_Handle==NULL)
	{

		if (log != NULL)
		{
			fprintf(log,"Error opening Service Control Manager: ");
			DisplayErrorText(GetLastError(),log);
		}
		
		return -1;
	}

	ServiceHandle=CreateService(SCM_Handle,
					ServiceName,
					ServiceDescriptionShort,
					SERVICE_ALL_ACCESS,
					SERVICE_WIN32_OWN_PROCESS,
					SERVICE_DEMAND_START,
					SERVICE_ERROR_NORMAL,
					ServicePath,
					NULL,
					NULL,
					"",
					NULL,
					NULL);

	if (ServiceHandle==NULL)
	{

		if (log != NULL)
		{
			fprintf(log,"Error creating service %s: ",ServiceName);
			DisplayErrorText(GetLastError(),log);
		}

		if (!CloseServiceHandle(SCM_Handle))
			if (log != NULL)
				fprintf(log,"Error closing Service control Manager\n");
					
		return -1;
	}

	if (log != NULL)
		fprintf(log,"Service %s successfully created.\n",ServiceName);

	//the following hack is necessary because the ChangeServiceConfig2 is not defined on NT4, so 
	//we cannot statically import ChangeServiceConfig2 from advapi32.dll, we have to load it dynamically

	hModule = LoadLibrary("advapi32.dll");
	
	if (hModule != NULL)
	{
		fMyChangeServiceConfig2 = (MyChangeServiceConfig2)GetProcAddress(hModule,"ChangeServiceConfig2A");
		if (fMyChangeServiceConfig2 != NULL)
		{
	
			ServiceDescription.lpDescription = (LPTSTR)ServiceDescriptionLong;

			if (!fMyChangeServiceConfig2(ServiceHandle,SERVICE_CONFIG_DESCRIPTION,&ServiceDescription))
			{
				if (log != NULL)
				{
					fprintf(log,"Error setting service description: ");
					DisplayErrorText(GetLastError(),log);
				}
			}
		}
		FreeLibrary(hModule);
	}

	
	ReturnValue=0;

	if (!CloseServiceHandle(ServiceHandle))
	{
		if (log != NULL)
			fprintf(log,"Error closing service %s.\n",ServiceName);
		ReturnValue=-1;
	}


	if (!CloseServiceHandle(SCM_Handle))
	{
		if (log != NULL)
			fprintf(log,"Error closing Service control Manager\n");
		ReturnValue=-1;
	}
	
	return ReturnValue;

}




void DisplayErrorText(DWORD dwLastError,FILE* output)
{
    HMODULE hModule = NULL; // default to system source
    LPSTR MessageBuffer;
    DWORD dwBufferLength;

    DWORD dwFormatFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_IGNORE_INSERTS |
        FORMAT_MESSAGE_FROM_SYSTEM ;

    //
    // If dwLastError is in the network range, 
    //  load the message source.
    //

    if(dwLastError >= NERR_BASE && dwLastError <= MAX_NERR) {
        hModule = LoadLibraryEx(
            TEXT("netmsg.dll"),
            NULL,
            LOAD_LIBRARY_AS_DATAFILE
            );

        if(hModule != NULL)
            dwFormatFlags |= FORMAT_MESSAGE_FROM_HMODULE;
    }

    //
    // Call FormatMessage() to allow for message 
    //  text to be acquired from the system 
    //  or from the supplied module handle.
    //

    if(dwBufferLength = FormatMessageA(
        dwFormatFlags,
        hModule, // module to get message from (NULL == system)
        dwLastError,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // default language
        (LPSTR) &MessageBuffer,
        0,
        NULL
        ))
    {
		fprintf(output,"%s",MessageBuffer);
/*        DWORD dwBytesWritten;

        //
        // Output message string on stderr.
        //
        WriteFile(
            GetStdHandle(STD_ERROR_HANDLE),
            MessageBuffer,
            dwBufferLength,
            &dwBytesWritten,
            NULL
            );
*/
        //
        // Free the buffer allocated by the system.
        //
        LocalFree(MessageBuffer);
    }

    //
    // If we loaded a message source, unload it.
    //
    if(hModule != NULL)
        FreeLibrary(hModule);
}

int change_start_type_service(FILE* log,LPCTSTR ServiceName, DWORD StartType)
{
	SC_HANDLE SCM_Handle;
	SC_HANDLE ServiceHandle;
	DWORD ReturnValue;

	SCM_Handle=OpenSCManager(NULL,  /*local machine  */
		NULL,						/*active database*/
		SC_MANAGER_ALL_ACCESS);

	if (SCM_Handle == NULL)
	{
		if (log != NULL)
		{
			fprintf(log,"Error opening Service Control Manager: ");
			DisplayErrorText(GetLastError(),log);
		}
		return -1;
	}

	ServiceHandle=OpenService(SCM_Handle,
					ServiceName,
					SERVICE_ALL_ACCESS);


	if (ServiceHandle == NULL)
	{
		if (log != NULL)
		{
			fprintf(log,"Error opening service %s: ",ServiceName);
			DisplayErrorText(GetLastError(),log);
		}

		if (!CloseServiceHandle(SCM_Handle))
			if (log != NULL)
				fprintf(log,"Error closing Service control Manager\n");
					
		return -1;
	}

	ReturnValue=0;

	if (!ChangeServiceConfig(ServiceHandle,
		SERVICE_NO_CHANGE,
		StartType,
		SERVICE_NO_CHANGE,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL))
	{
		if (log != NULL)
		{
			fprintf(log, "Error changing start type for service %s:",ServiceName);
			DisplayErrorText(GetLastError(),log);
		}
		ReturnValue=-1;
		
	}
	else
		if (log != NULL)
			fprintf(log,"Successfully changed start-type for service %s\n",ServiceName);
	
	if (!CloseServiceHandle(ServiceHandle))
	{
		if (log != NULL)
			fprintf(log,"Error closing service %s\n",ServiceName);
		ReturnValue=-1;
	}


	if (!CloseServiceHandle(SCM_Handle))
	{
		if (log != NULL)
			fprintf(log,"Error closing Service control Manager\n");
		ReturnValue=-1;
	}
	
	return ReturnValue;

}