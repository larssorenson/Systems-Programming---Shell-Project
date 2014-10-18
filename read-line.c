/*
 * CS354: Operating Systems. 
 * Purdue University
 * Example that shows how to read one line with simple editing
 * using raw terminal.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

#define MAX_BUFFER_LINE 2048

extern void tty_raw_mode();

// Buffer where line is stored
int line_length;
int line_pos=0;
char line_buffer[MAX_BUFFER_LINE];

extern char ** history;
extern int history_index;
extern int history_length;
int currenthistory_length;

/* 
 * Input a line with some basic editing.
 */
char * read_line() 
{
	struct termios orig_attr;
	tcgetattr(0,&orig_attr);
	// Set terminal in raw mode
	tty_raw_mode();

	line_length = 0;
	line_pos = 0;

	// Read one line until enter is typed
	while (1) 
	{

		// Read one character in raw mode.
		char ch;
		read(0, &ch, 1);

		if (ch>=32 && ch != 127) 
		{
			// It is a printable character. 
			
			if(line_pos != line_length)
			{
				char ch1 = ' ';
				for(int x = line_pos; x < line_length; x+=1)
					write(1, &ch1,1);
				ch1 = 8;
				for(int x = line_pos; x < line_length; x+=1)
					write(1,&ch1,1);
				write(1,&ch,1);
				for(int x = line_pos; x < line_length; x+=1)
					write(1,&line_buffer[x], 1);
				for(int x = line_pos; x < line_length; x+=1)
					write(1,&ch1,1);
				for(int x = line_length; x > line_pos; x-=1)
					line_buffer[x] = line_buffer[x-1];
				// If max number of character reached return.
				if (line_length==MAX_BUFFER_LINE-2) break;
				line_buffer[line_pos] = ch; 
			}
			else
			{
				// Do echo
				write(1,&ch,1);
				
				// If max number of character reached return.
				if (line_length==MAX_BUFFER_LINE-2) break; 
				
				// add char to buffer.
				line_buffer[line_length]=ch;
			}
			line_length++;
			line_pos+=1;
			
			if(history_index == 0)
				strcpy(history[0], line_buffer);
		}
		else if (ch==31)
		{
			printf("\nSorenson (S) Shell Usage\nCtrl+C : Interrupt Process\nExit/Quit: Exit the shell\nCtrl + ? : Print this prompt\n");
			line_buffer[0] = 0;
			break;
		}
		else if (ch==10) 
		{
			// <Enter> was typed. Return line

			// Print newline
			write(1,&ch,1);
			
			//Expand the history
			if(currenthistory_length+1 >= history_length)
			{
				history = (char**)realloc(history, sizeof(char*)*(history_length*2));
				history_length *=2;
				history[(history_length/2)] = (char*)malloc(MAX_BUFFER_LINE);
				for(int x = history_length; x > history_length/2; x -=1)
				{
					history[x] = (char*)malloc(MAX_BUFFER_LINE);
				}
				for(int x = history_length/2; x > 1; x-=1)
				{
					strncpy(history[x], history[x-1], MAX_BUFFER_LINE);
				}
			}
			else
			{	
				for(int x = history_length-1; x > 1; x-=1)
				{
					strncpy(history[x], history[x-1], MAX_BUFFER_LINE);
				}
			}
			strncpy(history[0],(char*)"", MAX_BUFFER_LINE);
			strncpy(history[1],(char*)"", MAX_BUFFER_LINE);
			strncpy(history[1], line_buffer, line_length);
			currenthistory_length += 1;
			
			break;
		}
		else if (ch == 127) 
		{
			// <backspace> was typed. Remove previous character read.
			if(line_length > 0 && line_pos > 0)
			{
				//Move all the characters over to delete the previous one
				for(int x = line_pos-1; x < line_length-1; x+=1)
				{
					line_buffer[x] = line_buffer[x+1];
				}
				
				//Blank the last char
				line_buffer[line_length-1] = '\0';
				
				//Move all the way to the beginning of the line
				ch = 8;
				for(int x = 0; x < line_pos; x+=1)
					write(1, &ch, 1);
					
				//Erase the whole line
				ch = ' ';
				for(int x = 0; x < line_length; x+=1)
					write(1,&ch,1);
				
				//Move back to the beginning again	
				ch = 8;
				for(int x = 0; x < line_length; x+=1)
					write(1, &ch, 1);
					
				//Write in the line buffer
				for(int x = 0; x < line_length-1; x+=1)
					write(1, &line_buffer[x], 1);
				
				//Move back to the position we were at again	
				for(int x = 0; x < (line_length-line_pos); x+=1)
				{
					write(1, &ch,1);
				}
				
				// Remove one character from buffer
				//if(line_pos == line_length)
					line_pos-=1;
				line_length--;
			}
		}
		else if (ch==27) 
		{
			// Escape sequence. Read two chars more
			//
			// HINT: Use the program "keyboard-example" to
			// see the ascii code for the different chars typed.
			//
			char ch1; 
			char ch2;
			read(0, &ch1, 1);
			read(0, &ch2, 1);
			if (ch1==79 && ch2==70)
			{
				//End key
				while(line_pos < line_length)
				{
					ch = line_buffer[line_pos];
					write(1, &ch,1);
					line_pos+=1;
				}
			}
			else if(ch1==79 && ch2==72)
			{
				//Home key
				while(line_pos > 0)
				{	
					ch = 8;
					write(1,&ch,1);
					line_pos-=1;
				}
			}
			else if (ch1==91 && ch2==65 && history_index < (history_length-1) && strlen(history[history_index+1]) != 0) 
			{
				// Up arrow. Print next line in history.

				// Erase old line
				// Print backspaces
				int i = 0;
				for (i =0; i < line_pos; i++) 
				{
					ch = 8;
					write(1,&ch,1);
				}

				// Print spaces on top
				for (i =0; i < line_length; i++) 
				{
					ch = ' ';
					write(1,&ch,1);
				}

				// Print backspaces
				for (i =0; i < line_length; i++) 
				{
					ch = 8;
					write(1,&ch,1);
				}	

				// Copy line from history
				history_index=(history_index+1)%history_length;
				strcpy(line_buffer, history[history_index]);
				line_length = strlen(line_buffer);
				line_pos = line_length;

				// echo line
				write(1, line_buffer, line_length);
			}
			else if (ch1==91 && ch2==66 && history_index > 0) 
			{
				// Down arrow. Print previous line in history.

				// Erase old line
				// Print backspaces
				int i = 0;
				for (i =0; i < line_pos; i++) 
				{
					ch = 8;
					write(1,&ch,1);
				}

				// Print spaces on top
				for (i =0; i < line_length; i++) 
				{
					ch = ' ';
					write(1,&ch,1);
				}

				// Print backspaces
				for (i =0; i < line_length; i++) 
				{
					ch = 8;
					write(1,&ch,1);
				}	

				// Copy line from history
				history_index=(history_index-1)%currenthistory_length;
				strcpy(line_buffer, history[history_index]);
				line_length = strlen(line_buffer);
				line_pos = line_length;

				// echo line
				write(1, line_buffer, line_length);
			}
			// Right arrow
			else if(ch1==91 && ch2==67 && line_pos < line_length)
			{
				ch = line_buffer[line_pos];
				write(1, &ch,1);
				line_pos+=1;
			}
			// Left arrow
			else if(ch1==91 && ch2==68 && line_pos > 0)
			{
				ch = 8;
				write(1,&ch,1);
				line_pos-=1;
			}
			else if(ch1==91 && ch2==51)
			{
				char ch3;
				read(1,&ch3,1);
				if(ch3 == 126)
				{
					if(line_length > 0 && line_pos < line_length)
					{
						//Move all the characters over to delete the previous one
						for(int x = line_pos; x < line_length-1; x+=1)
						{
							line_buffer[x] = line_buffer[x+1];
						}
				
						//Blank the last char
						line_buffer[line_length-1] = '\0';
				
						//Move all the way to the beginning of the line
						ch = 8;
						for(int x = 0; x < line_pos; x+=1)
							write(1, &ch, 1);
					
						//Erase the whole line
						ch = ' ';
						for(int x = 0; x < line_length; x+=1)
							write(1,&ch,1);
				
						//Move back to the beginning again	
						ch = 8;
						for(int x = 0; x < line_length; x+=1)
							write(1, &ch, 1);
					
						//Write in the line buffer
						for(int x = 0; x < line_length-1; x+=1)
							write(1, &line_buffer[x], 1);
							
							
						if(line_pos == 0)
						{
							for(int x = 0; x < line_length-1; x+=1)
								write(1, &ch, 1);
						}
						else if(line_length>1 && line_pos != line_length-1)
						{
							//Move back to the position we were at again	
							for(int x = 0; x < ((line_length-1)-line_pos); x+=1)
							{
								write(1, &ch, 1);
							}
						}
				
						// Remove one character from buffer
						line_length--;
					}

				}
			}
		}
	}

	// Add eol and null char at the end of string
	line_buffer[line_length]=10;
	line_length++;
	line_buffer[line_length]=0;
	tcsetattr(0,TCSANOW,&orig_attr);
	return line_buffer;
}

