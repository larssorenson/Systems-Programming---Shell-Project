
/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <regex.h>

#include "command.h"
#define MAXFILENAME 1024
#define MAX_BUFFER_LINE 2048
extern char **environ;
extern char *line_buffer;

// Simple history array
int history_index = 0;
char ** history = (char**)malloc(sizeof(char*)*20);
int history_length = 5;

char* shelldir;

SimpleCommand::SimpleCommand()
{
	// Creat available space for 5 arguments
	_numberOfAvailableArguments = 5;
	_numberOfArguments = 0;
	_arguments = (char **) malloc( _numberOfAvailableArguments * sizeof( char * ) );
}

void *
checkMalloc(size_t size)
{
	void *ret = malloc(size);
	if(ret == NULL)
		{
			fprintf(stderr, "Malloc Error\n");
			exit(1);
		}
	return ret;
}

void
SimpleCommand::insertArgument( char * argument )
{

	//Check to see if we need to do tilde expansion
	if(argument[0] == '~')
	{
	
		//Check to see if we do user's home directory
		if(argument[1] != '/' && argument[1] != '\0')
		{
		
			//Initialize variables for various data
			char* name;
			char* dir;
			int length = strlen(argument+1);
			
			//Keep track of the end of the username
			char* end = strstr(argument,"/");
			
			//If end was null, meaning no backslash whatsoever
			if(!end)
			{
			
				//Try and find a word
				end = strstr(argument," ");
				
				//If we didn't find a word or a directory
				if(!end)
				{
				
					//Malloc the name in size of the argument
					name = (char*)malloc(length);
					
					//Copy the argument into the name, so we can find the variable
					strcpy(name, argument+1);
					for(int x = length; x < strlen(name); x++)
					{
						name[x] = '\0';
					}
				}
				//If we found a word
				else
				{
					length = end-(argument+1);
					
					//Malloc the name for the name of the user to be used
					name = (char*)malloc(length);
					
					//Copy the name of the user
					strncpy(name, argument+1, length);
					for(int x = length; x < strlen(name); x++)
					{
						name[x] = '\0';
					}	
				}
			}
			//We found a backslash
			else
			{
			
				length = end-(argument+1);
			
				//Copy the name, between the end and argument
				name = (char*)malloc(length);
				
				//Copy only the name, nothing else
				strncpy(name, argument+1, length);
				for(int x = length; x < strlen(name); x++)
				{
					name[x] = '\0';
				}
			}
			
			//Struct for the home directory
			struct passwd* direct;
			
			//Get the directory for the user given
			direct = getpwnam(name);
				
			//Malloc the directory we're inserting
			dir = (char*)malloc(length);
			
			//Copy the directory of the user
			strcpy(dir, direct->pw_dir);
			
			//If there's more beyond the username
			if(end)
			{
				strcat(dir, end);
			}
			
			//Assign argument back
			argument = dir;
		}
		
		//No username defined
		else
		{
		
			//Get this user's home directory
			char* home = getenv("HOME");
			
			//Copy that in
			strcpy(argument, home);
		}
	}
	char reg[] = "^.*\\$\\{[^}]*\\}.*$";
	regex_t expbuf;
	regcomp( &expbuf, reg, REG_EXTENDED|REG_NOSUB);
	
	while(!regexec( &expbuf , argument , 0,0,0 ) )
	{
		
		//Get the location of the ${ beginning
		char* var = strstr(argument, "${") + 2;
		
		//Get the end of the variable name
		char* var2 = strstr(argument, "}");
		
		//Counter variable for pointer manip
		int count = 0;
		
		//Length of the variable name
		int length = var2-var;
		
		//Malloc'ing the variable name
		char* enviVar = (char*)checkMalloc(length);
		
		//Making sure to null terminate the pointer
		enviVar[length] = '\0';
		
		//Copy the variable name into enviVar
		strncpy(enviVar, var, length);
		
		//Fill the remaining space with null terminators
		while(*(enviVar+1) != '\0')
		{
		
			*enviVar = '\0';
			enviVar += 1;
			
		} 
		
		//Move back to the beginning of the string
		enviVar -= count;
		
		//Get the value for the environment variable
		char* data = getenv(enviVar);
		
		//Malloc the string for the new argument
		char* newArg = (char*)checkMalloc((strlen(argument)-3) + strlen(data));
		
		//move to the beginning of the ${...} string
		var-= count+2;
		
		//Fill the newArg string with null terminators
		while(*newArg != '\0')
		{
		
			*newArg++ = '\0';
			count++;
			
		}
		
		//Move back to the beginning of the string
		newArg -= count;
		
		//Zero out the count
		count = 0;
		
		//Copy argument into newArg
		memcpy(newArg, argument, var - argument);
		
		//Add the Environment variable
		strcat(newArg, data);
		
		//Move past the ${...}
		var += strlen(enviVar)+3;
		
		//Add the rest of the argument
		strcat(newArg, var);
		
		//Copy the new argument into the argument variable
		memcpy(argument, newArg, strlen(newArg)+1);
		
		//Free the environment variable string
		free(enviVar);
		//free(newArg);
	}
	if ( _numberOfAvailableArguments == _numberOfArguments  + 1 ) {
		// Double the available space
		_numberOfAvailableArguments *= 2;
		_arguments = (char **) realloc( _arguments,
				  _numberOfAvailableArguments * sizeof( char * ) );
	}
	
	_arguments[ _numberOfArguments ] = argument;

	// Add NULL argument at the end
	_arguments[ _numberOfArguments + 1] = NULL;
	_numberOfArguments++;
}

Command::Command()
{
	// Create available space for one simple command
	_numberOfAvailableSimpleCommands = 1;
	_simpleCommands = (SimpleCommand **)
		malloc( _numberOfSimpleCommands * sizeof( SimpleCommand * ) );

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
	_append = 0;
	_inNum = 0;
	_outNum = 0;
	_errNum = 0;
}

void
Command::insertSimpleCommand( SimpleCommand * simpleCommand )
{
	if ( _numberOfAvailableSimpleCommands == _numberOfSimpleCommands ) {
		_numberOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **) realloc( _simpleCommands,
			 _numberOfAvailableSimpleCommands * sizeof( SimpleCommand * ) );
	}
	
	_simpleCommands[ _numberOfSimpleCommands ] = simpleCommand;
	_numberOfSimpleCommands++;
}

void
Command:: clear()
{
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		for ( int j = 0; j < _simpleCommands[ i ]->_numberOfArguments; j ++ ) {
			free ( _simpleCommands[ i ]->_arguments[ j ] );
		}
		
		free ( _simpleCommands[ i ]->_arguments );
		free ( _simpleCommands[ i ] );
	}
	if( _outFile == _errFile )
	{
		free( _outFile );
	}
	else
	{
		if ( _outFile ) {
			free( _outFile );
		}

		if ( _inputFile ) {
			free( _inputFile );
		}

		if ( _errFile ) {
			free( _errFile );
		}
	}

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
	_append = 0;
	_inNum = 0;
	_outNum = 0;
	_errNum = 0;
}

void
Command::print()
{
	printf("\n\n");
	printf("              COMMAND TABLE                \n");
	printf("\n");
	printf("  #   Simple Commands\n");
	printf("  --- ----------------------------------------------------------\n");
	
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		printf("  %-3d ", i );
		for ( int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++ ) {
			printf("\"%s\" \t", _simpleCommands[i]->_arguments[ j ] );
		}
	}

	printf( "\n\n" );
	printf( "  Output       Input        Error        Background\n" );
	printf( "  ------------ ------------ ------------ ------------\n" );
	printf( "  %-12s %-12s %-12s %-12s\n", _outFile?_outFile:"default",
		_inputFile?_inputFile:"default", _errFile?_errFile:"default",
		_background?"YES":"NO");
	printf( "\n\n" );
	
}

void
Command::execute()
{
	// Don't do anything if there are no simple commands
	if ( _numberOfSimpleCommands == 0 ) 
	{
		clear();
		prompt();
		return;
	}
	if(strcmp(_simpleCommands[0]->_arguments[0], "cd") == 0)
	{
		if(_simpleCommands[0]->_arguments[1])
		{
			chdir(_simpleCommands[0]->_arguments[1]);
		}
		else
		{
			char* dir = getenv("HOME");
			chdir(dir);
		}
		clear();
		prompt();
		return;
	}
	if (_inNum > 1)
	{
		printf("Ambiguous input redirect");
	}
	if (_outNum > 1)
	{
		printf("Ambiguous output redirect");
	}
	if (_errNum > 1)
	{
		printf("Ambiguous error redirect");
	}
	if(strcmp(_simpleCommands[0]->_arguments[0], "exit") == 0 || strcmp(_simpleCommands[0]->_arguments[0],"quit") == 0)
	{
		exit(0);
	}
	
	//print();

	// Add execution here
	// For every simple command fork a new process
	// Setup i/o redirection
	// and call exec
	// 0 - stdin
	// 1 - stdout
	// 2 - stderr
	int in = dup(0);
	int out = dup(1);
	int error = dup(2);
	int fdpipe[2];
	int pid;
	int fdin;
	int piped = 0;
	int fdout;
	int fderr;
	if(_inputFile)
	{
		fdin = open(_inputFile, O_RDONLY);
	}
	else
		fdin = dup(in);
	if(_errFile)
	{
		if(_append)
		{
			fderr = open(_errFile, O_WRONLY|O_APPEND|O_CREAT, 0777);
		}
		else
		{
			fderr = open(_errFile, O_WRONLY|O_CREAT|O_TRUNC, 0777);
		}
	}
	else
		fderr = dup(error);
	for(int x = 0; x < _numberOfSimpleCommands; x++)
	{
		dup2(fdin, 0);
		dup2(fderr, 2);
		close(fderr);
		close(fdin);
		if(x == _numberOfSimpleCommands - 1)
		{
			if(_outFile)
			{
				if(_append)
				{
					fdout = open(_outFile, O_WRONLY|O_APPEND|O_CREAT, 0600);
				}
				else
				{
					fdout = open(_outFile, O_WRONLY|O_CREAT|O_TRUNC, 0600);
				}
			}
			else
				fdout = dup(out);
		}
		else
		{
			pipe(fdpipe);
			fdout = fdpipe[1];
			fdin = fdpipe[0];
			piped = 1;
		}
		dup2(fdout, 1);
		close(fdout);
		if(!strcmp(_simpleCommands[x]->_arguments[0], "setenv"))
		{
			char* name = _simpleCommands[x]->_arguments[1];
			char* value = _simpleCommands[x]->_arguments[2];
			if(getenv(name))
			{
				setenv(name, value, 1);
			}
			else
			{
				setenv(name, value, 0);
			}
		}
		else if(!strcmp(_simpleCommands[x]->_arguments[0], "unsetenv"))
		{
			char* name = _simpleCommands[x]->_arguments[1];			
			unsetenv(name);
			
		}
		else
		{
			pid = fork();
			if(pid == 0)
			{
				if(!strcmp(_simpleCommands[x]->_arguments[0], "printenv"))
				{
					char * name = (_simpleCommands[x]->_arguments[1] ? _simpleCommands[x]->_arguments[1] : NULL );
					char**p = environ;
					while(*p!=NULL)
					{
						if(name && !strcmp(*p, name))
							printf("%s\n",*p);
						else if(!name)
							printf("%s\n",*p);
						p+=1;
					}
					exit(0);
				}
				if(piped)
				{
					close(fdpipe[0]);
					close(fdpipe[1]);
				}
				execvp(_simpleCommands[x]->_arguments[0], _simpleCommands[x]->_arguments);
				perror("execvp");
				_exit(1);
			}
			else if (pid < 0)
			{
				perror("fork");
				return;
			}
		}
	}
	if(!_background)
	{
		waitpid(pid, NULL, 0);
	}
	dup2(in, 0);
	dup2(out, 1);
	dup2(error, 2);
	close(in);
	close(out);
	close(error);

	// Clear to prepare for next command
	clear();
	
	// Print new prompt
	prompt();
}


// Shell implementation

void sigchld( int sig )
{
		wait3(0,0,NULL);
		while(waitpid(-1,NULL,WNOHANG) > 0){}
}

void sigint( int sig )
{	
	line_buffer = (char*)"";
	Command::_currentCommand.clear();
	printf("\n");
	Command::_currentCommand.prompt();
}

void
Command::prompt()
{
	if(isatty(0))
	{
		printf("SShell>");
	}
	fflush(stdout);
}

Command Command::_currentCommand;
SimpleCommand * Command::_currentSimpleCommand;

int yyparse(void);


main()
{
	for(int x = 0; x < history_length; x++)
	{
		history[x] = (char*)malloc(MAX_BUFFER_LINE);
	}
	struct sigaction signalAction;
	signalAction.sa_handler = sigint;
	sigemptyset(&signalAction.sa_mask);
	signalAction.sa_flags = SA_RESTART;

	int error = sigaction(SIGINT, &signalAction, NULL );
	if ( error )
	{
		perror( "sigaction" );
		exit( -1 );
	}
	
	signalAction.sa_handler = sigchld;
	sigemptyset(&signalAction.sa_mask);
	error = sigaction(SIGCHLD, &signalAction, NULL );
	if ( error )
	{
		perror( "sigaction" );
		exit( -1 );
	}
	int len = sizeof(char*)*(strlen(getenv("PWD")) + 1 + strlen(getenv("_")));
	shelldir = (char*)malloc(len);
	strncpy(shelldir, getenv("PWD"), len);
	strcat(shelldir, "/");
	strcat(shelldir, getenv("_"));

	Command::_currentCommand.prompt();
	yyparse();
	//Command::_currentCommand.print();
}


