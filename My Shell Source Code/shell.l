
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


/////// START ADDED CODE ///////

extern "C" char * read_line();

int mygetc(FILE * f) {
	static char *p;
	char ch;

	if (!isatty(0)) {
		//stdin is not a tty. Call real getc
		return getc(f);
	}

	//stdin is a tty. Call our read_line.

	if (p == NULL || *p == 0) {
		char * s = read_line();
		p = s;
	}

	ch = *p;
	p++;

	return ch;
}

#undef getc
#define getc(f) mygetc(f)

/////// END ADDED CODE ///////

static  void yyunput (int c,char *buf_ptr  );

void myunputc(int c) {
    unput(c);
}

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

"|"	{
		return PIPE;
	}

"&"	{
		return AMPERSAND;
	}

">>&"	{
		return GREATGREATAMPERSAND;
	}

">&"	{
		return GREATAMPERSAND;
	}

"<"	{
		return LESS;
	}
"exit"	{
		printf("Good bye!!\n");
		exit(0);
	}

[`].*[`]	{
	//matches anything contained in backticks
	//extract info in backticks
	char * duplicate = strdup(yytext);
	char * inTicks = (char *) malloc(sizeof(char) * (strlen(yytext)));
	duplicate++;
	for (int i = 0; i < strlen(yytext) - 2; i++) {
		*inTicks = *duplicate;
		duplicate++;
		inTicks++;
	}
	*inTicks = '\n';
	inTicks++;
	*inTicks = 0;
	inTicks -= strlen(yytext) - 1;
	duplicate -= strlen(yytext) - 1;
	free(duplicate);

	int tempin = dup(0);
	int tempout = dup(1);

	int parentToChild[2];
	int childToParent[2];
	pipe(parentToChild);
	pipe(childToParent);
	
	write(parentToChild[1], inTicks, strlen(inTicks));
	close(parentToChild[1]);
	dup2(parentToChild[0], 0);
	close(parentToChild[0]);
	dup2(childToParent[1], 1);
	close(childToParent[1]);
	
	//fork
	int ret;
	ret = fork();
	
	//if in the child process
	if (ret == 0) {
		//load program into current process
		char * arr[2];
		arr[0] = "shell";
		arr[1] = NULL;
		execvp("/proc/self/exe", arr);	//execute
		perror("execvp");
		_exit(1);
	}
	else if (ret < 0) {
		perror("fork");
		_exit(1);
	}
		
	//parent shell continues
	//let parent read output of 'cmd args' from this pipe

	dup2(tempin, 0);
	dup2(tempout, 1);
	close(tempin);
	close(tempout);
	
	char buf[1024];
	memset(buf, 0, 1024);
	read(childToParent[0], buf, 1024);
	int len = strlen(buf);
	for (int i = 0; i < len; i++) {
		if (buf[i] == '\n' && i != len - 1) {
			buf[i] = ' ';
		}
		if (buf[i] == '\n' && i == len - 1) {
			buf[i] = 0;
		}
	}
	close(childToParent[0]);

	for (int i = len - 1; i >= 0; i--) {
		myunputc(buf[i]);
	}

	dup2(tempin, 0);
	dup2(tempout, 0);
	close(tempin);
	close(tempout);
}

["].*["]	{
	//matches anything contained in quotes
	//remove quotes and treat like a word
	char * duplicate = strdup(yytext);
	char * inQuote = (char *) malloc(sizeof(char) * (strlen(yytext) - 1));
	duplicate++;
	for (int i = 0; i < strlen(yytext) - 2; i++) {
		*inQuote = *duplicate;
		duplicate++;
		inQuote++;
	}
	*inQuote = 0;
	inQuote -= strlen(yytext) - 2;
	duplicate -= strlen(yytext) - 1;
	free(duplicate);
	yylval.string_val = inQuote;
	return WORD;
		}
[^ \t\n]*([\\][^ \t\n]+)+	{
	//matches anything that has a backslash char in it
	//remove backslashes and treat like a word
	char * duplicate = strdup(yytext);
	char * noBackslashString = (char *) malloc(sizeof(char) * (strlen(yytext) + 1));
	int dupCounter = 0;
	int noBSCounter = 0;
	for (int i = 0; i < strlen(yytext); i++) {
		if (*duplicate == '\\')	{
			duplicate++;
			i++;
			dupCounter++;
		}
		*noBackslashString = *duplicate;
		duplicate++;
		dupCounter++;
		noBackslashString++;
		noBSCounter++;
	}
	*noBackslashString = 0;
	noBackslashString -= noBSCounter;
	duplicate -= dupCounter;
	free(duplicate);
	yylval.string_val = noBackslashString;
	return WORD;
				}
[$][{][^ \t\n]+[}][^ \t\n]*[$][{][^ \t\n]+[}]	{
	//match environment variable, string, environment variable all concatenated
	char * duplicate = strdup(yytext);
	char * temp = duplicate;
	char part1[strlen(duplicate) + 1];
	char part2[strlen(duplicate) + 1];
	char part3[strlen(duplicate) + 1];
	duplicate += 2;
	char * s = strchr(duplicate, '}');
	int numOfCharsToCopy = s - duplicate;
	strncpy(part1, duplicate, numOfCharsToCopy);
	part1[numOfCharsToCopy] = '\0';
	duplicate = s + 1;
	s = strchr(duplicate, '$');
	numOfCharsToCopy = s - duplicate;
	strncpy(part2, duplicate, numOfCharsToCopy);
	part2[numOfCharsToCopy] = '\0';
	duplicate = s;
	duplicate += 2;
	s = strchr(duplicate, '}');
	numOfCharsToCopy = s - duplicate;
	strncpy(part3, duplicate, numOfCharsToCopy);
	part3[numOfCharsToCopy] = '\0';
	free(temp);
	char * part1var = getenv(part1);
	char * part3var = getenv(part3);
	part1var = strcat(part1var, part2);
	yylval.string_val = strcat(part1var, part3var);
	return WORD;
						}
[$][{][^ \t\n]+[}]	{
	//match environment variable expansion
	char * duplicate = strdup(yytext);
	char * s = duplicate + 2;
	int numOfCharsToCopy = strlen(duplicate) - 3;
	char var[numOfCharsToCopy + 1];
	strncpy(var, s, numOfCharsToCopy);
	//terminate var with 0
	var[numOfCharsToCopy] = '\0';
	free(duplicate);
	yylval.string_val = getenv(var);
	return WORD;
			}

[~][^ \t\n]+	{
		char * duplicate = strdup(yytext);
		char * temp = duplicate;
		char * homes = "/homes/";
		char home[strlen(homes) + strlen(duplicate)];
		char * point = home;
		strncpy(home, homes, strlen(homes));
		point += strlen(homes);
		duplicate++;
		strncpy(point, duplicate, strlen(duplicate));
		free(temp);
		yylval.string_val = home;
		return WORD;
	}

[~]	{
		yylval.string_val = getenv("HOME");
		return WORD;
	}


[^ \t\n|>][^ \t\n|>]*  {
		/* Assume that file names have only alpha chars */
		yylval.string_val = strdup(yytext);
		return WORD;
	}

