
/*
 *
 * CS-252 Fall 2013
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 */

%{

#include <string.h>
#include <unistd.h>
#include "y.tab.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>


void subshell();
void escapeChars();
extern char * read_line(); 
extern "C" { char yy_unput(int c); }

extern char* shelldir;

int mygetc(FILE * f) 
{
	static char *p;
	char ch;

	if (!isatty(0)) 
	{
		// stdin is not a tty. Call real getc
		return getc(f);
	}

	// stdin is a tty. Call our read_line.

	if (p==NULL || *p == 0) 
	{
		char * s = read_line();
		p = s;
	}

	ch = *p;
	p++;

	return ch;
}

#undef getc
#define getc(f) mygetc(f)

/////////// End added code ///////////

%}

%%

\n 	{
		return NEWLINE;
	}

[ \t] 	{
		/* Discard spaces and tabs */
     	}	

">" 	{
		return GREAT;
	}
	
">>"	{
		return GREATGREAT;
	}
	
">&"	{
		return GREATAMP;
	}
	
">>&"	{
		return GREATGREATAMP;
	}
	
"|"	{
		return PIPE;
	}
	
"<"	{
		return LESS;
	}
	
"&"	{
		return AMPERSAND;
	}
[^ \t\n]*(\\[^\s])[^ \t\n]*	{
		escapeChars();
		yylval.string_val = strdup(yytext);
		return WORD;
	}

[^ (\|)(>>)(>)(>&)(>>&)(<)(&)\t\n][^ (\|)(>>)(>)(>&)(>>&)(&)(<)\t\n]*	{
		/* Assume that file names have only alpha chars */
		yylval.string_val = strdup(yytext);
		return WORD;
	}

.	{
		/* Invalid character in input */
		return NOTOKEN;
	}
	
\"[^\n\"]*\"	{
		char* noquotes = yytext;
		noquotes++;
		yytext = noquotes;
		yytext[strlen(yytext)-1] = '\0';
		yylval.string_val = strdup(yytext);	
		return WORD;
	}
\`[^\n\"]*\`	{
		subshell();
		//yylval.string_val = strdup(yytext);
		//return WORD;
	}
%%

void subshell2()
{
	
	
}

void subshell()
{	
	yytext+=1;
	yytext[strlen(yytext)-1] = '\0';
	char* command = strdup(yytext);
	
	
	int pipein[2];
	int pipeout[2];
	int in = dup(0);
	int out = dup(1);
		
	int length = 10;
	
	int moved = 0;
	
	char c = 0;
	
	char* output = (char*)malloc(10);
	strncpy(output, (char*)"", 10);

	#define childWrite pipeout[1]
	#define childRead pipein[0]
	#define parentWrite pipein[1]
	#define parentRead pipeout[0]
	
	pipe(pipein);
	pipe(pipeout);
	
	
	int pid = fork();
	if(pid < 0)
	{
		perror("fork");
		_exit(1);
	}
	
	if(pid == 0)
	{
		write(parentWrite, command, strlen(command));
		
		write(parentWrite, "\nexit\n", 6);
		
		if(close(parentWrite) < 0)
		{
			perror("close parent pipe in");
			_exit(1);
		}
		if(close(parentRead) < 0)
		{
			perror("close child pipe out");
			_exit(1);
		}
		
		if(dup2(childRead, 0) < 0)
		{
			perror("parent pipe in");
			_exit(1);
		}
		if(dup2(childWrite, 1) < 0)
		{
			perror("child pipe out");
			_exit(1);
		}
		
		execv(shelldir, NULL);
		perror("subshell");
		_exit(0);
	}
	else
	{		
		if(close(parentWrite) < 0)
		{
			perror("close parent pipe in");
			_exit(1);
		}	
		waitpid(pid, NULL, 0);
		
		if(close(childWrite) < 0)
		{
			perror("close child pipe out");
			_exit(1);
		}
		if(close(childRead) < 0)
		{
			perror("close child pipe in");
			_exit(1);
		}

		if(read(parentRead, &c, 1) < 0)
		{
			perror("Reading sucks");
			_exit(0);
		}
		output[0] = c;
		moved +=1;
		while(read(parentRead, &c, 1) > 0)
		{
			output[moved] = c;
			if(moved+1 > length)
			{
				output = (char*)realloc(output, (sizeof(char*)*(length*2)));
				length *= 2;
			}
			moved += 1;
		}
		if(close(parentRead) < 0)
		{
			perror("close child pipe out");
			_exit(1);
		}

		if(dup2(in, 0) < 0)
		{
			perror("stdin");
		}
		if(dup2(out, 1) < 0)
		{
			perror("stdout");
		}
		int n = moved;
		for(;n > -1; n-=1)
		{
			if(output[n] == '\n')
			{
				output[n] = ' ';
			}
			unput(output[n]);
		}
	}
	
}

void escapeChars()
{
	char* yytext2 = (char*)malloc(strlen(yytext)+1);
	int escaped = 0;
	int y = 0;
	
	for(int x = 0; x < strlen(yytext); x+=1)
	{
		if(yytext[x] == '\\' && escaped == 0)
		{
			escaped = 1;
		}
		else if(escaped == 1)
		{
			yytext2[y] = yytext[x];
			y+=1;
			escaped = 0;
		}
		else
		{
			yytext2[y] = yytext[x];
			y+=1;
		}
	}
	yytext2[y] = '\0';
	strcpy(yytext, yytext2);
	free(yytext2);
}
