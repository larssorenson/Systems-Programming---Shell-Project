
/*
 * CS-252 Spring 2013
 * shell.y: parser for shell
 *
 * This parser compiles the following grammar:
 *
 *	cmd [arg]* [> filename]
 *
 * you must extend it to understand the complete shell grammar
 *
 */

%token	<string_val> WORD

%token 	NOTOKEN GREAT NEWLINE GREATGREAT GREATGREATAMP GREATAMP LESS PIPE AMPERSAND

%union	{
		char   *string_val;
	}

%{
//#define yylex yylex
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "command.h"
#include <regex.h>
#include <dirent.h>
void yyerror(const char * s);
int yylex();
void expandWildcard(char * prefix, char * suffix, int prefixMatch);
void expandWildcardIfNecessary(char * arg);
int arraySort(const void*, const void*);


%}

%%

goal:	
	commands
	;

commands: 
	command
	| commands command 
	;

command: 
	pipe_list io_modifier_list background_opt NEWLINE
	{
		//printf("   Yacc: Execute command\n");
		Command::_currentCommand.execute();
	}
	|	NEWLINE{Command::_currentCommand.clear();Command::_currentCommand.prompt();}
	|	error NEWLINE{yyerrok;}
	;
	
pipe_list:
	pipe_list PIPE command_and_args
	|	command_and_args
	;


command_and_args:
	command_word arg_list 
	{
		Command::_currentCommand.
			insertSimpleCommand( Command::_currentSimpleCommand );
	}
	;
	
arg_list:
	arg_list argument
	| /* can be empty */
	;

argument:
	WORD 
	{
		//printf("   Yacc: insert argument \"%s\"\n", $1);
		expandWildcardIfNecessary($1);
		//Command::_currentSimpleCommand->insertArgument( $1 );
	}
	;

command_word:
	WORD 
	{
               //printf("   Yacc: insert command \"%s\"\n", $1);
	       
		Command::_currentSimpleCommand = new SimpleCommand();
		//expandWildcard(NULL,$1);
		Command::_currentSimpleCommand->insertArgument( $1 );
	}
	;

io_modifier_list:
	io_modifier_list io_modifier_opt
	|
	;

io_modifier_opt:
	GREAT WORD 
	{
		//printf("   Yacc: insert output \"%s\"\n", $2);
		Command::_currentCommand._outFile = $2;
		Command::_currentCommand._outNum++;
	}
	| GREATGREAT WORD 
	{
		//printf("   Yacc: append output \"%s\"\n", $2);
		Command::_currentCommand._outFile = $2;
		Command::_currentCommand._append = 1;
		Command::_currentCommand._outNum++;
	}
	| GREATGREATAMP WORD 
	{
		//printf("   Yacc: append error \"%s\"\n", $2);
		Command::_currentCommand._outFile = $2;
		Command::_currentCommand._errFile = $2;
		Command::_currentCommand._append = 1;
		Command::_currentCommand._outNum++;
		Command::_currentCommand._errNum++;
	}
	| LESS WORD 
	{
		//printf("   Yacc: insert input \"%s\"\n", $2);
		Command::_currentCommand._inputFile = $2;
		Command::_currentCommand._inNum++;
	}
	| GREATAMP WORD 
	{
		//printf("   Yacc: insert error \"%s\"\n", $2);
		Command::_currentCommand._outFile = $2;
		Command::_currentCommand._errFile = $2;
		Command::_currentCommand._outNum++;
		Command::_currentCommand._errNum++;
	}
	;

background_opt:
	AMPERSAND 
	{
		//printf("   Yacc: execute in background\n");
		Command::_currentCommand._background = 1;
	}
	|
	;
%%
int maxEntries = 20;
int entries = 0;
char** array;
void expandWildcardIfNecessary(char * arg)
{
	// Return if arg does not contain
	if (!strchr(arg, '*') && !strchr(arg, '?')) 
	{
		Command::_currentSimpleCommand->insertArgument(arg);
		return;
	}
  	else
	{
		array = (char**)malloc(maxEntries*sizeof(char*));
		array[0] = NULL;
		//if(arg[0] == '/')
		//{
		//	arg+=1;
		//}
		expandWildcard((char*)"", arg, 0);
		if(array[0] == NULL)
		{
			Command::_currentSimpleCommand->insertArgument(arg);
		}
		qsort(array, entries, sizeof(char*), arraySort);
		for(int x=0; x<entries; x+=1)
		{
			Command::_currentSimpleCommand->insertArgument(array[x]);
		}
		maxEntries = 20;
		entries = 0;
		free(array);
	}
}
int arraySort(const void* s1,const void* s2)
{
	return strcmp(*(char**)s1,*(char**)s2);
}

#define MAXFILENAME 1024
void expandWildcard(char * prefix, char * suffix, int prefixMatch) 
{
	if(strstr(prefix, "//"))
	{
		char* dblSlash = strstr(prefix, "//");
		char* nPrefix = (char*)malloc(strlen(prefix)-1);
		memcpy(nPrefix, prefix, prefix-dblSlash);
		//memcpy(nPrefix, "/", 1);
		prefix = dblSlash+1;
		strcat(nPrefix, prefix);
		prefix = nPrefix;
	}
	if (suffix[0]== 0) 
	{
		// suffix is empty. Put prefix in argument.
		if(entries == maxEntries)
		{
			maxEntries *= 2;
			array =(char**)realloc(array,sizeof(char*)*maxEntries);
		}
		array[entries] = strdup(prefix);
		entries += 1;
		return;
	}
	// Obtain the next component in the suffix
	// Also advance suffix.
	char * s = strchr(suffix, '/');
	char component[MAXFILENAME];
	if (s!=0)
	{ 
		if(!(s-suffix))
		{
			strcpy(component,"");
		}
		else
		{
			// Copy up to the first
			strncpy(component,suffix, s-suffix);
		}
		suffix = s + 1;
	}
	else 
	{
		// Last part of path. Copy whole thing.
		strcpy(component, suffix);
		suffix = suffix + strlen(suffix);
	}
	// Now we need to expand the component
	char newPrefix[MAXFILENAME];
	if (!strchr(component, '*') && !strchr(component, '?')) 
	{
		// component does not have wildcards
		sprintf(newPrefix, "%s/%s", prefix, component);
		expandWildcard(newPrefix, suffix, 0);
		return;
	}
	// 1. Convert wildcard to regular expression
	// Convert "*" -> ".*"
	//         "?" -> "."
	//         "." -> "\."  and others you need
	// Also add ^ at the beginning and $ at the end to match
	// the beginning ant the end of the word.
	// Allocate enough space for regular expression
	char * reg = (char*)malloc(2*strlen(component)+10); 
	char * a = component;
	char * r = reg;
	*r = '^'; 
	r++; // match beginning of line
	while (*a) 
	{
		if (*a == '*') { *r='.'; r++; *r='*'; r++; }
		else if (*a == '?') { *r='.'; r++;}
		else if (*a == '.') { *r='\\'; r++; *r='.'; r++;}
		else { *r=*a; r++;}
		a++;
	}
	*r='$'; 
	r++; 
	*r=0;// match end of line and add null char
	
	// 2. compile regular expression
	regex_t expbuf;
	int regerr = regcomp( &expbuf, reg, REG_EXTENDED|REG_NOSUB); 
	if (regerr) 
	{
		perror("compile");
		return;
	}
	char * dir;
	// If prefix is empty then list current directory
	if (prefix[0] == 0) 
		dir = (char*)".";
	else 
		dir=prefix;
	DIR * d = opendir(dir);
	if (d==NULL) 
	{
		if(prefixMatch == 0)
		{
			// suffix is empty. Put prefix in argument.
			if(entries == maxEntries)
			{
				maxEntries *= 2;
				array =(char**)realloc(array,sizeof(char*)*maxEntries);
			}
			array[entries] = strdup(prefix);
			entries += 1;
		}
		return;
	}
	
	struct dirent * ent;
	while ( (ent = readdir(d))!= NULL) 
	{
		// Check if name matches
		if (!regexec( &expbuf , ent->d_name, 0, 0, 0)) 
		{
			if(component[0] == '.' && ent->d_name[0] == '.')
			{
				if(strlen(prefix) == 0)
				{
					sprintf(newPrefix, "%s", ent->d_name);
				}
				else
				{
					sprintf(newPrefix, "%s/%s", prefix, ent->d_name);
				}
				expandWildcard(newPrefix, suffix, 1);
			}
			else if(component[0] != '.' && ent->d_name[0] != '.')
			{
				if(strlen(prefix) == 0)
				{
					sprintf(newPrefix, "%s", ent->d_name);
				}
				else
				{
					sprintf(newPrefix, "%s/%s", prefix, ent->d_name);
				}
      				expandWildcard(newPrefix, suffix, 1);
			}
		}
	}
	closedir(d);
	regfree(&expbuf);
}

void
yyerror(const char * s)
{
	fprintf(stderr,"%s", s);
}

#if 0
main()
{
	yyparse();
}
#endif
